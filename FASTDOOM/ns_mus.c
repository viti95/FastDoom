/*---------------------------------------------------------------------
   File: ns_mus.c

   Native MUS module player for the Apogee Sound System.

   A MUS module is decoded directly from memory and its events are
   dispatched through the midifuncs device callbacks, so no
   MUS-to-MIDI conversion is required.

   The score is serviced by a taskman task which is run once per
   MUS time unit (24 time units per second at a tempo of 120 BPM).

   MUS score format:
     "MUS\x1a" header followed by the score.  The score is a
     sequence of event blocks.  Each block holds one or more
     events; all events except the last one in a block have bit 7
     of the event descriptor clear.  After the last event of a
     block follows a variable-length (base-128) time value
     giving the number of time units to wait before the next
     block is played.

   Event descriptor byte:
     bits 0-3:  channel number
     bits 3-5:  event command
     bit  7:    last event in block

---------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>

#include "ns_task.h"
#include "ns_std.h"
#include "ns_music.h"
#include "ns_midi.h"
#include "ns_midif.h"
#include "ns_mus.h"

/*---------------------------------------------------------------------
   Global variables
---------------------------------------------------------------------*/

/* Mapping table from MUS controller numbers to MIDI controller */
/* numbers.  Controller 0 (instrument) is handled separately as a */
/* program change. */

static const unsigned char MUS_ControllerMap[15] =
{
    0,      /* 0 */
    20,     /* 1  breath */
    1,      /* 2  modulation wheel */
    7,      /* 3  vibrato */
    10,     /* 4  pitch bend */
    11,     /* 5  expression */
    0x5b,   /* 6  modulation time */
    0x5d,   /* 7  decay time */
    40,     /* 8  vibrato rate */
    43,     /* 9  vibrato depth */
    0x78,   /* 10 release time */
    0x7b,   /* 11 all notes off */
    0x7e,   /* 12 reverb */
    0x7f,   /* 13 tremolo */
    0x79    /* 14 release */
};

static unsigned char *MUS_Ptr = NULL;        /* current read pointer */
static unsigned char *MUS_ScoreStart = NULL; /* start of the score */
static unsigned char *MUS_ScoreEnd = NULL;   /* end of the score */

static int MUS_Event = 0;                    /* current event descriptor */
static int MUS_Parameter = 0;                /* event parameter */
static int MUS_Vel = 127;                    /* velocity / controller value */
static int MUS_Delay = 0;                    /* time units until next event */
static int MUS_HaveEvent = FALSE;            /* an event is ready to play */
static int MUS_SongActive = FALSE;
static int MUS_SongLoaded = FALSE;
static int MUS_Loop = FALSE;
static int MUS_Tempo = 120;

static task *MUS_Task = NULL;
static midifuncs *MUS_Funcs = NULL;

/*---------------------------------------------------------------------
   Function prototypes
---------------------------------------------------------------------*/

static void MUS_ServiceRoutine(task *Task);
static int MUS_ReadEvent(void);
static int MUS_ReadTime(void);
static void MUS_AllNotesOff(void);
static void MUS_ResetScore(void);

/*---------------------------------------------------------------------
   Function: NMUS_IsMusFile

   Returns TRUE if the given data is a MUS module.
---------------------------------------------------------------------*/

int
NMUS_IsMusFile(
    unsigned char *data)
{
    if (data == NULL)
    {
        return (FALSE);
    }

    return (data[0] == 'M' &&
            data[1] == 'U' &&
            data[2] == 'S' &&
            data[3] == 0x1a);
}

/*---------------------------------------------------------------------
   Function: MUS_ReadTime

   Reads a variable-length (base-128) time value from the score.

   Returns the number of time units, or -1 on end of score.
---------------------------------------------------------------------*/

static int
MUS_ReadTime(
    void)
{
    int value;
    unsigned char b;

    value = 0;

    for (;;)
    {
        if (MUS_Ptr >= MUS_ScoreEnd)
        {
            return (-1);
        }

        b = *MUS_Ptr++;
        value = (value << 7) + (b & 0x7f);

        if ((b & 0x80) == 0)
        {
            return (value);
        }
    }
}

/*---------------------------------------------------------------------
   Function: MUS_ReadEvent

   Reads the next event from the score.

   Returns TRUE if an event is ready to be played, FALSE if the
   score has ended.
---------------------------------------------------------------------*/

