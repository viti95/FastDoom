#ifndef __AL_MIDI_H
#define __AL_MIDI_H

enum AL_Errors
{
    AL_Warning = -2,
    AL_Error = -1,
    AL_Ok = 0,
};

#define AL_MaxVolume 127
#define AL_DefaultChannelVolume 90
//#define AL_DefaultPitchBendRange 2
#define AL_DefaultPitchBendRange 200

#define ADLIB_PORT 0x388

void AL_SendOutputToPort(int port, int reg, int data);
void AL_SendOutputToPort_OPL2LPT(int port, int reg, int data);
void AL_SendOutputToPort_OPL3LPT(int port, int reg, int data);
void AL_SendOutput(int voice, int reg, int data);
void AL_StereoOn(void);
void AL_StereoOff(void);
int AL_ReleaseVoice(int voice);
void AL_Shutdown(void);
int AL_Init(int soundcard);
void AL_Reset(void);
void AL_NoteOff(int channel, int key, int velocity);
void AL_NoteOn(int channel, int key, int vel);
//Turned off to test if it works with Watcom 10a
//   #pragma aux AL_NoteOn frame;
void AL_AllNotesOff(int channel);
void AL_ControlChange(int channel, int type, int data);
void AL_ProgramChange(int channel, int patch);
void AL_SetPitchBend(int channel, int lsb, int msb);
int AL_DetectFM(void);
void AL_RegisterTimbreBank(unsigned char *timbres);

#endif
