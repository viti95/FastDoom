#ifndef __PCFX_H
#define __PCFX_H

enum PCFX_Errors
{
    PCFX_Warning = -2,
    PCFX_Error = -1,
    PCFX_Ok = 0,
    PCFX_NoVoices,
    PCFX_VoiceNotFound,
    PCFX_DPMI_Error
};

#define PCFX_MaxVolume 255
#define PCFX_MinVoiceHandle 1

typedef struct
{
    unsigned long length;
    short int priority;
    char data[];
} PCSound;

int PCFX_Stop(int handle);
void PCFX_UseLookup(int use, unsigned value);
int PCFX_Play(PCSound *sound, int priority, unsigned long callbackval);
int PCFX_SoundPlaying(int handle);
int PCFX_SetTotalVolume(int volume);
int PCFX_Init(void);
int PCFX_Shutdown(void);
#pragma aux PCFX_Shutdown frame;
void PCFX_UnlockMemory(void);
int PCFX_LockMemory(void);

#endif
