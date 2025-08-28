#include <stdlib.h>
#include <time.h>
#include <dos.h>
#include <string.h>
#include "ns_cards.h"
#include "ns_inter.h"
#include "ns_dpmi.h"
#include "ns_std.h"
#include "ns_task.h"
#include "ns_llm.h"
#include "ns_music.h"
#include "ns_midi.h"
#include "ns_midif.h"
#include "options.h"
#include "fastmath.h"
#include "z_zone.h"

extern int MUSIC_SoundDevice;

static const char _MIDI_CommandLengths[NUM_MIDI_CHANNELS] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 0};

static track *_MIDI_TrackPtr = NULL;
static int _MIDI_TrackMemSize;
static int _MIDI_NumTracks;

static int _MIDI_SongActive = FALSE;
static int _MIDI_SongLoaded = FALSE;
static int _MIDI_Loop = FALSE;

static task *_MIDI_PlayRoutine = NULL;

static int _MIDI_Division;
static int _MIDI_Tick = 0;
static int _MIDI_Beat = 1;
static int _MIDI_Measure = 1;
static unsigned _MIDI_Time;
static int _MIDI_BeatsPerMeasure;
static int _MIDI_TicksPerBeat;
static int _MIDI_TimeBase;
static long _MIDI_FPSecondsPerTick;
static unsigned _MIDI_TotalTime;
static int _MIDI_TotalTicks;
static int _MIDI_TotalBeats;
static int _MIDI_TotalMeasures;

static unsigned long _MIDI_PositionInTicks;

static int _MIDI_Context;

static int _MIDI_ActiveTracks;
static int _MIDI_TotalVolume = MIDI_MaxVolume;

static int _MIDI_ChannelVolume[NUM_MIDI_CHANNELS];

static midifuncs *_MIDI_Funcs = NULL;

static int Reset = FALSE;

int MIDI_Tempo = 120;

char MIDI_PatchMap[128];

/*---------------------------------------------------------------------
   Function: _MIDI_ReadNumber

   Reads a variable length number from a MIDI track.
---------------------------------------------------------------------*/

static long _MIDI_ReadNumber(
    void *from,
    size_t size)

{
    unsigned char *FromPtr;
    long value;

    if (size > 4)
    {
        size = 4;
    }

    FromPtr = (unsigned char *)from;

    value = 0;
    while (size--)
    {
        value <<= 8;
        value += *FromPtr++;
    }

    return (value);
}

/*---------------------------------------------------------------------
   Function: _MIDI_ReadDelta

   Reads a variable length encoded delta delay time from the MIDI data.
---------------------------------------------------------------------*/

static long _MIDI_ReadDelta(
    track *ptr)

{
    long value;
    unsigned char c;

    GET_NEXT_EVENT(ptr, value);

    if (value & 0x80)
    {
        value &= 0x7f;
        do
        {
            GET_NEXT_EVENT(ptr, c);
            value = (value << 7) + (c & 0x7f);
        } while (c & 0x80);
    }

    return (value);
}

/*---------------------------------------------------------------------
   Function: _MIDI_ResetTracks

   Sets the track pointers to the beginning of the song.
---------------------------------------------------------------------*/

static void _MIDI_ResetTracks(
    void)

{
    int i;
    track *ptr;

    _MIDI_Tick = 0;
    _MIDI_Beat = 1;
    _MIDI_Measure = 1;
    _MIDI_Time = 0;
    _MIDI_BeatsPerMeasure = 4;
    _MIDI_TicksPerBeat = _MIDI_Division;
    _MIDI_TimeBase = 4;

    _MIDI_PositionInTicks = 0;
    _MIDI_ActiveTracks = 0;
    _MIDI_Context = 0;

    ptr = _MIDI_TrackPtr;
    for (i = 0; i < _MIDI_NumTracks; i++)
    {
        ptr->pos = ptr->start;
        ptr->delay = _MIDI_ReadDelta(ptr);
        ptr->active = TRUE;
        ptr->RunningStatus = 0;

        _MIDI_ActiveTracks += ptr->active != 0;

        ptr++;
    }
}

/*---------------------------------------------------------------------
   Function: _MIDI_AdvanceTick

   Increment tick counters.
---------------------------------------------------------------------*/

