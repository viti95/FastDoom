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

/* Control chip "busy" flag (CTRL_ADDRESS, bit 6) */
#define GOLD_CTRL_BUSY 0x40

/* Bits set in the IRQ/DMA select register to enable the engine */
/* (DEN0 = bit 3, AEN = bit 7; AIL2 enables with 0x88) */
#define GOLD_CTRL_DMA_ENABLE 0x08
#define GOLD_CTRL_ENGINE_ENABLE 0x80

/*---------------------------------------------------------------------
   PCM engine registers.
---------------------------------------------------------------------*/

#define GOLD_PCM_RATE 9
#define GOLD_PCM_VOLUME 10
#define GOLD_PCM_FIFO_INIT 11
#define GOLD_PCM_FORMAT 12

/*
   PCM engine status register bits.  FIF0/FIF1 latch when the channel
   FIFO drops to the FIFO INT level and are cleared by reading the
   status port.  The hardware FIFO interrupts are masked in the SFC
   registers (see GOLD_SFC_*_MSK) because the DMA request level of
   the card keeps the FIFO pinned around the FIFO INT level, so an
   unmasked FIFO interrupt would fire on nearly every sample tick.
*/
#define GOLD_PCM_FIFO0_INT 0x01
#define GOLD_PCM_FIFO1_INT 0x02
#define GOLD_PCM_TIMER0_INT 0x10

/* GO bit of the rate/mode register */
#define GOLD_PCM_GO 0x01

/*---------------------------------------------------------------------
   MMA timer 0 registers and control bits.  Timer 0 is used as the
   mixer clock: it ticks once per mix chunk period (see
   GOLD_StartTimer).  Its clock is the MMA base tick, 12 x 44100 Hz
   (1.88964 usec), and it auto-reloads its latch at every tick.
---------------------------------------------------------------------*/

#define GOLD_PCM_TIMER0_LO 2
#define GOLD_PCM_TIMER0_HI 3
#define GOLD_PCM_TIMER_CTRL 8

/* Bits of the timer control register (08H) */
#define GOLD_TIMER_BASE_START 0x08 /* STB: start the base tick clock */
#define GOLD_TIMER0_START 0x01     /* ST0: load the latch, start timer 0 */

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

/*
   SFC0 of the channel that drives the DMA (channel 0) carries the
   MSK bit (bit 1) set: the hardware FIFO interrupt is masked.  The
   mixer is clocked by MMA timer 0 instead, and an unmasked FIFO
   interrupt would fire on nearly every sample tick because the card
   serves the DMA requests in a way that keeps the FIFO pinned around
   the FIFO INT level.
*/
#define GOLD_SFC_MSK 0x02

/* mono 8-bit */
#define GOLD_PRC_MONO_CH0 0x66
#define GOLD_PRC_MONO_CH1 0x00
#define GOLD_SFC_MONO_CH0 (0x05 | GOLD_SFC_MSK)
#define GOLD_SFC_MONO_CH1 0x02

/* stereo 8-bit.
   In interleave mode the Gold consumes the DMA stream channel 0,
   channel 1, channel 0, channel 1, ... and the game's mix buffer
   holds the left sample first (L R L R ..., see MV_Mix8BitStereo).
   Channel 0 therefore receives the left samples and must be routed
   to the left output (L bit, D5, set); channel 1 receives the right
   samples and must be routed to the right output (R bit, D6, set).
   Routing them the other way plays the two channels reversed. */
#define GOLD_PRC_STEREO_CH0 0x26
#define GOLD_PRC_STEREO_CH1 0x46
#define GOLD_SFC_STEREO_CH0 (0x85 | GOLD_SFC_MSK)
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

static int GOLD_TransferLength = 0;
static int GOLD_NumDivisions = 0;

/* The mix page (counted from the start of the DMA buffer) that the
   8237 was last seen on; the interrupt handler mixes one page per
   page the 8237 has advanced past it. */
static int GOLD_LastMixPage;
static int GOLD_MixMode = GOLD_MONO_8BIT;
static int GOLD_FreqIndex;

/* Shadow of the channel 0 rate/mode register (without the GO bit). */
static unsigned char GOLD_PrcShadow;

static int GOLD_IntController1Mask;
static int GOLD_IntController2Mask;
static int GOLD_OriginalIrqDmaReg;

static int GOLD_SoundPlaying;

static void (*GOLD_CallBack)(void);

