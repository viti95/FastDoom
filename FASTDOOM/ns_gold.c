#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "std_func.h"
#include "ns_dma.h"
#include "ns_irq.h"
#include "ns_gold.h"
#include "ns_muldf.h"
#include "fastmath.h"
#include "i_debug.h"

/*---------------------------------------------------------------------
   Ad Lib Gold I/O ports, relative to the base address.
---------------------------------------------------------------------*/

#define GOLD_CTRL_ADDRESS 2
#define GOLD_CTRL_DATA 3
#define GOLD_PCM_ADDRESS 4
#define GOLD_PCM_DATA0 5
#define GOLD_PCM_DATA1 7

/*---------------------------------------------------------------------
   Control chip registers.
---------------------------------------------------------------------*/

#define GOLD_CTRL_LEFT_VOLUME 9
#define GOLD_CTRL_RIGHT_VOLUME 10
#define GOLD_CTRL_AUDIO_SELECT 11
#define GOLD_CTRL_IRQ_DMA_SELECT 13

// Control chip "busy" flag (CTRL_ADDRESS, bit 6)
#define GOLD_CTRL_BUSY 0x40

// Bits set in the IRQ/DMA select register to enable the engine
// (DEN0 = bit 3, AEN = bit 7; AIL2 enables with 0x88)
#define GOLD_CTRL_DMA_ENABLE 0x08
#define GOLD_CTRL_ENGINE_ENABLE 0x80

/*---------------------------------------------------------------------
   PCM engine registers.
---------------------------------------------------------------------*/

#define GOLD_PCM_RATE 9
#define GOLD_PCM_VOLUME 10
#define GOLD_PCM_FIFO_INIT 11
#define GOLD_PCM_FORMAT 12

// PCM engine status bit: FIFO 0 empty (end of transfer) interrupt
#define GOLD_PCM_FIFO0_INT 0x01

// GO bit of the rate/mode register
#define GOLD_PCM_GO 0x01

/*---------------------------------------------------------------------
   PCM engine rate table.  The rate register selects one of these
   rates.  In stereo mode the interleaved L R L R DMA stream is
   consumed at this rate (each channel clocking at the rate), so
   the rate register is programmed with the sample rate directly.
---------------------------------------------------------------------*/

static const unsigned int GOLD_PCMRates[4] = {44100, 22050, 11025, 7350};
static const unsigned char GOLD_FreqBits[4] = {0x00, 0x08, 0x10, 0x18};

/*---------------------------------------------------------------------
   PCM engine rate/mode and format register values for the 8-bit
   mono and stereo formats.
---------------------------------------------------------------------*/

// mono 8-bit
#define GOLD_PRC_MONO_CH0 0x66
#define GOLD_PRC_MONO_CH1 0x00
#define GOLD_SFC_MONO_CH0 0x05
#define GOLD_SFC_MONO_CH1 0x02

// stereo 8-bit
#define GOLD_PRC_STEREO_CH0 0x46
#define GOLD_PRC_STEREO_CH1 0x26
#define GOLD_SFC_STEREO_CH0 0x85
#define GOLD_SFC_STEREO_CH1 0x03

/*---------------------------------------------------------------------
   IRQ lines selectable on the card (control chip, INT SEL bits).
---------------------------------------------------------------------*/

static const int GOLD_IrqTable[8] = {3, 4, 5, 7, 10, 11, 12, 15};

/*---------------------------------------------------------------------
   Function: GOLD_BuildIrqDmaReg

   Builds the control chip IRQ/DMA select register value from an IRQ
   line and a DMA channel.  Returns -1 if the IRQ line is not
   selectable on the card.
---------------------------------------------------------------------*/

static int GOLD_BuildIrqDmaReg(
    int irq,
    int dma)

{
    int i;

    for (i = 0; i < 8; i++)
    {
        if (GOLD_IrqTable[i] == irq)
        {
            return ((dma << 4) | i);
        }
    }

    return (-1);
}

/*---------------------------------------------------------------------
   Module data.
---------------------------------------------------------------------*/

GOLD_CONFIG GOLD_Config =
    {
        0, GOLD_UNDEFINED, GOLD_UNDEFINED};

int GOLD_DMAChannel = GOLD_UNDEFINED;

int GOLD_Installed = FALSE;

unsigned GOLD_SampleRate = GOLD_DefaultSampleRate;

static void(__interrupt __far *GOLD_OldInt)(void);

static char *GOLD_DMABuffer;
static char *GOLD_DMABufferEnd;
static char *GOLD_CurrentDMABuffer;

static int GOLD_TransferLength = 0;
static int GOLD_MixMode = GOLD_MONO_8BIT;
static int GOLD_FreqIndex;

// Shadow of the channel 0 rate/mode register (without the GO bit).
static unsigned char GOLD_PrcShadow;

