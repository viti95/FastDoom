#ifndef __CMS_H
#define __CMS_H

#define MONO_8BIT 0

#define CMS_SampleRate 11025

// max tone channels. Channels 10 and 11 are reserved for percussion
#define MAX_CMS_CHANNELS 10

enum CMS_Errors
{
    CMS_Warning = -2,
    CMS_Error = -1,
    CMS_Ok = 0,
    CMS_NoVoices,
    CMS_VoiceNotFound,
    CMS_DPMI_Error
};

enum CMS_Modes
{
    CMS_None,
    CMS_OnlyFX,
    CMS_OnlyMusic,
    CMS_MusicFX
};

typedef struct {
        unsigned char note;
        unsigned char priority;
        unsigned char ch;
        unsigned char voice;
	unsigned char velocity;
} mid_channel;

void CMS_SetMode(unsigned char mode);

// FX sound proto
void CMS_StopPlayback(void);
int CMS_BeginBufferedPlayback(char *BufferStart, int BufferSize, int NumDivisions, void (*CallBackFunc)(void));
int CMS_Init(int soundcard, int port);
void CMS_Shutdown(void);

// Music proto
int CMS_MIDI_Init(int port);
void CMS_MIDI_Shutdown(void);
void CMS_NoteOff(int channel, int key, int velocity);
void CMS_NoteOn(int channel, int key, int velocity);
void CMS_ControlChange(int channel, int number, int value);

void CMS_ProgramChange(int channel, int program);
void CMS_PitchBend(int channel, int lsb, int msb);
#endif
