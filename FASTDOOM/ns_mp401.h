#ifndef __MPU401_H
#define __MPU401_H

#define MPU_DefaultAddress 0x330

enum MPU_ERRORS
{
    MPU_Warning = -2,
    MPU_Error = -1,
    MPU_Ok = 0,
    MPU_DPMI_Error
};

#define MPU_NotFound -1
#define MPU_UARTFailed -2

#define MPU_ReadyToWrite 0x40
#define MPU_ReadyToRead 0x80
#define MPU_CmdEnterUART 0x3f
#define MPU_CmdReset 0xff
#define MPU_CmdAcknowledge 0xfe

extern int MPU_BaseAddr;
extern unsigned MPU_Delay;

void MPU_SendCommand(int data);
void MPU_SendMidi(int data);
int MPU_Reset(void);
int MPU_EnterUART(void);
int MPU_Init(int addr);
void MPU_NoteOff(int channel, int key, int velocity);
void MPU_NoteOn(int channel, int key, int velocity);
void MPU_PolyAftertouch(int channel, int key, int pressure);
void MPU_ControlChange(int channel, int number, int value);
void MPU_ProgramChange(int channel, int program);
void MPU_ChannelAftertouch(int channel, int pressure);
void MPU_PitchBend(int channel, int lsb, int msb);

#endif