/* Set in the interrupt handler; used to verify that the card is */
/* generating end-of-data interrupts (I_Printf may not be called */
/* from an ISR, so it is reported from the main context). */
static volatile int GOLD_FirstIrq;

#if (DEBUG_ENABLED == 1)

static volatile int GOLD_SpuriousIrqCount;

#endif

/*
   Mixer clock self-test diagnostics.  GOLD_TimerSelfTest runs the
   card's MMA timer 0 for two short runs and checks that it delivers
   interrupts; the interrupt handler contributes the counts and the
   OR of the status flags it observed.  Reported from the main
   context (I_Printf may not be called from an ISR).
*/
static volatile int GOLD_DiagStatusOr;
static volatile int GOLD_DiagIrqCount;
static volatile int GOLD_DiagTimerIrqCount;

/*
   Fallback mixer clock: PIT channel 2 on IRQ 2, used when the
   card's MMA timer 0 does not deliver interrupts on this card.
*/
static int GOLD_UsingPit;
static void(__interrupt __far *GOLD_PitOldInt)(void);
static long GOLD_PitDivisor;
static int GOLD_Port61;

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

    /* Tweak a few bits and write them back... */
    left ^= 0x05;
    right ^= 0x0A;

    GOLD_WriteControlReg(GOLD_CTRL_LEFT_VOLUME, left);
    GOLD_WriteControlReg(GOLD_CTRL_RIGHT_VOLUME, right);

    /* ...and see if the changes took effect. */
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

    /* Control chip found: restore the original values. */
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

    /* Reset both FIFOs. */
    GOLD_WritePCMReg(0, GOLD_PCM_RATE, 0x80);
    GOLD_WritePCMReg(1, GOLD_PCM_RATE, 0x80);

    /* Write 4 dummy bytes to allow proper FIFO DMA initialization. */
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

   Sets the GO bit to start the PCM engine.  Called only when
   playback begins; the GO bit then stays set for the whole
   playback run (the interrupt handler must not rewrite the
   rate/mode register, see GOLD_ServiceInterrupt) and is cleared by
   GOLD_HaltTransfer.
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
   Function: GOLD_FlipChunk

   Flips the high bit of each sample byte in a chunk of the mix
   buffer.  The Gold plays 8-bit unsigned samples (128 = silence).

   Some emulators scale the 8-bit sample into a 16-bit value with
   an overflow:  sample * 256 wraps to negative for samples >= 128,
   so silence (128) becomes full negative swing and positive peaks
   fold near zero, producing a badly distorted (rectified) waveform.
   Flipping the high bit before the DMA reads the chunk turns that
   broken scaling into an exact linear one:  the wrapped value
   becomes (sample - 128) * 256 for every sample.  Each chunk is
   flipped exactly once, immediately before it is programmed for
   DMA (the mixer always rewrites the chunk with fresh samples
   before it is played again).
---------------------------------------------------------------------*/

static void GOLD_FlipChunk(
    char *buffer,
    int count)

{
    int index;

    for (index = 0; index < count; index++)
    {
        buffer[index] ^= 0x80;
    }
}

/*---------------------------------------------------------------------
   Function: GOLD_ClearChunk

   Fills a chunk of the mix buffer with silence (0x80, the game's
   8-bit silence value, see SILENCE_8BIT).  GOLD_DoMix calls this on
   the page it is about to mix so the high-bit flip is always applied
   to a clean game format base.  Without it an empty page keeps the
   bytes its last flip left behind, and the repeated flip alternates
   the page between silence and full swing, a click on every other
   page while no sound is playing.
---------------------------------------------------------------------*/

static void GOLD_ClearChunk(
    char *buffer,
    int count)

{
    int index;

    for (index = 0; index < count; index++)
    {
        buffer[index] = 0x80;
    }
}

/*---------------------------------------------------------------------
   Function: GOLD_StartTimer

   Starts MMA timer 0 with a period of one mix chunk.  The timer
   clock is the MMA base tick (12 x 44100 Hz, 1.88964 usec), so the
   chunk period in base ticks is

       chunk bytes * 529200 / DMA bytes per second,

   which is an exact integer for every supported rate.  The tick
   therefore stays phase locked to the 8237 page boundaries (no
   drift) and fires once per page, a full page ahead of the DMA.
---------------------------------------------------------------------*/

