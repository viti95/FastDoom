#ifndef __MULTIVOC_H
#define __MULTIVOC_H

#define MV_MinVoiceHandle 1

extern int MV_ErrorCode;

enum MV_Errors
{
    MV_Warning = -2,
    MV_Error = -1,
    MV_Ok = 0,
    MV_UnsupportedCard,
    MV_NotInstalled,
    MV_NoVoices,
    MV_NoMem,
    MV_VoiceNotFound,
    MV_BlasterError,
    MV_PasError,
    MV_SoundScapeError,
    MV_SoundSourceError,
    MV_DPMI_Error,
    MV_InvalidVOCFile,
    MV_InvalidWAVFile,
    MV_InvalidMixMode,
    MV_SoundSourceFailure,
    MV_IrqFailure,
    MV_DMAFailure,
    MV_DMA16Failure,
    MV_NullRecordFunction
};

int MV_VoicePlaying(int handle);
int MV_KillAllVoices(void);
int MV_Kill(int handle);
int MV_VoicesPlaying(void);
int MV_VoiceAvailable(int priority);
int MV_SetPitch(int handle, int pitchoffset);
int MV_SetFrequency(int handle, int frequency);
int MV_EndLooping(int handle);
int MV_SetPan(int handle, int vol, int left, int right);
int MV_GetMaxReverbDelay(void);
int MV_SetMixMode(int numchannels, int samplebits);
int MV_StartPlayback(void);
void MV_StopPlayback(void);
int MV_StartDemandFeedPlayback(void (*function)(char **ptr, unsigned long *length),
                               int rate, int pitchoffset, int vol, int left, int right,
                               int priority, unsigned long callbackval);
int MV_PlayRaw(char *ptr, unsigned long length,
               unsigned rate, int pitchoffset, int vol, int left,
               int right, int priority, unsigned long callbackval);
int MV_PlayLoopedRaw(char *ptr, unsigned long length,
                     char *loopstart, char *loopend, unsigned rate, int pitchoffset,
                     int vol, int left, int right, int priority,
                     unsigned long callbackval);
int MV_PlayWAV(char *ptr, int pitchoffset, int vol, int left,
               int right, int priority, unsigned long callbackval);
int MV_PlayLoopedWAV(char *ptr, long loopstart, long loopend,
                     int pitchoffset, int vol, int left, int right, int priority,
                     unsigned long callbackval);
int MV_PlayVOC(char *ptr, int pitchoffset, int vol, int left, int right,
               int priority, unsigned long callbackval);
int MV_PlayLoopedVOC(char *ptr, long loopstart, long loopend,
                     int pitchoffset, int vol, int left, int right, int priority,
                     unsigned long callbackval);
void MV_CreateVolumeTable(int index, int volume, int MaxVolume);
void MV_SetVolume(int volume);
int MV_GetVolume(void);
void MV_SetCallBack(void (*function)(unsigned long));
void MV_SetReverseStereo(int setting);
int MV_GetReverseStereo(void);
int MV_Init(int soundcard, int MixRate, int Voices, int numchannels,
            int samplebits);
int MV_Shutdown(void);
void MV_UnlockMemory(void);
int MV_LockMemory(void);

#endif
