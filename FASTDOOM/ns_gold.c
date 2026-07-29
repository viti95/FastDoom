
/*
 * AdLib Gold 1000/2000 Digital Audio Driver for FastDoom
 *
 * Implements DMA-based PCM playback via the YMZ263 (MMA) sampling
 * channels on the AdLib Gold sound card.
 *
 * Based on the AIL2 DMASOUND.ASM driver by John Miles (Miles Design, Inc.)
 * and the FastDoom Sound Blaster driver architecture.
 */

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "std_func.h"
#include "ns_dpmi.h"
#include "ns_dma.h"
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

int GOLD_DMAChannel;
unsigned GOLD_SampleRate = GOLD_DefaultSampleRate;

static char *GOLD_DMABuffer;
static char *GOLD_DMABufferEnd;
static char *GOLD_CurrentDMABuffer;
static int GOLD_TotalDMABufferSize;

static int GOLD_TransferLength = 0;
static int GOLD_MainVolume = 255;

static volatile int GOLD_SoundPlaying = FALSE;

void (*GOLD_CallBack)(void);

static unsigned short GOLD_Interrupt;

/*
 * Shadow of PRC (playback/recording control) register values
 * so we can toggle GO bit without disturbing other settings.
 */
static unsigned char GOLD_PRC0_Shadow;
static unsigned char GOLD_PRC1_Shadow;

/* Saved control register 0x13 value for restore on shutdown */
static unsigned char GOLD_CTR13_Save;

/*
 * DMA and interrupt controller state
 */
static int GOLD_IntController1Mask;
static int GOLD_IntController2Mask;

/*
 * IRQ handler stack (allocated in conventional memory via DPMI)
 */
#define GOLD_STACK_SIZE 2048

static unsigned short GOLD_StackSelector = 0;
static unsigned long GOLD_StackPointer;

/*
 * External stack switch functions (declared for pragma aux)
 */
extern void GOLD_GetStack(unsigned short *selptr, unsigned long *stackptr);
extern void GOLD_SetStack(unsigned short selector, unsigned long stackptr);

#pragma aux GOLD_GetStack = \
    "mov  [edi],esp"   \
    "mov  ax,ss"       \
    "mov  [esi],ax" parm[esi][edi] modify[eax esi edi];

#pragma aux GOLD_SetStack = \
    "mov  ss,ax"       \
    "mov  esp,edx" parm[ax][edx] modify[eax edx];

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

   Wait at least 470 nsec. between MMA register accesses.
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
     */
    prc0 = (unsigned char)(
        GOLD_MMA_R_BIT |
        GOLD_MMA_L_BIT |
        ((rateIdx & 3) << GOLD_MMA_FREQ_SHIFT) |
        GOLD_MMA_PCM_BIT |
        GOLD_MMA_PR_BIT
    );

    prc1 = prc0;

    /*
     * Format control register 0x0C:
     *   D7: ILV   - Interleave (0 for mono)
     *   D6-5: DATA FORMAT[1:0] - 0 = 8-bit MSB format
     *   D4-1: FIFO INT[3:0]    - Interrupt level (5 = 32 bytes remain)
     *   D0: ENB   - DMA enable
     */
    fmt0 = (unsigned char)(
        ((5 << GOLD_MMA_FIFO_INT_SHIFT) & 0x1E) |
        GOLD_MMA_DMA_ENB_BIT
    );
    fmt1 = fmt0;

    /* Reset FIFOs first */
    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, GOLD_MMA_RST_BIT);
    GOLD_WriteMMAReg(1, GOLD_MMA_PLAY_REC_CTL, GOLD_MMA_RST_BIT);

    /*
     * Write 4 dummy bytes to each channel's FIFO for proper
     * DMA initialization (from AIL2 driver).
     */
    outp(GOLD_MMA0Addr, GOLD_MMA_PCM_DATA);
    GOLD_MMADelay();
    outp(GOLD_MMA0Data, 0);
    GOLD_MMADelay();
    outp(GOLD_MMA0Addr, GOLD_MMA_PCM_DATA);
    GOLD_MMADelay();
    outp(GOLD_MMA0Data, 0);
    GOLD_MMADelay();
    outp(GOLD_MMA0Addr, GOLD_MMA_PCM_DATA);
    GOLD_MMADelay();
    outp(GOLD_MMA0Data, 0);
    GOLD_MMADelay();
    outp(GOLD_MMA0Addr, GOLD_MMA_PCM_DATA);
    GOLD_MMADelay();
    outp(GOLD_MMA0Data, 0);
    GOLD_MMADelay();

    outp(GOLD_MMA1Addr, GOLD_MMA_PCM_DATA);
    GOLD_MMADelay();
    outp(GOLD_MMA1Data, 0);
    GOLD_MMADelay();
    outp(GOLD_MMA1Addr, GOLD_MMA_PCM_DATA);
    GOLD_MMADelay();
    outp(GOLD_MMA1Data, 0);
    GOLD_MMADelay();
    outp(GOLD_MMA1Addr, GOLD_MMA_PCM_DATA);
    GOLD_MMADelay();
    outp(GOLD_MMA1Data, 0);
    GOLD_MMADelay();
    outp(GOLD_MMA1Addr, GOLD_MMA_PCM_DATA);
    GOLD_MMADelay();
    outp(GOLD_MMA1Data, 0);
    GOLD_MMADelay();

    /* Set format control on both channels */
    GOLD_WriteMMAReg(0, GOLD_MMA_FMT_CTL, fmt0);
    GOLD_WriteMMAReg(1, GOLD_MMA_FMT_CTL, fmt1);

    /* Save PRC shadows with GO=0 (set in StartPlayback) */
    GOLD_PRC0_Shadow = prc0;
    GOLD_PRC1_Shadow = prc1;

    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, prc0);
    GOLD_WriteMMAReg(1, GOLD_MMA_PLAY_REC_CTL, prc1);

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

    /* Recompute format control based on mix mode */
    fmt0 = (unsigned char)(
        ((5 << GOLD_MMA_FIFO_INT_SHIFT) & 0x1E) |
        GOLD_MMA_DMA_ENB_BIT
    );

    if (mode & GOLD_STEREO)
    {
        fmt0 |= GOLD_MMA_ILV_BIT;
    }

    fmt1 = fmt0;

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
   Function: GOLD_StartPlayback

   Start DMA playback by setting the GO bit on both MMA channels.
