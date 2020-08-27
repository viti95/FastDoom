#ifndef __SNDSRC_H
#define __SNDSRC_H

enum SS_ERRORS
{
   SS_Warning = -2,
   SS_Error = -1,
   SS_Ok = 0,
   SS_NotFound,
   SS_NoSoundPlaying,
   SS_DPMI_Error
};

#define SELECT_SOUNDSOURCE_PORT1 "ss1"
#define SELECT_SOUNDSOURCE_PORT2 "ss2"
#define SELECT_SOUNDSOURCE_PORT3 "ss3"
#define SELECT_TANDY_SOUNDSOURCE "sst"

#define SS_Port1 0x3bc
#define SS_Port2 0x378
#define SS_Port3 0x278

#define SS_DefaultPort 0x378
#define SS_SampleRate 7000
#define SS_DMAChannel -1

void SS_StopPlayback(void);
int SS_GetCurrentPos(void);
int SS_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int SS_GetPlaybackRate(void);
int SS_SetMixMode(int mode);
int SS_SetPort(int port);
void SS_SetCallBack(void (*func)(void));
int SS_Init(int soundcard);
void SS_Shutdown(void);

#endif
