#ifndef __RS232_H
#define __RS232_H

#define RS232_DefaultAddress 0x3F8

enum RS232_ERRORS
{
    RS232_Warning = -2,
    RS232_Error = -1,
    RS232_Ok = 0,
    RS232_DPMI_Error
};

#define RS232_NotFound -1
#define RS232_UARTFailed -2

#define RS232_ReadyToWrite 0x40
#define RS232_ReadyToRead 0x80
#define RS232_CmdEnterUART 0x3f
#define RS232_CmdReset 0xff
#define RS232_CmdAcknowledge 0xfe

extern int RS232_BaseAddr;
extern unsigned RS232_Delay;

void RS232_SendCommand(int data);
void RS232_SendMidi(int data);
int RS232_Reset(void);
int RS232_EnterUART(void);
int RS232_Init(int addr);
void RS232_NoteOff(int channel, int key, int velocity);
void RS232_NoteOn(int channel, int key, int velocity);
void RS232_PolyAftertouch(int channel, int key, int pressure);
void RS232_ControlChange(int channel, int number, int value);
void RS232_ProgramChange(int channel, int program);
void RS232_ChannelAftertouch(int channel, int pressure);
void RS232_PitchBend(int channel, int lsb, int msb);
void RS232_SysEx(unsigned char *ptr, int length);

#endif
