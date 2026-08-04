/*
 * AdLib Gold 1000/2000 Digital Audio Driver for FastDoom
 *
 * Implements PIO-based PCM playback via the YMZ263 (MMA) sampling
 * channels on the AdLib Gold sound card. No DMA is used.
 *
 * Mono 8-bit mode only. FastDoom's mixer generates mono 8-bit samples
 * which are written ONE byte per tick by the task manager (TS_ScheduleTask).
 * PRC0 has both L+R bits set so mono output goes to both speakers.
 *
 * The YMZ263 interleaved stereo (ILV=1) mode does not generate FIFO
 * threshold interrupts on some emulators (DOSBox-X). Mono mode (ILV=0)
 * with task-driven output works reliably.
 *
 * Based on the AIL2 DMASOUND.ASM driver by John Miles (Miles Design, Inc.)
 * and the AdLib Gold Developer Toolkit (YMZ263 programming guide).
 *
 * Task-driven PIO mode (mirrors ns_lpt.c):
 *   - YMZ263 has a 128-byte FIFO (channel 0, mono output L+R)
 *   - FIFO_INT threshold set to highest (112 bytes, FIFO_INT=0)
 *   - Task manager fires at sample rate (e.g. 11025 Hz)
 *   - Each task tick writes ONE mono byte to the YMZ263 FIFO
 *   - FIFO acts as a natural timing buffer (smooths out timing)
 *   - Mixer callback called every N ticks (buffer division boundary)
 */

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "std_func.h"
#include "ns_task.h"
#include "ns_fxm.h"
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

/* PIO audio buffer pointers (used for task-driven PIO writes) */
static char *GOLD_AudioBuffer;
static char *GOLD_AudioBufferEnd;
static char *GOLD_CurrentBuffer;      /* start of current division */
static char *GOLD_SoundPtr;           /* current write position within division */
static int GOLD_NumBuffers;           /* number of divisions */
static int GOLD_BufferNum;            /* current division index */
static int GOLD_TotalBufferSize;
static int GOLD_TransferLength = 0;

static int GOLD_MainVolume = 255;
static int GOLD_MixMode = GOLD_DefaultMixMode;

static volatile int GOLD_SoundPlaying = FALSE;
static volatile long GOLD_BytesWritten = 0;

void (*GOLD_CallBack)(void);

/*
 * Task manager handles per-sample output.
 * TS_ScheduleTask fires at the sample rate (e.g. 11025 Hz).
 * Each tick writes ONE byte to the YMZ263 FIFO.
 */
static task *GOLD_Task;
static int GOLD_CurrentLength = 0;  /* remaining bytes in current division */

/*
 * Shadow of PRC (playback/recording control) register values
 * so we can toggle GO bit without disturbing other settings.
 */
static unsigned char GOLD_PRC0_Shadow;
static unsigned char GOLD_PRC1_Shadow;

/* Saved control register 0x13 value for restore on shutdown */
static unsigned char GOLD_CTR13_Save;

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
 * FIFO threshold constant.
 * We use FIFO_INT=0 (112 bytes) so the FIFO interrupt rarely fires.
 * Audio output is driven by the task manager (TS_ScheduleTask),
 * which fires at the sample rate and writes ONE byte per tick.
 * The FIFO just acts as a natural timing buffer.
 *
 * FIFO_INT values: 0=112, 1=96, 2=80, 3=64, 4=48, 5=32, 6=16 (bytes remaining)
 */
