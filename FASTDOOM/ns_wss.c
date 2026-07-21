//
// WSS (Windows Sound System) driver for FastDoom
// Based on AD1848-compatible sound cards
//
// Uses ULTRA16 environment variable for configuration:
//   SET ULTRA16=<I/O hex>,<IRQ dec>,<DMA dec>
//   Example: SET ULTRA16=530,5,1
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
#include "ns_wss.h"
#include "ns_wssdef.h"
#include "ns_muldf.h"
#include "options.h"
#include "fastmath.h"

#define INVALID -1

/* Debug: set to 0 to silence debug output */
#define WSS_DEBUG 1
#define WSS_DEBUG_FILE "WSS.LOG"

static FILE *WSS_DebugFile = NULL;

static void WSS_DebugOpen(void)
{
    if (WSS_DebugFile == NULL)
    {
        WSS_DebugFile = fopen(WSS_DEBUG_FILE, "w");
    }
}

static void WSS_DebugClose(void)
{
    if (WSS_DebugFile != NULL)
    {
        fclose(WSS_DebugFile);
        WSS_DebugFile = NULL;
    }
}

#define WSS_LOG(fmt, ...) \
    do { \
        if (WSS_DEBUG && WSS_DebugFile != NULL) { \
            fprintf(WSS_DebugFile, "WSS: " fmt, ##__VA_ARGS__); \
        } \
    } while (0)

static const unsigned char WSS_Interrupts[WSS_MaxIrq + 1] =
    {
        INVALID, INVALID, 0xa, 0xb,
        INVALID, 0xd, INVALID, 0xf,
        INVALID, INVALID, 0x72, 0x73,
        0x74, INVALID, INVALID, 0x77
    };

static void(__interrupt __far *WSS_OldInt)(void);

WSS_CONFIG WSS_Config =
    {
        WSS_DEFAULT_BASE, WSS_DEFAULT_IRQ, WSS_DEFAULT_DMA
    };

static int WSS_Installed = FALSE;
static int WSS_TransferLength = 0;
static int WSS_MixMode = WSS_DefaultMixMode;
static int WSS_SamplePacketSize = WSS_MONO_8BIT_SAMPLE_SIZE;
unsigned WSS_SampleRate = WSS_DefaultSampleRate;

volatile int WSS_SoundPlaying;
int WSS_DMAChannel;

static int WSS_IntController1Mask;
static int WSS_IntController2Mask;

static int WSS_OriginalPCMLeftVolume = 127;
static int WSS_OriginalPCMRightVolume = 127;

void (*WSS_CallBack)(void);

static char *WSS_DMABuffer;
static char *WSS_DMABufferEnd;
static char *WSS_CurrentDMABuffer;
static int WSS_TotalDMABufferSize;
static int WSS_NumDivisions;

static int WSS_PCMVolume = 255;
static int WSS_InterruptVector;

// adequate stack size
#define kStackSize 2048

static unsigned short StackSelector = 0;
static unsigned long StackPointer;

static unsigned short oldStackSelector;
static unsigned long oldStackPointer;

/* Stack switching helpers for interrupt handler */
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
 * WSS timeout loop count for waiting on card ready
 */
#define WSS_TIMEOUT_LOOPS 100000

/*
 * Sample rate to clock configuration mapping
 */
typedef struct
{
    unsigned sample_rate;
    unsigned char cfs;
    unsigned char css;
} WSS_SAMPLE_RATE_MAP;

static const WSS_SAMPLE_RATE_MAP WSS_SampleRateMap[] =
    {
        { 8000,   0,                     0              },
        {11025,   WSS_CSS_BIT,           WSS_CFS0_BIT   },
        {16000,   0,                     WSS_CFS0_BIT   },
        {22050,   WSS_CSS_BIT,           WSS_CFS1_BIT | WSS_CFS0_BIT },
        {32000,   0,                     WSS_CFS1_BIT | WSS_CFS0_BIT },
        {44100,   WSS_CSS_BIT,           WSS_CFS2_BIT | WSS_CFS0_BIT },
        {48000,   0,                     WSS_CFS2_BIT | WSS_CFS1_BIT  }
    };

static const WSS_SAMPLE_RATE_MAP *WSS_GetSampleRateConfig(unsigned sample_rate)
{
    int i;
    int num_rates = sizeof(WSS_SampleRateMap) / sizeof(WSS_SAMPLE_RATE_MAP);

    for (i = 0; i < num_rates; i++)
    {
        if (WSS_SampleRateMap[i].sample_rate == sample_rate)
        {
            return &WSS_SampleRateMap[i];
        }
    }
    return NULL;
}

/*
 * Wait until WSS card is ready (INIT bit clears)
 * Returns WSS_Ok on success, WSS_CardNotReady on timeout
 */
static int WSS_Wait(void)
{
    unsigned timeout = WSS_TIMEOUT_LOOPS;

    while ((inp(WSS_Config.Address + WSS_INDEX_REG_OFFSET) & WSS_INIT_BIT) != 0)
    {
        if (timeout == 0)
        {
#if WSS_DEBUG
            WSS_LOG("TIMEOUT waiting for card ready at 0x%03x!\n", WSS_Config.Address);
#endif
            return WSS_CardNotReady;
        }
        timeout--;
    }
    return WSS_Ok;
}

/*
 * Read from an indirect WSS register
 */
static int WSS_ReadIndirect(unsigned char addr, unsigned char *data)
{
    if (data == NULL)
    {
        return WSS_InvalidParameter;
    }

    outp(WSS_Config.Address + WSS_INDEX_REG_OFFSET, addr);
    *data = inp(WSS_Config.Address + WSS_IDATA_REG_OFFSET);

#if WSS_DEBUG
    WSS_LOG("READ reg 0x%02x = 0x%02x\n", addr, *data);
#endif

    return WSS_Ok;
}

/*
 * Write to an indirect WSS register
 */
static int WSS_WriteIndirect(unsigned char addr, unsigned char data)
{
    int status;

    outp(WSS_Config.Address + WSS_INDEX_REG_OFFSET, addr);
    status = WSS_Wait();
    if (status != WSS_Ok)
    {
        return status;
    }

    outp(WSS_Config.Address + WSS_IDATA_REG_OFFSET, data);
    status = WSS_Wait();

#if WSS_DEBUG
    WSS_LOG("WRITE reg 0x%02x = 0x%02x (status=%d)\n", addr, data, status);
#endif

    return status;
}

/*
 * WSS_EnableInterrupt
 * Unmask the WSS interrupt in the PIC
 */
static void WSS_EnableInterrupt(void)
{
    int irq = WSS_Config.Interrupt;
    int mask;

    if (irq < 8)
    {
        mask = inp(0x21) & ~(1 << irq);
        outp(0x21, mask);
#if WSS_DEBUG
        WSS_LOG("IRQ %d enabled in PIC1 (PIC1=0x%02x)\n", irq, inp(0x21));
#endif
    }
    else
    {
        mask = inp(0xA1) & ~(1 << (irq - 8));
        outp(0xA1, mask);

        mask = inp(0x21) & ~(1 << 2);
        outp(0x21, mask);
#if WSS_DEBUG
        WSS_LOG("IRQ %d enabled in PIC2 (PIC2=0x%02x, PIC1=0x%02x)\n", irq, inp(0xA1), inp(0x21));
#endif
    }
}

/*
 * WSS_DisableInterrupt
 * Re-mask the WSS interrupt in the PIC
 */
static void WSS_DisableInterrupt(void)
{
    int irq = WSS_Config.Interrupt;
    int mask;

    if (irq < 8)
    {
        mask = inp(0x21) & ~(1 << irq);
        mask |= WSS_IntController1Mask & (1 << irq);
        outp(0x21, mask);
#if WSS_DEBUG
        WSS_LOG("IRQ %d disabled in PIC1 (PIC1=0x%02x)\n", irq, inp(0x21));
#endif
    }
    else
    {
        mask = inp(0x21) & ~(1 << 2);
        mask |= WSS_IntController1Mask & (1 << 2);
        outp(0x21, mask);

        mask = inp(0xA1) & ~(1 << (irq - 8));
        mask |= WSS_IntController2Mask & (1 << (irq - 8));
        outp(0xA1, mask);
#if WSS_DEBUG
        WSS_LOG("IRQ %d disabled in PIC2 (PIC2=0x%02x, PIC1=0x%02x)\n", irq, inp(0xA1), inp(0x21));
#endif
    }
}

/*
 * WSS_ServiceInterrupt
 * ISR for WSS DMA buffer completion interrupts
 */
void __interrupt __far WSS_ServiceInterrupt(void)
{
    /* Save current stack */
    GetStack(&oldStackSelector, &oldStackPointer);

    /* Switch to our stack */
    SetStack(StackSelector, StackPointer);

    /* Clear WSS interrupt status register */
    {
        unsigned char status = inp(WSS_Config.Address + WSS_STATUS_REG_OFFSET);
        outp(WSS_Config.Address + WSS_STATUS_REG_OFFSET, 0x00);
    }

    /* Track current DMA buffer position */
    WSS_CurrentDMABuffer += WSS_TransferLength;
    if (WSS_CurrentDMABuffer >= WSS_DMABufferEnd)
    {
        WSS_CurrentDMABuffer = WSS_DMABuffer;
    }

    /* Call mixer callback to refill the buffer */
    if (WSS_CallBack != NULL)
    {
        MV_ServiceVoc();
    }

    /* Restore original stack */
    SetStack(oldStackSelector, oldStackPointer);

    /* Send EOI to the PIC */
    if (WSS_Config.Interrupt > 7)
    {
        outp(0xA0, 0x20);  /* Slave PIC EOI */
    }
    outp(0x20, 0x20);      /* Master PIC EOI */
}

/*
 * WSS_GetEnv
 * Parse ULTRA16 environment variable for card configuration
 */
int WSS_GetEnv(WSS_CONFIG *Config)
{
    char *ultra16;
    int parsed;
    unsigned int base;
    unsigned int irq;
    unsigned int dma;

#if WSS_DEBUG
    WSS_LOG("Looking for ULTRA16 environment variable...\n");
#endif

    ultra16 = getenv("ULTRA16");
    if (ultra16 == NULL)
    {
#if WSS_DEBUG
        WSS_LOG("ULTRA16 not found in environment\n");
#endif
        return WSS_EnvNotFound;
    }

    parsed = sscanf(ultra16, "%x,%u,%u", &base, &irq, &dma);
    if (parsed != 3)
    {
#if WSS_DEBUG
        WSS_LOG("ULTRA16='%s' (bad format, parsed %d fields)\n", ultra16, parsed);
#endif
        return WSS_EnvNotFound;
    }

#if WSS_DEBUG
    WSS_LOG("ULTRA16='%s' -> base=0x%03x irq=%u dma=%u\n", ultra16, base, irq, dma);
#endif

    if (Config != NULL)
    {
        Config->Address = base;
        Config->Interrupt = irq;
        Config->Dma = dma;
    }

    return WSS_Ok;
}

/*
 * WSS_SetCardSettings
 * Apply the given configuration
 */
int WSS_SetCardSettings(WSS_CONFIG Config)
{
    if (Config.Address == 0 || Config.Interrupt == 0 || Config.Interrupt > 15 ||
        Config.Dma > 3)
    {
        return WSS_InvalidParameter;
    }

    WSS_Config.Address = Config.Address;
    WSS_Config.Interrupt = Config.Interrupt;
    WSS_Config.Dma = Config.Dma;

    return WSS_Ok;
}

/*
 * WSS_GetCardSettings
 * Get current configuration
 */
int WSS_GetCardSettings(WSS_CONFIG *Config)
{
    if (Config == NULL)
    {
        return WSS_InvalidParameter;
    }

    *Config = WSS_Config;
    return WSS_Ok;
}

/*
 * WSS_SetPlaybackRate
 * Set the DAC playback sample rate
 */
void WSS_SetPlaybackRate(unsigned rate)
{
    if (rate < WSS_MinSamplingRate)
    {
        rate = WSS_MinSamplingRate;
    }
    if (rate > WSS_MaxSamplingRate)
    {
        rate = WSS_MaxSamplingRate;
    }

    WSS_SampleRate = rate;
}

/*
 * WSS_GetPlaybackRate
 * Get the current playback sample rate
 */
unsigned WSS_GetPlaybackRate(void)
{
    return WSS_SampleRate;
}

/*
 * WSS_SetMixMode
 * Set the output mix mode (mono/stereo)
 */
int WSS_SetMixMode(int mode)
{
    /* WSS supports mono and stereo 8-bit */
    if (mode > WSS_STEREO_8BIT)
    {
        mode = WSS_STEREO_8BIT;
    }

    WSS_MixMode = mode;

    if (mode & WSS_STEREO)
    {
        WSS_SamplePacketSize = WSS_STEREO_8BIT_SAMPLE_SIZE;
    }
    else
    {
        WSS_SamplePacketSize = WSS_MONO_8BIT_SAMPLE_SIZE;
    }

#if WSS_DEBUG
    WSS_LOG("SetMixMode mode=%d packetSize=%d\n", mode, WSS_SamplePacketSize);
#endif

    return WSS_Ok;
}

/*
 * WSS_SetPCMVolume
 * Set the PCM output volume (0-255)
 */
int WSS_SetPCMVolume(int volume)
{
    unsigned char reg;
    int status;

    if (volume < 0)
    {
        volume = 0;
    }
    if (volume > 255)
    {
        volume = 255;
    }

    WSS_PCMVolume = volume;

    /* Map 0-255 to WSS attenuation register value */
    if (volume == 0)
    {
        reg = WSS_DAC_MUTE_BIT;
    }
    else
    {
        /* Scale: higher volume = lower attenuation */
        reg = (unsigned char)(((255 - volume) * WSS_DAC_ATTEN_MAX_VAL) / 255);
    }

#if WSS_DEBUG
    WSS_LOG("SetPCMVolume volume=%d -> reg=0x%02x\n", volume, reg);
#endif

    status = WSS_WriteIndirect(WSS_LDAC_REG, reg);
    if (status != WSS_Ok)
    {
        return status;
    }

    status = WSS_WriteIndirect(WSS_RDAC_REG, reg);
    return status;
}

/*
 * WSS_GetCardInfo
 * Get maximum capabilities of the WSS card
 */
int WSS_GetCardInfo(int *MaxSampleBits, int *MaxChannels)
{
    if (MaxSampleBits != NULL)
    {
        *MaxSampleBits = 8;  /* WSS driver uses 8-bit in FastDoom mixer */
    }
    if (MaxChannels != NULL)
    {
        *MaxChannels = 2;    /* Stereo */
    }
    return WSS_Ok;
}

/*
 * WSS_StopPlayback
 * Stop digital playback on the WSS card
 */
void WSS_StopPlayback(void)
{
    unsigned char reg;
    int status;

    if (!WSS_Installed)
    {
        return;
    }

#if WSS_DEBUG
    WSS_LOG("StopPlayback called\n");
#endif

    WSS_SoundPlaying = FALSE;

    /* Disable playback in config register */
    status = WSS_ReadIndirect(WSS_CONFIG_REG, &reg);
    if (status == WSS_Ok)
    {
        reg &= ~WSS_PEN_BIT;
        WSS_WriteIndirect(WSS_CONFIG_REG, reg);
    }

    /* Stop DMA transfer */
    DMA_EndTransfer(WSS_DMAChannel);

    /* Disable interrupt */
    WSS_DisableInterrupt();
}

/*
 * WSS_BeginBufferedPlayback
 * Start buffered digital audio playback via DMA
 */
int WSS_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions,
                               unsigned SampleRate, int MixMode, void (*CallBackFunc)(void))
{
    int status;
    unsigned char dma_count_lo, dma_count_hi;
    const WSS_SAMPLE_RATE_MAP *rate_cfg;
    int channels;
    int dma_transfer_length;

#if WSS_DEBUG
    WSS_LOG("BeginBufferedPlayback buf=%p size=%d div=%d rate=%u mix=%d\n",
           BufferStart, BufferSize, NumDivisions, SampleRate, MixMode);
#endif

    /* Validate sample rate */
    rate_cfg = WSS_GetSampleRateConfig(SampleRate);
    if (rate_cfg == NULL)
    {
        /* Find closest match - default to 11025 */
        rate_cfg = &WSS_SampleRateMap[1];
        SampleRate = 11025;
#if WSS_DEBUG
        WSS_LOG("Unknown rate, using 11025\n");
#endif
    }
    else
    {
#if WSS_DEBUG
        WSS_LOG("Rate %u -> cfs=0x%02x css=0x%02x\n", SampleRate, rate_cfg->cfs, rate_cfg->css);
#endif
    }

    WSS_SampleRate = SampleRate;
    WSS_MixMode = MixMode;
    WSS_CallBack = CallBackFunc;
    WSS_NumDivisions = NumDivisions;

    /* Determine sample size based on mix mode */
    channels = (MixMode & WSS_STEREO) ? 2 : 1;
    WSS_SamplePacketSize = channels;

    /* Set DMA channel */
    WSS_DMAChannel = WSS_Config.Dma;

    /* Calculate half-buffer transfer length for double buffering */
    WSS_TransferLength = BufferSize / WSS_NUM_BUFFERS;
    dma_transfer_length = WSS_TransferLength;

#if WSS_DEBUG
    WSS_LOG("DMA ch=%d halfBuffer=%d channels=%d\n",
           WSS_DMAChannel, WSS_TransferLength, channels);
#endif

    /* Configure WSS DAC volume */
    WSS_SetPCMVolume(WSS_PCMVolume);

    /* Configure AUX inputs (mute FM, CD, etc.) */
    WSS_WriteIndirect(WSS_LAUX1_REG, WSS_AUX_MUTE_BIT);
    WSS_WriteIndirect(WSS_RAUX1_REG, WSS_AUX_MUTE_BIT);
    WSS_WriteIndirect(WSS_LAUX2_REG, WSS_AUX_MUTE_BIT);
    WSS_WriteIndirect(WSS_RAUX2_REG, WSS_AUX_MUTE_BIT);

    /* Configure ADC (line input, 0dB gain) */
    WSS_WriteIndirect(WSS_LADC_REG, 0x00);
    WSS_WriteIndirect(WSS_RADC_REG, 0x00);

    /* Disable loopback */
    WSS_WriteIndirect(WSS_MIX_REG, 0x00);

    /* Enable interrupt pin */
    WSS_WriteIndirect(WSS_PIN_REG, WSS_IEN_BIT);

    /* Calculate DMA transfer count per half-buffer (minus 1 as per WSS spec) */
    {
        unsigned short dma_count = (unsigned short)(dma_transfer_length / channels) - 1;
        dma_count_lo = (unsigned char)(dma_count & 0xFF);
        dma_count_hi = (unsigned char)((dma_count >> 8) & 0xFF);
    }

#if WSS_DEBUG
    WSS_LOG("DMA count=0x%04x (lo=0x%02x hi=0x%02x)\n",
           (unsigned short)(dma_transfer_length / channels - 1), dma_count_lo, dma_count_hi);
#endif

    /* Set playback DMA count */
    status = WSS_WriteIndirect(WSS_PLAYBACK_LCNT_REG, dma_count_lo);
    if (status != WSS_Ok)
    {
#if WSS_DEBUG
        WSS_LOG("Failed to write PLAYBACK_LCNT_REG\n");
#endif
        return status;
    }
    status = WSS_WriteIndirect(WSS_PLAYBACK_UCNT_REG, dma_count_hi);
    if (status != WSS_Ok)
    {
#if WSS_DEBUG
        WSS_LOG("Failed to write PLAYBACK_UCNT_REG\n");
#endif
        return status;
    }

    /* Configure format register with MCE */
    {
        unsigned char format_reg = rate_cfg->cfs | rate_cfg->css;
        if (MixMode & WSS_STEREO)
        {
            format_reg |= WSS_S_M_BIT;
        }

#if WSS_DEBUG
        WSS_LOG("Setting FORMAT reg=0x%02x\n", format_reg);
#endif

        outp(WSS_Config.Address + WSS_INDEX_REG_OFFSET, WSS_FORMAT_REG | WSS_MCE_BIT);
        status = WSS_Wait();
        if (status != WSS_Ok)
        {
#if WSS_DEBUG
            WSS_LOG("Wait failed before writing FORMAT reg\n");
#endif
            return status;
        }
        outp(WSS_Config.Address + WSS_IDATA_REG_OFFSET, format_reg);
        status = WSS_Wait();
        if (status != WSS_Ok)
        {
#if WSS_DEBUG
            WSS_LOG("Wait failed after writing FORMAT reg\n");
#endif
            return status;
        }
    }

    /* Enable playback with auto-calibration */
    {
        unsigned char config_val = WSS_PEN_BIT | WSS_ACAL_BIT;

#if WSS_DEBUG
        WSS_LOG("Enabling playback with ACAL (config=0x%02x)\n", config_val);
#endif

        outp(WSS_Config.Address + WSS_INDEX_REG_OFFSET, WSS_CONFIG_REG | WSS_MCE_BIT);
        status = WSS_Wait();
        if (status != WSS_Ok)
        {
#if WSS_DEBUG
            WSS_LOG("Wait failed before writing CONFIG reg\n");
#endif
            return status;
        }
        outp(WSS_Config.Address + WSS_IDATA_REG_OFFSET, config_val);
        status = WSS_Wait();
        if (status != WSS_Ok)
        {
#if WSS_DEBUG
            WSS_LOG("Wait failed after writing CONFIG reg\n");
#endif
            return status;
        }
    }

    /* Wait for auto-calibration to complete */
    {
        unsigned int wait_count = 0;
#if WSS_DEBUG
        WSS_LOG("Waiting for auto-calibration...\n");
#endif

        outp(WSS_Config.Address + WSS_INDEX_REG_OFFSET, WSS_TEST_INIT_REG);
        while ((inp(WSS_Config.Address + WSS_IDATA_REG_OFFSET) & WSS_ACI_BIT) != 0)
        {
            wait_count++;
            if (wait_count > 100000)
            {
#if WSS_DEBUG
                WSS_LOG("Auto-calibration TIMEOUT!\n");
#endif
                break;
            }
        }
#if WSS_DEBUG
        WSS_LOG("Auto-calibration done (waited %u iterations)\n", wait_count);
#endif
    }

    /* Setup DMA for auto-init transfer */
#if WSS_DEBUG
    WSS_LOG("Calling DMA_SetupTransfer(ch=%d, buf=%p, size=%d)\n",
           WSS_DMAChannel, BufferStart, BufferSize);
#endif
    status = DMA_SetupTransfer(WSS_DMAChannel, BufferStart, BufferSize, DMA_AutoInitRead);
    if (status != DMA_Ok)
    {
#if WSS_DEBUG
        WSS_LOG("DMA_SetupTransfer FAILED (status=%d)\n", status);
#endif
        return WSS_DmaError;
    }
#if WSS_DEBUG
    WSS_LOG("DMA_SetupTransfer OK\n");
#endif

    /* Save DMA buffer info */
    WSS_DMABuffer = BufferStart;
    WSS_DMABufferEnd = BufferStart + BufferSize;
    WSS_CurrentDMABuffer = BufferStart;
    WSS_TotalDMABufferSize = BufferSize;

    /* Enable WSS interrupt */
    WSS_EnableInterrupt();

    WSS_SoundPlaying = TRUE;

#if WSS_DEBUG
    WSS_LOG("BeginBufferedPlayback SUCCESS\n");
#endif

    return WSS_Ok;
}