static void GOLD_StartTimer(void)
{
    long ticks;
    int bps;

    bps = (int)GOLD_SampleRate * ((GOLD_MixMode & GOLD_STEREO) ? 2 : 1);
    ticks = (long)GOLD_TransferLength * 529200L / (long)bps;

    GOLD_WritePCMReg(0, GOLD_PCM_TIMER0_LO, (int)(ticks & 0xFF));
    GOLD_WritePCMReg(0, GOLD_PCM_TIMER0_HI, (int)((ticks >> 8) & 0xFF));

    /* Load the latch and start timer 0 (interrupt unmasked). */
    GOLD_WritePCMReg(0, GOLD_PCM_TIMER_CTRL,
                     GOLD_TIMER_BASE_START | GOLD_TIMER0_START);
}

/*---------------------------------------------------------------------
   Function: GOLD_StopTimer

   Stops MMA timer 0 (and the base tick clock).
---------------------------------------------------------------------*/

static void GOLD_StopTimer(void)
{
    GOLD_WritePCMReg(0, GOLD_PCM_TIMER_CTRL, 0x00);
}

static void GOLD_DoMix(void);

/*---------------------------------------------------------------------
   Function: GOLD_PitInterrupt

   Mixer clock interrupt from PIT channel 2 (IRQ 2), the fallback
   clock used when the card's MMA timer 0 does not deliver
   interrupts (see GOLD_StartPitClock).
---------------------------------------------------------------------*/

void __interrupt __far GOLD_PitInterrupt(
    void)

{
    if (GOLD_SoundPlaying)
    {
        GOLD_FirstIrq = TRUE;
        GOLD_DoMix();
    }

    /* Send EOI, then let the previous IRQ 2 handler run (the PC
       speaker driver, if one is installed). */
    OutByte20h(0x20);
    _chain_intr(GOLD_PitOldInt);
}

/*---------------------------------------------------------------------
   Function: GOLD_StartPitClock

   Starts PIT channel 2 (IRQ 2) as the mixer clock.  The rate
   generator is set to one tick per mix chunk period.  The tick is
   not exact (the PIT clock is 1.193182 MHz, not the card's
   44100 x 12 clock), but the drift is less than one page in about
   ten minutes and the position based catch-up in GOLD_DoMix absorbs
   the residual drift.
---------------------------------------------------------------------*/

static void GOLD_StartPitClock(void)
{
    long divisor;
    int bps;
    int mask;

    bps = (int)GOLD_SampleRate * ((GOLD_MixMode & GOLD_STEREO) ? 2 : 1);
    divisor = (1193182L * (long)GOLD_TransferLength + (long)bps / 2) /
              (long)bps;
    if (divisor > 65535)
    {
        divisor = 65535;
    }
    GOLD_PitDivisor = divisor;

    /* Save and replace the old IRQ 2 handler. */
    GOLD_PitOldInt = _dos_getvect(0x0A);
    _dos_setvect(0x0A, GOLD_PitInterrupt);

    /* Save port 0x61 and make sure the PIT channel 2 clock is gated
       in (bit 0) while the speaker is disconnected (bit 1), so the
       rate generator runs but no tone is heard.  If the clock gate
       (bit 0) is off, channel 2 never counts and no interrupt is
       produced. */
    GOLD_Port61 = inp(0x61);
    outp(0x61, (GOLD_Port61 & ~0x03) | 0x01);

    /* PIT channel 2: mode 2 (rate generator), 16-bit binary count.
       0xF4 = channel 2, both bytes, mode 2, binary.  (0xE4 would
       load only the low byte and truncate the divisor.) */
    outp(0x43, 0xF4);
    outp(0x42, (int)(divisor & 0xFF));
    outp(0x42, (int)((divisor >> 8) & 0xFF));

    /* Unmask IRQ 2. */
    mask = inp(0x21) & ~0x04;
    outp(0x21, mask);

    GOLD_UsingPit = TRUE;

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        I_Printf("GOLD: PIT channel 2 mixer clock started, divisor %s\n",
                 GOLD_LogNumber(b1, (int)GOLD_PitDivisor, 10));
    }
#endif
}

/*---------------------------------------------------------------------
   Function: GOLD_StopPitClock

   Stops the PIT channel 2 mixer clock and restores the system
   (speaker muted, IRQ 2 masked, previous handler restored).
---------------------------------------------------------------------*/