#define GOLD_SFC_FIFO_INT_NORMAL  0   /* IRQ fires at 112 bytes (rarely) */

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
     * We use PIO mode (ENB=0) with FIFO_INT=0 (112 bytes threshold).
     * Audio output is driven by the task manager (TS_ScheduleTask),
     * which fires at the sample rate and writes ONE byte per tick.
     * The FIFO threshold interrupt is set to the highest threshold
     * so it rarely fires - audio output is task-driven.
     *
     * NOTE: ILV=1 (interleaved stereo) mode does not generate FIFO
     * threshold interrupts on some YMZ263 emulators. We use ILV=0
     * (mono mode) with mono 8-bit data for reliable playback.
     *
     * Channel 1: MSK=1 (mask interrupt), not used for mono playback.
     */
    fmt0 = (unsigned char)((GOLD_SFC_FIFO_INT_NORMAL << 1) & 0x1E);
    fmt1 = 0x02;    /* MSK=1, interrupt masked for ch1 */

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

    GOLD_WriteLog("GOLD_SetPlaybackRate: requested=");
    GOLD_WriteLogNum((int)rate);
    GOLD_WriteLog(" actual=");
    GOLD_WriteLogNum((int)GOLD_SampleRate);
    GOLD_WriteLog(" freqIdx=");
    GOLD_WriteLogNum(rateIdx);
    GOLD_WriteLog("\n");
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

    /*
     * Force mono 8-bit mode regardless of caller request.
     *
     * The YMZ263 interleaved stereo (ILV=1) mode does not generate FIFO
     * threshold interrupts on some emulators (DOSBox-X), causing zero
     * IRQs and complete loss of sound. Mono mode (ILV=0) works reliably.
     *
     * FastDoom's mixer generates mono 8-bit data when told mono mode.
     * The buffer contains mono samples (1 byte per sample), written
     * directly to the FIFO. PRC0 has both L+R bits set so output
     * goes to both speakers.
     */
    GOLD_MixMode = GOLD_MONO_8BIT;

    if (mode != GOLD_MixMode)
    {
        GOLD_WriteLog("GOLD_SetMixMode: WARNING - caller requested mode ");
        GOLD_WriteLogNum(mode);
        GOLD_WriteLog(", forced to mono 8-bit (ILV interrupt workaround)\n");
    }

    /*
     * Set format control for mono 8-bit, PIO mode.
     * ILV=0 (no interleaving), ENB=0 (DMA disabled).
     * FIFO_INT = 0: IRQ fires at 112 bytes (highest threshold).
     * Audio output is driven by the task manager, not FIFO interrupts.
     *
     * SFC register layout:
     *   Bit 7: ILV (interleave stereo channels) - 0 = mono
     *   Bits 6-5: DATAFMT (00=8-bit)
     *   Bits 4-1: FIFO_INT (threshold index)
     *   Bit 1: MSK (1=mask interrupt)
     *   Bit 0: ENB (1=DMA mode, 0=PIO mode)
     */
    fmt0 = (unsigned char)((GOLD_SFC_FIFO_INT_NORMAL << 1) & 0x1E);
    fmt1 = 0x02;    /* MSK=1, interrupt masked for ch1 */

    /* Only reconfigure if not playing */
    if (!GOLD_SoundPlaying)
    {
        GOLD_WriteMMAReg(0, GOLD_MMA_FMT_CTL, fmt0);
        GOLD_WriteMMAReg(1, GOLD_MMA_FMT_CTL, fmt1);
    }

    GOLD_WriteLog("GOLD_SetMixMode: requested=");
    GOLD_WriteLogNum(mode);
    GOLD_WriteLog(" forced=");
    GOLD_WriteLogNum(GOLD_MixMode);
    GOLD_WriteLog(" (mono 8-bit)\n");
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
   We need >112 bytes in the FIFO (FIFO_INT=0 threshold).

   NOTE: GOLD_SetPlaybackRate already writes 4 dummy bytes to the FIFO
   during initialization. Since the FIFO is 128 bytes, we only write
   124 more bytes here to avoid overflowing the FIFO.

   In mono mode, the buffer contains mono 8-bit data from the mixer.
   Each byte is one mono sample - written directly to the FIFO.