static int GOLD_IntController1Mask;
static int GOLD_IntController2Mask;
static int GOLD_OriginalIrqDmaReg;

static int GOLD_SoundPlaying;

static void (*GOLD_CallBack)(void);

// Set in the interrupt handler; used to verify that the card is
// generating end-of-data interrupts (I_Printf may not be called
// from an ISR, so it is reported from the main context).
static volatile int GOLD_FirstIrq;

#if (DEBUG_ENABLED == 1)

static volatile int GOLD_SpuriousIrqCount;

#endif

/*---------------------------------------------------------------------
   Function: GOLD_LogNumber

   I_Printf formats all of its numeric conversions as fixed point
   values, so diagnostic numbers are converted to text here and
   logged as strings.  The text is written into buf, and buf is
   returned.
---------------------------------------------------------------------*/

#if (DEBUG_ENABLED == 1)

static char *GOLD_LogNumber(
    char *buf,
    int value,
    int radix)

{
    char tmp[16];
    char *dst;
    int i;
    int j;
    int digit;

    dst = buf;

    if (value < 0)
    {
        *dst++ = '-';
        value = -value;
    }

    i = 0;
    do
    {
        digit = value % radix;
        tmp[i++] = (char)(digit < 10 ? ('0' + digit) : ('A' + digit - 10));
        value /= radix;
    } while (value > 0);

    for (j = 0; j < i; j++)
    {
        *dst++ = tmp[i - 1 - j];
    }

    *dst = '\0';

    return (buf);
}

#endif

/*---------------------------------------------------------------------
   Function: GOLD_WaitControl

   Waits until the control chip is not busy.  Returns GOLD_Ok on
   success, GOLD_Error on timeout.
---------------------------------------------------------------------*/

static int GOLD_WaitControl(void)
{
    int count;
    int status;

    count = 500;
    status = GOLD_Error;

    do
    {
        if ((inp(GOLD_Config.Address + GOLD_CTRL_ADDRESS) &
             GOLD_CTRL_BUSY) == 0)
        {
            status = GOLD_Ok;
            break;
        }

        count--;
    } while (count > 0);

#if (DEBUG_ENABLED == 1)
    if (status == GOLD_Error)
    {
        I_Printf("GOLD: control chip busy timeout\n");
    }
#endif

    return (status);
}

/*---------------------------------------------------------------------
   Function: GOLD_EnableControl

   Enables access to the control chip.
---------------------------------------------------------------------*/

static void GOLD_EnableControl(void)
{
    outp(GOLD_Config.Address + GOLD_CTRL_ADDRESS, 0xFF);
}

/*---------------------------------------------------------------------
   Function: GOLD_DisableControl

   Disables access to the control chip.
---------------------------------------------------------------------*/

static void GOLD_DisableControl(void)
{
    GOLD_WaitControl();
    outp(GOLD_Config.Address + GOLD_CTRL_ADDRESS, 0xFE);
}

/*---------------------------------------------------------------------
   Function: GOLD_ReadControlReg

   Reads a control chip register.
---------------------------------------------------------------------*/

static int GOLD_ReadControlReg(int reg)
{
    int data;

    GOLD_WaitControl();
    outp(GOLD_Config.Address + GOLD_CTRL_ADDRESS, reg);
    GOLD_WaitControl();
    data = inp(GOLD_Config.Address + GOLD_CTRL_DATA);

    return (data);
}

/*---------------------------------------------------------------------
   Function: GOLD_WriteControlReg

   Writes a control chip register.
---------------------------------------------------------------------*/

static void GOLD_WriteControlReg(int reg, int data)
{
    GOLD_WaitControl();
    outp(GOLD_Config.Address + GOLD_CTRL_ADDRESS, reg);
    GOLD_WaitControl();
    outp(GOLD_Config.Address + GOLD_CTRL_DATA, data);
}

/*---------------------------------------------------------------------
   Function: GOLD_PCMDelay

   Waits at least 470 nsec between PCM engine accesses.
---------------------------------------------------------------------*/

static void GOLD_PCMDelay(void)
{
    volatile int count;

    count = 100;
    while (count > 0)
    {
        count--;
    }
}

/*---------------------------------------------------------------------
   Function: GOLD_WritePCMReg

   Writes a byte to the given PCM engine register of the given
   channel (0 or 1).
---------------------------------------------------------------------*/

static void GOLD_WritePCMReg(int channel, int reg, int data)
{
    int port;

    outp(GOLD_Config.Address + GOLD_PCM_ADDRESS, reg);
    GOLD_PCMDelay();

    port = GOLD_Config.Address + GOLD_PCM_DATA0 + (channel << 1);
    outp(port, data);
    GOLD_PCMDelay();
}