static void GOLD_StopPitClock(void)
{
    int mask;

    /* Stop channel 2 (mode 3, count 0) and restore port 0x61.
       0xF6 = channel 2, both bytes, mode 3, binary.  (A latch
       command such as 0xB6 does not stop the counter.) */
    outp(0x43, 0xF6);
    outp(0x42, 0);
    outp(0x42, 0);
    outp(0x61, GOLD_Port61);

    /* Mask IRQ 2 again (restore the original mask bit). */
    mask = inp(0x21) & ~0x04;
    mask |= GOLD_IntController1Mask & 0x04;
    outp(0x21, mask);

    /* Restore the old IRQ 2 handler. */
    _dos_setvect(0x0A, GOLD_PitOldInt);

    GOLD_UsingPit = FALSE;
}

/*---------------------------------------------------------------------
   Function: GOLD_WaitPitTicks

   Spins until PIT channel 0 has completed `ticks` output cycles, or
   until `stopflag` becomes nonzero, whichever comes first.

   A channel 0 output cycle is detected by latching its count and
   noticing it reload (the latched value jumps back up to the top of
   its count).  This is a CPU-speed independent delay: at the
   standard 18.2 Hz channel 0 rate one cycle is about 55 ms.

   (The previous version read the free-running count of channel 0,
   which changes every 838 ns, so the "wait" actually returned after
   a few tens of microseconds - far too short to observe a mixer
   clock interrupt whose first tick comes about 23 ms later.)
---------------------------------------------------------------------*/

static void GOLD_WaitPitTicks(
    int ticks,
    volatile int *stopflag)

{
    int prev;
    int cur;
    int count;

    outp(0x43, 0xB0);
    prev = inp(0x40);
    prev |= inp(0x40) << 8;

    count = 0;
    while (count < ticks)
    {
        if ((stopflag != NULL) && (*stopflag != 0))
        {
            break;
        }

        outp(0x43, 0xB0);
        cur = inp(0x40);
        cur |= inp(0x40) << 8;

        if (cur > prev)
        {
            count++;
        }

        prev = cur;
    }
}

/*---------------------------------------------------------------------
   Function: GOLD_TimerSelfTest

   Verifies that the card's MMA timer 0 can deliver the mixer clock
   interrupt on this card, and that its high latch byte loads.  The
   timer is run twice (periods of 1000 and 2000 base ticks) for four
   channel 0 output cycles each, and the interrupt handler counts the
   interrupts of each run.  If the high byte loads, doubling the
   period halves the interrupt rate, so the first count is about
   twice the second; if the high byte does not load, both periods
   collapse to their low bytes (232 and 208) and the two counts are
   about equal.

   Returns GOLD_Ok if the timer fired a few times and its high byte
   loads (first count about twice the second), GOLD_Error otherwise;
   the caller then uses PIT channel 2 as the mixer clock.
---------------------------------------------------------------------*/

static int GOLD_TimerSelfTest(void)
{
    int n1;
    int n2;
    int t;

    /* Measurement 1: timer 0 with a period of 1000 base ticks, run
       for four channel 0 output cycles.  The interrupt handler
       counts the timer interrupts it receives. */
    GOLD_DiagStatusOr = 0;
    GOLD_DiagIrqCount = 0;
    GOLD_DiagTimerIrqCount = 0;
    GOLD_FirstIrq = FALSE;

    GOLD_WritePCMReg(0, GOLD_PCM_TIMER0_LO, 1000 & 0xFF);
    GOLD_WritePCMReg(0, GOLD_PCM_TIMER0_HI, 1000 >> 8);
    GOLD_WritePCMReg(0, GOLD_PCM_TIMER_CTRL,
                     GOLD_TIMER_BASE_START | GOLD_TIMER0_START);

    GOLD_EnableInterrupt();
    for (t = 0; t < 4; t++)
    {
        /* The interrupt handler counts the timer interrupts and ORs
           the status flags it sees into GOLD_DiagStatusOr.  The main
           context must not read the status here: the read clears the
           latched flags and could race with the handler (clearing a
           flag the handler has not read yet). */
        GOLD_WaitPitTicks(1, NULL);
    }
    n1 = GOLD_DiagTimerIrqCount;
    GOLD_DisableInterrupt();
    GOLD_StopTimer();
    (void)inp(GOLD_Config.Address + GOLD_PCM_ADDRESS); /* clear stale flags */

    /* Measurement 2: the same, but with a period of 2000 base ticks.
       If the high latch byte loads, doubling the period halves the
       interrupt rate, so n1 is about 2 x n2.  If the high byte does
       not load, both periods collapse to their low bytes (232 and
       208) and n1 is about 0.9 x n2. */
    GOLD_DiagIrqCount = 0;
    GOLD_DiagTimerIrqCount = 0;

    GOLD_WritePCMReg(0, GOLD_PCM_TIMER0_LO, 2000 & 0xFF);
    GOLD_WritePCMReg(0, GOLD_PCM_TIMER0_HI, 2000 >> 8);
    GOLD_WritePCMReg(0, GOLD_PCM_TIMER_CTRL,
                     GOLD_TIMER_BASE_START | GOLD_TIMER0_START);

    GOLD_EnableInterrupt();
    for (t = 0; t < 4; t++)
    {
        GOLD_WaitPitTicks(1, NULL);
    }
    n2 = GOLD_DiagTimerIrqCount;
    GOLD_DisableInterrupt();
    GOLD_StopTimer();

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        char b2[12];
        char b3[12];
        I_Printf("GOLD: timer self-test: 1000 ticks -> %s IRQs, 2000 ticks -> %s IRQs, status OR=0x%s\n",
                 GOLD_LogNumber(b1, n1, 10),
                 GOLD_LogNumber(b2, n2, 10),
                 GOLD_LogNumber(b3, GOLD_DiagStatusOr, 16));
    }
