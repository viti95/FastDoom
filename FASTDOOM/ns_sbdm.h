#ifndef __SBDM_H
#define __SBDM_H

#define MONO_8BIT 0

#define SBDM_SampleRate 11025

enum SBDM_Errors
{
    SBDM_Warning = -2,
    SBDM_Error = -1,
    SBDM_Ok = 0,
    SBDM_NoVoices,
    SBDM_VoiceNotFound,
    SBDM_DPMI_Error
};

void SBDM_StopPlayback(void);
int SBDM_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int SBDM_SetMixMode(int mode);
int SBDM_Init(int soundcard);
void SBDM_Shutdown(void);

#endif