/*
 * WSS_Detect
 * Try to detect a WSS-compatible sound card
 */
static int WSS_Detect(void)
{
    unsigned char test_val;
    unsigned char read_val;
    int status;

#if WSS_DEBUG
    WSS_LOG("Detecting card at base 0x%03x...\n", WSS_Config.Address);
#endif

    /* Read config register to check if card responds */
    status = WSS_ReadIndirect(WSS_CONFIG_REG, &test_val);
    if (status != WSS_Ok)
    {
#if WSS_DEBUG
        WSS_LOG("Card NOT detected (read failed, status=%d)\n", status);
#endif
        return WSS_Error;
    }

    /* Try to write and read back a test value */
    status = WSS_WriteIndirect(WSS_CONFIG_REG, test_val | WSS_PEN_BIT);
    if (status != WSS_Ok)
    {
#if WSS_DEBUG
        WSS_LOG("Card NOT detected (write failed, status=%d)\n", status);
#endif
        return WSS_Error;
    }

    /* Read back and verify */
    status = WSS_ReadIndirect(WSS_CONFIG_REG, &read_val);
    if (status != WSS_Ok)
    {
#if WSS_DEBUG
        WSS_LOG("Card NOT detected (readback failed, status=%d)\n", status);
#endif
        return WSS_Error;
    }

#if WSS_DEBUG
    WSS_LOG("Card detected! (orig=0x%02x, write=0x%02x, readback=0x%02x)\n",
           test_val, test_val | WSS_PEN_BIT, read_val);
#endif

    /* Restore original value */
    WSS_WriteIndirect(WSS_CONFIG_REG, test_val);

    return WSS_Ok;
}

