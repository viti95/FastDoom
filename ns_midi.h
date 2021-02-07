#ifndef __MIDI_H
#define __MIDI_H

enum MIDI_Errors
{
    MIDI_Warning = -2,
    MIDI_Error = -1,
    MIDI_Ok = 0,
    MIDI_NullMidiModule,
    MIDI_InvalidMidiFile,
    MIDI_UnknownMidiFormat,
    MIDI_NoTracks,
    MIDI_InvalidTrack,
    MIDI_NoMemory,
    MIDI_DPMI_Error
};

#define MIDI_PASS_THROUGH 1
#define MIDI_DONT_PLAY 0

#define MIDI_MaxVolume 255

extern char MIDI_PatchMap[128];

typedef struct
{
    void (*NoteOff)(int channel, int key, int velocity);
    void (*NoteOn)(int channel, int key, int velocity);
    void (*PolyAftertouch)(int channel, int key, int pressure);
    void (*ControlChange)(int channel, int number, int value);
    void (*ProgramChange)(int channel, int program);
    void (*ChannelAftertouch)(int channel, int pressure);
    void (*PitchBend)(int channel, int lsb, int msb);
    void (*SetVolume)(int volume);
    int (*GetVolume)(void);
} midifuncs;

int MIDI_AllNotesOff(void);
int MIDI_Reset(void);
int MIDI_SetVolume(int volume);
void MIDI_SetMidiFuncs(midifuncs *funcs);
void MIDI_ContinueSong(void);
void MIDI_PauseSong(void);
void MIDI_StopSong(void);
int MIDI_PlaySong(unsigned char *song, int loopflag);
void MIDI_SetTempo(int tempo);
void MIDI_LoadTimbres(void);

#endif
