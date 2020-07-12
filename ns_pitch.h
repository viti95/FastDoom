#ifndef __PITCH_H
#define __PITCH_H

enum PITCH_ERRORS
{
    PITCH_Warning = -2,
    PITCH_Error = -1,
    PITCH_Ok = 0,
};

unsigned long PITCH_GetScale(int pitchoffset);
void PITCH_UnlockMemory(void);
int PITCH_LockMemory(void);
#endif
