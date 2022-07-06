#ifndef __PC_SPEAKER_H
#define __PC_SPEAKER_H

#define MONO_8BIT 0

#define PCSpeaker_SampleRate 16000

enum PCSpeaker_Errors
{
    PCSpeaker_Warning = -2,
    PCSpeaker_Error = -1,
    PCSpeaker_Ok = 0,
    PCSpeaker_NoVoices,
    PCSpeaker_VoiceNotFound,
    PCSpeaker_DPMI_Error
};

void PCSpeaker_StopPlayback(void);
int PCSpeaker_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int PCSpeaker_Init(int soundcard);
void PCSpeaker_Shutdown(void);

#endif
