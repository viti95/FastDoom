#ifndef __PAS16_H
#define __PAS16_H

enum PAS_ERRORS
{
    PAS_Warning = -2,
    PAS_Error = -1,
    PAS_Ok = 0,
    PAS_DriverNotFound,
    PAS_DmaError,
    PAS_InvalidIrq,
    PAS_UnableToSetIrq,
    PAS_Dos4gwIrqError,
    PAS_NoSoundPlaying,
    PAS_CardNotFound,
    PAS_DPMI_Error,
    PAS_OutOfMemory
};

#define PAS_MaxMixMode STEREO_8BIT
#define PAS_DefaultSampleRate 11000
#define PAS_DefaultMixMode MONO_8BIT
#define PAS_MaxIrq 15

#define PAS_MinSamplingRate 4000
#define PAS_MaxSamplingRate 44000

extern unsigned int PAS_DMAChannel;

void PAS_SetPlaybackRate(unsigned rate);
unsigned PAS_GetPlaybackRate(void);
int PAS_SetMixMode(int mode);
void PAS_StopPlayback(void);
int PAS_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, unsigned SampleRate, int MixMode, void (*CallBackFunc)(void));
int PAS_SetPCMVolume(int volume);
void PAS_SetFMVolume(int volume);
int PAS_GetFMVolume(void);
int PAS_GetCardInfo(int *MaxSampleBits, int *MaxChannels);
int PAS_SaveMusicVolume(void);
void PAS_RestoreMusicVolume(void);
int PAS_Init(void);
void PAS_Shutdown(void);

#endif
