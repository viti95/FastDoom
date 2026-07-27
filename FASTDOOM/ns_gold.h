//
// AdLib Gold 1000/2000 digital audio driver for FastDoom
// Header definitions for the YMZ263 (MMA) chip driver
//

#ifndef __GOLD_H
#define __GOLD_H

#include "ns_golddef.h"

#define GOLD_MONO_8BIT_SAMPLE_SIZE 1
#define GOLD_STEREO_8BIT_SAMPLE_SIZE (2 * GOLD_MONO_8BIT_SAMPLE_SIZE)

enum GOLD_ERRORS
{
    GOLD_Warning = -2,
    GOLD_Error = -1,
    GOLD_Ok = 0,
    GOLD_EnvNotFound,
    GOLD_AddrNotSet,
    GOLD_DMANotSet,
    GOLD_InvalidIrq,
    GOLD_UnableToSetIrq,
    GOLD_DmaError,
    GOLD_CardNotReady,
    GOLD_NoSoundPlaying,
    GOLD_InvalidParameter,
    GOLD_DPMI_Error,
    GOLD_OutOfMemory
};

typedef struct
{
    unsigned Address;
    unsigned Interrupt;
    unsigned Dma;
} GOLD_CONFIG;

extern GOLD_CONFIG GOLD_Config;
extern int GOLD_DMAChannel;
extern unsigned GOLD_SampleRate;

int GOLD_GetEnv(GOLD_CONFIG *Config);
int GOLD_SetCardSettings(GOLD_CONFIG Config);
int GOLD_GetCardSettings(GOLD_CONFIG *Config);

void GOLD_SetPlaybackRate(unsigned rate);
unsigned GOLD_GetPlaybackRate(void);

int GOLD_SetMixMode(int mode);
void GOLD_StopPlayback(void);

int GOLD_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions,
                              unsigned SampleRate, int MixMode, void (*CallBackFunc)(void));

int GOLD_SetPCMVolume(int volume);
int GOLD_GetCardInfo(int *MaxSampleBits, int *MaxChannels);

int GOLD_Init(void);
void GOLD_Shutdown(void);

#endif
