#ifndef __MULTIVOC_H
#define __MULTIVOC_H

#define MV_MinVoiceHandle 1

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
int MV_SetMixMode(int numchannels, int samplebits);
int MV_StartPlayback(void);
void MV_StopPlayback(void);
int MV_PlayRaw(unsigned char *ptr, unsigned long length,
               unsigned rate, int vol, int left,
               int right, int priority, unsigned long callbackval);
int MV_PlayLoopedRaw(unsigned char *ptr, unsigned long length,
                     char *loopstart, char *loopend, unsigned rate,
                     int vol, int left, int right, int priority,
                     unsigned long callbackval);
void MV_CreateVolumeTable(int index, int volume, int MaxVolume);
void MV_SetVolume(int volume);
void MV_SetCallBack(void (*function)(unsigned long));
void MV_SetReverseStereo(int setting);
int MV_Init(int soundcard, int MixRate, int Voices, int numchannels,
            int samplebits);
int MV_Shutdown(void);

#endif