/*---------------------------------------------------------------------
   Function: GOLD_FindCard

   Checks for the presence of the Gold's control chip by toggling a
   few bits of the volume registers and verifying the change.
---------------------------------------------------------------------*/

static int GOLD_FindCard(void)
{
    int left;
    int right;
    int testleft;
    int testright;

    GOLD_EnableControl();

    left = GOLD_ReadControlReg(GOLD_CTRL_LEFT_VOLUME);
    right = GOLD_ReadControlReg(GOLD_CTRL_RIGHT_VOLUME);

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        char b2[12];
        char b3[12];
        I_Printf("GOLD: detection at 0x%s: volume registers L=0x%s R=0x%s\n",
                 GOLD_LogNumber(b1, (int)GOLD_Config.Address, 16),
                 GOLD_LogNumber(b2, left, 16),
                 GOLD_LogNumber(b3, right, 16));
    }
#endif

    // Tweak a few bits and write them back...
    left ^= 0x05;
    right ^= 0x0A;

    GOLD_WriteControlReg(GOLD_CTRL_LEFT_VOLUME, left);
    GOLD_WriteControlReg(GOLD_CTRL_RIGHT_VOLUME, right);

    // ...and see if the changes took effect.
    testleft = GOLD_ReadControlReg(GOLD_CTRL_LEFT_VOLUME);
    testright = GOLD_ReadControlReg(GOLD_CTRL_RIGHT_VOLUME);

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        char b2[12];
        char b3[12];
        char b4[12];
        I_Printf("GOLD: detection: wrote L=0x%s R=0x%s, read back L=0x%s R=0x%s\n",
                 GOLD_LogNumber(b1, left, 16),
                 GOLD_LogNumber(b2, right, 16),
                 GOLD_LogNumber(b3, testleft, 16),
                 GOLD_LogNumber(b4, testright, 16));
    }
#endif

    if ((testleft != left) || (testright != right))
    {
        GOLD_DisableControl();
        return (GOLD_Error);
    }

    // Control chip found: restore the original values.
    left ^= 0x05;
    right ^= 0x0A;

    GOLD_WriteControlReg(GOLD_CTRL_LEFT_VOLUME, left);
    GOLD_WriteControlReg(GOLD_CTRL_RIGHT_VOLUME, right);

    GOLD_DisableControl();

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        I_Printf("GOLD: control chip found at 0x%s\n",
                 GOLD_LogNumber(b1, (int)GOLD_Config.Address, 16));
    }
#endif

    return (GOLD_Ok);
}

/*---------------------------------------------------------------------
   Function: GOLD_Reset

   Resets the PCM engine FIFOs and loads the default rate/mode
   register values.
---------------------------------------------------------------------*/

static void GOLD_Reset(void)
{
    GOLD_WritePCMReg(0, GOLD_PCM_RATE, 0x80);
    GOLD_WritePCMReg(0, GOLD_PCM_RATE, 0x76);
    GOLD_WritePCMReg(1, GOLD_PCM_RATE, 0x80);
    GOLD_WritePCMReg(1, GOLD_PCM_RATE, 0x76);

    GOLD_PrcShadow = 0x76;
}

/*---------------------------------------------------------------------
   Function: GOLD_SetPCMFormat

   Programs the PCM engine rate, mode and format registers for the
   current sample rate and mix mode.  The GO bit is left clear.
---------------------------------------------------------------------*/

static void GOLD_SetPCMFormat(void)
{
    int prc0;
    int prc1;
    int sfc0;
    int sfc1;
    int freq;

    freq = GOLD_FreqBits[GOLD_FreqIndex];

    // Reset both FIFOs.
    GOLD_WritePCMReg(0, GOLD_PCM_RATE, 0x80);
    GOLD_WritePCMReg(1, GOLD_PCM_RATE, 0x80);

    // Write 4 dummy bytes to allow proper FIFO DMA initialization.
    GOLD_WritePCMReg(0, GOLD_PCM_FIFO_INIT, 0);
    GOLD_WritePCMReg(0, GOLD_PCM_FIFO_INIT, 0);
    GOLD_WritePCMReg(0, GOLD_PCM_FIFO_INIT, 0);
    GOLD_WritePCMReg(0, GOLD_PCM_FIFO_INIT, 0);

    if (GOLD_MixMode & GOLD_STEREO)
    {
        prc0 = GOLD_PRC_STEREO_CH0 | freq;
        prc1 = GOLD_PRC_STEREO_CH1 | freq;
        sfc0 = GOLD_SFC_STEREO_CH0;
        sfc1 = GOLD_SFC_STEREO_CH1;
    }
    else
    {
        prc0 = GOLD_PRC_MONO_CH0 | freq;
        prc1 = GOLD_PRC_MONO_CH1 | freq;
        sfc0 = GOLD_SFC_MONO_CH0;
        sfc1 = GOLD_SFC_MONO_CH1;
    }

    GOLD_PrcShadow = (unsigned char)prc0;

    GOLD_WritePCMReg(0, GOLD_PCM_RATE, prc0);
    GOLD_WritePCMReg(1, GOLD_PCM_RATE, prc1);
    GOLD_WritePCMReg(0, GOLD_PCM_FORMAT, sfc0);
    GOLD_WritePCMReg(1, GOLD_PCM_FORMAT, sfc1);
}