/*
 * WSS_Init
 * Initialize the WSS driver
 */
int WSS_Init(void)
{
    WSS_CONFIG config;
    int status;

#if WSS_DEBUG
    WSS_DebugOpen();
    WSS_LOG("=================================================\n");
    WSS_LOG("Initializing WSS driver\n");
    WSS_LOG("=================================================\n");
#endif

    if (WSS_Installed)
    {
        WSS_Shutdown();
    }

    /* Get configuration from environment variable */
    status = WSS_GetEnv(&config);
    if (status == WSS_Ok)
    {
        WSS_Config.Address = config.Address;
        WSS_Config.Interrupt = config.Interrupt;
        WSS_Config.Dma = config.Dma;
    }
    else
    {
        /* Use defaults if ULTRA16 not found */
        WSS_Config.Address = WSS_DEFAULT_BASE;
        WSS_Config.Interrupt = WSS_DEFAULT_IRQ;
        WSS_Config.Dma = WSS_DEFAULT_DMA;
#if WSS_DEBUG
        WSS_LOG("Using default config: base=0x%03x irq=%u dma=%u\n",
               WSS_Config.Address, WSS_Config.Interrupt, WSS_Config.Dma);
#endif
    }

    /* Verify DMA channel is valid (8-bit channels 0-3) */
    if (WSS_Config.Dma > 3)
    {
#if WSS_DEBUG
        WSS_LOG("DMA channel %u invalid (must be 0-3), using default %u\n",
               WSS_Config.Dma, WSS_DEFAULT_DMA);
#endif
        WSS_Config.Dma = WSS_DEFAULT_DMA;
    }

    /* Calculate interrupt vector from IRQ */
    if (WSS_Config.Interrupt < 8)
    {
        WSS_InterruptVector = 0x08 + WSS_Config.Interrupt;
    }
    else
    {
        WSS_InterruptVector = 0x70 + (WSS_Config.Interrupt - 8);
    }

#if WSS_DEBUG
    WSS_LOG("Final config: base=0x%03x irq=%u dma=%u vector=0x%02x\n",
           WSS_Config.Address, WSS_Config.Interrupt, WSS_Config.Dma, WSS_InterruptVector);
#endif

    /* Try to detect the card */
    status = WSS_Detect();
    if (status != WSS_Ok)
    {
#if WSS_DEBUG
        WSS_LOG("Init FAILED - card not detected\n");
#endif
        return WSS_Error;
    }

    /* Save original IRQ masks for restoration */
    if (WSS_Config.Interrupt < 8)
    {
        WSS_IntController1Mask = inp(0x21);
    }
    else
    {
        WSS_IntController1Mask = inp(0x21);
        WSS_IntController2Mask = inp(0xA1);
    }

#if WSS_DEBUG
    WSS_LOG("PIC masks saved: PIC1=0x%02x PIC2=0x%02x\n",
           WSS_IntController1Mask, WSS_IntController2Mask);
#endif

    /* Save original volume settings */
    status = WSS_ReadIndirect(WSS_LDAC_REG, (unsigned char *)&WSS_OriginalPCMLeftVolume);
    if (status == WSS_Ok)
    {
        WSS_ReadIndirect(WSS_RDAC_REG, (unsigned char *)&WSS_OriginalPCMRightVolume);
    }

    /* Setup interrupt vector */
    WSS_OldInt = _dos_getvect(WSS_InterruptVector);
#if WSS_DEBUG
    WSS_LOG("Old IRQ handler at 0x%04x:0x%04x\n", FP_SEG(WSS_OldInt), FP_OFF(WSS_OldInt));
#endif
    _dos_setvect(WSS_InterruptVector, WSS_ServiceInterrupt);

    /* Allocate interrupt stack in conventional memory via DPMI */
    {
        union REGS regs;
        memset(&regs, 0, sizeof(regs));
        regs.w.ax = 0x100;
        regs.w.bx = (kStackSize + 15) / 16;  /* paragraphs */
        int386(0x31, &regs, &regs);
        if (regs.w.cflag)
        {
#if WSS_DEBUG
            WSS_LOG("DPMI allocate low memory FAILED!\n");
#endif
            _dos_setvect(WSS_InterruptVector, WSS_OldInt);
            return WSS_DPMI_Error;
        }
        StackSelector = regs.w.dx;
        /* Point to top of stack with a little room */
        StackPointer = kStackSize - sizeof(long);
#if WSS_DEBUG
        WSS_LOG("DPMI stack allocated, selector=0x%04x\n", StackSelector);
#endif
    }

    /* Set default volume */
    WSS_PCMVolume = 255;

    WSS_Installed = TRUE;

#if WSS_DEBUG
    WSS_LOG("Init SUCCESS\n");
    WSS_LOG("=================================================\n");
#endif

    return WSS_Ok;
}

/*
 * WSS_Shutdown
 * Clean up WSS driver resources
 */
void WSS_Shutdown(void)
{
    if (!WSS_Installed)
    {
        return;
    }

#if WSS_DEBUG
    WSS_LOG("Shutting down\n");
    WSS_DebugClose();
#endif

    /* Stop playback if active */
    if (WSS_SoundPlaying)
    {
        WSS_StopPlayback();
    }

    /* Restore original IRQ masks */
    {
        int irq = WSS_Config.Interrupt;
        if (irq < 8)
        {
            outp(0x21, WSS_IntController1Mask);
        }
        else
        {
            outp(0xA1, WSS_IntController2Mask);
            outp(0x21, WSS_IntController1Mask);
        }
    }

    /* Restore interrupt vector */
    _dos_setvect(WSS_InterruptVector, WSS_OldInt);

    /* Restore original volumes */
    WSS_WriteIndirect(WSS_LDAC_REG, (unsigned char)WSS_OriginalPCMLeftVolume);
    WSS_WriteIndirect(WSS_RDAC_REG, (unsigned char)WSS_OriginalPCMRightVolume);

    /* Free interrupt stack */
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

    WSS_Installed = FALSE;
    WSS_SoundPlaying = FALSE;
}
