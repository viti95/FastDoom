#ifndef __PC_SPEAKER_PWM_H
#define __PC_SPEAKER_PWM_H

#define MONO_8BIT 0

#define PCSpeaker_PWM_SampleRate 16000

enum PCSpeaker_PWM_Errors
{
    PCSpeaker_PWM_Warning = -2,
    PCSpeaker_PWM_Error = -1,
    PCSpeaker_PWM_Ok = 0,
    PCSpeaker_PWM_NoVoices,
    PCSpeaker_PWM_VoiceNotFound,
    PCSpeaker_PWM_DPMI_Error
};

void PCSpeaker_PWM_StopPlayback(void);
int PCSpeaker_PWM_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int PCSpeaker_PWM_Init(int soundcard);
void PCSpeaker_PWM_Shutdown(void);

#endif