/*---------------------------------------------------------------------
   Function: GOLD_StartTransfer

   Sets the GO bit to start the PCM engine.
---------------------------------------------------------------------*/

static void GOLD_StartTransfer(void)
{
    GOLD_WritePCMReg(0, GOLD_PCM_RATE, GOLD_PrcShadow | GOLD_PCM_GO);
}

/*---------------------------------------------------------------------
   Function: GOLD_HaltTransfer

   Clears the GO bit to stop the PCM engine.
---------------------------------------------------------------------*/

static void GOLD_HaltTransfer(void)
{
    GOLD_WritePCMReg(0, GOLD_PCM_RATE, GOLD_PrcShadow & ~GOLD_PCM_GO);
}

/*---------------------------------------------------------------------
   Function: GOLD_GetDMACount

   Returns the remaining word count of the programmed DMA channel.
---------------------------------------------------------------------*/

static int GOLD_GetDMACount(void)
{
    int port;
    int count;

    if (GOLD_DMAChannel < 4)
    {
        port = GOLD_DMAChannel << 1;
    }
    else
    {
        port = 0x0C + ((GOLD_DMAChannel - 4) << 2);
    }

    // Reading the port twice returns the low, then the high byte.
    count = inp(port);
    count |= (inp(port) << 8);

    return (count);
}

/*---------------------------------------------------------------------
   Function: GOLD_EnableInterrupt

   Enables the triggering of the Gold's end-of-data interrupt.
---------------------------------------------------------------------*/

void GOLD_EnableInterrupt(
    void)

{
    int Irq;
    int mask;

    // Unmask system interrupt
    Irq = GOLD_Config.Interrupt;
    if (Irq < 8)
    {
        mask = inp(0x21) & ~(1 << Irq);
        outp(0x21, mask);
    }
    else
    {
        mask = inp(0xA1) & ~(1 << (Irq - 8));
        outp(0xA1, mask);

        mask = inp(0x21) & ~(1 << 2);
        outp(0x21, mask);
    }
}

/*---------------------------------------------------------------------
   Function: GOLD_DisableInterrupt

   Disables the triggering of the Gold's end-of-data interrupt.
---------------------------------------------------------------------*/

void GOLD_DisableInterrupt(
    void)

{
    int Irq;
    int mask;

    // Restore interrupt mask
    Irq = GOLD_Config.Interrupt;
    if (Irq < 8)
    {
        mask = inp(0x21) & ~(1 << Irq);
        mask |= GOLD_IntController1Mask & (1 << Irq);
        outp(0x21, mask);
    }
    else
    {
        mask = inp(0x21) & ~(1 << 2);
        mask |= GOLD_IntController1Mask & (1 << 2);
        outp(0x21, mask);

        mask = inp(0xA1) & ~(1 << (Irq - 8));
        mask |= GOLD_IntController2Mask & (1 << (Irq - 8));
        outp(0xA1, mask);
    }
}

/*---------------------------------------------------------------------
   Function: GOLD_ServiceInterrupt

   Handles the interrupt generated by the Gold when the DMA buffer
   has been completely consumed.  Programs the next chunk of the mix
   buffer and calls the user supplied callback function.
---------------------------------------------------------------------*/

void __interrupt __far GOLD_ServiceInterrupt(
    void)

