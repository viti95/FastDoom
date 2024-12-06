#ifndef __SBMIDI_H
#define __SBMIDI_H

#define SBMIDI_DefaultAddress 0x220

enum SBMIDI_ERRORS
{
    SBMIDI_Warning = -2,
    SBMIDI_Error = -1,
    SBMIDI_Ok = 0,
    SBMIDI_DPMI_Error
};

#define SBMIDI_NotFound -1
#define SBMIDI_UARTFailed -2

#define SBMIDI_ReadyToWrite 0x40
#define SBMIDI_ReadyToRead 0x80
#define SBMIDI_CmdEnterUART 0x3f
#define SBMIDI_CmdReset 0xff
#define SBMIDI_CmdAcknowledge 0xfe

extern int SBMIDI_BaseAddr;
extern unsigned SBMIDI_Delay;

void SBMIDI_SendCommand(int data);
void SBMIDI_SendMidi(int data);
int SBMIDI_Reset(void);
int SBMIDI_EnterUART(void);
int SBMIDI_Init();
void SBMIDI_NoteOff(int channel, int key, int velocity);
void SBMIDI_NoteOn(int channel, int key, int velocity);
void SBMIDI_PolyAftertouch(int channel, int key, int pressure);
void SBMIDI_ControlChange(int channel, int number, int value);
void SBMIDI_ProgramChange(int channel, int program);
void SBMIDI_ChannelAftertouch(int channel, int pressure);
void SBMIDI_PitchBend(int channel, int lsb, int msb);
void SBMIDI_SysEx(unsigned char *ptr, int length);

#endif
