#ifndef __GUSWAVE_H
#define __GUSWAVE_H

#define GUSWAVE_MinVoiceHandle 1

enum GUSWAVE_Errors
{
    GUSWAVE_Warning = -2,
    GUSWAVE_Error = -1,
    GUSWAVE_Ok = 0,
    GUSWAVE_GUSError,
    GUSWAVE_NotInstalled,
    GUSWAVE_NoVoices,
    GUSWAVE_UltraNoMem,
    GUSWAVE_UltraNoMemMIDI,
    GUSWAVE_VoiceNotFound,
    GUSWAVE_InvalidVOCFile,
    GUSWAVE_InvalidWAVFile
};

int GUSWAVE_VoicePlaying(int handle);
int GUSWAVE_VoicesPlaying(void);
int GUSWAVE_Kill(int handle);
int GUSWAVE_KillAllVoices(void);
int GUSWAVE_SetPitch(int handle, int pitchoffset);
int GUSWAVE_SetPan3D(int handle, int angle, int distance);
void GUSWAVE_SetVolume(int volume);
int GUSWAVE_GetVolume(void);
int GUSWAVE_VoiceAvailable(int priority);
int GUSWAVE_PlayVOC(char *sample, int pitchoffset, int angle, int volume,
                    int priority, unsigned long callbackval);
int GUSWAVE_PlayWAV(char *sample, int pitchoffset, int angle, int volume,
                    int priority, unsigned long callbackval);
int GUSWAVE_StartDemandFeedPlayback(void (*function)(char **ptr, unsigned long *length),
                                    int channels, int bits, int rate, int pitchoffset, int angle,
                                    int volume, int priority, unsigned long callbackval);
void GUSWAVE_SetCallBack(void (*function)(unsigned long));
void GUSWAVE_SetReverseStereo(int setting);
int GUSWAVE_GetReverseStereo(void);
int GUSWAVE_Init(int numvoices);
void GUSWAVE_Shutdown(void);
#pragma aux GUSWAVE_Shutdown frame;

#endif