{
    int status;
    int count;

    // Acknowledge the PCM engine and check if this is a FIFO
    // interrupt.
    status = inp(GOLD_Config.Address + GOLD_PCM_ADDRESS);
    if ((status & GOLD_PCM_FIFO0_INT) == 0)
    {
        // Wasn't our interrupt.  Call the old one.
        _chain_intr(GOLD_OldInt);
        return;
    }

    // Spurious interrupts occur on IRQ 7 (shared with the LPT port):
    // verify that the DMA transfer is truly over.
    if (GOLD_Config.Interrupt == 7)
    {
        count = GOLD_GetDMACount();
        if ((count != 0) && (count != 0xFFFF))
        {
#if (DEBUG_ENABLED == 1)
            // Count spurious IRQs; reported at stop time.
            GOLD_SpuriousIrqCount++;
#endif
            // Not a real end-of-transfer interrupt.
            if (GOLD_Config.Interrupt > 7)
            {
                OutByteA0h(0x20);
            }
            OutByte20h(0x20);
            return;
        }
    }

    // Remember that the card generated an end-of-data interrupt;
    // verified in GOLD_BeginBufferedPlayback and reported at stop
    // time (I_Printf may not be called from an ISR).
    GOLD_FirstIrq = TRUE;

    // Keep track of the current buffer and program the next chunk.
    if (GOLD_SoundPlaying)
    {
        GOLD_CurrentDMABuffer += GOLD_TransferLength;

        if (GOLD_CurrentDMABuffer >= GOLD_DMABufferEnd)
        {
            GOLD_CurrentDMABuffer = GOLD_DMABuffer;
        }

        DMA_SetupTransfer(GOLD_DMAChannel, GOLD_CurrentDMABuffer,
                          GOLD_TransferLength, DMA_SingleShotRead);

        // Re-assert the GO bit so the PCM engine resumes DMA fetching
        // for the new chunk.  AIL2 does this on every chunk
        // (hardware_xfer -> MMA_write(0, 9, PRC_0_shadow | GO));
        // without it the engine stops after the first chunk and the
        // audio pops out.
        GOLD_StartTransfer();
    }

    // Call the caller's callback function
    if (GOLD_CallBack != NULL)
    {
        MV_ServiceVoc();
    }

    // send EOI to Interrupt Controller
    if (GOLD_Config.Interrupt > 7)
    {
        OutByteA0h(0x20);
    }

    OutByte20h(0x20);
}

/*---------------------------------------------------------------------
   Function: GOLD_SetMixMode

   Sets the Gold to play samples in mono or stereo.
---------------------------------------------------------------------*/

int GOLD_SetMixMode(
    int mode)

{
    // The Gold only plays 8-bit samples.
    mode &= GOLD_MaxMixMode;

    GOLD_MixMode = mode;

    GOLD_SetPCMFormat();

#if (DEBUG_ENABLED == 1)
    if (mode & GOLD_STEREO)
    {
        I_Printf("GOLD: mix mode: stereo 8-bit\n");
    }
    else
    {
        I_Printf("GOLD: mix mode: mono 8-bit\n");
    }
#endif

    return (mode);
}

/*---------------------------------------------------------------------
   Function: GOLD_SetPlaybackRate

   Sets the rate at which the digitized sound will be played in
   hertz.  The actual rate is snapped to the nearest rate supported
   by the card.
---------------------------------------------------------------------*/

void GOLD_SetPlaybackRate(
    unsigned rate)

{
    int testrate;
    int index;
    int delta;
    int bestdelta;

    testrate = (int)rate;

    // The stereo pair rate equals the channel rate (the interleaved
    // L R L R stream is consumed one byte per channel per sample
    // tick), so no adjustment is needed for stereo.

    // Find the nearest supported rate.
    GOLD_FreqIndex = 0;
    bestdelta = testrate - (int)GOLD_PCMRates[0];
    if (bestdelta < 0)
    {
        bestdelta = -bestdelta;
    }

    for (index = 1; index < 4; index++)
    {
        delta = testrate - (int)GOLD_PCMRates[index];
        if (delta < 0)
        {
            delta = -delta;
        }

        if (delta < bestdelta)
        {
            bestdelta = delta;
            GOLD_FreqIndex = index;
        }
    }

    // Keep track of what the actual rate is.
    GOLD_SampleRate = GOLD_PCMRates[GOLD_FreqIndex];

    GOLD_SetPCMFormat();

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        char b2[12];
        I_Printf("GOLD: sample rate: requested %s Hz, card uses %s Hz\n",
                 GOLD_LogNumber(b1, (int)rate, 10),
                 GOLD_LogNumber(b2, (int)GOLD_SampleRate, 10));
    }
#endif
}

/*---------------------------------------------------------------------
   Function: GOLD_GetPlaybackRate

   Returns the rate at which the digitized sound is being played in
   hertz.
---------------------------------------------------------------------*/

unsigned GOLD_GetPlaybackRate(
    void)

{
    return (GOLD_SampleRate);
}

/*---------------------------------------------------------------------
   Function: GOLD_StopPlayback

   Ends the DMA transfer of digitized sound to the Gold.
---------------------------------------------------------------------*/

void GOLD_StopPlayback(
    void)

{
    if (!GOLD_Installed)
    {
        return;
    }

    // Don't allow anymore interrupts
    GOLD_DisableInterrupt();

    // Stop the PCM engine
    GOLD_HaltTransfer();

    // Disable the DMA channel
    DMA_EndTransfer(GOLD_DMAChannel);

    GOLD_SoundPlaying = FALSE;

    GOLD_DMABuffer = NULL;

#if (DEBUG_ENABLED == 1)
    if (GOLD_FirstIrq)
    {
        I_Printf("GOLD: end-of-data interrupt received, IRQ working\n");
    }

    if (GOLD_SpuriousIrqCount > 0)
    {
        char b1[12];
        I_Printf("GOLD: %s spurious IRQ 7 ignored\n",
                 GOLD_LogNumber(b1, GOLD_SpuriousIrqCount, 10));
    }

    I_Printf("GOLD: playback stopped\n");
#endif
}