---------------------------------------------------------------------*/
static void GOLD_StartPlayback(void)
{
    unsigned char prc0;
    unsigned char prc1;

    prc0 = GOLD_PRC0_Shadow | GOLD_MMA_GO_BIT;
    prc1 = GOLD_PRC1_Shadow | GOLD_MMA_GO_BIT;

    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, prc0);
    GOLD_WriteMMAReg(1, GOLD_MMA_PLAY_REC_CTL, prc1);

    GOLD_SoundPlaying = TRUE;

    GOLD_WriteLog("GOLD_StartPlayback: playback started\n");
}

/*---------------------------------------------------------------------
   Function: GOLD_StopPlayback

   Stops DMA playback by clearing the GO bit, disabling interrupts,
   and ending the DMA transfer.
---------------------------------------------------------------------*/
void GOLD_StopPlayback(void)
{
    unsigned char prc0;
    unsigned char prc1;

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

    /* Clear GO bit on both channels */
    prc0 = GOLD_PRC0_Shadow & (unsigned char)(~GOLD_MMA_GO_BIT);
    prc1 = GOLD_PRC1_Shadow & (unsigned char)(~GOLD_MMA_GO_BIT);

    GOLD_WriteMMAReg(0, GOLD_MMA_PLAY_REC_CTL, prc0);
    GOLD_WriteMMAReg(1, GOLD_MMA_PLAY_REC_CTL, prc1);

    /* End DMA transfer */
    DMA_EndTransfer(GOLD_DMAChannel);

    GOLD_SoundPlaying = FALSE;
    GOLD_DMABuffer = NULL;

    GOLD_WriteLog("GOLD_StopPlayback: playback stopped\n");
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
   Function: GOLD_SetupDMABuffer

   Programs the DMAC for sound transfer using auto-init read mode.
---------------------------------------------------------------------*/
static int GOLD_SetupDMABuffer(char *BufferPtr, int BufferSize)
{
    int DmaStatus;

    if (GOLD_DMAChannel == GOLD_INVALID)
    {
        return GOLD_Error;
    }

    DmaStatus = DMA_SetupTransfer(GOLD_DMAChannel, BufferPtr, BufferSize,
                                   DMA_AutoInitRead);
    if (DmaStatus == DMA_Error)
    {
        GOLD_WriteLog("GOLD_SetupDMABuffer: DMA setup failed\n");
        return GOLD_Error;
    }

    GOLD_DMABuffer = BufferPtr;
    GOLD_CurrentDMABuffer = BufferPtr;
    GOLD_TotalDMABufferSize = BufferSize;
    GOLD_DMABufferEnd = BufferPtr + BufferSize;

    GOLD_WriteLog("GOLD_SetupDMABuffer: DMA configured\n");
    return GOLD_Ok;
}

/*---------------------------------------------------------------------
   Function: GOLD_GetCurrentPos

   Returns the offset within the current sound being played.
---------------------------------------------------------------------*/
int GOLD_GetCurrentPos(void)
{
    char *CurrentAddr;
    long offset;

    if (!GOLD_SoundPlaying)
    {
        return GOLD_Error;
    }

    CurrentAddr = DMA_GetCurrentPos(GOLD_DMAChannel);

    offset = (long)((unsigned long)CurrentAddr -
                    (unsigned long)GOLD_CurrentDMABuffer);

    /* Handle wraparound */
    if (offset < 0)
    {
        offset += GOLD_TotalDMABufferSize;
    }
    if (offset >= GOLD_TotalDMABufferSize)
    {
        offset -= GOLD_TotalDMABufferSize;
    }

    return (int)offset;
}

/*---------------------------------------------------------------------
   Function: GOLD_BeginBufferedPlayback

   Begins multibuffered playback of digitized sound.
---------------------------------------------------------------------*/
int GOLD_BeginBufferedPlayback(char *BufferStart, int BufferSize,
                                int NumDivisions, unsigned SampleRate,
                                int MixMode, void (*CallBackFunc)(void))
{
    int DmaStatus;

    GOLD_WriteLog("GOLD_BeginBufferedPlayback: starting\n");

    /* Stop any ongoing playback */
    GOLD_StopPlayback();

    /* Set mix mode */
    GOLD_SetMixMode(MixMode);

    /* Setup DMA buffer */
    DmaStatus = GOLD_SetupDMABuffer(BufferStart, BufferSize);
    if (DmaStatus == GOLD_Error)
    {
        return GOLD_Error;
    }

    /* Set the sample rate (configures MMA registers) */
    GOLD_SetPlaybackRate(SampleRate);

    /* Store the callback */
    GOLD_CallBack = CallBackFunc;

    /* Set initial volume */
    GOLD_SetPCMVolume(GOLD_MainVolume);

    /* Enable interrupts on PIC */
    GOLD_EnableInterrupt();

    /* Calculate transfer length per division */
    if (NumDivisions > 0)
    {
        GOLD_TransferLength = BufferSize / NumDivisions;
    }
    else
    {
        GOLD_TransferLength = BufferSize;
    }

    /* Start playback on the Gold card */
    GOLD_StartPlayback();

    GOLD_WriteLog("GOLD_BeginBufferedPlayback: playback running\n");
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

/*---------------------------------------------------------------------
   Function: GOLD_ServiceInterrupt

   ISR for the Gold card DMA/FIFO interrupt.

   The DMA controller is in auto-init mode, so it wraps automatically.
   We acknowledge the interrupt, advance the software pointer, call
   the mixer callback, and send EOI to the PIC.
---------------------------------------------------------------------*/
void __interrupt __far GOLD_ServiceInterrupt(void)
{
    unsigned char status;

    /* Acknowledge interrupt by reading control chip status register */
    status = inp(GOLD_CTAddr);

    /* Check if this was a sampling interrupt from the Gold card */
    if (!(status & GOLD_CT_SMP_BIT))
    {
        /* Not our interrupt */
        _chain_intr(GOLD_OldInt);
        return;
    }

    /* Advance the software buffer pointer */
    GOLD_CurrentDMABuffer += GOLD_TransferLength;

    if (GOLD_CurrentDMABuffer >= GOLD_DMABufferEnd)
    {
        GOLD_CurrentDMABuffer = GOLD_DMABuffer;
    }

    /* Call the mixer callback */
    if (GOLD_CallBack != NULL)
    {
        MV_ServiceVoc();
    }

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
    GOLD_PRC1_Shadow = GOLD_PRC0_Shadow;

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
   Function: GOLD_AllocateIRQStack

   Allocate conventional memory for the IRQ handler stack via DPMI.
---------------------------------------------------------------------*/
static unsigned short GOLD_AllocateIRQStack(unsigned short size)
{
    union REGS regs;

    memset(&regs, 0, sizeof(regs));
    regs.w.ax = 0x0100;  /* DPMI allocate DOS memory */
    regs.w.bx = (size + 15) / 16;

    int386(0x31, &regs, &regs);
    if (!regs.w.cflag)
    {
        return regs.w.dx;  /* Selector in DX */
    }

    return 0;
}

/*---------------------------------------------------------------------
   Function: GOLD_DeallocateIRQStack

   Free conventional memory allocated via DPMI.
---------------------------------------------------------------------*/
static void GOLD_DeallocateIRQStack(unsigned short selector)
{
    union REGS regs;

    if (selector != 0)
    {
        memset(&regs, 0, sizeof(regs));
        regs.w.ax = 0x0101;
        regs.w.dx = selector;
        int386(0x31, &regs, &regs);
    }
}

/*---------------------------------------------------------------------
   Function: GOLD_Init

   Initializes the Gold card and prepares the driver for playback.
---------------------------------------------------------------------*/
int GOLD_Init(void)
{
    int status;
    int irq;

    GOLD_WriteLog("GOLD_Init: initializing AdLib Gold driver\n");

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

    /* Verify DMA channel */
    if (GOLD_Config.Dma != GOLD_INVALID)
    {
        status = DMA_VerifyChannel(GOLD_Config.Dma);
        if (status == DMA_Error)
        {
            GOLD_WriteLog("GOLD_Init: ERROR - DMA channel invalid\n");
            return GOLD_Error;
        }
    }

    /* Use DMA channel from config (or auto-detected from control chip) */
    GOLD_DMAChannel = GOLD_Config.Dma;

    /*
     * If DMA was not set in config, try to read it from the control
     * chip register 0x13 (DMA SEL 0 bits).
     */
    if (GOLD_DMAChannel == GOLD_INVALID || GOLD_DMAChannel == GOLD_DEFAULT_DMA)
    {
        unsigned char ctr13;

        GOLD_EnableCtrl();
        ctr13 = GOLD_ReadCtrlReg(GOLD_CT_REG_AUDIO_IRQ_DMA0);
        GOLD_DisableCtrl();

        /* Extract DMA channel from bits 6-4 */
        GOLD_DMAChannel = (ctr13 >> GOLD_CT_DMA_SEL_SHIFT) & 0x07;

        /* Also extract IRQ from bits 2-0 */
        {
            unsigned char irqIdx;
            const unsigned char selected_IRQ[] =
            { 3, 4, 5, 7, 10, 11, 12, 15 };

            irqIdx = ctr13 & GOLD_CT_INT_SEL_MASK;
            if (irqIdx < 8)
            {
                GOLD_Config.Interrupt = selected_IRQ[irqIdx];
            }
        }

        GOLD_WriteLog("GOLD_Init: auto-detected DMA and IRQ from control chip\n");
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

    /* Allocate IRQ handler stack in conventional memory */
    GOLD_StackSelector = GOLD_AllocateIRQStack(GOLD_STACK_SIZE);
    if (GOLD_StackSelector == 0)
    {
        GOLD_WriteLog("GOLD_Init: ERROR - could not allocate IRQ stack\n");
        return GOLD_Error;
    }

    /* Leave room at top of stack */
    GOLD_StackPointer = GOLD_STACK_SIZE - sizeof(long);

    /* Save old interrupt handler and install ours */
    GOLD_OldInt = _dos_getvect(GOLD_Interrupt);
    if (irq < 8)
    {
        _dos_setvect(GOLD_Interrupt, GOLD_ServiceInterrupt);
    }
    else
    {
        status = IRQ_SetVector(GOLD_Interrupt, GOLD_ServiceInterrupt);
        if (status != IRQ_Ok)
        {
            GOLD_DeallocateIRQStack(GOLD_StackSelector);
            GOLD_StackSelector = 0;
            GOLD_WriteLog("GOLD_Init: ERROR - IRQ_SetVector failed\n");
            return GOLD_Error;
        }
    }

    /* Initialize state */
    GOLD_SoundPlaying = FALSE;
    GOLD_CallBack = NULL;
    GOLD_DMABuffer = NULL;
    GOLD_MainVolume = 255;

    /* Configure control chip: set mix/filter to playback mode */
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

        /* Enable DMA and audio interrupt for channel 0 */
        ctr13 |= GOLD_CT_DENO_BIT | GOLD_CT_AEN_BIT;
        /* Set DMA channel in register */
        ctr13 &= ~(0x70);  /* Clear DMA SEL bits */
        ctr13 |= ((GOLD_DMAChannel & 0x07) << GOLD_CT_DMA_SEL_SHIFT);
        GOLD_WriteCtrlReg(GOLD_CT_REG_AUDIO_IRQ_DMA0, ctr13);
    }
    GOLD_DisableCtrl();

    /* Reset MMA channels */
    GOLD_ResetMMA();

    /* Set default sample rate and volume */
    GOLD_SetPlaybackRate(GOLD_DefaultSampleRate);
    GOLD_SetPCMVolume(GOLD_MainVolume);

    GOLD_Installed = TRUE;

    GOLD_WriteLog("GOLD_Init: initialization complete\n");
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

    /* Free IRQ stack */
    GOLD_DeallocateIRQStack(GOLD_StackSelector);
    GOLD_StackSelector = 0;

    GOLD_SoundPlaying = FALSE;
    GOLD_DMABuffer = NULL;
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
