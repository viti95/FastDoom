//
// AdLib Gold 1000/2000 digital audio driver for FastDoom
// Uses the YMZ263 (MMA) chip for PCM playback via DMA
//
// Based on:
//   - AdLib Gold Developer Toolkit (section 7.3: Digital I/O)
//   - AIL2 DIGPAK driver reference implementation
//   - FastDoom ns_wss.c as structural template
//
// Uses GOLD environment variable for configuration:
//   SET GOLD=<I/O hex>,<IRQ dec>,<DMA dec>
//   Example: SET GOLD=388,5,1
//

#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ns_dpmi.h"
#include "ns_dma.h"
#include "ns_inter.h"
#include "ns_irq.h"
#include "ns_gold.h"
#include "ns_muldf.h"
#include "options.h"

#define INVALID -1

/*
 * Interrupt vector mapping for IRQ lines.
 * IRQ 0-7  -> vectors 0x08-0x0F
 * IRQ 8-15 -> vectors 0x70-0x77
 */
static const unsigned char GOLD_Interrupts[GOLD_MaxIrq + 1] =
    {
        INVALID, INVALID, 0xa, 0xb,
        INVALID, 0xd, INVALID, 0xf,
        INVALID, INVALID, 0x72, 0x73,
        0x74, INVALID, INVALID, 0x77
    };

static void(__interrupt __far *GOLD_OldInt)(void);

GOLD_CONFIG GOLD_Config =
    {
        GOLD_DEFAULT_BASE, GOLD_DEFAULT_IRQ, GOLD_DEFAULT_DMA
    };

static int GOLD_Installed = FALSE;
static int GOLD_TransferLength = 0;
static int GOLD_SamplePacketSize = GOLD_MONO_8BIT_SAMPLE_SIZE;
unsigned GOLD_SampleRate = GOLD_DefaultSampleRate;

volatile int GOLD_SoundPlaying;
int GOLD_DMAChannel;

static int GOLD_IntController1Mask;
static int GOLD_IntController2Mask;

static int GOLD_MMAVolume = 0xFF;
static int GOLD_InterruptVector;

#define kStackSize 2048

static unsigned short StackSelector = 0;
static unsigned long StackPointer;

static unsigned short oldStackSelector;
static unsigned long oldStackPointer;

extern void GetStack(unsigned short *selptr, unsigned long *stackptr);
extern void SetStack(unsigned short selector, unsigned long stackptr);

#pragma aux GetStack = \
    "mov  [edi],esp"   \
    "mov  ax,ss"       \
    "mov  [esi],ax" parm[esi][edi] modify[eax esi edi];

#pragma aux SetStack = \
    "mov  ss,ax"       \
    "mov  esp,edx" parm[ax][edx] modify[eax edx];

/*
 * Sample rate to FREQ[1:0] encoding for the YMZ263.
 * FREQ=0 -> 44.1 kHz, FREQ=1 -> 22.05 kHz,
 * FREQ=2 -> 11.025 kHz, FREQ=3 -> 7.35 kHz
 */
typedef struct
{
    unsigned sample_rate;
    unsigned char freq_code;
} GOLD_SAMPLE_RATE_MAP;

static const GOLD_SAMPLE_RATE_MAP GOLD_SampleRateMap[] =
    {
        { 44100, 0 },
        { 22050, 1 },
        { 11025, 2 },
        {  7350, 3 }
    };

/* FIFO interrupt level encodings for YMZ263 register 0x0C. */
/* Interrupt fires when remaining FIFO bytes hit the threshold. */
static const unsigned char GOLD_FIFO_INT_LEVELS[] =
    {
        0x00, /* 112 bytes */
        0x01, /*  96 bytes */
        0x02, /*  80 bytes */
        0x03, /*  64 bytes  */
        0x04, /*  48 bytes  */
        0x05, /*  32 bytes  */
        0x06, /*  16 bytes  */
        /* 0x07 is prohibited */
    };

/*
 * 470ns delay between MMA register accesses.
 * Two short jumps provide ~470ns on a 20 MHz 386.
 */
static void GOLD_MMA_Delay(void)
{
    _asm nop
    _asm nop
}

/* ------------------------------------------------------------------ */
/*  Control chip access (YMZ294 / custom VLSI)                        */
/* ------------------------------------------------------------------ */