/*---------------------------------------------------------------------
   Function: GOLD_BeginBufferedPlayback

   Begins multibuffered playback of digitized sound on the Gold.

   The Gold's PCM engine generates the DMA requests, so the 8237 is
   programmed for one chunk (buffer division) at a time in single
   transfer mode; the end-of-data interrupt fires when a chunk is
   consumed and the handler programs the next one.
---------------------------------------------------------------------*/

int GOLD_BeginBufferedPlayback(
    char *BufferStart,
    int BufferSize,
    int NumDivisions,
    unsigned SampleRate,
    int MixMode,
    void (*CallBackFunc)(void))

{
    int status;

    GOLD_SetMixMode(MixMode);

    GOLD_SetPlaybackRate(SampleRate);

    GOLD_CallBack = CallBackFunc;

    GOLD_DMABuffer = BufferStart;
    GOLD_CurrentDMABuffer = BufferStart;
    GOLD_DMABufferEnd = BufferStart + BufferSize;

    GOLD_TransferLength = BufferSize / NumDivisions;

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        char b2[12];
        char b3[12];
        char b4[12];
        char b5[12];
        I_Printf("GOLD: starting playback: %s bytes in %s chunks of %s bytes, IRQ %s, DMA %s\n",
                 GOLD_LogNumber(b1, BufferSize, 10),
                 GOLD_LogNumber(b2, NumDivisions, 10),
                 GOLD_LogNumber(b3, GOLD_TransferLength, 10),
                 GOLD_LogNumber(b4, (int)GOLD_Config.Interrupt, 10),
                 GOLD_LogNumber(b5, GOLD_DMAChannel, 10));
    }
#endif

    // Program the first chunk.
    status = DMA_SetupTransfer(GOLD_DMAChannel, GOLD_CurrentDMABuffer,
                               GOLD_TransferLength, DMA_SingleShotRead);
    if (status == DMA_Error)
    {
#if (DEBUG_ENABLED == 1)
        {
            char b1[12];
            I_Printf("GOLD: DMA channel %s setup failed\n",
                     GOLD_LogNumber(b1, GOLD_DMAChannel, 10));
        }
#endif
        return (GOLD_Error);
    }

    GOLD_EnableInterrupt();

    // Start the PCM engine.
    GOLD_StartTransfer();

    GOLD_SoundPlaying = TRUE;

    // Wait for the first end-of-data interrupt.  The first chunk
    // takes about 23 ms to play at 11025 Hz; if the IRQ line or
    // DMA channel is wrong the interrupt never arrives and the
    // mixer would stall silently, so bail out instead.  PIT
    // channel 0 (about 18.2 Hz) is used as the clock so the
    // timeout does not depend on the CPU speed.
    {
        int pit;
        int ticks;

        pit = inp(0x40);
        ticks = 0;
        while ((!GOLD_FirstIrq) && (ticks < 10))
        {
            if (inp(0x40) != pit)
            {
                pit = inp(0x40);
                ticks++;
            }
        }

        /*if (!GOLD_FirstIrq)
        {
#if (DEBUG_ENABLED == 1)
            I_Printf("GOLD: no end-of-data interrupt, check the card's "
                     "IRQ/DMA settings (e.g. GOLD=388:5:1)\n");
#endif
            return (GOLD_Error);
        }*/
    }

#if (DEBUG_ENABLED == 1)
    I_Printf("GOLD: PCM engine started\n");
#endif

    return (GOLD_Ok);
}

/*---------------------------------------------------------------------
   Function: GOLD_GetCardInfo

   Returns the maximum number of bits that can represent a sample
   (8) and the number of channels (2 for stereo).
---------------------------------------------------------------------*/

int GOLD_GetCardInfo(
    int *MaxSampleBits,
    int *MaxChannels)

{
    *MaxSampleBits = 8;
    *MaxChannels = 2;

    return (GOLD_Ok);
}

/*---------------------------------------------------------------------
   Function: GOLD_GetEnv

   Retrieves the GOLD environment settings and returns them to the
   caller.  The variable holds the base address as a hexadecimal
   number, optionally followed by the IRQ line and the DMA channel
   in decimal, e.g. "GOLD=388" or "GOLD=388:5:1".  The default
   address is 0x388.
---------------------------------------------------------------------*/