static void _MIDI_AdvanceTick(void)
{
    int tickDivision;
    int beatDivision;

    _MIDI_PositionInTicks++;
    _MIDI_Time += _MIDI_FPSecondsPerTick;

    _MIDI_Tick++;

    if (_MIDI_TicksPerBeat != 70)
    {
        tickDivision = (_MIDI_Tick / _MIDI_TicksPerBeat) + 1;
        _MIDI_Tick -= tickDivision * _MIDI_TicksPerBeat;
    }
    else
    {
        tickDivision = Div70(_MIDI_Tick) + 1;
        _MIDI_Tick -= Mul70(tickDivision);
    }

    _MIDI_Beat += tickDivision;

    if (_MIDI_BeatsPerMeasure != 4)
    {
        while (_MIDI_Beat > _MIDI_BeatsPerMeasure)
        {
            _MIDI_Beat -= _MIDI_BeatsPerMeasure;
            _MIDI_Measure++;
        }
    }
    else
    {
        tickDivision = (_MIDI_Tick >> 2) + 1;
        _MIDI_Tick -= tickDivision << 2;
        _MIDI_Beat += tickDivision;
    }
}

/*---------------------------------------------------------------------
   Function: _MIDI_SysEx

   Interpret SysEx Event.
---------------------------------------------------------------------*/

static void _MIDI_SysEx(track *Track)
{
    int length;

    length = _MIDI_ReadDelta(Track);
    if (_MIDI_Funcs->SysEx) {
        _MIDI_Funcs->SysEx(Track->pos, length);
    }
    Track->pos += length;
}

void MIDI_SysEx_Ext(unsigned char *data, int length)
{
    if (_MIDI_Funcs->SysEx) {
        _MIDI_Funcs->SysEx(data, length);
    }
}

/*---------------------------------------------------------------------
   Function: _MIDI_MetaEvent

   Interpret Meta Event.
---------------------------------------------------------------------*/

static void _MIDI_MetaEvent(
    track *Track)

{
    int command;
    int length;
    int denominator;
    long tempo;

    GET_NEXT_EVENT(Track, command);
    GET_NEXT_EVENT(Track, length);

    switch (command)
    {
    case MIDI_END_OF_TRACK:
        Track->active = FALSE;

        _MIDI_ActiveTracks--;
        break;

    case MIDI_TEMPO_CHANGE:
        // VITI95: OPTIMIZE
        tempo = 60000000L / _MIDI_ReadNumber(Track->pos, 3);
        MIDI_SetTempo(tempo);
        break;

    case MIDI_TIME_SIGNATURE:
        if ((_MIDI_Tick > 0) || (_MIDI_Beat > 1))
        {
            _MIDI_Measure++;
        }

        _MIDI_Tick = 0;
        _MIDI_Beat = 1;

        _MIDI_BeatsPerMeasure = (int)*Track->pos;
        denominator = (int)*(Track->pos + 1);
        _MIDI_TimeBase = 1;
        while (denominator > 0)
        {
            _MIDI_TimeBase += _MIDI_TimeBase;
            denominator--;
        }
        // VITI95: OPTIMIZE
        _MIDI_TicksPerBeat = (_MIDI_Division * 4) / _MIDI_TimeBase;
        break;
    }

    Track->pos += length;
}

/*---------------------------------------------------------------------
   Function: _MIDI_InterpretControllerInfo

   Interprets the MIDI controller info.
---------------------------------------------------------------------*/

static int _MIDI_InterpretControllerInfo(
    track *Track,
    int TimeSet,
    int channel,
    int c1,
    int c2)

{
    track *trackptr;
    int tracknum;
    int loopcount;

    switch (c1)
    {
    case MIDI_MONO_MODE_ON:
        Track->pos++;
        break;

    case MIDI_VOLUME:
        _MIDI_SetChannelVolume(channel, c2);
        break;

    default:
        _MIDI_Funcs->ControlChange(channel, c1, c2);
    }

    return TimeSet;
}