/* Enter control chip mode: write 0xFF to FM bank 1 address port. */
static void GOLD_CtrlEnter(void)
{
    outp(GOLD_Config.Address + GOLD_CT_ADDR_OFFSET, 0xFF);
    GOLD_MMA_Delay();
}

/* Exit control chip mode: write 0xFE to FM bank 1 address port. */
static void GOLD_CtrlExit(void)
{
    outp(GOLD_Config.Address + GOLD_CT_ADDR_OFFSET, 0xFE);
    GOLD_MMA_Delay();
}

/* Poll status register until both RB and SB bits clear. */
static int GOLD_CtrlWait(void)
{
    unsigned timeout = 100000;

    do
    {
        unsigned char status = inp(GOLD_Config.Address + GOLD_CT_ADDR_OFFSET);

        if (!(status & (GOLD_CT_RB_BIT | GOLD_CT_SB_BIT)))
        {
            return GOLD_Ok;
        }

        timeout--;
    }
    while (timeout != 0);

    return GOLD_CardNotReady;
}

/* Write a control chip register. */
static int GOLD_CtrlWrite(unsigned char addr, unsigned char data)
{
    int status;

    status = GOLD_CtrlWait();
    if (status != GOLD_Ok)
    {
        return status;
    }

    outp(GOLD_Config.Address + GOLD_CT_ADDR_OFFSET, addr);
    GOLD_MMA_Delay();
    outp(GOLD_Config.Address + GOLD_CT_DATA_OFFSET, data);

    /* After writing registers 4-8, need ~450us delay; 9-16 need ~5us.
     * Polling SB bit handles this automatically. */
    return GOLD_CtrlWait();
}

/* Read a control chip register. */
static int GOLD_CtrlRead(unsigned char addr, unsigned char *data)
{
    if (data == NULL)
    {
        return GOLD_InvalidParameter;
    }

    outp(GOLD_Config.Address + GOLD_CT_ADDR_OFFSET, addr);
    GOLD_MMA_Delay();
    *data = inp(GOLD_Config.Address + GOLD_CT_DATA_OFFSET);

    return GOLD_Ok;
}

/* ------------------------------------------------------------------ */
/*  MMA (YMZ263) chip access                                          */
/* ------------------------------------------------------------------ */

/*
 * Accessing a MMA register is a two-step process:
 * 1) Write register index to the register select port (38C for ch0, 38E for ch1)
 * 2) Read/write data at the channel port (38D for ch0, 38F for ch1)
 *
 * A 470ns delay is required between accesses.
 */

/* Read MMA status register (same as channel 0 register select port). */
static unsigned char GOLD_MMAReadStatus(void)
{
    return inp(GOLD_Config.Address + GOLD_MMA0_ADDR_OFFSET);
}

/* Write a MMA register on the specified channel. */
static void GOLD_MMAWrite(unsigned char channel, unsigned char reg, unsigned char data)
{
    unsigned base;

    if (channel == 0)
    {
        base = GOLD_Config.Address + GOLD_MMA0_ADDR_OFFSET;
    }
    else
    {
        base = GOLD_Config.Address + GOLD_MMA1_ADDR_OFFSET;
    }

    outp(base, reg);
    GOLD_MMA_Delay();
    outp(base + 1, data);
    GOLD_MMA_Delay();
}

/* Read a MMA register on the specified channel. */
static unsigned char GOLD_MMARead(unsigned char channel, unsigned char reg)
{
    unsigned base;
    unsigned char data;

    if (channel == 0)
    {
        base = GOLD_Config.Address + GOLD_MMA0_ADDR_OFFSET;
    }
    else
    {
        base = GOLD_Config.Address + GOLD_MMA1_ADDR_OFFSET;
    }

    outp(base, reg);
    GOLD_MMA_Delay();
    data = inp(base + 1);
    GOLD_MMA_Delay();

    return data;
}

/* Write PCM data into the MMA FIFO on the specified channel. */
static void GOLD_MMAPutData(unsigned char channel, unsigned char data)
{
    unsigned base;

    if (channel == 0)
    {
        base = GOLD_Config.Address + GOLD_MMA0_ADDR_OFFSET;
    }
    else
    {
        base = GOLD_Config.Address + GOLD_MMA1_ADDR_OFFSET;
    }

    /* Ensure register 0x0B (PCM DATA) is selected, then write. */
    outp(base, GOLD_MMA_PCM_DATA);
    GOLD_MMA_Delay();
    outp(base + 1, data);
    GOLD_MMA_Delay();
}