int GOLD_GetEnv(
    GOLD_CONFIG *Config)
{
    char *Gold;
    char end;
    int irq;
    int dma;

    Config->Address = 0x388;
    Config->Interrupt = GOLD_UNDEFINED;
    Config->Dma8 = GOLD_UNDEFINED;

    Gold = getenv("GOLD");
    if (Gold == NULL)
    {
#if (DEBUG_ENABLED == 1)
        I_Printf("GOLD: no GOLD environment variable, using default base address 0x388\n");
#endif
        return (GOLD_Ok);
    }

    while (*Gold == ' ')
    {
        Gold++;
    }

    if (!isxdigit(*Gold))
    {
#if (DEBUG_ENABLED == 1)
        I_Printf("GOLD: GOLD environment variable is not a hex number: %s\n", Gold);
#endif
        return (GOLD_Error);
    }

    if (sscanf(Gold, "%x", &Config->Address) != 1)
    {
        return (GOLD_Error);
    }

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        char b2[12];
        char b3[12];
        I_Printf("GOLD: from GOLD environment variable: address 0x%s, IRQ %s, DMA %s\n",
                 GOLD_LogNumber(b1, (int)Config->Address, 16),
                 (Config->Interrupt != GOLD_UNDEFINED) ?
                     GOLD_LogNumber(b2, Config->Interrupt, 10) : "-",
                 (Config->Dma8 != GOLD_UNDEFINED) ?
                     GOLD_LogNumber(b3, Config->Dma8, 10) : "-");
    }
#endif

    return (GOLD_Ok);
}

/*---------------------------------------------------------------------
   Function: GOLD_SetCardSettings

   Sets up the Gold's parameters.
---------------------------------------------------------------------*/

int GOLD_SetCardSettings(
    GOLD_CONFIG Config)
{
    if (GOLD_Installed)
    {
        GOLD_Shutdown();
    }

    GOLD_Config.Address = Config.Address;
    GOLD_Config.Interrupt = Config.Interrupt;
    GOLD_Config.Dma8 = Config.Dma8;

    return (GOLD_Ok);
}

/*---------------------------------------------------------------------
   Function: GOLD_Init

   Initializes the Gold and prepares the module to play digitized
   sounds.
---------------------------------------------------------------------*/

int GOLD_Init(
    void)
{
    int status;
    int irqdma;
    int irq;
    int vector;

    if (GOLD_Installed)
    {
        GOLD_Shutdown();
    }

    // Use the settings provided by the caller, if any.
    if (GOLD_Config.Address == 0)
    {
        status = GOLD_GetEnv(&GOLD_Config);
        if (status != GOLD_Ok)
        {
            return (GOLD_Error);
        }
    }

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        I_Printf("GOLD: initializing, base address 0x%s\n",
                 GOLD_LogNumber(b1, (int)GOLD_Config.Address, 16));
    }
#endif

    // Save the interrupt masks
    GOLD_IntController1Mask = inp(0x21);
    GOLD_IntController2Mask = inp(0xA1);

    // Look for the control chip
    status = GOLD_FindCard();
    if (status != GOLD_Ok)
    {
#if (DEBUG_ENABLED == 1)
        I_Printf("GOLD: no Ad Lib Gold found at this address, "
                 "try the GOLD environment variable (e.g. GOLD=220:5:1)\n");
#endif
        return (GOLD_Error);
    }

    // Configure the control chip for digital playback
    GOLD_EnableControl();

    // Set the audio filter to playback mode
    GOLD_WriteControlReg(GOLD_CTRL_AUDIO_SELECT,
                         GOLD_ReadControlReg(GOLD_CTRL_AUDIO_SELECT) & 0xFC);

    // Read the IRQ/DMA select register.
    irqdma = GOLD_ReadControlReg(GOLD_CTRL_IRQ_DMA_SELECT);
    GOLD_OriginalIrqDmaReg = irqdma;

    // Some Gold cards read the select register back as 0xFF, so the
    // IRQ line and DMA channel can be supplied via the GOLD
    // environment variable.  Anything not supplied is taken from the
    // register when it is readable, otherwise the common default of
    // IRQ 5, DMA 1 is used.
    if (GOLD_Config.Interrupt != GOLD_UNDEFINED)
    {
        irq = GOLD_Config.Interrupt;
    }
    else if (irqdma != 0xFF)
    {
        irq = GOLD_IrqTable[irqdma & 7];
    }
    else
    {
        irq = 5;
    }

    if (GOLD_Config.Dma8 != GOLD_UNDEFINED)
    {
        GOLD_DMAChannel = GOLD_Config.Dma8;
    }
    else if (irqdma != 0xFF)
    {
        GOLD_DMAChannel = (irqdma >> 4) & 7;
    }
    else
    {
        GOLD_DMAChannel = 1;
    }

    {
        int reg;

        // Make sure the register value matches the IRQ/DMA in use.
        reg = GOLD_BuildIrqDmaReg(irq, GOLD_DMAChannel);
        if (reg < 0)
        {
#if (DEBUG_ENABLED == 1)
            {
                char b1[12];
                I_Printf("GOLD: IRQ %s is not selectable on the Gold\n",
                         GOLD_LogNumber(b1, irq, 10));
            }
#endif
            return (GOLD_Error);
        }

        irqdma = reg;
    }

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        char b2[12];
        char b3[12];
        const char *source;

        if ((GOLD_Config.Interrupt != GOLD_UNDEFINED) ||
            (GOLD_Config.Dma8 != GOLD_UNDEFINED))
        {
            source = " (GOLD env)";
        }
        else if (GOLD_OriginalIrqDmaReg == 0xFF)
        {
            source = " (default, register reads 0xff; set GOLD=388:5:1)";
        }
        else
        {
            source = " (card register)";
        }

        I_Printf("GOLD: IRQ/DMA select register 0x%s: using IRQ %s, DMA %s%s\n",
                 GOLD_LogNumber(b1, GOLD_OriginalIrqDmaReg, 16),
                 GOLD_LogNumber(b2, irq, 10),
                 GOLD_LogNumber(b3, GOLD_DMAChannel, 10),
                 source);
    }