#endif

    /* The timer works if it fired a few times (n1 >= 3) and the
       high latch byte loads (n1 about 2 x n2, i.e. 3*n2 <= 2*n1
       <= 5*n2).  A dead timer gives n1 = 0; a broken high byte
       gives n1 about 0.9 x n2 (fails the lower bound). */
    if (n1 >= 3 && (3 * n2 <= 2 * n1) && (2 * n1 <= 5 * n2))
    {
        return (GOLD_Ok);
    }

    return (GOLD_Error);
}

/*---------------------------------------------------------------------
   Function: GOLD_WaitFirstIrq

   Waits for the first mixer clock interrupt (GOLD_FirstIrq, set in
   the interrupt handler).  PIT channel 0 (about 18.2 Hz) is used as
   the clock so the timeout (about 550 ms) does not depend on the
   CPU speed.  Returns GOLD_FirstIrq.
---------------------------------------------------------------------*/

static int GOLD_WaitFirstIrq(void)
{
    GOLD_WaitPitTicks(10, &GOLD_FirstIrq);

    return (GOLD_FirstIrq);
}

/*---------------------------------------------------------------------
   Function: GOLD_EnableInterrupt

   Enables the triggering of the Gold's interrupt (MMA timer 0).
---------------------------------------------------------------------*/

void GOLD_EnableInterrupt(
    void)

