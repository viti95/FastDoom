#ifndef __SNDSCAPE_H
#define __SNDSCAPE_H

extern int SOUNDSCAPE_DMAChannel;
extern int SOUNDSCAPE_ErrorCode;

enum SOUNDSCAPE_ERRORS
{
    SOUNDSCAPE_Warning = -2,
    SOUNDSCAPE_Error = -1,
    SOUNDSCAPE_Ok = 0,
    SOUNDSCAPE_EnvNotFound,
    SOUNDSCAPE_InitFileNotFound,
    SOUNDSCAPE_MissingProductInfo,
    SOUNDSCAPE_MissingPortInfo,
    SOUNDSCAPE_MissingDMAInfo,
    SOUNDSCAPE_MissingIRQInfo,
    SOUNDSCAPE_MissingSBIRQInfo,
    SOUNDSCAPE_MissingSBENABLEInfo,
    SOUNDSCAPE_MissingWavePortInfo,
    SOUNDSCAPE_HardwareError,
    SOUNDSCAPE_NoSoundPlaying,
    SOUNDSCAPE_InvalidSBIrq,
    SOUNDSCAPE_UnableToSetIrq,
    SOUNDSCAPE_DmaError,
    SOUNDSCAPE_DPMI_Error,
    SOUNDSCAPE_OutOfMemory
};

void SOUNDSCAPE_SetPlaybackRate(unsigned rate);
unsigned SOUNDSCAPE_GetPlaybackRate(void);
int SOUNDSCAPE_SetMixMode(int mode);
void SOUNDSCAPE_StopPlayback(void);
int SOUNDSCAPE_GetCurrentPos(void);
int SOUNDSCAPE_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, unsigned SampleRate, int MixMode, void (*CallBackFunc)(void));
int SOUNDSCAPE_GetCardInfo(int *MaxSampleBits, int *MaxChannels);
void SOUNDSCAPE_SetCallBack(void (*func)(void));
int SOUNDSCAPE_GetMIDIPort(void);
int SOUNDSCAPE_Init(void);
void SOUNDSCAPE_Shutdown(void);
int SOUNDSCAPE_FindCard(void);

#endif
