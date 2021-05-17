#ifndef __AWE32_H
#define __AWE32_H

enum AWE32_ERRORS
{
    AWE32_Warning = -2,
    AWE32_Error = -1,
    AWE32_Ok = 0,
    AWE32_SoundBlasterError,
    AWE32_NotDetected,
    AWE32_UnableToInitialize,
    AWE32_MPU401Error,
    AWE32_DPMI_Error
};

int AWE32_Init(void);
void AWE32_Shutdown(void);
void AWE32_NoteOff(int channel, int key, int velocity);
void AWE32_NoteOn(int channel, int key, int velocity);
void AWE32_PolyAftertouch(int channel, int key, int pressure);
void AWE32_ChannelAftertouch(int channel, int pressure);
void AWE32_ControlChange(int channel, int number, int value);
void AWE32_ProgramChange(int channel, int program);
void AWE32_PitchBend(int channel, int lsb, int msb);

#endif