static int
MUS_ReadEvent(
    void)
{
    int command;

    if (MUS_Ptr >= MUS_ScoreEnd)
    {
        return (FALSE);
    }

    MUS_Event = *MUS_Ptr++;
    MUS_Vel = 127;

    command = MUS_Event & 0x70;

    switch (command)
    {
    case MUS_KEYRELEASE:
        if (MUS_Ptr >= MUS_ScoreEnd)
        {
            return (FALSE);
        }
        MUS_Parameter = *MUS_Ptr++;
        break;

    case MUS_KEYPRESS:
        if (MUS_Ptr >= MUS_ScoreEnd)
        {
            return (FALSE);
        }
        MUS_Parameter = *MUS_Ptr++;
        if (MUS_Parameter & 0x80)
        {
            /* A velocity byte follows the key number */
            MUS_Parameter &= 0x7f;
            if (MUS_Ptr >= MUS_ScoreEnd)
            {
                return (FALSE);
            }
            MUS_Vel = *MUS_Ptr++;
        }
        break;

    case MUS_PITCHWHEEL:
    case MUS_SYSTEMEVENT:
        if (MUS_Ptr >= MUS_ScoreEnd)
        {
            return (FALSE);
        }
        MUS_Parameter = *MUS_Ptr++;
        break;

    case MUS_CONTROLLER:
        if (MUS_Ptr + 1 >= MUS_ScoreEnd)
        {
            return (FALSE);
        }
        MUS_Parameter = *MUS_Ptr++;
        MUS_Vel = *MUS_Ptr++;
        break;

    case MUS_SCOREEND:
        MUS_Parameter = 0;
        break;

    default:
        /* Unknown event command, the score is corrupt */
        return (FALSE);
    }

    if (MUS_Event & 0x80)
    {
        /* Last event in this block, a time value follows */
        MUS_Delay = MUS_ReadTime();
        if (MUS_Delay < 0)
        {
            return (FALSE);
        }
    }

    return (TRUE);
}

/*---------------------------------------------------------------------
   Function: MUS_AllNotesOff

   Sends an all-notes-off command on every channel.
---------------------------------------------------------------------*/

static void
MUS_AllNotesOff(
    void)
{
    int channel;

    if (MUS_Funcs == NULL || MUS_Funcs->ControlChange == NULL)
    {
        return;
    }

    for (channel = 0; channel < NUM_MIDI_CHANNELS; channel++)
    {
        MUS_Funcs->ControlChange(channel, MIDI_ALL_NOTES_OFF, 0);
    }
}

/*---------------------------------------------------------------------
   Function: MUS_ResetScore

   Rewinds the score to its beginning.
---------------------------------------------------------------------*/

static void
MUS_ResetScore(
    void)
{
    MUS_Ptr = MUS_ScoreStart;
    MUS_HaveEvent = FALSE;
    MUS_Delay = 0;
}

/*---------------------------------------------------------------------
   Function: MUS_ServiceRoutine

   Task that decodes the MUS score and dispatches its events to
   the music device.  Run once per MUS time unit.
---------------------------------------------------------------------*/

static void
MUS_ServiceRoutine(
    task *Task)
{
    int channel;
    int command;

    if (!MUS_SongActive)
    {
        return;
    }

    if (MUS_Delay > 0)
    {
        MUS_Delay--;
        return;
    }

    if (!MUS_HaveEvent)
    {
        MUS_HaveEvent = MUS_ReadEvent();

        if (!MUS_HaveEvent)
        {
            /* The score has ended */
            if (MUS_Loop)
            {
                MUS_ResetScore();
                MUS_HaveEvent = MUS_ReadEvent();
            }

            if (!MUS_HaveEvent)
            {
                MUS_SongActive = FALSE;
                return;
            }
        }
    }

    channel = MUS_Event & 0x0f;
    command = MUS_Event & 0x70;

    switch (command)
    {
    case MUS_KEYRELEASE:
        if (MUS_Funcs->NoteOff != NULL)
        {
            MUS_Funcs->NoteOff(channel, MUS_Parameter, 0);
        }
        break;

    case MUS_KEYPRESS:
        if (MUS_Funcs->NoteOn != NULL)
        {
            MUS_Funcs->NoteOn(channel, MUS_Parameter, MUS_Vel);
        }
        break;

    case MUS_PITCHWHEEL:
        if (MUS_Funcs->PitchBend != NULL)
        {
            MUS_Funcs->PitchBend(channel, MUS_Parameter, 0);
        }
        break;

    case MUS_SYSTEMEVENT:
        if (MUS_Funcs->ControlChange != NULL)
        {
            switch (MUS_Parameter)
            {
            case MUS_ALL_NOTES_OFF:
                MUS_Funcs->ControlChange(channel, MIDI_ALL_NOTES_OFF, 0);
                break;

            case MUS_ALL_SOUNDS_OFF:
            case MUS_LOCAL_CONTROL:
            case MUS_RESET_CONTROLLERS:
            case MUS_ALL_POLY_OFF:
                MUS_Funcs->ControlChange(channel,
                                         MIDI_RESET_ALL_CONTROLLERS, 0);
                break;

            default:
                /* Ignore unknown system events */
                break;
            }
        }
        break;

    case MUS_CONTROLLER:
        if (MUS_Parameter == 0)
        {
            /* Instrument change */
            if (MUS_Funcs->ProgramChange != NULL)
            {
                MUS_Funcs->ProgramChange(channel, MUS_Vel);
            }
        }
        else if (MUS_Parameter < 15)
        {
            if (MUS_Funcs->ControlChange != NULL)
            {
                MUS_Funcs->ControlChange(channel,
                                         MUS_ControllerMap[MUS_Parameter],
                                         MUS_Vel);
            }
        }
        break;

    case MUS_SCOREEND:
        /* The score ends, nothing to do here */
        break;
    }

    MUS_HaveEvent = FALSE;
}

