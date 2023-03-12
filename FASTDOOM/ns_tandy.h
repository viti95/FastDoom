#ifndef __TANDY_H
#define __TANDY_H

#define MONO_8BIT 0

enum TANDY_Errors
{
    TANDY_Warning = -2,
    TANDY_Error = -1,
    TANDY_Ok = 0,
    TANDY_NoVoices,
    TANDY_VoiceNotFound,
    TANDY_DPMI_Error
};

void TANDY_StopPlayback(void);
int TANDY_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int TANDY_Init(int soundcard);
void TANDY_Shutdown(void);

#endif
