#ifndef __PC_SPEAKER_PWM_H
#define __PC_SPEAKER_PWM_H

#define MONO_8BIT 0

#define PCSpeakerPWM_SampleRate 16000

enum PCSpeakerPWM_Errors
{
    PCSpeakerPWM_Warning = -2,
    PCSpeakerPWM_Error = -1,
    PCSpeakerPWM_Ok = 0,
    PCSpeakerPWM_NoVoices,
    PCSpeakerPWM_VoiceNotFound,
    PCSpeakerPWM_DPMI_Error
};

void PCSpeakerPWM_StopPlayback(void);
int PCSpeakerPWM_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int PCSpeakerPWM_SetMixMode(int mode);
void PCSpeakerPWM_SetCallBack(void (*func)(void));
int PCSpeakerPWM_Init(int soundcard);
void PCSpeakerPWM_Shutdown(void);

#endif