---------------------------------------------------------------------*/
static void GOLD_FifoPreFill(void)
{
    int bytesToWrite;
    char *pioAddr;
    int i;

    /* 128-byte FIFO minus 4 dummy bytes from SetPlaybackRate = 124 */
    bytesToWrite = GOLD_MMA_FIFO_SIZE - 4;
    pioAddr = GOLD_SoundPtr;

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

        /* Advance sound pointer */
        GOLD_SoundPtr += bytesToWrite;
        GOLD_CurrentLength -= bytesToWrite;
    }

    GOLD_WriteLog("GOLD_FifoPreFill: filled ");
    GOLD_WriteLogNum(bytesToWrite);
    GOLD_WriteLog(" mono bytes\n");
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

    GOLD_WriteLog("GOLD_StartPlayback: playback started\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_StopPlayback

   Stops playback by clearing the GO bit and terminating the task.
---------------------------------------------------------------------*/
void GOLD_StopPlayback(void)
{
    unsigned char prc0;

    GOLD_WriteLog("GOLD_StopPlayback: stopping playback\n");

    /* Terminate the output task */
    if (GOLD_Task != NULL)
    {
        TS_Terminate(GOLD_Task);
        GOLD_Task = NULL;
    }

    /* Clear GO bit on channel 0 (controls both channels) */
    prc0 = GOLD_PRC0_Shadow & (unsigned char)(~GOLD_MMA_GO_BIT);
    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, prc0);

    GOLD_SoundPlaying = FALSE;
    GOLD_AudioBuffer = NULL;

    GOLD_WriteLog("GOLD_StopPlayback: playback stopped\n");
    GOLD_WriteLog("GOLD_StopPlayback: bytesWritten=");
    GOLD_WriteLogNumSigned(GOLD_BytesWritten);
    GOLD_WriteLog(" rate=");
    GOLD_WriteLogNum((int)GOLD_SampleRate);
    GOLD_WriteLog("\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_ServiceTask

   Task manager callback for per-sample audio output.
   Fired at the sample rate (e.g. 11025 Hz). Each tick writes ONE
   byte to the YMZ263 FIFO. This approach mirrors ns_lpt.c.

   The YMZ263 FIFO acts as a natural timing buffer. The FIFO
   threshold interrupt is set to the highest threshold (112 bytes)
   so it rarely fires - audio output is driven by the task manager.

   When a buffer division is exhausted, the mixer callback is called
   to refill the audio data.
---------------------------------------------------------------------*/
static void GOLD_ServiceTask(task *Task)
{
    /* Write ONE byte to the YMZ263 FIFO */
    outp(GOLD_MMA0Addr, GOLD_MMA_PCM_DATA);
    GOLD_MMADelay();
    outp(GOLD_MMA0Data, (unsigned char)*GOLD_SoundPtr);

    GOLD_SoundPtr++;
    GOLD_BytesWritten++;

    GOLD_CurrentLength--;
    if (GOLD_CurrentLength == 0)
    {
        /* Advance to next buffer division (mirrors ns_lpt.c) */
        GOLD_CurrentBuffer += GOLD_TransferLength;
        GOLD_BufferNum++;
        if (GOLD_BufferNum >= GOLD_NumBuffers)
        {
            GOLD_BufferNum = 0;
            GOLD_CurrentBuffer = GOLD_AudioBuffer;
        }

        GOLD_CurrentLength = GOLD_TransferLength;
        GOLD_SoundPtr = GOLD_CurrentBuffer;

        /* Call the mixer callback to refill audio data */
        if (GOLD_CallBack != NULL)
        {
            MV_ServiceVoc();
        }
    }
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



    /* Initialize state */
    GOLD_SoundPlaying = FALSE;
    GOLD_CallBack = NULL;
    GOLD_AudioBuffer = NULL;
    GOLD_MainVolume = 255;

    /*
     * Configure control chip:
     * - Set mix/filter to playback mode
     * - DISABLE audio interrupt (clear AEN bit) - we use task manager
     * - DISABLE DMA (clear DENO bit) - we use PIO mode
     *
     * Audio output is driven by the task manager (TS_ScheduleTask),
     * not by FIFO threshold interrupts. The card's IRQ line must be
     * disabled to prevent hardware interrupts from interfering with
     * the task manager timing.
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

        /* DISABLE audio interrupt, DISABLE DMA for PIO mode */
        ctr13 &= ~GOLD_CT_AEN_BIT;     /* Disable audio interrupt */
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
   The task writes from GOLD_SoundPtr to the FIFO (mirrors ns_lpt.c).
---------------------------------------------------------------------*/
int GOLD_BeginBufferedPlayback(char *BufferStart, int BufferSize,
                                int NumDivisions, unsigned SampleRate,
                                int MixMode, void (*CallBackFunc)(void))
{
    GOLD_WriteLog("GOLD_BeginBufferedPlayback: starting (PIO mode)\n");

    /* Stop any ongoing playback (logs counters from previous session) */
    GOLD_StopPlayback();

    /* Reset counters for new session */
    GOLD_BytesWritten = 0;

    /* Set mix mode */
    GOLD_SetMixMode(MixMode);

    /* Setup audio buffer pointers (mirrors ns_lpt.c) */
    GOLD_AudioBuffer = BufferStart;
    GOLD_AudioBufferEnd = BufferStart + BufferSize;
    GOLD_TotalBufferSize = BufferSize;

    /* VITI95: OPTIMIZE */
    GOLD_TransferLength = BufferSize / NumDivisions;
    GOLD_CurrentLength = GOLD_TransferLength;
    GOLD_CurrentBuffer = BufferStart;
    GOLD_SoundPtr = BufferStart;
    GOLD_BufferNum = 0;
    GOLD_NumBuffers = NumDivisions;

    /* Set the sample rate (configures MMA registers) */
    GOLD_SetPlaybackRate(SampleRate);

    /* Store the callback */
    GOLD_CallBack = CallBackFunc;

    /* Set initial volume */
    GOLD_SetPCMVolume(GOLD_MainVolume);

    /*
     * Pre-fill the YMZ263 FIFO before starting playback.
     * Per datasheet: "The FIFO buffers should never be left to empty
     * themselves during playback (that is when GO bit is set)."
     * We fill the FIFO with audio data via PIO, then set GO.
     */
    GOLD_FifoPreFill();

    /* Start playback on the Gold card (sets GO bit) */
    GOLD_StartPlayback();

    /*
     * Schedule the per-sample output task.
     * TS_ScheduleTask fires at the sample rate (e.g. 11025 Hz).
     * Each tick writes ONE byte to the YMZ263 FIFO.
     * This mirrors the LPT driver approach (ns_lpt.c).
     */
    GOLD_Task = TS_ScheduleTask(GOLD_ServiceTask, (int)GOLD_SampleRate, 1, NULL);
    TS_Dispatch();

    GOLD_WriteLog("GOLD_BeginBufferedPlayback: playback running (task)\n");
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