#endif

    if (!VALID_IRQ(irq))
    {
        return (GOLD_Error);
    }

    status = DMA_VerifyChannel(GOLD_DMAChannel);
    if (status == DMA_Error)
    {
#if (DEBUG_ENABLED == 1)
        {
            char b1[12];
            I_Printf("GOLD: DMA channel %s not available\n",
                     GOLD_LogNumber(b1, GOLD_DMAChannel, 10));
        }
#endif
        return (GOLD_Error);
    }

    GOLD_Config.Interrupt = irq;

    // Tell the card which DMA channel the PCM engine must use, and
    // enable the DMA channel and the audio engine.
    GOLD_WriteControlReg(GOLD_CTRL_IRQ_DMA_SELECT,
                         irqdma | GOLD_CTRL_DMA_ENABLE |
                         GOLD_CTRL_ENGINE_ENABLE);

    GOLD_DisableControl();

    // Reset the PCM engine
    GOLD_Reset();

    // Set the volume to maximum
    GOLD_WritePCMReg(0, GOLD_PCM_VOLUME, 127);
    GOLD_WritePCMReg(1, GOLD_PCM_VOLUME, 127);

    GOLD_SoundPlaying = FALSE;

    GOLD_CallBack = NULL;

    GOLD_DMABuffer = NULL;

    GOLD_MixMode = GOLD_MONO_8BIT;
    GOLD_SetPlaybackRate(GOLD_DefaultSampleRate);

    // Install our interrupt handler
    vector = 0x08 + irq;
    GOLD_OldInt = _dos_getvect(vector);
    if (irq < 8)
    {
        _dos_setvect(vector, GOLD_ServiceInterrupt);
    }
    else
    {
        status = IRQ_SetVector(vector, GOLD_ServiceInterrupt);
        if (status != IRQ_Ok)
        {
            return (GOLD_Error);
        }
    }

    GOLD_Installed = TRUE;

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        char b2[12];
        char b3[12];
        char b4[12];
        I_Printf("GOLD: ready: address 0x%s, IRQ %s, DMA %s, %s Hz, 8-bit mono\n",
                 GOLD_LogNumber(b1, (int)GOLD_Config.Address, 16),
                 GOLD_LogNumber(b2, irq, 10),
                 GOLD_LogNumber(b3, GOLD_DMAChannel, 10),
                 GOLD_LogNumber(b4, (int)GOLD_SampleRate, 10));
    }
#endif

    return (GOLD_Ok);
}

/*---------------------------------------------------------------------
   Function: GOLD_Shutdown

   Ends transfer of sound data to the Gold and restores the system
   resources used by the card.
---------------------------------------------------------------------*/

void GOLD_Shutdown(
    void)
{
    int irq;
    int vector;

    if (!GOLD_Installed)
    {
        return;
    }

#if (DEBUG_ENABLED == 1)
    I_Printf("GOLD: shutting down\n");
#endif

    // Halt the DMA transfer
    GOLD_StopPlayback();

    // Restore the original IRQ/DMA selection
    GOLD_EnableControl();
    GOLD_WriteControlReg(GOLD_CTRL_IRQ_DMA_SELECT, GOLD_OriginalIrqDmaReg);
    GOLD_DisableControl();

    // Reset the PCM engine
    GOLD_Reset();

    // Restore the original interrupt
    irq = GOLD_Config.Interrupt;
    vector = 0x08 + irq;
    if (irq >= 8)
    {
        IRQ_RestoreVector(vector);
    }
    _dos_setvect(vector, GOLD_OldInt);

    GOLD_SoundPlaying = FALSE;

    GOLD_DMABuffer = NULL;

    GOLD_CallBack = NULL;

    GOLD_Installed = FALSE;
}