/* Read PCM data from the MMA FIFO on the specified channel. */
static unsigned char GOLD_MMAGetData(unsigned char channel)
{
    unsigned base;
    unsigned char data;

    if (channel == 0)
    {
        base = GOLD_Config.Address + GOLD_MMA0_ADDR_OFFSET;
    }
    else
    {
        base = GOLD_Config.Address + GOLD_MMA1_ADDR_OFFSET;
    }

    outp(base, GOLD_MMA_PCM_DATA);
    GOLD_MMA_Delay();
    data = inp(base + 1);
    GOLD_MMA_Delay();

    return data;
}

/*
 * Reset a MMA channel. This clears the FIFO buffers and resets FIFO flags.
 * Sequence: write 0x80 (RST) to register 0x09, then write desired config.
 */
static void GOLD_MMAResetChannel(unsigned char channel)
{
    unsigned char reg_val;

    /* First stop any playback/recording on this channel */
    GOLD_MMAWrite(channel, GOLD_MMA_PLAY_REC_CTL, 0x00);

    /* Write RST bit to reset the channel */
    GOLD_MMAWrite(channel, GOLD_MMA_PLAY_REC_CTL, GOLD_MMA_RST_BIT);

    /* Wait a brief moment for the reset to complete */
    {
        unsigned i;
        for (i = 0; i < 100; i++)
        {
            GOLD_MMA_Delay();
        }
    }

    /* Clear RST bit - write 0 to finish reset sequence */
    GOLD_MMAWrite(channel, GOLD_MMA_PLAY_REC_CTL, 0x00);
    GOLD_MMA_Delay();
}

/* Find the best matching sample rate configuration. */
static const GOLD_SAMPLE_RATE_MAP *GOLD_GetSampleRateConfig(unsigned sample_rate)
{
    int i;
    int num_rates = sizeof(GOLD_SampleRateMap) / sizeof(GOLD_SAMPLE_RATE_MAP);
    unsigned best_diff = 0xFFFFFFFF;
    int best_idx = 0;

    for (i = 0; i < num_rates; i++)
    {
        unsigned diff;

        if (GOLD_SampleRateMap[i].sample_rate >= sample_rate)
        {
            diff = GOLD_SampleRateMap[i].sample_rate - sample_rate;
        }
        else
        {
            diff = sample_rate - GOLD_SampleRateMap[i].sample_rate;
        }

        if (diff < best_diff)
        {
            best_diff = diff;
            best_idx = i;
        }
    }

    return &GOLD_SampleRateMap[best_idx];
}

/* Convert IRQ number to INT SEL encoding for control chip. */
static int GOLD_IrqToIntSel(unsigned irq)
{
    switch (irq)
    {
        case 3:  return 0;
        case 4:  return 1;
        case 5:  return 2;
        case 7:  return 3;
        case 10: return 4;
        case 11: return 5;
        case 12: return 6;
        case 15: return 7;
        default: return -1;
    }
}

/* ------------------------------------------------------------------ */
/*  IRQ handling                                                      */
/* ------------------------------------------------------------------ */

static void GOLD_EnableInterrupt(void)
{
    int irq = GOLD_Config.Interrupt;
    int mask;

    if (irq < 8)
    {
        mask = inp(0x21) & ~(1 << irq);
        outp(0x21, mask);
    }
    else
    {
        mask = inp(0xA1) & ~(1 << (irq - 8));
        outp(0xA1, mask);

        mask = inp(0x21) & ~(1 << 2);
        outp(0x21, mask);
    }
}

static void GOLD_DisableInterrupt(void)
{
    int irq = GOLD_Config.Interrupt;
    int mask;

    if (irq < 8)
    {
        mask = inp(0x21) & ~(1 << irq);
        mask |= GOLD_IntController1Mask & (1 << irq);
        outp(0x21, mask);
    }
    else
    {
        mask = inp(0x21) & ~(1 << 2);
        mask |= GOLD_IntController1Mask & (1 << 2);
        outp(0x21, mask);

        mask = inp(0xA1) & ~(1 << (irq - 8));
        mask |= GOLD_IntController2Mask & (1 << (irq - 8));
        outp(0xA1, mask);
    }
}

void(*GOLD_CallBack)(void);

static char *GOLD_DMABuffer;
static char *GOLD_DMABufferEnd;
static char *GOLD_CurrentDMABuffer;
static int GOLD_TotalDMABufferSize;