/*---------------------------------------------------------------------
   Function: NMUS_PlaySong

   Begins playback of a MUS module.
---------------------------------------------------------------------*/

int
NMUS_PlaySong(
    unsigned char *song,
    int loop,
    midifuncs *funcs)
{
    unsigned int scorelength;
    unsigned int scorestart;

    if (funcs == NULL)
    {
        return (MUSIC_Warning);
    }

    if (!NMUS_IsMusFile(song))
    {
        return (MUSIC_Warning);
    }

    if (MUS_SongLoaded)
    {
        NMUS_StopSong();
    }

    MUS_Funcs = funcs;
    MUS_Loop = loop;

    /* Grab the score location out of the MUS header */
    scorelength = (unsigned int)song[4] | ((unsigned int)song[5] << 8);
    scorestart = (unsigned int)song[6] | ((unsigned int)song[7] << 8);

    MUS_ScoreStart = song + scorestart;
    MUS_ScoreEnd = MUS_ScoreStart + scorelength;

    MUS_ResetScore();

    MUS_Tempo = 120;

    /* MUS has 24 time units per second at a tempo of 120 BPM */
    MUS_Task = TS_ScheduleTask(MUS_ServiceRoutine, MUS_Tempo / 5, 1, NULL);

    if (MUS_Task == NULL)
    {
        return (MUSIC_Error);
    }

    TS_Dispatch();

    MUS_SongLoaded = TRUE;
    MUS_SongActive = TRUE;

    return (MUSIC_Ok);
}

/*---------------------------------------------------------------------
   Function: NMUS_SetTempo

   Sets the tempo of the current MUS module.
---------------------------------------------------------------------*/

void
NMUS_SetTempo(
    int tempo)
{
    if (tempo < 5)
    {
        tempo = 5;
    }

    MUS_Tempo = tempo;

    if (MUS_Task != NULL)
    {
        TS_SetTaskRate(MUS_Task, tempo / 5);
    }
}

/*---------------------------------------------------------------------
   Function: NMUS_StopSong

   Stops playback of the current MUS module.
---------------------------------------------------------------------*/

void
NMUS_StopSong(
    void)
{
    if (MUS_SongLoaded)
    {
        if (MUS_Task != NULL)
        {
            TS_Terminate(MUS_Task);
            MUS_Task = NULL;
        }

        MUS_SongActive = FALSE;
        MUS_SongLoaded = FALSE;
        MUS_HaveEvent = FALSE;
        MUS_Delay = 0;

        MUS_AllNotesOff();
    }
}

/*---------------------------------------------------------------------
   Function: NMUS_PauseSong

   Pauses playback of the current MUS module.
---------------------------------------------------------------------*/

void
NMUS_PauseSong(
    void)
{
    if (MUS_SongLoaded)
    {
        MUS_SongActive = FALSE;
        MUS_AllNotesOff();
    }
}

/*---------------------------------------------------------------------
   Function: NMUS_ContinueSong

   Continues playback of a paused MUS module.
---------------------------------------------------------------------*/

void
NMUS_ContinueSong(
    void)
{
    if (MUS_SongLoaded)
    {
        MUS_SongActive = TRUE;
    }
}

/*---------------------------------------------------------------------
   Function: NMUS_SongPlaying

   Returns whether a MUS module is playing.
---------------------------------------------------------------------*/

int
NMUS_SongPlaying(
    void)
{
    return (MUS_SongActive);
}
