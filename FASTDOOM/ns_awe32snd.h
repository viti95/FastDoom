#ifndef __AWE32SND_H
#define __AWE32SND_H

enum AWE32SND_ERRORS
{
    AWE32SND_Warning = -2,
    AWE32SND_Error = -1,
    AWE32SND_Ok = 0,
    AWE32SND_SoundBlasterError,
    AWE32SND_NotDetected,
    AWE32SND_UnableToInitialize,
    AWE32SND_MPU401Error,
    AWE32SND_DPMI_Error
};

int AWE32SND_Init(void);
void AWE32SND_Shutdown(void);

#endif