/*
 * Interrupt service routine for the Gold card.
 *
 * The YMZ263 has a 128-byte FIFO per channel. In DMA mode, the FIFO
 * triggers an interrupt when the remaining bytes drop below the threshold
 * set by FIFO_INT in register 0x0C. The ISR refills the DMA transfer
 * by advancing the current buffer pointer.
 */
void __interrupt __far GOLD_ServiceInterrupt(void)
{
    unsigned char status;

    GetStack(&oldStackSelector, &oldStackPointer);
    SetStack(StackSelector, StackPointer);

    /* Read MMA status to check for FIFO interrupt */
    status = GOLD_MMAReadStatus();

    /* Check if this is a FIFO interrupt from channel 0 (FIFO bit).
     * FIFO (bit 1) corresponds to channel 0. */
    if (!(status & GOLD_MMA_STATUS_FIFO_BIT))
    {
        /* Not our interrupt - chain to the old handler */
        SetStack(oldStackSelector, oldStackPointer);
        _chain_intr(GOLD_OldInt);
        return;
    }

    /*
     * In DMA auto-init mode, the DMA controller automatically restarts
     * transfers. We just need to update our tracking pointer.
     */
    GOLD_CurrentDMABuffer += GOLD_TransferLength;
    if (GOLD_CurrentDMABuffer >= GOLD_DMABufferEnd)
    {
        GOLD_CurrentDMABuffer = GOLD_DMABuffer;
    }

    /* Call the mixing callback to prepare next buffer section */
    if (GOLD_CallBack != NULL)
    {
        MV_ServiceVoc();
    }

    SetStack(oldStackSelector, oldStackPointer);

    /* Send EOI to PIC(s) */
    if (GOLD_Config.Interrupt > 7)
    {
        outp(0xA0, 0x20);
    }
    outp(0x20, 0x20);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

int GOLD_GetEnv(GOLD_CONFIG *Config)
{
    char *gold_env;
    int parsed;
    unsigned int base;
    unsigned int irq;
    unsigned int dma;

    gold_env = getenv("GOLD");
    if (gold_env == NULL)
    {
        return GOLD_EnvNotFound;
    }

    parsed = sscanf(gold_env, "%x,%u,%u", &base, &irq, &dma);
    if (parsed != 3)
    {
        return GOLD_EnvNotFound;
    }

    if (Config != NULL)
    {
        Config->Address = base;
        Config->Interrupt = irq;
        Config->Dma = dma;
    }

    return GOLD_Ok;
}

int GOLD_SetCardSettings(GOLD_CONFIG Config)
{
    if (Config.Address == 0 || Config.Interrupt == 0 ||
        Config.Interrupt > 15 || Config.Dma > 3)
    {
        return GOLD_InvalidParameter;
    }

    GOLD_Config.Address = Config.Address;
    GOLD_Config.Interrupt = Config.Interrupt;
    GOLD_Config.Dma = Config.Dma;

    return GOLD_Ok;
}

int GOLD_GetCardSettings(GOLD_CONFIG *Config)
{
    if (Config == NULL)
    {
        return GOLD_InvalidParameter;
    }

    *Config = GOLD_Config;
    return GOLD_Ok;
}

void GOLD_SetPlaybackRate(unsigned rate)
{
    if (rate < GOLD_MinSamplingRate)
    {
        rate = GOLD_MinSamplingRate;
    }
    if (rate > GOLD_MaxSamplingRate)
    {
        rate = GOLD_MaxSamplingRate;
    }

    GOLD_SampleRate = rate;
}

unsigned GOLD_GetPlaybackRate(void)
{
    return GOLD_SampleRate;
}

int GOLD_SetMixMode(int mode)
{
    /* The YMZ263 uses interleaved stereo (ILV mode).
     * We support mono and interleaved stereo in 8-bit format. */
    if (mode > GOLD_MONO_8BIT)
    {
        mode = GOLD_MONO_8BIT;
    }

    if (mode & GOLD_STEREO)
    {
        GOLD_SamplePacketSize = GOLD_STEREO_8BIT_SAMPLE_SIZE;
    }
    else
    {
        GOLD_SamplePacketSize = GOLD_MONO_8BIT_SAMPLE_SIZE;
    }

    return GOLD_Ok;
}

/*
 * Set the MMA output volume for the playback channel(s).
 * Register 0x0A: 0 = minimum volume, 0xFF = maximum volume.
 */
int GOLD_SetPCMVolume(int volume)
{
    if (volume < 0)
    {
        volume = 0;
    }
    if (volume > 255)
    {
        volume = 255;
    }

    GOLD_MMAVolume = volume;

    /* Map volume to MMA range (0=silent, 255=max). */
    GOLD_MMAWrite(0, GOLD_MMA_VOLUME, (unsigned char)volume);
    GOLD_MMAWrite(1, GOLD_MMA_VOLUME, (unsigned char)volume);

    return GOLD_Ok;
}

int GOLD_GetCardInfo(int *MaxSampleBits, int *MaxChannels)
{
    if (MaxSampleBits != NULL)
    {
        *MaxSampleBits = 8;
    }
    if (MaxChannels != NULL)
    {
        *MaxChannels = 2;
    }
    return GOLD_Ok;
}

void GOLD_StopPlayback(void)
{
    if (!GOLD_Installed)
    {
        return;
    }

    GOLD_SoundPlaying = FALSE;

    /* Stop playback on channel 0 by clearing GO bit. */
    GOLD_MMAWrite(0, GOLD_MMA_PLAY_REC_CTL,
        GOLD_MMA_R_BIT | GOLD_MMA_PR_BIT | GOLD_MMA_PCM_BIT);

    /* Disable DMA mode on channel 0. */
    GOLD_MMAWrite(0, GOLD_MMA_FMT_CTL, GOLD_MMA_FIFO_INT_64);

    /* Reset the channel to clear FIFO. */
    GOLD_MMAResetChannel(0);

    DMA_EndTransfer(GOLD_DMAChannel);
    GOLD_DisableInterrupt();
}

/*
 * Begin buffered digital audio playback using DMA.
 *
 * Uses MMA channel 0 with DMA auto-init mode. The YMZ263 FIFO
 * is set to trigger interrupts at the specified FIFO_INT level.
 * Data format is 8-bit (most significant byte of 12-bit sample).
 */
int GOLD_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions,
                               unsigned SampleRate, int MixMode,
                               void(*CallBackFunc)(void))
{
    const GOLD_SAMPLE_RATE_MAP *rate_cfg;
    int status;
    int dma_channel;
    int int_sel;
    unsigned char play_rec_ctl;
    unsigned char fmt_ctl;

    rate_cfg = GOLD_GetSampleRateConfig(SampleRate);
    if (rate_cfg == NULL)
    {
        rate_cfg = &GOLD_SampleRateMap[2]; /* Default to 11.025 kHz */
        SampleRate = 11025;
    }

    GOLD_SampleRate = SampleRate;
    GOLD_CallBack = CallBackFunc;

    if (MixMode & GOLD_STEREO)
    {
        GOLD_SamplePacketSize = GOLD_STEREO_8BIT_SAMPLE_SIZE;
    }
    else
    {
        GOLD_SamplePacketSize = GOLD_MONO_8BIT_SAMPLE_SIZE;
    }

    GOLD_DMAChannel = GOLD_Config.Dma;
    dma_channel = GOLD_Config.Dma;

    /* Calculate transfer length per division (FIFO interrupt boundary).
     * The YMZ263 FIFO is 128 bytes. We want interrupts at 64-byte
     * remaining threshold, meaning DMA should transfer 64 bytes at a
     * time for optimal performance. However, to match FastDoom's
     * buffer division scheme, we use the NumDivisions parameter. */
    GOLD_TransferLength = BufferSize / NumDivisions;

    /*
     * Step 1: Reset the MMA channel 0.
     */
    GOLD_MMAResetChannel(0);

    /*
     * Step 2: Configure control chip - set IRQ and DMA.
     */
    GOLD_CtrlEnter();

    int_sel = GOLD_IrqToIntSel(GOLD_Config.Interrupt);
    if (int_sel < 0)
    {
        GOLD_CtrlExit();
        return GOLD_InvalidIrq;
    }

    /* Register 0x13: Audio IRQ/DMA Select - Channel 0
     * D7: DENO (DMA enable), D6-D4: DMA SEL 0,
     * D3: AEN (Audio enable), D2-D0: INT SEL A */
    status = GOLD_CtrlWrite(0x13,
        GOLD_CT_DENO_BIT |
        ((dma_channel & 0x07) << GOLD_CT_DMA_SEL_SHIFT) |
        GOLD_CT_AEN_BIT |
        (int_sel & 0x07));
    if (status != GOLD_Ok)
    {
        GOLD_CtrlExit();
        return status;
    }

    /* Register 0x14: DMA Select - Channel 1 (leave disabled). */
    status = GOLD_CtrlWrite(0x14, 0x00);
    if (status != GOLD_Ok)
    {
        GOLD_CtrlExit();
        return status;
    }

    GOLD_CtrlExit();

    /*
     * Step 3: Set MMA channel 0 volume.
     */
    GOLD_MMAWrite(0, GOLD_MMA_VOLUME, (unsigned char)GOLD_MMAVolume);
    GOLD_MMAWrite(1, GOLD_MMA_VOLUME, (unsigned char)GOLD_MMAVolume);

    /*
     * Step 4: Configure playback/recording control (register 0x09).
     *
     * Bits: RST | R | L | FREQ[1:0] | PCM | PR | GO
     *
     * - R=1, L=1: enable both left and right output
     * - FREQ: sample rate select
     * - PCM=1: PCM mode (uncompressed)
     * - PR=1: playback mode
     * - GO=0 initially (start after FIFO is filled)
     */
    play_rec_ctl = GOLD_MMA_R_BIT | GOLD_MMA_L_BIT |
        (rate_cfg->freq_code & 0x03) << GOLD_MMA_FREQ_SHIFT |
        GOLD_MMA_PCM_BIT | GOLD_MMA_PR_BIT;

    GOLD_MMAWrite(0, GOLD_MMA_PLAY_REC_CTL, play_rec_ctl);

    /*
     * Step 5: Configure format and DMA control (register 0x0C).
     *
     * Bits: ILV | DATA_FMT[1:0] | FIFO_INT[3:0] | MSK | ENB
     *
     * - ILV=0: no interleaving (channel 0 only)
     * - DATA_FMT=0: 8-bit format (8 MSBs of 12-bit sample)
     * - FIFO_INT: set to 64-byte threshold (value 3)
     * - MSK=0: enable FIFO interrupt
     * - ENB=1: enable DMA mode
     */
    fmt_ctl = GOLD_MMA_DATA_FMT_8BIT |
        GOLD_MMA_FIFO_INT_64 |
        GOLD_MMA_DMA_ENB_BIT;

    GOLD_MMAWrite(0, GOLD_MMA_FMT_CTL, fmt_ctl);

    /*
     * Step 6: Set up DMA channel for auto-init read (memory to device).
     * DMA transfers data from system memory to the MMA FIFO.
     * The YMZ263 uses DMA request line DRQ0 for channel 0.
     */
    status = DMA_SetupTransfer(dma_channel, BufferStart, BufferSize,
                               DMA_AutoInitRead);
    if (status != DMA_Ok)
    {
        return GOLD_DmaError;
    }

    /*
     * Step 7: Fill the FIFO before starting playback.
     * The MMA FIFO should never be empty when GO is set.
     * We pre-fill it with enough data.
     */
    {
        unsigned int fill_count = 128; /* Fill to near the top of the 128-byte FIFO */
        unsigned int i;
        unsigned char *fill_ptr = (unsigned char *)BufferStart;

        for (i = 0; i < fill_count && i < (unsigned int)BufferSize; i++)
        {
            GOLD_MMAPutData(0, fill_ptr[i]);
        }
    }

    /*
     * Step 8: Start playback by setting the GO bit.
     */
    GOLD_MMAWrite(0, GOLD_MMA_PLAY_REC_CTL, play_rec_ctl | GOLD_MMA_GO_BIT);

    /*
     * Step 9: Enable the IRQ line.
     */
    GOLD_EnableInterrupt();

    /* Save buffer tracking state. */
    GOLD_DMABuffer = BufferStart;
    GOLD_DMABufferEnd = BufferStart + BufferSize;
    GOLD_CurrentDMABuffer = BufferStart;
    GOLD_TotalDMABufferSize = BufferSize;

    GOLD_SoundPlaying = TRUE;

    return GOLD_Ok;
}