/*---------------------------------------------------------------------
   Function: _MIDI_ServiceRoutine

   Task that interperates the MIDI data.
---------------------------------------------------------------------*/
// NOTE: We have to use a stack frame here because of a strange bug
// that occurs with Watcom.  This means that we cannot access Task!
//Turned off to test if it works with Watcom 10a
//#pragma aux _MIDI_ServiceRoutine frame;
/*
static void test
   (
   task *Task
   )
   {
   _MIDI_ServiceRoutine( Task );
   _MIDI_ServiceRoutine( Task );
   _MIDI_ServiceRoutine( Task );
   _MIDI_ServiceRoutine( Task );
   }
*/
static void _MIDI_ServiceRoutine(task *Task)
{
    int event;
    int channel;
    int command;
    track *Track;
    int tracknum;
    int status;
    int c1;
    int c2;
    int TimeSet;

    if (!_MIDI_SongActive)
    {
        return;
    }

    TimeSet = FALSE;
    Track = _MIDI_TrackPtr;
    tracknum = 0;
    while (tracknum < _MIDI_NumTracks)
    {
        while ((Track->active) && (Track->delay == 0))
        {
            GET_NEXT_EVENT(Track, event);

            if (GET_MIDI_COMMAND(event) == MIDI_SPECIAL)
            {
                switch (event)
                {
                case MIDI_SYSEX:
                    _MIDI_SysEx(Track);
                    break;
                case MIDI_SYSEX_CONTINUE:
                    Track->pos += _MIDI_ReadDelta(Track);
                    break;
                case MIDI_META_EVENT:
                    _MIDI_MetaEvent(Track);
                    break;
                }

                if (Track->active)
                {
                    Track->delay = _MIDI_ReadDelta(Track);
                }

                continue;
            }

            if (event & MIDI_RUNNING_STATUS)
            {
                Track->RunningStatus = event;
            }
            else
            {
                event = Track->RunningStatus;
                Track->pos--;
            }

            channel = GET_MIDI_CHANNEL(event);
            command = GET_MIDI_COMMAND(event);

            if (_MIDI_CommandLengths[command] > 0)
            {
                GET_NEXT_EVENT(Track, c1);
                if (_MIDI_CommandLengths[command] > 1)
                {
                    GET_NEXT_EVENT(Track, c2);
                }
            }

            switch (command)
            {
            case MIDI_NOTE_OFF:
                _MIDI_Funcs->NoteOff(channel, c1, c2);
                break;

            case MIDI_NOTE_ON:
                _MIDI_Funcs->NoteOn(channel, c1, c2);
                break;

            case MIDI_POLY_AFTER_TCH:
                if (_MIDI_Funcs->PolyAftertouch)
                {
                    _MIDI_Funcs->PolyAftertouch(channel, c1, c2);
                }
                break;

            case MIDI_CONTROL_CHANGE:
                TimeSet = _MIDI_InterpretControllerInfo(Track, TimeSet,
                                                        channel, c1, c2);
                break;

            case MIDI_PROGRAM_CHANGE:
                    _MIDI_Funcs->ProgramChange(channel, MIDI_PatchMap[c1 & 0x7f]);
                break;

            case MIDI_AFTER_TOUCH:
                if (_MIDI_Funcs->ChannelAftertouch)
                {
                    _MIDI_Funcs->ChannelAftertouch(channel, c1);
                }
                break;

            case MIDI_PITCH_BEND:
                _MIDI_Funcs->PitchBend(channel, c1, c2);
                break;
            }

            Track->delay = _MIDI_ReadDelta(Track);
        }

        Track->delay--;
        Track++;
        tracknum++;

        if (_MIDI_ActiveTracks == 0)
        {
            _MIDI_ResetTracks();
            if (_MIDI_Loop)
            {
                tracknum = 0;
                Track = _MIDI_TrackPtr;
            }
            else
            {
                _MIDI_SongActive = FALSE;
                break;
            }
        }
    }

    _MIDI_AdvanceTick();
}

/*---------------------------------------------------------------------
   Function: _MIDI_SendControlChange

   Sends a control change to the proper device
---------------------------------------------------------------------*/

static int _MIDI_SendControlChange(
    int channel,
    int c1,
    int c2)

{
    int status;

    _MIDI_Funcs->ControlChange(channel, c1, c2);

    return (MIDI_Ok);
}

/*---------------------------------------------------------------------
   Function: MIDI_AllNotesOff

   Sends all notes off commands on all midi channels.
---------------------------------------------------------------------*/

int MIDI_AllNotesOff(
    void)

