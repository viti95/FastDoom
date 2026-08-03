/*
 * AdLib Gold 1000/2000 Digital Audio Driver for FastDoom
 *
 * Implements PIO-based PCM playback via the YMZ263 (MMA) sampling
 * channels on the AdLib Gold sound card. No DMA is used.
 *
 * Audio data is written directly to the YMZ263 FIFO via PIO in the
 * IRQ handler, triggered by the FIFO threshold interrupt.
 *
 * Based on the AIL2 DMASOUND.ASM driver by John Miles (Miles Design, Inc.)
 * and the AdLib Gold Developer Toolkit (YMZ263 programming guide).
 *
 * PIO mode operation:
 *   - YMZ263 has a 128-byte FIFO per channel
 *   - FIFO_INT threshold set to 80 bytes (FIFO_INT=2)
 *   - IRQ fires when FIFO drops to 80 bytes (48 bytes of audio remaining)
 *   - IRQ handler writes 48 bytes to refill FIFO back to 128
 *   - At 11025 Hz mono: ~4.3ms between IRQs, ~230 IRQs/sec
 *   - Dynamic FIFO threshold: temporarily raised to 16 bytes during
 *     PIO writes to prevent false trigger interrupts (per datasheet tip)
 */

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "std_func.h"
#include "ns_irq.h"
#include "ns_gold.h"
#include "ns_golddef.h"
#include "ns_muldf.h"
#include "options.h"
#include "fastmath.h"

/*
 * Explicit prototypes for I/O port functions.
 * These are provided by <dos.h> but some OpenWatcom versions
 * need explicit declarations for the int16 variants.
 */
unsigned char inp(unsigned short port);
void outp(unsigned short port, unsigned char value);

#define GOLD_VALID    (1 == 1)
#define GOLD_INVALID  (!GOLD_VALID)

/* IRQ vector mapping for IRQs 0-15 */
const unsigned char GOLD_Interrupts[GOLD_MaxIrq + 1] =
{
    GOLD_INVALID, GOLD_INVALID, 0x0a, 0x0b,
    GOLD_INVALID, 0x0d, GOLD_INVALID, 0x0f,
    GOLD_INVALID, GOLD_INVALID, 0x72, 0x73,
    0x74, GOLD_INVALID, GOLD_INVALID, 0x77
};

/* Sample size in bytes per sample for each mix mode */
const char GOLD_SampleSizeTable[GOLD_STEREO_8BIT + 1] =
{
    GOLD_MONO_8BIT_SAMPLE_SIZE,
    GOLD_STEREO_8BIT_SAMPLE_SIZE
};

/*
 * Logging - all driver operations logged to gold.log for debugging.
 */
static FILE *GOLD_LogFile = NULL;

static void GOLD_WriteLog(const char *msg)
{
    if (GOLD_LogFile == NULL)
    {
        GOLD_LogFile = fopen("gold.log", "a");
    }
    if (GOLD_LogFile != NULL)
    {
        fputs(msg, GOLD_LogFile);
        fflush(GOLD_LogFile);
    }
}

static void GOLD_WriteHexChar(unsigned char val)
{
    char buf[3];
    static const char *hexDigits = "0123456789ABCDEF";

    buf[0] = hexDigits[(val >> 4) & 0x0F];
    buf[1] = hexDigits[val & 0x0F];
    buf[2] = '\0';
    GOLD_WriteLog(buf);
}