{
    int Irq;
    int mask;

    /* Unmask system interrupt */
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

    /* Restore interrupt mask */
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
   Function: GOLD_DoMix

   Mixes in as many pages of the mix buffer as the 8237 has advanced
   since the last mix, and flips the page that was just mixed.

   Called from the mixer clock interrupt (the card's MMA timer 0, or
   PIT channel 2 as a fallback, see GOLD_StartPitClock).  The 8237
   is not reprogrammed at all: it runs a single auto-initializing
   transfer over the whole mix buffer for the entire playback run.
   Instead the 8237's live address is read and the number of pages
   it has advanced past the last mix is determined.  MV_ServiceVoc()
   derives its target page from the 8237's current position itself
   (the page after the 8237's current one), so one call per advanced
   page keeps the voices in step with the DMA.  In steady state
   exactly one page has advanced per tick; more than one only happens
   after the CPU was stalled, in which case the catch-up
   re-synchronizes the voices with the DMA.
---------------------------------------------------------------------*/

static void GOLD_DoMix(void)
{
    int page;
    int delta;
    int index;
    char *pos;

    /* Page of the mix buffer the 8237 is currently reading. */
    pos = DMA_GetCurrentPos(GOLD_DMAChannel);
    page = (int)((unsigned long)(pos - GOLD_DMABuffer) /
                 (unsigned long)GOLD_TransferLength);
    page %= GOLD_NumDivisions;

    /* How many pages the 8237 has advanced past the last mix. */
    delta = page - GOLD_LastMixPage;
    if (delta < 0)
    {
        delta += GOLD_NumDivisions;
    }

    if (delta > 0)
    {
        GOLD_LastMixPage = page;

        /* Mix one page per advanced page.  Every call mixes the */
        /* page after the 8237's current one and advances the */
        /* voices by one page, so the voices end up exactly in */
        /* step with the DMA.  Each page is cleared to silence */
        /* first and flipped to Gold format after, so the flip is */
        /* always applied to a clean base (see GOLD_ClearChunk). */
        for (index = 0; index < delta; index++)
        {
            int mixpage = (page + 1 + index) % GOLD_NumDivisions;
            char *mixbuf = GOLD_DMABuffer +
                           (unsigned long)mixpage * (unsigned long)GOLD_TransferLength;

            GOLD_ClearChunk(mixbuf, GOLD_TransferLength);

            if (GOLD_CallBack != NULL)
            {
                MV_ServiceVoc();
            }

            /* Flip the page to Gold format, before the 8237 reaches */
            /* it (one page period later, see GOLD_FlipChunk). */
            GOLD_FlipChunk(mixbuf, GOLD_TransferLength);
        }
    }
    else
    {
#if (DEBUG_ENABLED == 1)
        /* A mixer clock interrupt without any page advance (false */
        /* trigger or spurious): count it, reported at stop time. */
        GOLD_SpuriousIrqCount++;
#endif
    }
}

/*---------------------------------------------------------------------
   Function: GOLD_ServiceInterrupt

   Handles the card's interrupt line.  While playback is running the
   line is driven by the mixer clock, which is the card's MMA timer 0
   when the self-test (GOLD_TimerSelfTest) proved that it delivers
   interrupts, and PIT channel 2 otherwise.  The hardware FIFO
   interrupts are masked (see GOLD_SFC_*), so a flag in the PCM
   status register means a mixer clock tick.

   If no Gold flag is set, the interrupt belongs to someone else on
   a shared IRQ line and the previous handler is called.
---------------------------------------------------------------------*/

void __interrupt __far GOLD_ServiceInterrupt(
    void)

{
    int status;

    /* Read the PCM engine status; the read also clears the latched */
    /* interrupt flags. */
    status = inp(GOLD_Config.Address + GOLD_PCM_ADDRESS);

    /* Collect diagnostic status bits (reported from the main */
    /* context). */
    GOLD_DiagStatusOr |= status;

    if ((status & (GOLD_PCM_TIMER0_INT | GOLD_PCM_FIFO0_INT |
                   GOLD_PCM_FIFO1_INT)) == 0)
    {
        /* No Gold flag is set: the interrupt belongs to someone */
        /* else on a shared IRQ line.  Call the old handler. */
        _chain_intr(GOLD_OldInt);
        return;
    }

    /* Remember that the card generated an interrupt; verified in */
    /* GOLD_BeginBufferedPlayback and reported at stop time */
    /* (I_Printf may not be called from an ISR). */
    GOLD_FirstIrq = TRUE;
    GOLD_DiagIrqCount++;
    if (status & GOLD_PCM_TIMER0_INT)
    {
        GOLD_DiagTimerIrqCount++;
    }

    if (GOLD_SoundPlaying)
    {
        GOLD_DoMix();
    }

    /* Send EOI to the interrupt controller. */
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
    /* The Gold only plays 8-bit samples. */
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

    /* The stereo pair rate equals the channel rate (the interleaved */
    /* L R L R stream is consumed one byte per channel per sample */
    /* tick), so no adjustment is needed for stereo. */

    /* Find the nearest supported rate. */
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

    /* Keep track of what the actual rate is. */
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
    int usingpit;

    if (!GOLD_Installed)
    {
        return;
    }

    /* Make the interrupt handler stop mixing; a late interrupt can */
    /* still arrive after the mask below. */
    GOLD_SoundPlaying = FALSE;

    /* Don't allow anymore interrupts */
    GOLD_DisableInterrupt();

    /* Stop the PIT channel 2 mixer clock, if it is in use */
    usingpit = GOLD_UsingPit;
    if (GOLD_UsingPit)
    {
        GOLD_StopPitClock();
    }

    /* Stop the PCM engine (clears the GO bit) */
    GOLD_HaltTransfer();

    /* Stop the MMA timer */
    GOLD_StopTimer();

    /* Disable the DMA channel */
    DMA_EndTransfer(GOLD_DMAChannel);

    GOLD_DMABuffer = NULL;

#if (DEBUG_ENABLED == 1)
    if (GOLD_FirstIrq)
    {
        I_Printf("GOLD: mixer clock interrupt received, IRQ working\n");
    }

    if (usingpit)
    {
        I_Printf("GOLD: mixer clock: PIT channel 2 (card timer 0 unusable)\n");
    }
    else
    {
        I_Printf("GOLD: mixer clock: card MMA timer 0\n");
    }

    if (GOLD_SpuriousIrqCount > 0)
    {
        char b1[12];
        I_Printf("GOLD: %s false mixer interrupts ignored\n",
                 GOLD_LogNumber(b1, GOLD_SpuriousIrqCount, 10));
    }

    I_Printf("GOLD: playback stopped\n");
#endif
}

/*---------------------------------------------------------------------
   Function: GOLD_BeginBufferedPlayback

   Begins multibuffered playback of digitized sound on the Gold.

   The Gold's PCM engine generates the DMA requests.  The 8237 is
   programmed once, for a single auto-initializing transfer over
   the entire mix buffer, and then loops on its own for the whole
   playback run: the FIFO never runs dry and the 8237 is never
   reprogrammed (reprogramming it on every chunk masked the channel
   and changed its end-of-transfer state, which showed up as a
   periodic pop).  MMA timer 0 ticks once per chunk period and its
   interrupt (GOLD_ServiceInterrupt) drives the mixer.
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
    int usecardtimer;

    GOLD_SetMixMode(MixMode);

    GOLD_SetPlaybackRate(SampleRate);

    GOLD_CallBack = CallBackFunc;

    GOLD_DMABuffer = BufferStart;
    GOLD_TransferLength = BufferSize / NumDivisions;
    GOLD_NumDivisions = NumDivisions;
    GOLD_LastMixPage = 0;

#if (DEBUG_ENABLED == 1)
    {
        char b1[12];
        char b2[12];
        char b3[12];
        char b4[12];
        char b5[12];
        I_Printf("GOLD: starting playback: %s bytes in %s chunks of %s bytes, IRQ %s, DMA %s (auto-init)\n",
                 GOLD_LogNumber(b1, BufferSize, 10),
                 GOLD_LogNumber(b2, NumDivisions, 10),
                 GOLD_LogNumber(b3, GOLD_TransferLength, 10),
                 GOLD_LogNumber(b4, (int)GOLD_Config.Interrupt, 10),
                 GOLD_LogNumber(b5, GOLD_DMAChannel, 10));
    }
#endif

    /* Pre-flip the whole buffer (the game cleared it to zero, which
       is full negative swing on the Gold; flipped, it is silence).
       The pages are then re-mixed (and re-flipped) by the interrupt
       handler as the mixer advances, one page ahead of the DMA.
       Pre-flipping everything keeps the first couple of pages
       (played before the first mix lands) silent instead of a loud
       tone. */
    GOLD_FlipChunk(GOLD_DMABuffer, BufferSize);

    /* If a previous run left the PIT clock running, stop it first;
       the self-test below re-decides the mixer clock. */
    if (GOLD_UsingPit)
    {
        GOLD_StopPitClock();
    }

    /* Verify that the card's MMA timer 0 can deliver the mixer
       clock interrupt on this card.  On cards where it cannot, PIT
       channel 2 is used as the mixer clock instead. */
    usecardtimer = (GOLD_TimerSelfTest() == GOLD_Ok);

#if (DEBUG_ENABLED == 1)
    if (usecardtimer)
    {
        I_Printf("GOLD: mixer clock: card MMA timer 0\n");
    }
    else
    {
        I_Printf("GOLD: mixer clock: PIT channel 2 (card timer 0 unusable)\n");
    }
#endif

    /* One auto-initializing transfer over the whole buffer.  At
       every end of transfer the 8237 reloads the start address and
       the full count and keeps going, so the DMA never stops and
       never has to be reprogrammed. */
    status = DMA_SetupTransfer(GOLD_DMAChannel, GOLD_DMABuffer,
                               BufferSize, DMA_AutoInitRead);
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

    /* Start the mixer clock (one tick per chunk period): the
       card's timer, or PIT channel 2 as the fallback. */
    if (usecardtimer)
    {
        GOLD_StartTimer();
        GOLD_EnableInterrupt();
    }
    else
    {
        GOLD_StartPitClock();
    }

    /* Start the PCM engine.  The GO bit stays set until
       GOLD_StopPlayback; the engine keeps running over the whole
       loop, fed by the auto-initializing transfer. */
    GOLD_StartTransfer();

    GOLD_SoundPlaying = TRUE;

    /* Wait for the first mixer clock interrupt.  It comes after one
       chunk period (about 23 ms at 11025 Hz stereo); if the clock
       never interrupts the mixer would stall silently, so bail out
       instead. */
    GOLD_FirstIrq = FALSE;
    if (!GOLD_WaitFirstIrq() && usecardtimer && !GOLD_UsingPit)
    {
        /* The self-test passed but no interrupt arrived at the real
           period: the card's timer is still unusable; try the PIT. */
        GOLD_StopTimer();
        GOLD_DisableInterrupt();
        GOLD_StartPitClock();

#if (DEBUG_ENABLED == 1)
        I_Printf("GOLD: no timer interrupt at the real period, "
                 "falling back to PIT channel 2\n");
#endif

        GOLD_FirstIrq = FALSE;
        GOLD_WaitFirstIrq();
    }

    if (!GOLD_FirstIrq)
    {
#if (DEBUG_ENABLED == 1)
        I_Printf("GOLD: no mixer clock interrupt, check the card's "
                 "IRQ/DMA settings (e.g. GOLD=388:5:1)\n");
#endif
        GOLD_StopPlayback();
        return (GOLD_Error);
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

    /* Use the settings provided by the caller, if any. */
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

    /* Save the interrupt masks */
    GOLD_IntController1Mask = inp(0x21);
    GOLD_IntController2Mask = inp(0xA1);

    /* Look for the control chip */
    status = GOLD_FindCard();
    if (status != GOLD_Ok)
    {
#if (DEBUG_ENABLED == 1)
        I_Printf("GOLD: no Ad Lib Gold found at this address, "
                 "try the GOLD environment variable (e.g. GOLD=220:5:1)\n");
#endif
        return (GOLD_Error);
    }

    /* Configure the control chip for digital playback */
    GOLD_EnableControl();

    /* Set the audio filter to playback mode */
    GOLD_WriteControlReg(GOLD_CTRL_AUDIO_SELECT,
                         GOLD_ReadControlReg(GOLD_CTRL_AUDIO_SELECT) & 0xFC);

    /* Read the IRQ/DMA select register. */
    irqdma = GOLD_ReadControlReg(GOLD_CTRL_IRQ_DMA_SELECT);
    GOLD_OriginalIrqDmaReg = irqdma;

    /* Some Gold cards read the select register back as 0xFF, so the */
    /* IRQ line and DMA channel can be supplied via the GOLD */
    /* environment variable.  Anything not supplied is taken from the */
    /* register when it is readable, otherwise the common default of */
    /* IRQ 5, DMA 1 is used. */
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

        /* Make sure the register value matches the IRQ/DMA in use. */
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

    /* Tell the card which DMA channel the PCM engine must use, and */
    /* enable the DMA channel and the audio engine. */
    GOLD_WriteControlReg(GOLD_CTRL_IRQ_DMA_SELECT,
                         irqdma | GOLD_CTRL_DMA_ENABLE |
                         GOLD_CTRL_ENGINE_ENABLE);

    GOLD_DisableControl();

    /* Reset the PCM engine */
    GOLD_Reset();

    /* Make sure the MMA timer is stopped */
    GOLD_StopTimer();

    /* Set the volume to maximum */
    GOLD_WritePCMReg(0, GOLD_PCM_VOLUME, 127);
    GOLD_WritePCMReg(1, GOLD_PCM_VOLUME, 127);

    GOLD_SoundPlaying = FALSE;

    GOLD_CallBack = NULL;

    GOLD_DMABuffer = NULL;

    GOLD_MixMode = GOLD_MONO_8BIT;
    GOLD_SetPlaybackRate(GOLD_DefaultSampleRate);

    /* Install our interrupt handler */
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

    /* Halt the DMA transfer */
    GOLD_StopPlayback();

    /* Restore the original IRQ/DMA selection */
    GOLD_EnableControl();
    GOLD_WriteControlReg(GOLD_CTRL_IRQ_DMA_SELECT, GOLD_OriginalIrqDmaReg);
    GOLD_DisableControl();

    /* Reset the PCM engine */
    GOLD_Reset();

    /* Restore the original interrupt */
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
