#ifndef __LPT_H
#define __LPT_H

#define MONO_8BIT 0

#define LPT_SampleRate 11025

enum LPT_Errors
{
    LPT_Warning = -2,
    LPT_Error = -1,
    LPT_Ok = 0,
    LPT_NoVoices,
    LPT_VoiceNotFound,
    LPT_DPMI_Error
};

void LPT_StopPlayback(void);
int LPT_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int LPT_SetMixMode(int mode);
int LPT_Init(int soundcard);
void LPT_Shutdown(void);

#endif