{
    int channel;

    for (channel = 0; channel < NUM_MIDI_CHANNELS; channel++)
    {
        _MIDI_SendControlChange(channel, 0x40, 0);
        _MIDI_SendControlChange(channel, MIDI_ALL_NOTES_OFF, 0);
        _MIDI_SendControlChange(channel, 0x78, 0);
    }

    return (MIDI_Ok);
}

/*---------------------------------------------------------------------
   Function: _MIDI_SetChannelVolume

   Sets the volume of the specified midi channel.
---------------------------------------------------------------------*/

static void _MIDI_SetChannelVolume(
    int channel,
    int volume)

{
    int status;
    int remotevolume;

    _MIDI_ChannelVolume[channel] = volume;

    if (_MIDI_Funcs->SetVolume == NULL)
    {
        volume *= _MIDI_TotalVolume;
        volume = Div255(volume);
    }

    _MIDI_Funcs->ControlChange(channel, MIDI_VOLUME, volume);
}

/*---------------------------------------------------------------------
   Function: _MIDI_SendChannelVolumes

   Sets the volume on all the midi channels.
---------------------------------------------------------------------*/

static void _MIDI_SendChannelVolumes(
    void)

{
    int channel;

    for (channel = 0; channel < NUM_MIDI_CHANNELS; channel++)
    {
        _MIDI_SetChannelVolume(channel, _MIDI_ChannelVolume[channel]);
    }
}

/*---------------------------------------------------------------------
   Function: MIDI_Reset

   Resets the MIDI device to General Midi defaults.
---------------------------------------------------------------------*/

int MIDI_Reset(
    void)

{
    int channel;
    long time;
    unsigned flags;

    MIDI_AllNotesOff();

    flags = DisableInterrupts();
    _enable();
    time = clock() + CLOCKS_PER_SEC / 24;
    while (clock() < time)
        ;

    RestoreInterrupts(flags);

    for (channel = 0; channel < NUM_MIDI_CHANNELS; channel++)
    {
        _MIDI_SendControlChange(channel, MIDI_RESET_ALL_CONTROLLERS, 0);
        _MIDI_SendControlChange(channel, MIDI_RPN_MSB, MIDI_PITCHBEND_MSB);
        _MIDI_SendControlChange(channel, MIDI_RPN_LSB, MIDI_PITCHBEND_LSB);
        _MIDI_SendControlChange(channel, MIDI_DATAENTRY_MSB, 2); /* Pitch Bend Sensitivity MSB */
        _MIDI_SendControlChange(channel, MIDI_DATAENTRY_LSB, 0); /* Pitch Bend Sensitivity LSB */
        _MIDI_ChannelVolume[channel] = GENMIDI_DefaultVolume;
    }

    _MIDI_SendChannelVolumes();

    Reset = TRUE;

    return (MIDI_Ok);
}

/*---------------------------------------------------------------------
   Function: MIDI_SetVolume

   Sets the total volume of the music.
---------------------------------------------------------------------*/

int MIDI_SetVolume(
    int volume)

{
    int i;

    if (_MIDI_Funcs == NULL)
    {
        return (MIDI_NullMidiModule);
    }

    volume = min(MIDI_MaxVolume, volume);
    volume = max(0, volume);

    _MIDI_TotalVolume = volume;

    if (_MIDI_Funcs->SetVolume)
    {
        _MIDI_Funcs->SetVolume(volume);
    }
    else
    {
        _MIDI_SendChannelVolumes();
    }

    return (MIDI_Ok);
}

/*---------------------------------------------------------------------
   Function: MIDI_ContinueSong

   Continues playback of a paused song.
---------------------------------------------------------------------*/

void MIDI_ContinueSong(
    void)

{
    if (_MIDI_SongLoaded)
    {
        _MIDI_SongActive = TRUE;
    }
}

/*---------------------------------------------------------------------
   Function: MIDI_PauseSong

   Pauses playback of the current song.
---------------------------------------------------------------------*/

void MIDI_PauseSong(
    void)

{
    if (_MIDI_SongLoaded)
    {
        _MIDI_SongActive = FALSE;
        MIDI_AllNotesOff();
    }
}

/*---------------------------------------------------------------------
   Function: MIDI_SetMidiFuncs

   Selects the routines that send the MIDI data to the music device.
---------------------------------------------------------------------*/

void MIDI_SetMidiFuncs(
    midifuncs *funcs)

{
    _MIDI_Funcs = funcs;
}

