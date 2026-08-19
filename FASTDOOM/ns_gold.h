#ifndef __GOLD_H
#define __GOLD_H

/*---------------------------------------------------------------------
   Ad Lib Gold digital audio driver.

   The Ad Lib Gold card adds a stereo 8-bit PCM section (two DACs,
   each with its own FIFO) to the regular Ad Lib FM hardware.  The
   PCM section is controlled through a small I/O mapper ("control
   chip") and a "PCM engine" that generates the DMA requests and the
   end-of-data interrupt.

   I/O ports, relative to the base address (0x388 by default):
     base+2  control chip register select
     base+3  control chip data
     base+4  PCM engine register select (also status port)
     base+5  PCM engine data, channel 0
     base+7  PCM engine data, channel 1

   The IRQ line and DMA channel are selected on the card itself and
   are read back from the control chip.  The base address can be
   overridden with the GOLD environment variable (hex), e.g.:
     SET GOLD=388
---------------------------------------------------------------------*/

typedef struct
{
    unsigned Address;
    unsigned Interrupt;
    unsigned Dma8;
} GOLD_CONFIG;

extern GOLD_CONFIG GOLD_Config;
extern int GOLD_DMAChannel;
extern int GOLD_Installed;
extern unsigned GOLD_SampleRate;

#define GOLD_UNDEFINED -1

enum GOLD_ERRORS
{
    GOLD_Error = -1,
    GOLD_Ok = 0
};

/* Only 8-bit samples are supported, mono or stereo. */
#define GOLD_MONO_8BIT 0
#define GOLD_STEREO 1
#define GOLD_STEREO_8BIT (GOLD_STEREO)
#define GOLD_MaxMixMode GOLD_STEREO_8BIT

#define GOLD_DefaultSampleRate 11025

int GOLD_GetEnv(GOLD_CONFIG *Config);
int GOLD_SetCardSettings(GOLD_CONFIG Config);
int GOLD_GetCardInfo(int *MaxSampleBits, int *MaxChannels);
int GOLD_SetMixMode(int mode);
void GOLD_SetPlaybackRate(unsigned rate);
unsigned GOLD_GetPlaybackRate(void);
void GOLD_EnableInterrupt(void);
void GOLD_DisableInterrupt(void);
int GOLD_BeginBufferedPlayback(char *BufferStart,
                               int BufferSize, int NumDivisions,
                               unsigned SampleRate, int MixMode,
                               void (*CallBackFunc)(void));
void GOLD_StopPlayback(void);
int GOLD_Init(void);
void GOLD_Shutdown(void);

#endif