/* ------------------------------------------------------------------ */
/*  Detection                                                         */
/* ------------------------------------------------------------------ */

static int GOLD_Detect(void)
{
    unsigned char id_byte;
    int status;

    /* Enter control chip mode. */
    GOLD_CtrlEnter();

    /* Read register 0 to get model ID.
     * Lower 2 bits are MODEL ID: 0=Gold 2000, 1=Gold 1000, 2=Gold 2000MC */
    status = GOLD_CtrlRead(0x00, &id_byte);
    if (status != GOLD_Ok)
    {
        GOLD_CtrlExit();
        return GOLD_Error;
    }

    GOLD_CtrlExit();

    /* Check if lower 2 bits are a valid model ID (0, 1, or 2). */
    if ((id_byte & 0x03) > 2)
    {
        return GOLD_Error;
    }

    return GOLD_Ok;
}

/* ------------------------------------------------------------------ */
/*  Initialization / Shutdown                                         */
/* ------------------------------------------------------------------ */

int GOLD_Init(void)
{
    GOLD_CONFIG config;
    int status;

    if (GOLD_Installed)
    {
        GOLD_Shutdown();
    }

    /* Try to get configuration from environment variable. */
    status = GOLD_GetEnv(&config);
    if (status == GOLD_Ok)
    {
        GOLD_Config.Address = config.Address;
        GOLD_Config.Interrupt = config.Interrupt;
        GOLD_Config.Dma = config.Dma;
    }
    else
    {
        /* Fall back to defaults. */
        GOLD_Config.Address = GOLD_DEFAULT_BASE;
        GOLD_Config.Interrupt = GOLD_DEFAULT_IRQ;
        GOLD_Config.Dma = GOLD_DEFAULT_DMA;
    }

    /* Clamp DMA channel (Gold 1000 supports only DMA 1-3). */
    if (GOLD_Config.Dma > 3)
    {
        GOLD_Config.Dma = GOLD_DEFAULT_DMA;
    }

    /* Map IRQ to interrupt vector. */
    if (GOLD_Config.Interrupt < 8)
    {
        GOLD_InterruptVector = 0x08 + GOLD_Config.Interrupt;
    }
    else
    {
        GOLD_InterruptVector = 0x70 + (GOLD_Config.Interrupt - 8);
    }

    /* Verify card presence. */
    status = GOLD_Detect();
    if (status != GOLD_Ok)
    {
        return GOLD_Error;
    }

    /* Save current PIC masks for restoration later. */
    if (GOLD_Config.Interrupt < 8)
    {
        GOLD_IntController1Mask = inp(0x21);
    }
    else
    {
        GOLD_IntController1Mask = inp(0x21);
        GOLD_IntController2Mask = inp(0xA1);
    }

    /* Save old interrupt vector and install ours. */
    GOLD_OldInt = _dos_getvect(GOLD_InterruptVector);
    _dos_setvect(GOLD_InterruptVector, GOLD_ServiceInterrupt);

    /* Allocate DPMI stack for interrupt handler. */
    {
        union REGS regs;
        memset(&regs, 0, sizeof(regs));
        regs.w.ax = 0x100;
        regs.w.bx = (kStackSize + 15) / 16;
        int386(0x31, &regs, &regs);
        if (regs.w.cflag)
        {
            _dos_setvect(GOLD_InterruptVector, GOLD_OldInt);
            return GOLD_DPMI_Error;
        }
        StackSelector = regs.w.dx;
        StackPointer = kStackSize - sizeof(long);
    }

    /* Default volume. */
    GOLD_MMAVolume = 0xFF;

    GOLD_Installed = TRUE;

    return GOLD_Ok;
}

void GOLD_Shutdown(void)
{
    if (!GOLD_Installed)
    {
        return;
    }

    if (GOLD_SoundPlaying)
    {
        GOLD_StopPlayback();
    }

    /* Restore PIC masks. */
    {
        int irq = GOLD_Config.Interrupt;
        if (irq < 8)
        {
            outp(0x21, GOLD_IntController1Mask);
        }
        else
        {
            outp(0xA1, GOLD_IntController2Mask);
            outp(0x21, GOLD_IntController1Mask);
        }
    }

    /* Restore old interrupt vector. */
    _dos_setvect(GOLD_InterruptVector, GOLD_OldInt);

    /* Free DPMI stack. */
    if (StackSelector != 0)
    {
        union REGS regs;
        memset(&regs, 0, sizeof(regs));
        regs.w.ax = 0x101;
        regs.w.dx = StackSelector;
        int386(0x31, &regs, &regs);
        StackPointer = 0;
        StackSelector = 0;
    }

    GOLD_Installed = FALSE;
    GOLD_SoundPlaying = FALSE;
}