/*---------------------------------------------------------------------
   Function: MIDI_StopSong

   Stops playback of the currently playing song.
---------------------------------------------------------------------*/

void MIDI_StopSong(
    void)

{
    if (_MIDI_SongLoaded)
    {
        TS_Terminate(_MIDI_PlayRoutine);

        _MIDI_PlayRoutine = NULL;
        _MIDI_SongActive = FALSE;
        _MIDI_SongLoaded = FALSE;

        MIDI_Reset();
        _MIDI_ResetTracks();

        Z_Free(_MIDI_TrackPtr);

        _MIDI_TrackPtr = NULL;
        _MIDI_NumTracks = 0;
        _MIDI_TrackMemSize = 0;

        _MIDI_TotalTime = 0;
        _MIDI_TotalTicks = 0;
        _MIDI_TotalBeats = 0;
        _MIDI_TotalMeasures = 0;
    }
}

/*---------------------------------------------------------------------
   Function: MIDI_PlaySong

   Begins playback of a MIDI song.
---------------------------------------------------------------------*/

int MIDI_PlaySong(
    unsigned char *song,
    int loopflag)

{
    int numtracks;
    int format;
    long headersize;
    long tracklength;
    track *CurrentTrack;
    unsigned char *ptr;

    if (_MIDI_SongLoaded)
    {
        MIDI_StopSong();
    }

    _MIDI_Loop = loopflag;

    if (_MIDI_Funcs == NULL)
    {
        return (MIDI_NullMidiModule);
    }

    if (*(unsigned long *)song != MIDI_HEADER_SIGNATURE)
    {
        return (MIDI_InvalidMidiFile);
    }

    song += 4;

    headersize = _MIDI_ReadNumber(song, 4);
    song += 4;
    format = _MIDI_ReadNumber(song, 2);
    _MIDI_NumTracks = _MIDI_ReadNumber(song + 2, 2);
    _MIDI_Division = _MIDI_ReadNumber(song + 4, 2);
    if (_MIDI_Division < 0)
    {
        // If a SMPTE time division is given, just set to 96 so no errors occur
        _MIDI_Division = 96;
    }

    if (format > MAX_FORMAT)
    {
        return (MIDI_UnknownMidiFormat);
    }

    ptr = song + headersize;

    if (_MIDI_NumTracks == 0)
    {
        return (MIDI_NoTracks);
    }

    _MIDI_TrackMemSize = _MIDI_NumTracks * sizeof(track);
    _MIDI_TrackPtr = Z_MallocUnowned(_MIDI_TrackMemSize, PU_STATIC);

    CurrentTrack = _MIDI_TrackPtr;
    numtracks = _MIDI_NumTracks;
    while (numtracks--)
    {
        if (*(unsigned long *)ptr != MIDI_TRACK_SIGNATURE)
        {
            Z_Free(_MIDI_TrackPtr);

            _MIDI_TrackPtr = NULL;
            _MIDI_TrackMemSize = 0;

            return (MIDI_InvalidTrack);
        }

        tracklength = _MIDI_ReadNumber(ptr + 4, 4);
        ptr += 8;
        CurrentTrack->start = ptr;
        ptr += tracklength;
        CurrentTrack++;
    }

    if (_MIDI_Funcs->GetVolume != NULL)
    {
        _MIDI_TotalVolume = _MIDI_Funcs->GetVolume();
    }

    _MIDI_InitEMIDI();

    _MIDI_ResetTracks();

    if (!Reset)
    {
        MIDI_Reset();
    }

    Reset = FALSE;

    _MIDI_PlayRoutine = TS_ScheduleTask(_MIDI_ServiceRoutine, 100, 1, NULL);
    //   _MIDI_PlayRoutine = TS_ScheduleTask( test, 100, 1, NULL );
    MIDI_SetTempo(120);
    TS_Dispatch();

    _MIDI_SongLoaded = TRUE;
    _MIDI_SongActive = TRUE;

    return (MIDI_Ok);
}

/*---------------------------------------------------------------------
   Function: MIDI_SetTempo

   Sets the song tempo.
---------------------------------------------------------------------*/

