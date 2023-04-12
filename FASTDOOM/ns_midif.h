#ifndef ___MIDI_H
#define ___MIDI_H

#define RELATIVE_BEAT(measure, beat, tick) \
    ((tick) + ((beat) << 9) + ((measure) << 16))

//Bobby Prince thinks this may be 100
//#define GENMIDI_DefaultVolume 100
#define GENMIDI_DefaultVolume 90

#define MAX_FORMAT 1

#define NUM_MIDI_CHANNELS 16

#define TIME_PRECISION 16

#define MIDI_HEADER_SIGNATURE 0x6468544d // "MThd"
#define MIDI_TRACK_SIGNATURE 0x6b72544d  // "MTrk"

#define MIDI_VOLUME 7
#define MIDI_PAN 10
#define MIDI_DETUNE 94
#define MIDI_RHYTHM_CHANNEL 9
#define MIDI_RPN_MSB 100
#define MIDI_RPN_LSB 101
#define MIDI_DATAENTRY_MSB 6
#define MIDI_DATAENTRY_LSB 38
#define MIDI_PITCHBEND_MSB 0
#define MIDI_PITCHBEND_LSB 0
#define MIDI_RUNNING_STATUS 0x80
#define MIDI_NOTE_OFF 0x8
#define MIDI_NOTE_ON 0x9
#define MIDI_POLY_AFTER_TCH 0xA
#define MIDI_CONTROL_CHANGE 0xB
#define MIDI_PROGRAM_CHANGE 0xC
#define MIDI_AFTER_TOUCH 0xD
#define MIDI_PITCH_BEND 0xE
#define MIDI_SPECIAL 0xF
#define MIDI_SYSEX 0xF0
#define MIDI_SYSEX_CONTINUE 0xF7
#define MIDI_META_EVENT 0xFF
#define MIDI_END_OF_TRACK 0x2F
#define MIDI_TEMPO_CHANGE 0x51
#define MIDI_TIME_SIGNATURE 0x58
#define MIDI_RESET_ALL_CONTROLLERS 0x79
#define MIDI_ALL_NOTES_OFF 0x7b
#define MIDI_MONO_MODE_ON 0x7E
#define MIDI_SYSTEM_RESET 0xFF

#define GET_NEXT_EVENT(track, data) \
    (data) = *(track)->pos;         \
    (track)->pos += 1

#define GET_MIDI_CHANNEL(event) ((event)&0xf)
#define GET_MIDI_COMMAND(event) ((event) >> 4)

#define EMIDI_INFINITE -1
#define EMIDI_END_LOOP_VALUE 127
#define EMIDI_ALL_CARDS 127
#define EMIDI_INCLUDE_TRACK 110
#define EMIDI_EXCLUDE_TRACK 111
#define EMIDI_PROGRAM_CHANGE 112
#define EMIDI_VOLUME_CHANGE 113
#define EMIDI_CONTEXT_START 114
#define EMIDI_CONTEXT_END 115
#define EMIDI_LOOP_START 116
#define EMIDI_LOOP_END 117
#define EMIDI_SONG_LOOP_START 118
#define EMIDI_SONG_LOOP_END 119

#define EMIDI_GeneralMIDI 0
#define EMIDI_SoundCanvas 1
#define EMIDI_AWE32 2
#define EMIDI_WaveBlaster 3
#define EMIDI_SoundBlaster 4
#define EMIDI_ProAudio 5
#define EMIDI_SoundMan16 6
#define EMIDI_Adlib 7
#define EMIDI_Soundscape 8
#define EMIDI_Ultrasound 9

#define EMIDI_AffectsCurrentCard(c, type) \
    (((c) == EMIDI_ALL_CARDS) || ((c) == (type)))

#define EMIDI_NUM_CONTEXTS 7
typedef struct
{
    unsigned char *pos;
    unsigned char *loopstart;
    short loopcount;
    short RunningStatus;
    unsigned time;
    long FPSecondsPerTick;
    short tick;
    short beat;
    short measure;
    short BeatsPerMeasure;
    short TicksPerBeat;
    short TimeBase;
    long delay;
    short active;
} songcontext;

typedef struct
{
    unsigned char *start;
    unsigned char *pos;

    long delay;
    short active;
    short RunningStatus;

    short currentcontext;
    songcontext context[EMIDI_NUM_CONTEXTS];

    char EMIDI_IncludeTrack;
} track;

static long _MIDI_ReadNumber(void *from, size_t size);
static long _MIDI_ReadDelta(track *ptr);
static void _MIDI_ResetTracks(void);
static void _MIDI_AdvanceTick(void);
static void _MIDI_MetaEvent(track *Track);
static void _MIDI_SysEx(track *Track);
static int _MIDI_InterpretControllerInfo(track *Track, int TimeSet,
                                         int channel, int c1, int c2);
//static
static void _MIDI_ServiceRoutine(task *Task);
static int _MIDI_SendControlChange(int channel, int c1, int c2);
static void _MIDI_SetChannelVolume(int channel, int volume);
static void _MIDI_SendChannelVolumes(void);
static void _MIDI_InitEMIDI(void);

#endif
