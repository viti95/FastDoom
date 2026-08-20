//
// WSS (Windows Sound System) driver for FastDoom
//

#ifndef __WSS_H
#define __WSS_H

#include "ns_wssdef.h"

#define WSS_MONO_8BIT_SAMPLE_SIZE 1
#define WSS_STEREO_8BIT_SAMPLE_SIZE (2 * WSS_MONO_8BIT_SAMPLE_SIZE)

enum WSS_ERRORS
{
    WSS_Warning = -2,
    WSS_Error = -1,
    WSS_Ok = 0,
    WSS_EnvNotFound,
    WSS_AddrNotSet,
    WSS_DMANotSet,
    WSS_InvalidIrq,
    WSS_UnableToSetIrq,
    WSS_DmaError,
    WSS_CardNotReady,
    WSS_NoSoundPlaying,
    WSS_InvalidParameter,
    WSS_DPMI_Error,
    WSS_OutOfMemory
};

typedef struct
{
    unsigned Address;
    unsigned Interrupt;
    unsigned Dma;
} WSS_CONFIG;

extern WSS_CONFIG WSS_Config;
extern int WSS_DMAChannel;
extern unsigned WSS_SampleRate;

int WSS_GetEnv(WSS_CONFIG *Config);
int WSS_SetCardSettings(WSS_CONFIG Config);
int WSS_GetCardSettings(WSS_CONFIG *Config);

void WSS_SetPlaybackRate(unsigned rate);
unsigned WSS_GetPlaybackRate(void);

int WSS_SetMixMode(int mode);
void WSS_StopPlayback(void);

int WSS_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions,
                              unsigned SampleRate, int MixMode, void (*CallBackFunc)(void));

int WSS_SetPCMVolume(int volume);
int WSS_GetCardInfo(int *MaxSampleBits, int *MaxChannels);

int WSS_Init(void);
void WSS_Shutdown(void);

#endif