void MIDI_SetTempo(int tempo)
{
    long tickspersecond;

    MIDI_Tempo = tempo;
    // VITI95: OPTIMIZE
    tickspersecond = (tempo * _MIDI_Division) / 60;
    if (_MIDI_PlayRoutine != NULL)
    {
        TS_SetTaskRate(_MIDI_PlayRoutine, tickspersecond);
        //      TS_SetTaskRate( _MIDI_PlayRoutine, tickspersecond / 4 );
    }
    // VITI95: OPTIMIZE
    _MIDI_FPSecondsPerTick = (1 << TIME_PRECISION) / tickspersecond;
}

/*---------------------------------------------------------------------
   Function: MIDI_SongPlaying

   Returns whether a song is playing or not.
---------------------------------------------------------------------*/

int MIDI_SongPlaying(void)
{
   return _MIDI_SongActive;
}

/*---------------------------------------------------------------------
   Function: MIDI_InitEMIDI

   Sets up the EMIDI
---------------------------------------------------------------------*/

static void _MIDI_InitEMIDI(
    void)

{
    int event;
    int command;
    int channel;
    int length;
    int IncludeFound;
    track *Track;
    int tracknum;
    int c1;
    int c2;

    _MIDI_ResetTracks();

    _MIDI_TotalTime = 0;
    _MIDI_TotalTicks = 0;
    _MIDI_TotalBeats = 0;
    _MIDI_TotalMeasures = 0;

    Track = _MIDI_TrackPtr;
    tracknum = 0;
    while ((tracknum < _MIDI_NumTracks) && (Track != NULL))
    {
        _MIDI_Tick = 0;
        _MIDI_Beat = 1;
        _MIDI_Measure = 1;
        _MIDI_Time = 0;
        _MIDI_BeatsPerMeasure = 4;
        _MIDI_TicksPerBeat = _MIDI_Division;
        _MIDI_TimeBase = 4;

        _MIDI_PositionInTicks = 0;
        _MIDI_ActiveTracks = 0;
        _MIDI_Context = -1;

        Track->RunningStatus = 0;
        Track->active = TRUE;

        while (Track->delay > 0)
        {
            _MIDI_AdvanceTick();
            Track->delay--;
        }

        IncludeFound = FALSE;
        while (Track->active)
        {
            GET_NEXT_EVENT(Track, event);

            if (GET_MIDI_COMMAND(event) == MIDI_SPECIAL)
            {
                switch (event)
                {
                case MIDI_SYSEX:
                    _MIDI_SysEx(Track);
                    break;
                case MIDI_SYSEX_CONTINUE:
                    Track->pos += _MIDI_ReadDelta(Track);
                    break;
                case MIDI_META_EVENT:
                    _MIDI_MetaEvent(Track);
                    break;
                }

                if (Track->active)
                {
                    Track->delay = _MIDI_ReadDelta(Track);
                    while (Track->delay > 0)
                    {
                        _MIDI_AdvanceTick();
                        Track->delay--;
                    }
                }

                continue;
            }

            if (event & MIDI_RUNNING_STATUS)
            {
                Track->RunningStatus = event;
            }
            else
            {
                event = Track->RunningStatus;
                Track->pos--;
            }

            channel = GET_MIDI_CHANNEL(event);
            command = GET_MIDI_COMMAND(event);
            length = _MIDI_CommandLengths[command];

            if (command == MIDI_CONTROL_CHANGE)
            {
                length += *Track->pos == MIDI_MONO_MODE_ON;
                GET_NEXT_EVENT(Track, c1);
                GET_NEXT_EVENT(Track, c2);
                length -= 2;
            }

            Track->pos += length;
            Track->delay = _MIDI_ReadDelta(Track);

            while (Track->delay > 0)
            {
                _MIDI_AdvanceTick();
                Track->delay--;
            }
        }

        _MIDI_TotalTime = max(_MIDI_TotalTime, _MIDI_Time);
        if (RELATIVE_BEAT(_MIDI_Measure, _MIDI_Beat, _MIDI_Tick) >
            RELATIVE_BEAT(_MIDI_TotalMeasures, _MIDI_TotalBeats,
                          _MIDI_TotalTicks))
        {
            _MIDI_TotalTicks = _MIDI_Tick;
            _MIDI_TotalBeats = _MIDI_Beat;
            _MIDI_TotalMeasures = _MIDI_Measure;
        }

        Track++;
        tracknum++;
    }

    _MIDI_ResetTracks();
}
