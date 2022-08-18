#ifndef __CMS_H
#define __CMS_H

#define MONO_8BIT 0

#define CMS_SampleRate 11025

enum CMS_Errors
{
    CMS_Warning = -2,
    CMS_Error = -1,
    CMS_Ok = 0,
    CMS_NoVoices,
    CMS_VoiceNotFound,
    CMS_DPMI_Error
};

void CMS_StopPlayback(void);
int CMS_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int CMS_Init(int soundcard, int port);
void CMS_Shutdown(void);

#endif
