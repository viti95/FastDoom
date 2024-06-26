#ifndef __LPTMIDI_H
#define __LPTMIDI_H

#define LPTMIDI_DefaultAddress 0x378

enum LPTMIDI_ERRORS
{
    LPTMIDI_Warning = -2,
    LPTMIDI_Error = -1,
    LPTMIDI_Ok = 0,
    LPTMIDI_DPMI_Error
};

#define LPTMIDI_NotFound -1
#define LPTMIDI_UARTFailed -2

#define LPTMIDI_ReadyToWrite 0x40
#define LPTMIDI_ReadyToRead 0x80
#define LPTMIDI_CmdEnterUART 0x3f
#define LPTMIDI_CmdReset 0xff
#define LPTMIDI_CmdAcknowledge 0xfe

extern int LPTMIDI_BaseAddr;
extern unsigned LPTMIDI_Delay;

void LPTMIDI_SendCommand(int data);
void LPTMIDI_SendMidi(int data);
int LPTMIDI_Reset(void);
int LPTMIDI_EnterUART(void);
int LPTMIDI_Init(int addr);
void LPTMIDI_NoteOff(int channel, int key, int velocity);
void LPTMIDI_NoteOn(int channel, int key, int velocity);
void LPTMIDI_PolyAftertouch(int channel, int key, int pressure);
void LPTMIDI_ControlChange(int channel, int number, int value);
void LPTMIDI_ProgramChange(int channel, int program);
void LPTMIDI_ChannelAftertouch(int channel, int pressure);
void LPTMIDI_PitchBend(int channel, int lsb, int msb);

#endif
