#ifndef __ADBFX_H
#define __ADBFX_H

#define MONO_8BIT 0

#define ADBFX_SampleRate 11025

enum ADBFX_Errors
{
    ADBFX_Warning = -2,
    ADBFX_Error = -1,
    ADBFX_Ok = 0,
    ADBFX_NoVoices,
    ADBFX_VoiceNotFound,
    ADBFX_DPMI_Error
};

void ADBFX_StopPlayback(void);
int ADBFX_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int ADBFX_SetMixMode(int mode);
int ADBFX_Init(int soundcard);
void ADBFX_Shutdown(void);

#endif