static void GOLD_WriteLogNum(int val)
{
    char buf[16];
    int i;

    if (val == 0)
    {
        GOLD_WriteLog("0");
        return;
    }
    i = 0;
    while (val > 0 && i < 15)
    {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    buf[i] = '\0';
    /* Reverse */
    {
        int j;
        for (j = 0; j < i / 2; j++)
        {
            char tmp;
            tmp = buf[j];
            buf[j] = buf[i - 1 - j];
            buf[i - 1 - j] = tmp;
        }
    }
    GOLD_WriteLog(buf);
}

static void GOLD_WriteLogNumSigned(long val)
{
    if (val < 0)
    {
        GOLD_WriteLog("-");
        GOLD_WriteLogNum(-(int)val);
    }
    else
    {
        GOLD_WriteLogNum((int)val);
    }
}

/*
 * Global state
 */
static void(__interrupt __far *GOLD_OldInt)(void);

GOLD_CONFIG GOLD_Config =
{
    GOLD_DEFAULT_BASE,
    GOLD_DEFAULT_IRQ,
    GOLD_DEFAULT_DMA
};

static int GOLD_Installed = FALSE;

/*
 * DMA channel is set to -1 (invalid) since we use PIO mode only.
 * ns_multi.c reads this to decide whether to use DMA-based position
 * tracking. Value of -1 tells it to skip DMA position queries.
 */
int GOLD_DMAChannel = -1;

unsigned GOLD_SampleRate = GOLD_DefaultSampleRate;

/* PIO audio buffer pointers (used for IRQ-driven PIO writes) */
static char *GOLD_AudioBuffer;
static char *GOLD_AudioBufferEnd;
static char *GOLD_CurrentAudioPtr;

static int GOLD_TotalBufferSize;
static int GOLD_TransferLength = 0;

static int GOLD_MainVolume = 255;
static int GOLD_MixMode = GOLD_DefaultMixMode;

static volatile int GOLD_SoundPlaying = FALSE;
static volatile long GOLD_IrqCount = 0;
static volatile long GOLD_SpuriousIrqCount = 0;
static volatile long GOLD_PioBytesWritten = 0;
static volatile unsigned char GOLD_LastIrqStatus = 0;
static volatile unsigned char GOLD_LastIrqMma0 = 0;
static volatile unsigned char GOLD_LastIrqMma1 = 0;

void (*GOLD_CallBack)(void);

static unsigned short GOLD_Interrupt;

/*
 * Shadow of PRC (playback/recording control) register values
 * so we can toggle GO bit without disturbing other settings.
 */
static unsigned char GOLD_PRC0_Shadow;
static unsigned char GOLD_PRC1_Shadow;

/* Saved format control register for FIFO threshold adjustment */
static unsigned char GOLD_SFC0_Normal;
static unsigned char GOLD_SFC0_HighThreshold;

/* Saved control register 0x13 value for restore on shutdown */
static unsigned char GOLD_CTR13_Save;

/*
 * Interrupt controller state
 */
static int GOLD_IntController1Mask;
static int GOLD_IntController2Mask;

/*
 * NOTE: DPMI stack switching removed.
 * In protected mode, the IRQ handler runs on a valid DPMI-managed stack.
 * Manually switching SS/ESP via inline asm causes GPFs.
 */

/*
 * I/O port addresses (computed during init)
 */
static unsigned short GOLD_CTAddr;
static unsigned short GOLD_CTData;
static unsigned short GOLD_MMA0Addr;
static unsigned short GOLD_MMA0Data;
static unsigned short GOLD_MMA1Addr;
static unsigned short GOLD_MMA1Data;

/*
 * Hardware PCM sample rates supported by YMZ263
 */
static const unsigned int GOLD_PCM_Rates[4] =
{
    44100, 22050, 11025, 7350
};

/*
 * FIFO threshold constants for dynamic adjustment.
 * Normal: FIFO_INT=2 -> IRQ at 80 bytes remaining (48 bytes of audio)
 * High:   FIFO_INT=6 -> IRQ at 16 bytes remaining (used during PIO write
 *         to prevent false trigger interrupts per datasheet programming tip)
 *
 * SFC register: bit 7=ILV, bits 6-5=DATAFMT, bits 4-1=FIFO_INT, bit 0=ENB
 * FIFO_INT values: 0=112, 1=96, 2=80, 3=64, 4=48, 5=32, 6=16 (bytes remaining)
 */
#define GOLD_SFC_FIFO_INT_NORMAL  2   /* IRQ fires at 80 bytes remaining */
#define GOLD_SFC_FIFO_INT_HIGH    6   /* IRQ fires at 16 bytes (PIO safe) */

/*---------------------------------------------------------------------
   Function: GOLD_CTWait

   Wait for the control chip to become ready.  Bit 6 (RB) and bit 5 (SB)
   must both be clear before we can communicate.
---------------------------------------------------------------------*/
static void GOLD_CTWait(void)
{
    unsigned int count;

    count = 0x10000;
    do
    {
        unsigned char status;

        status = inp(GOLD_CTAddr);
        if ((status & 0xC0) == 0)
        {
            break;
        }
        count--;
    }
    while (count > 0);
}

/*---------------------------------------------------------------------
   Function: GOLD_EnableCtrl

   Enable access to the control chip by writing 0xFF to the CT address
   port.  This is the "unlock" sequence from the AIL2 driver.
---------------------------------------------------------------------*/
static void GOLD_EnableCtrl(void)
{
    outp(GOLD_CTAddr, 0xFF);
}

/*---------------------------------------------------------------------
   Function: GOLD_DisableCtrl

   Disable control chip access by writing 0xFE to the CT address port.
   Must wait for ready first.
---------------------------------------------------------------------*/
static void GOLD_DisableCtrl(void)
{
    GOLD_CTWait();
    outp(GOLD_CTAddr, 0xFE);
}

/*---------------------------------------------------------------------
   Function: GOLD_ReadCtrlReg

   Read a byte from the specified control chip register.
   Waits for ready, writes register number, waits, reads data.
---------------------------------------------------------------------*/
static unsigned char GOLD_ReadCtrlReg(unsigned char reg)
{
    unsigned char val;

    GOLD_CTWait();
    outp(GOLD_CTAddr, reg);
    GOLD_CTWait();
    val = inp(GOLD_CTData);

    return val;
}

/*---------------------------------------------------------------------
   Function: GOLD_WriteCtrlReg

   Write a byte to the specified control chip register.
   Waits for ready, writes register number, waits, writes data.
---------------------------------------------------------------------*/
static void GOLD_WriteCtrlReg(unsigned char reg, unsigned char val)
{
    GOLD_CTWait();
    outp(GOLD_CTAddr, reg);
    GOLD_CTWait();
    outp(GOLD_CTData, val);
}

/*---------------------------------------------------------------------
   Function: GOLD_MMADelay

   Wait at least 470 nsec between MMA register accesses.
   The YMZ263 datasheet requires a minimum inter-byte delay.
---------------------------------------------------------------------*/
static void GOLD_MMADelay(void)
{
    volatile unsigned int i;

    for (i = 0; i < 50; i++)
    {
        /* busy-wait */
    }
}

/*---------------------------------------------------------------------
   Function: GOLD_WriteMMAReg

   Write a byte to the specified MMA channel register.
   Channel 0 or 1, register number, and value are passed in.
---------------------------------------------------------------------*/
static void GOLD_WriteMMAReg(unsigned char channel, unsigned char reg,
                              unsigned char val)
{
    unsigned short addrPort;
    unsigned short dataPort;

    if (channel == 0)
    {
        addrPort = GOLD_MMA0Addr;
        dataPort = GOLD_MMA0Data;
    }
    else
    {
        addrPort = GOLD_MMA1Addr;
        dataPort = GOLD_MMA1Data;
    }

    outp(addrPort, reg);
    GOLD_MMADelay();
    outp(dataPort, val);
    GOLD_MMADelay();
}

/*---------------------------------------------------------------------
   Function: GOLD_ReadMMAStatus

   Read the status register from the specified MMA channel.
   The status is read from the address port (same port used for
   register selection).
---------------------------------------------------------------------*/
static unsigned char GOLD_ReadMMAStatus(unsigned char channel)
{
    unsigned short addrPort;
    unsigned char status;

    if (channel == 0)
    {
        addrPort = GOLD_MMA0Addr;
    }
    else
    {
        addrPort = GOLD_MMA1Addr;
    }

    status = inp(addrPort);
    return status;
}

/*---------------------------------------------------------------------
   Function: GOLD_FindClosestRate

   Find the index of the closest hardware-supported sample rate.
   The YMZ263 supports four fixed clock-derived rates:
   44100, 22050, 11025, and 7350 Hz.
---------------------------------------------------------------------*/
static unsigned int GOLD_FindClosestRate(unsigned int rate)
{
    unsigned int i;
    unsigned int bestIdx;
    long bestDelta;
    long delta;

    bestIdx = 0;

    if (rate > GOLD_PCM_Rates[0])
    {
        bestDelta = (long)rate - (long)GOLD_PCM_Rates[0];
    }
    else
    {
        bestDelta = (long)GOLD_PCM_Rates[0] - (long)rate;
    }

    for (i = 1; i < 4; i++)
    {
        if (rate > GOLD_PCM_Rates[i])
        {
            delta = (long)rate - (long)GOLD_PCM_Rates[i];
        }
        else
        {
            delta = (long)GOLD_PCM_Rates[i] - (long)rate;
        }

        if (delta < bestDelta)
        {
            bestDelta = delta;
            bestIdx = i;
        }
    }

    return bestIdx;
}

/*---------------------------------------------------------------------
   Function: GOLD_SetPlaybackRate

   Sets the rate at which the digitized sound will be played.
   The YMZ263 has four fixed sample rate clock dividers, so the
   actual rate may differ from the requested rate.
---------------------------------------------------------------------*/
void GOLD_SetPlaybackRate(unsigned rate)
{
    unsigned int rateIdx;
    unsigned char prc0;
    unsigned char prc1;
    unsigned char fmt0;
    unsigned char fmt1;

    rateIdx = GOLD_FindClosestRate(rate);

    if (rateIdx > 3)
    {
        rateIdx = 3;
    }

    /*
     * PRC (playback/recording control) register 0x09:
     *   D7: RST  - Reset (0 for normal operation)
     *   D6: R    - Right channel output enable
     *   D5: L    - Left channel output enable
     *   D4-3: FREQ[1:0] - Sample rate select
     *   D2: PCM  - 1 = PCM mode
     *   D1: PR   - 1 = playback mode
     *   D0: GO   - Start/stop (0 = stopped)
     *
     * AIL2 for mono 8-bit PCM: PRC_0 = 01100110b | FREQ
     *                           PRC_1 = 00000000b (disabled)
     * AIL2 for stereo 8-bit:   PRC_0 = 01000110b | FREQ (left)
     *                           PRC_1 = 00100110b | FREQ (right)
     */
    if (GOLD_MixMode & GOLD_STEREO)
    {
        prc0 = (unsigned char)(
            GOLD_MMA_L_BIT |
            ((rateIdx & 3) << GOLD_MMA_FREQ_SHIFT) |
            GOLD_MMA_PCM_BIT |
            GOLD_MMA_PR_BIT
        );
        prc1 = (unsigned char)(
            GOLD_MMA_R_BIT |
            ((rateIdx & 3) << GOLD_MMA_FREQ_SHIFT) |
            GOLD_MMA_PCM_BIT |
            GOLD_MMA_PR_BIT
        );
    }
    else
    {
        /* Mono: channel 0 outputs both L+R, channel 1 disabled */
        prc0 = (unsigned char)(
            GOLD_MMA_R_BIT |
            GOLD_MMA_L_BIT |
            ((rateIdx & 3) << GOLD_MMA_FREQ_SHIFT) |
            GOLD_MMA_PCM_BIT |
            GOLD_MMA_PR_BIT
        );
        prc1 = 0;
    }

    /*
     * Format control register 0x0C:
     *   D7: ILV   - Interleave (channel 0 only)
     *   D6-5: DATA FORMAT[1:0] - 0 = 8-bit MSB format
     *   D4-1: FIFO INT[3:0]    - Interrupt threshold
     *   D0: ENB   - DMA enable (0 = PIO mode)
     *
     * FIFO_INT threshold values:
     *   0 = 112 bytes remaining, 1 = 96, 2 = 80, 3 = 64,
     *   4 = 48, 5 = 32, 6 = 16
     *
     * Per AIL2 DMASOUND.ASM:
     *   mono 8-bit PCM:   SFC_0 = 00000101b (0x05), SFC_1 = 00000010b (0x02)
     *   stereo 8-bit PCM: SFC_0 = 10000101b (0x85), SFC_1 = 00000011b (0x03)
     *
     * We use PIO mode (ENB=0) with FIFO_INT=2 (80 bytes threshold).
     * IRQ fires when FIFO drops to 80 bytes, leaving 48 bytes of audio.
     * IRQ handler writes 48 bytes to refill back to 128.
     *
     * Channel 1: MSK=1 (mask interrupt), not used for mono playback.
     */
    if (GOLD_MixMode & GOLD_STEREO)
    {
        fmt0 = (unsigned char)(GOLD_MMA_ILV_BIT |
                               ((GOLD_SFC_FIFO_INT_NORMAL << 1) & 0x1E));
        fmt1 = 0x02;    /* MSK=1, interrupt masked for ch1 */
    }
    else
    {
        fmt0 = (unsigned char)((GOLD_SFC_FIFO_INT_NORMAL << 1) & 0x1E);
        fmt1 = 0x02;    /* MSK=1, interrupt masked for ch1 */
    }

    /* Save SFC shadows for dynamic threshold adjustment */
    GOLD_SFC0_Normal = fmt0;
    GOLD_SFC0_HighThreshold = (unsigned char)(
        (fmt0 & ~0x1E) |  /* Clear FIFO_INT bits */
        ((GOLD_SFC_FIFO_INT_HIGH << 1) & 0x1E)  /* Set high threshold */
    );

    /*
     * Per AIL2 DMASOUND.ASM set_sample_rate:
     *   1. Reset both FIFOs
     *   2. Write 4 dummy bytes to channel 0 FIFO
     *   3. Set PRC (playback control) with frequency
     *   4. Set SFC (format control)
     */
    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, GOLD_MMA_RST_BIT);
    GOLD_WriteMMAReg(1, GOLD_MMA_PLAY_REC_CTL, GOLD_MMA_RST_BIT);

    /* Write 4 dummy bytes to channel 0 FIFO for proper init */
    GOLD_WriteMMAReg(0, GOLD_MMA_PCM_DATA, 0);
    GOLD_WriteMMAReg(0, GOLD_MMA_PCM_DATA, 0);
    GOLD_WriteMMAReg(0, GOLD_MMA_PCM_DATA, 0);
    GOLD_WriteMMAReg(0, GOLD_MMA_PCM_DATA, 0);

    /* Save PRC shadows with GO=0 (set in StartPlayback) */
    GOLD_PRC0_Shadow = prc0;
    GOLD_PRC1_Shadow = prc1;

    /* Set PRC with frequency and channel config */
    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, prc0);
    GOLD_WriteMMAReg(1, GOLD_MMA_PLAY_REC_CTL, prc1);

    /* Set format control on both channels (PIO mode, ENB=0) */
    GOLD_WriteMMAReg(0, GOLD_MMA_FMT_CTL, fmt0);
    GOLD_WriteMMAReg(1, GOLD_MMA_FMT_CTL, fmt1);

    /* Update the actual sample rate */
    GOLD_SampleRate = GOLD_PCM_Rates[rateIdx];

    GOLD_WriteLog("GOLD_SetPlaybackRate: rate configured\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_GetPlaybackRate

   Returns the current playback sample rate.
---------------------------------------------------------------------*/
unsigned GOLD_GetPlaybackRate(void)
{
    return GOLD_SampleRate;
}

/*---------------------------------------------------------------------
   Function: GOLD_SetMixMode

   Sets the sound card to play samples in the specified mode.
   The AdLib Gold supports mono 8-bit PCM and stereo 8-bit.
   Returns GOLD_Ok on success.
---------------------------------------------------------------------*/
int GOLD_SetMixMode(int mode)
{
    unsigned char fmt0;
    unsigned char fmt1;

    if (mode > GOLD_MaxMixMode)
    {
        mode = GOLD_MaxMixMode;
    }

    GOLD_MixMode = mode;

    /*
     * Set format control based on mix mode.
     * PIO mode: ENB=0 (DMA disabled), MSK=0 (interrupt enabled).
     * FIFO_INT = 2: IRQ fires at 80 bytes remaining (48 bytes of audio).
     * IRQ handler writes 48 bytes to FIFO via PIO.
     *
     * SFC register layout:
     *   Bit 7: ILV (interleave stereo channels)
     *   Bits 6-5: DATAFMT (00=8-bit, 01=16-bit)
     *   Bits 4-1: FIFO_INT (threshold index)
     *   Bit 1: MSK (1=mask interrupt)
     *   Bit 0: ENB (1=DMA mode, 0=PIO mode)
     *
     * FIFO_INT table:
     *   0=112, 1=96, 2=80, 3=64, 4=48, 5=32, 6=16 (bytes remaining)
     */
    if (mode & GOLD_STEREO)
    {
        fmt0 = (unsigned char)(GOLD_MMA_ILV_BIT |
                               ((GOLD_SFC_FIFO_INT_NORMAL << 1) & 0x1E));
        fmt1 = 0x02;
    }
    else
    {
        fmt0 = (unsigned char)((GOLD_SFC_FIFO_INT_NORMAL << 1) & 0x1E);
        fmt1 = 0x02;
    }

    /* Save SFC shadows for dynamic threshold adjustment */
    GOLD_SFC0_Normal = fmt0;
    GOLD_SFC0_HighThreshold = (unsigned char)(
        (fmt0 & ~0x1E) |
        ((GOLD_SFC_FIFO_INT_HIGH << 1) & 0x1E)
    );

    /* Only reconfigure if not playing */
    if (!GOLD_SoundPlaying)
    {
        GOLD_WriteMMAReg(0, GOLD_MMA_FMT_CTL, fmt0);
        GOLD_WriteMMAReg(1, GOLD_MMA_FMT_CTL, fmt1);
    }

    GOLD_WriteLog("GOLD_SetMixMode: mode set\n");
    return GOLD_Ok;
}

/*---------------------------------------------------------------------
   Function: GOLD_SetPCMVolume

   Sets the volume of the PCM playback output (0-255).
   Written directly to both MMA channels via register 0x0A.
---------------------------------------------------------------------*/
int GOLD_SetPCMVolume(int volume)
{
    unsigned char vol;

    if (volume < 0)
    {
        volume = 0;
    }
    if (volume > 255)
    {
        volume = 255;
    }

    vol = (unsigned char)volume;
    GOLD_MainVolume = volume;

    /* Write volume to both MMA channels */
    GOLD_WriteMMAReg(0, GOLD_MMA_VOLUME, vol);
    GOLD_WriteMMAReg(1, GOLD_MMA_VOLUME, vol);

    GOLD_WriteLog("GOLD_SetPCMVolume: volume set\n");
    return GOLD_Ok;
}

/*---------------------------------------------------------------------
   Function: GOLD_FifoPreFill

   Fill the YMZ263 FIFO with audio data before starting playback.
   Per datasheet: "The FIFO buffers should be filled to a level exceeding
   the FIFO interrupt level before the GO bit is set."
   With FIFO_INT=2 (80 byte threshold), we need >80 bytes in the FIFO.

   NOTE: GOLD_SetPlaybackRate already writes 4 dummy bytes to the FIFO
   during initialization. Since the FIFO is 128 bytes, we only write
   124 more bytes here to avoid overflowing the FIFO.
---------------------------------------------------------------------*/
static void GOLD_FifoPreFill(void)
{
    int bytesToWrite;
    char *pioAddr;
    int i;

    /* 128-byte FIFO minus 4 dummy bytes from SetPlaybackRate = 124 */
    bytesToWrite = GOLD_MMA_FIFO_SIZE - 4;
    pioAddr = GOLD_CurrentAudioPtr;

    /* Don't cross buffer boundary */
    if (pioAddr + bytesToWrite > GOLD_AudioBufferEnd)
    {
        bytesToWrite = (int)(GOLD_AudioBufferEnd - pioAddr);
    }

    if (bytesToWrite > 0)
    {
        /* Select FIFO data register (0x0B) on channel 0 */
        outp(GOLD_MMA0Addr, GOLD_MMA_PCM_DATA);

        /* Write each byte to the FIFO data port */
        for (i = 0; i < bytesToWrite; i++)
        {
            GOLD_MMADelay();
            outp(GOLD_MMA0Data, (unsigned char)pioAddr[i]);
        }

        /* Advance buffer pointer */
        GOLD_CurrentAudioPtr += bytesToWrite;
        if (GOLD_CurrentAudioPtr >= GOLD_AudioBufferEnd)
        {
            GOLD_CurrentAudioPtr = GOLD_AudioBuffer;
        }
    }

    GOLD_WriteLog("GOLD_FifoPreFill: filled ");
    GOLD_WriteLogNum(bytesToWrite);
    GOLD_WriteLog(" bytes\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_StartPlayback

   Start PIO playback by setting the GO bit on the MMA channel.
   FIFO must be pre-filled before this is called.
---------------------------------------------------------------------*/
static void GOLD_StartPlayback(void)
{
    unsigned char prc0;

    /* Set GO on channel 0. Per datasheet, in stereo interleaved mode,
     * channel 0 controls both P/R, FREQ and GO bits for both channels.
     * In mono mode, only channel 0 is used.
     * Per AIL2 DMASOUND.ASM continue_DMA: only writes GO to ch0.
     */
    prc0 = GOLD_PRC0_Shadow | GOLD_MMA_GO_BIT;
    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, prc0);

    GOLD_SoundPlaying = TRUE;

    /* Reset diagnostic counters */
    GOLD_IrqCount = 0;
    GOLD_SpuriousIrqCount = 0;
    GOLD_PioBytesWritten = 0;

    GOLD_WriteLog("GOLD_StartPlayback: playback started\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_StopPlayback

   Stops playback by clearing the GO bit and disabling interrupts.
---------------------------------------------------------------------*/
void GOLD_StopPlayback(void)
{
    unsigned char prc0;

    GOLD_WriteLog("GOLD_StopPlayback: stopping playback\n");

    /* Disable interrupts first */
    {
        int irq;
        int mask;

        irq = GOLD_Config.Interrupt;
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

    /* Clear GO bit on channel 0 (controls both channels) */
    prc0 = GOLD_PRC0_Shadow & (unsigned char)(~GOLD_MMA_GO_BIT);
    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, prc0);

    GOLD_SoundPlaying = FALSE;
    GOLD_AudioBuffer = NULL;

    GOLD_WriteLog("GOLD_StopPlayback: playback stopped\n");
    GOLD_WriteLog("GOLD_StopPlayback: validIRQs=");
    GOLD_WriteLogNumSigned(GOLD_IrqCount);
    GOLD_WriteLog(" spuriousIRQs=");
    GOLD_WriteLogNumSigned(GOLD_SpuriousIrqCount);
    GOLD_WriteLog(" pioBytes=");
    GOLD_WriteLogNumSigned(GOLD_PioBytesWritten);
    GOLD_WriteLog(" lastYMZ263Status=0x");
    GOLD_WriteHexChar(GOLD_LastIrqStatus);
    GOLD_WriteLog(" mma0=0x");
    GOLD_WriteHexChar(GOLD_LastIrqMma0);
    GOLD_WriteLog(" mma1=0x");
    GOLD_WriteHexChar(GOLD_LastIrqMma1);
    GOLD_WriteLog("\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_EnableInterrupt

   Enables the Gold card interrupt on the PIC.
---------------------------------------------------------------------*/
void GOLD_EnableInterrupt(void)
{
    int irq;
    int mask;

    irq = GOLD_Config.Interrupt;

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

    GOLD_WriteLog("GOLD_EnableInterrupt: IRQ enabled\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_DisableInterrupt

   Disables the Gold card interrupt on the PIC.
---------------------------------------------------------------------*/
void GOLD_DisableInterrupt(void)
{
    int irq;
    int mask;

    irq = GOLD_Config.Interrupt;

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

    GOLD_WriteLog("GOLD_DisableInterrupt: IRQ disabled\n");
}



/*---------------------------------------------------------------------
   Function: GOLD_ServiceInterrupt

   ISR for the Gold card FIFO interrupt.

   Uses PIO mode: audio data is written directly to the YMZ263 FIFO
   via I/O port writes. No DMA is used.

   The YMZ263 has a 128-byte FIFO per channel. The FIFO interrupt
   fires when the FIFO drops below the configured threshold
   (FIFO_INT=2: 80 bytes remaining = 48 bytes of audio). The IRQ
   handler writes 48 bytes to refill the FIFO.

   Dynamic FIFO threshold adjustment (per datasheet programming tip):
   - On entry: raise FIFO threshold to 16 bytes (FIFO_INT=6) to prevent
     false trigger interrupts while PIO writes are in progress
   - On exit: restore normal FIFO threshold of 80 bytes (FIFO_INT=2)

   Per AIL2 DMASOUND.ASM (ADLIBG section), the IRQ handler reads
   MMA channel 0 address port for FIF0 bit, then services the IRQ.
---------------------------------------------------------------------*/
void __interrupt __far GOLD_ServiceInterrupt(void)
{
    unsigned char status;
    int bytesToWrite;
    char *pioAddr;
    int i;

    /*
     * Read YMZ263 channel 0 status register.
     * Bit 0 (FIFO0) indicates the FIFO interrupt has fired.
     *
     * Per AIL2 DMASOUND.ASM:
     *    mov  dx,DSP_ADDR
     *    in   al,dx
     *    and  al,00000001b      ;FIF0 interrupt?
     *    jz   __EOI              ;no, exit
     */
    status = inp(GOLD_MMA0Addr);

    /* Save for diagnostics */
    GOLD_LastIrqStatus = status;

    /* Check bit 0: FIFO0 (FIFO0 interrupt from YMZ263) */
    if ((status & 0x01) == 0)
    {
        /* Spurious interrupt - still send EOI to avoid lockup */
        GOLD_SpuriousIrqCount++;

        if (GOLD_Config.Interrupt > 7)
        {
            outp(0xA0, 0x20);
        }
        outp(0x20, 0x20);
        return;
    }

    /* Count valid interrupts for diagnostics */
    GOLD_IrqCount++;

    /* Capture YMZ263 status for diagnostics */
    GOLD_LastIrqMma0 = status;
    GOLD_LastIrqMma1 = inp(GOLD_MMA1Addr);

    /*
     * Dynamic FIFO threshold adjustment (per datasheet tip):
     * Raise threshold to 16 bytes (FIFO_INT=6) to prevent false
     * triggers while we are writing to the FIFO simultaneously
     * with playback consuming from it.
     */
    GOLD_WriteMMAReg(0, GOLD_MMA_FMT_CTL, GOLD_SFC0_HighThreshold);

    /* Call the mixer callback to refill the consumed portion */
    if (GOLD_CallBack != NULL)
    {
        MV_ServiceVoc();
    }

    /*
     * PIO mode: write audio data directly to YMZ263 FIFO.
     *
     * FIFO_INT = 2: IRQ fires at 80 bytes remaining.
     * We write 48 bytes to bring FIFO back to 128.
     * Next IRQ fires when FIFO drains back to 80.
     * Cycle: 48 bytes @ 11025Hz = 4.3ms, IRQ rate ~231/sec.
     *
     * Inline the FIFO write here for speed - avoid function call
     * overhead in the IRQ handler.
     */
    if (GOLD_SoundPlaying)
    {
        bytesToWrite = 48;
        pioAddr = GOLD_CurrentAudioPtr;

        /* Don't cross buffer boundary */
        if (pioAddr + bytesToWrite > GOLD_AudioBufferEnd)
        {
            bytesToWrite = (int)(GOLD_AudioBufferEnd - pioAddr);
        }

        /* Write bytes directly to YMZ263 FIFO via PIO */
        if (bytesToWrite > 0)
        {
            /* Check for FIFO overrun */
            status = inp(GOLD_MMA0Addr);
            if (status & 0x80)
            {
                /* FIFO overrun - reset and restart playback */
                GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, GOLD_MMA_RST_BIT);
                GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, GOLD_PRC0_Shadow | GOLD_MMA_GO_BIT);
            }

            /* Select FIFO data register (0x0B) on channel 0 */
            outp(GOLD_MMA0Addr, GOLD_MMA_PCM_DATA);

            /* Write each byte to the FIFO data port */
            for (i = 0; i < bytesToWrite; i++)
            {
                GOLD_MMADelay();
                outp(GOLD_MMA0Data, (unsigned char)pioAddr[i]);
            }

            GOLD_PioBytesWritten += bytesToWrite;

            /* Advance buffer pointer */
            GOLD_CurrentAudioPtr += bytesToWrite;
            if (GOLD_CurrentAudioPtr >= GOLD_AudioBufferEnd)
            {
                GOLD_CurrentAudioPtr = GOLD_AudioBuffer;
            }
        }
    }

    /*
     * Restore normal FIFO threshold (80 bytes).
     * Next interrupt will fire when FIFO drops back to 80.
     */
    GOLD_WriteMMAReg(0, GOLD_MMA_FMT_CTL, GOLD_SFC0_Normal);

    /* Send EOI to the PIC */
    if (GOLD_Config.Interrupt > 7)
    {
        outp(0xA0, 0x20);  /* Slave PIC EOI */
    }
    outp(0x20, 0x20);  /* Master PIC EOI */
}

/*---------------------------------------------------------------------
   Function: GOLD_ResetMMA

   Resets the MMA sampling channels.
---------------------------------------------------------------------*/
static void GOLD_ResetMMA(void)
{
    /* Reset FIFOs on both channels */
    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, GOLD_MMA_RST_BIT);
    GOLD_WriteMMAReg(1, GOLD_MMA_PLAY_REC_CTL, GOLD_MMA_RST_BIT);

    /* Clear reset by writing normal PRC with GO=0 */
    GOLD_PRC0_Shadow = (unsigned char)(
        GOLD_MMA_R_BIT |
        GOLD_MMA_L_BIT |
        GOLD_MMA_PCM_BIT |
        GOLD_MMA_PR_BIT
    );
    GOLD_PRC1_Shadow = 0;  /* Channel 1 disabled in mono mode */

    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, GOLD_PRC0_Shadow);
    GOLD_WriteMMAReg(1, GOLD_MMA_PLAY_REC_CTL, GOLD_PRC1_Shadow);

    GOLD_WriteLog("GOLD_ResetMMA: MMA channels reset\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_DetectDevice

   Detect the AdLib Gold card at the configured base I/O address.
   Uses the control chip register echo test.
---------------------------------------------------------------------*/
static int GOLD_DetectDevice(void)
{
    unsigned char volL, volR;
    unsigned char volL_tweak, volR_tweak;

    GOLD_WriteLog("GOLD_DetectDevice: attempting detection\n");

    /* Set up port addresses */
    GOLD_CTAddr = (unsigned short)(GOLD_Config.Address + GOLD_CT_ADDR_OFFSET);
    GOLD_CTData = (unsigned short)(GOLD_Config.Address + GOLD_CT_DATA_OFFSET);
    GOLD_MMA0Addr = (unsigned short)(GOLD_Config.Address + GOLD_MMA0_ADDR_OFFSET);
    GOLD_MMA0Data = (unsigned short)(GOLD_Config.Address + GOLD_MMA0_DATA_OFFSET);
    GOLD_MMA1Addr = (unsigned short)(GOLD_Config.Address + GOLD_MMA1_ADDR_OFFSET);
    GOLD_MMA1Data = (unsigned short)(GOLD_Config.Address + GOLD_MMA1_DATA_OFFSET);

    /* Enable control chip access */
    GOLD_EnableCtrl();

    /* Read current FM volume registers */
    volL = GOLD_ReadCtrlReg(0x09);
    volR = GOLD_ReadCtrlReg(0x0A);

    /* Tweak bits for verification */
    volL_tweak = volL ^ 0x05;
    volR_tweak = volR ^ 0x0A;

    /* Write tweaked values back */
    GOLD_WriteCtrlReg(0x09, volL_tweak);
    GOLD_WriteCtrlReg(0x0A, volR_tweak);

    /* Read back and verify */
    if (GOLD_ReadCtrlReg(0x09) != volL_tweak)
    {
        GOLD_DisableCtrl();
        GOLD_WriteLog("GOLD_DetectDevice: FAILED (reg 0x09 mismatch)\n");
        return FALSE;
    }

    if (GOLD_ReadCtrlReg(0x0A) != volR_tweak)
    {
        GOLD_DisableCtrl();
        GOLD_WriteLog("GOLD_DetectDevice: FAILED (reg 0x0A mismatch)\n");
        return FALSE;
    }

    /* Control chip verified - restore original values */
    GOLD_WriteCtrlReg(0x09, volL);
    GOLD_WriteCtrlReg(0x0A, volR);

    /* Disable control chip access */
    GOLD_DisableCtrl();

    GOLD_WriteLog("GOLD_DetectDevice: card detected successfully\n");
    return TRUE;
}

/*---------------------------------------------------------------------
   Function: GOLD_GetEnv

   Retrieves the GOLD environment settings.
   Format: GOLD=Axxxx Ii Dd
---------------------------------------------------------------------*/
int GOLD_GetEnv(GOLD_CONFIG *Config)
{
    char *GoldEnv;
    char param;

    Config->Address = GOLD_DEFAULT_BASE;
    Config->Interrupt = GOLD_DEFAULT_IRQ;
    Config->Dma = GOLD_DEFAULT_DMA;

    GoldEnv = getenv("GOLD");
    if (GoldEnv == NULL)
    {
        GOLD_WriteLog("GOLD_GetEnv: no GOLD env var, using defaults\n");
        return GOLD_Ok;
    }

    while (*GoldEnv != 0)
    {
        if (*GoldEnv == ' ')
        {
            GoldEnv++;
            continue;
        }

        param = toupper(*GoldEnv);
        GoldEnv++;

        if (!isxdigit(*GoldEnv))
        {
            continue;
        }

        switch (param)
        {
        case 'A':
            sscanf(GoldEnv, "%x", &Config->Address);
            break;
        case 'I':
            sscanf(GoldEnv, "%d", &Config->Interrupt);
            break;
        case 'D':
            sscanf(GoldEnv, "%d", &Config->Dma);
            break;
        }

        while (isxdigit(*GoldEnv))
        {
            GoldEnv++;
        }
    }

    GOLD_WriteLog("GOLD_GetEnv: parsed GOLD environment\n");
    return GOLD_Ok;
}

/*---------------------------------------------------------------------
   Function: GOLD_SetCardSettings

   Sets up the Gold card's parameters.
---------------------------------------------------------------------*/
int GOLD_SetCardSettings(GOLD_CONFIG Config)
{
    if (GOLD_Installed)
    {
        GOLD_Shutdown();
    }

    GOLD_Config.Address = Config.Address;
    GOLD_Config.Interrupt = Config.Interrupt;
    GOLD_Config.Dma = Config.Dma;

    GOLD_WriteLog("GOLD_SetCardSettings: settings updated\n");
    return GOLD_Ok;
}

/*---------------------------------------------------------------------
   Function: GOLD_GetCardSettings

   Returns the current Gold card parameters.
---------------------------------------------------------------------*/
int GOLD_GetCardSettings(GOLD_CONFIG *Config)
{
    if (GOLD_Config.Address == 0)
    {
        return GOLD_Warning;
    }

    Config->Address = GOLD_Config.Address;
    Config->Interrupt = GOLD_Config.Interrupt;
    Config->Dma = GOLD_Config.Dma;

    return GOLD_Ok;
}

/*---------------------------------------------------------------------
   Function: GOLD_Init

   Initializes the Gold card and prepares the driver for playback.
   Uses PIO mode only (no DMA).
---------------------------------------------------------------------*/
int GOLD_Init(void)
{
    int irq;

    GOLD_WriteLog("GOLD_Init: initializing AdLib Gold driver (PIO mode)\n");

    if (GOLD_Installed)
    {
        GOLD_Shutdown();
    }

    if (GOLD_Config.Address == 0)
    {
        GOLD_WriteLog("GOLD_Init: ERROR - no base address configured\n");
        return GOLD_Error;
    }

    /* Save the interrupt controller masks */
    GOLD_IntController1Mask = inp(0x21);
    GOLD_IntController2Mask = inp(0xA1);

    /* Detect the card */
    if (!GOLD_DetectDevice())
    {
        GOLD_WriteLog("GOLD_Init: ERROR - card not detected\n");
        return GOLD_Error;
    }

    /*
     * PIO mode: no DMA channel needed.
     * Set to -1 so ns_multi.c knows not to use DMA position tracking.
     */
    GOLD_DMAChannel = -1;

    /*
     * Read IRQ from control chip register 0x13 if not configured.
     */
    if (GOLD_Config.Interrupt == GOLD_DEFAULT_IRQ ||
        GOLD_Config.Interrupt == 0)
    {
        unsigned char ctr13;
        unsigned char irqIdx;
        const unsigned char selected_IRQ[] =
        { 3, 4, 5, 7, 10, 11, 12, 15 };

        GOLD_EnableCtrl();
        ctr13 = GOLD_ReadCtrlReg(GOLD_CT_REG_AUDIO_IRQ_DMA0);
        GOLD_DisableCtrl();

        irqIdx = ctr13 & GOLD_CT_INT_SEL_MASK;
        if (irqIdx < 8)
        {
            GOLD_Config.Interrupt = selected_IRQ[irqIdx];
        }

        GOLD_WriteLog("GOLD_Init: auto-detected IRQ from control chip\n");
        GOLD_WriteLog("GOLD_Init:   ctr13=0x");
        GOLD_WriteHexChar((ctr13 >> 4) & 0x0F);
        GOLD_WriteHexChar(ctr13 & 0x0F);
        GOLD_WriteLog(" IRQ=");
        GOLD_WriteHexChar(GOLD_Config.Interrupt);
        GOLD_WriteLog("\n");
    }

    /* Verify IRQ is valid */
    irq = GOLD_Config.Interrupt;
    if (!VALID_IRQ(irq))
    {
        GOLD_WriteLog("GOLD_Init: ERROR - invalid IRQ\n");
        return GOLD_Error;
    }

    GOLD_Interrupt = GOLD_Interrupts[irq];
    if (GOLD_Interrupt == GOLD_INVALID)
    {
        GOLD_WriteLog("GOLD_Init: ERROR - no interrupt vector for IRQ\n");
        return GOLD_Error;
    }

    /* Save old interrupt handler and install ours */
    GOLD_OldInt = _dos_getvect(GOLD_Interrupt);
    if (irq < 8)
    {
        _dos_setvect(GOLD_Interrupt, GOLD_ServiceInterrupt);
    }
    else
    {
        int status;
        status = IRQ_SetVector(GOLD_Interrupt, GOLD_ServiceInterrupt);
        if (status != IRQ_Ok)
        {
            GOLD_WriteLog("GOLD_Init: ERROR - IRQ_SetVector failed\n");
            return GOLD_Error;
        }
    }

    GOLD_WriteLog("GOLD_Init: IRQ=");
    GOLD_WriteHexChar(irq);
    GOLD_WriteLog(" vector=0x");
    GOLD_WriteHexChar((GOLD_Interrupt >> 4) & 0x0F);
    GOLD_WriteHexChar(GOLD_Interrupt & 0x0F);
    GOLD_WriteLog("\n");

    /* Initialize state */
    GOLD_SoundPlaying = FALSE;
    GOLD_CallBack = NULL;
    GOLD_AudioBuffer = NULL;
    GOLD_MainVolume = 255;

    /*
     * Configure control chip:
     * - Set mix/filter to playback mode
     * - Enable audio interrupt (AEN bit)
     * - DISABLE DMA (clear DENO bit) - we use PIO mode
     */
    GOLD_EnableCtrl();
    {
        unsigned char mixSel;
        unsigned char ctr13;

        /* Set mix select register 0x11 to playback mode */
        mixSel = GOLD_ReadCtrlReg(GOLD_CT_REG_MIX_SELECT);
        mixSel &= 0xFC;  /* Clear bits 0-1 for playback filters */
        GOLD_WriteCtrlReg(GOLD_CT_REG_MIX_SELECT, mixSel);

        /* Read current IRQ/DMA register for saving */
        ctr13 = GOLD_ReadCtrlReg(GOLD_CT_REG_AUDIO_IRQ_DMA0);
        GOLD_CTR13_Save = ctr13;

        /* Enable audio interrupt, DISABLE DMA for PIO mode */
        ctr13 |= GOLD_CT_AEN_BIT;      /* Enable audio interrupt */
        ctr13 &= ~GOLD_CT_DENO_BIT;    /* Disable DMA (PIO mode) */

        GOLD_WriteCtrlReg(GOLD_CT_REG_AUDIO_IRQ_DMA0, ctr13);

        /* Verify: read back the register */
        {
            unsigned char ctr13_v;
            ctr13_v = GOLD_ReadCtrlReg(GOLD_CT_REG_AUDIO_IRQ_DMA0);
            GOLD_WriteLog("GOLD_Init:   ctr13 before=0x");
            GOLD_WriteHexChar((GOLD_CTR13_Save >> 4) & 0x0F);
            GOLD_WriteHexChar(GOLD_CTR13_Save & 0x0F);
            GOLD_WriteLog(" after=0x");
            GOLD_WriteHexChar((ctr13_v >> 4) & 0x0F);
            GOLD_WriteHexChar(ctr13_v & 0x0F);
            GOLD_WriteLog("\n");
        }
    }
    GOLD_DisableCtrl();

    /* Reset MMA channels */
    GOLD_ResetMMA();

    /* Set default sample rate and volume */
    GOLD_SetPlaybackRate(GOLD_DefaultSampleRate);
    GOLD_SetPCMVolume(GOLD_MainVolume);

    GOLD_Installed = TRUE;

    GOLD_WriteLog("GOLD_Init: initialization complete (PIO mode)\n");
    return GOLD_Ok;
}

/*---------------------------------------------------------------------
   Function: GOLD_Shutdown

   Ends playback and restores system resources.
---------------------------------------------------------------------*/
void GOLD_Shutdown(void)
{
    int irq;

    GOLD_WriteLog("GOLD_Shutdown: shutting down AdLib Gold driver\n");

    if (!GOLD_Installed)
    {
        return;
    }

    /* Stop playback */
    GOLD_StopPlayback();

    /* Reset MMA */
    GOLD_ResetMMA();

    /* Restore control chip settings */
    GOLD_EnableCtrl();
    GOLD_WriteCtrlReg(GOLD_CT_REG_AUDIO_IRQ_DMA0, GOLD_CTR13_Save);
    GOLD_DisableCtrl();

    /* Restore the original interrupt vector */
    irq = GOLD_Config.Interrupt;
    if (irq >= 8)
    {
        IRQ_RestoreVector(GOLD_Interrupt);
    }
    _dos_setvect(GOLD_Interrupt, GOLD_OldInt);

    GOLD_SoundPlaying = FALSE;
    GOLD_AudioBuffer = NULL;
    GOLD_CallBack = NULL;
    GOLD_Installed = FALSE;

    /* Close log file */
    if (GOLD_LogFile != NULL)
    {
        fclose(GOLD_LogFile);
        GOLD_LogFile = NULL;
    }

    GOLD_WriteLog("GOLD_Shutdown: shutdown complete\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_BeginBufferedPlayback

   Begins buffered PIO playback of digitized sound.

   Audio data is transferred from the buffer to the YMZ263 FIFO via PIO
   in the IRQ handler. No DMA is used.

   The buffer is a circular buffer that the mixer callback refills.
   The IRQ handler writes from GOLD_CurrentAudioPtr to the FIFO.
---------------------------------------------------------------------*/
int GOLD_BeginBufferedPlayback(char *BufferStart, int BufferSize,
                                int NumDivisions, unsigned SampleRate,
                                int MixMode, void (*CallBackFunc)(void))
{
    GOLD_WriteLog("GOLD_BeginBufferedPlayback: starting (PIO mode)\n");

    /* Stop any ongoing playback (logs counters from previous session) */
    GOLD_StopPlayback();

    /* Reset counters for new session */
    GOLD_IrqCount = 0;
    GOLD_SpuriousIrqCount = 0;
    GOLD_PioBytesWritten = 0;
    GOLD_LastIrqMma0 = 0;
    GOLD_LastIrqMma1 = 0;

    /* Set mix mode */
    GOLD_SetMixMode(MixMode);

    /* Setup audio buffer pointers for PIO writes */
    GOLD_AudioBuffer = BufferStart;
    GOLD_CurrentAudioPtr = BufferStart;
    GOLD_TotalBufferSize = BufferSize;
    GOLD_AudioBufferEnd = BufferStart + BufferSize;

    /* Set the sample rate (configures MMA registers) */
    GOLD_SetPlaybackRate(SampleRate);

    /* Store the callback */
    GOLD_CallBack = CallBackFunc;

    /* Set initial volume */
    GOLD_SetPCMVolume(GOLD_MainVolume);

    /* Enable interrupts on PIC */
    GOLD_EnableInterrupt();

    /* Calculate transfer length per division (for logging) */
    if (NumDivisions > 0)
    {
        GOLD_TransferLength = BufferSize / NumDivisions;
    }
    else
    {
        GOLD_TransferLength = BufferSize;
    }

    /* Log diagnostic state */
    {
        unsigned char pic1;
        unsigned char ctStatus;
        unsigned char mma0Status;
        unsigned char mma1Status;

        pic1 = inp(0x21);
        ctStatus = inp(GOLD_CTAddr);
        mma0Status = inp(GOLD_MMA0Addr);
        mma1Status = inp(GOLD_MMA1Addr);
        GOLD_WriteLog("GOLD_BeginBufferedPlayback: PIC1=0x");
        GOLD_WriteHexChar(pic1);
        GOLD_WriteLog(" CT=0x");
        GOLD_WriteHexChar(ctStatus);
        GOLD_WriteLog(" MMA0=0x");
        GOLD_WriteHexChar(mma0Status);
        GOLD_WriteLog(" MMA1=0x");
        GOLD_WriteHexChar(mma1Status);
        GOLD_WriteLog(" bufSz=");
        GOLD_WriteLogNum(BufferSize);
        GOLD_WriteLog("\n");
    }

    /*
     * Pre-fill the YMZ263 FIFO before starting playback.
     * Per datasheet: "The FIFO buffers should never be left to empty
     * themselves during playback (that is when GO bit is set)."
     * We fill the FIFO with audio data via PIO, then set GO.
     */
    GOLD_FifoPreFill();

    /* Start playback on the Gold card (sets GO bit) */
    GOLD_StartPlayback();

    /* Post-startup diagnostic: check YMZ263 status after a brief delay */
    {
        unsigned int waitCount;
        unsigned char mma0Status;

        waitCount = 0x10000;
        while (waitCount > 0)
        {
            waitCount--;
        }
        mma0Status = inp(GOLD_MMA0Addr);
        GOLD_WriteLog("GOLD_BeginBufferedPlayback: postStart MMA0=0x");
        GOLD_WriteHexChar(mma0Status);
        GOLD_WriteLog("\n");
    }

    GOLD_WriteLog("GOLD_BeginBufferedPlayback: playback running (PIO)\n");
    return GOLD_Ok;
}

/*---------------------------------------------------------------------
   Function: GOLD_GetCardInfo

   Returns the maximum sample bits and channels supported.
---------------------------------------------------------------------*/
int GOLD_GetCardInfo(int *MaxSampleBits, int *MaxChannels)
{
    *MaxSampleBits = 8;
    *MaxChannels = 2;

    return GOLD_Ok;
}
