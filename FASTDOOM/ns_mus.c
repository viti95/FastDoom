/*---------------------------------------------------------------------
   File: ns_mus.c

   Native MUS module player for the Apogee Sound System.

   A MUS module is decoded directly from memory and its events are
   dispatched through the midifuncs device callbacks, so no
   MUS-to-MIDI conversion is required.

   The score is serviced by a taskman task which is run once per
   MUS time unit.  The time division used by the Doom engine is
   140 Hz, so one MUS tick lasts 1/140th of a second and the task
   is scheduled at 140 Hz.  (This is the same division that the
   original mus2mid converter relied on: it emitted the MUS delay
   values verbatim as delta times of a 70 ticks/quarter MIDI file
   played back at 140 BPM, i.e. 140 ticks per second.)

   MUS score format:
     "MUS\x1a" header followed by the score.  The score is a
     sequence of event blocks.  Each block holds one or more
     events; all events except the last one in a block have bit 7
     of the event descriptor clear.  After the last event of a
     block follows a variable-length (base-128) time value
     giving the number of time units to wait before the next
     block is played.  All events of a block are due at the same
     time.

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

/* MUS time division of the Doom engine: number of MUS ticks per
   second.  One MUS tick lasts 1/140th of a second. */

#define MUS_TIME_DIVISION 140

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
    0x78,   /* 10 all sounds off */
    0x7b,   /* 11 all notes off */
    0x7e,   /* 12 local control */
    0x7f,   /* 13 all controllers off */
    0x79    /* 14 all notes off (poly) */
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

/* Last velocity used on each channel.  A key press without an
   explicit velocity byte re-uses the previous velocity of its
   channel. */

static unsigned char MUS_ChannelVel[NUM_MIDI_CHANNELS];

static task *MUS_Task = NULL;
static midifuncs *MUS_Funcs = NULL;

/*---------------------------------------------------------------------
   Function prototypes
---------------------------------------------------------------------*/

static void MUS_ServiceRoutine(task *Task);
static void MUS_PlayEvent(void);
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

   The first byte holds the least significant 7 bits, the second
   byte the next most significant 7 bits, and so on.  The high
   bit of each byte is set if another byte follows.

   Returns the number of time units, or -1 on end of score.
---------------------------------------------------------------------*/

static int
MUS_ReadTime(
    void)
{
    int value;
    int shift;
    unsigned char b;

    value = 0;
    shift = 0;

    for (;;)
    {
        if (MUS_Ptr >= MUS_ScoreEnd)
        {
            return (-1);
        }

        b = *MUS_Ptr++;
        value = value + ((b & 0x7f) << shift);
        shift += 7;

        if ((b & 0x80) == 0)
        {
            return (value);
        }
    }
}

/*---------------------------------------------------------------------
   Function: MUS_ReadEvent

   Reads the next event from the score.  If the event is the last
   one of its block, the following time value is read into
   MUS_Delay as well.

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
        else
        {
            /* No velocity byte, keep the last velocity used on
               this channel */
            MUS_Vel = MUS_ChannelVel[MUS_Event & 0x0f];
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

    case MUS_ENDOFMEASURE:
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
   Function: MUS_PlayEvent

   Dispatches the current event to the music device.
---------------------------------------------------------------------*/

static void
MUS_PlayEvent(
    void)
{
    int channel;
    int command;
    int value;

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
        MUS_ChannelVel[channel] = (unsigned char)(MUS_Vel & 0x7f);
        if (MUS_Funcs->NoteOn != NULL)
        {
            MUS_Funcs->NoteOn(channel, MUS_Parameter, MUS_Vel & 0x7f);
        }
        break;

    case MUS_PITCHWHEEL:
        /* The 8-bit MUS pitch value (128 is the center position) */
        /* is scaled to the 14-bit MIDI pitch bend range */
        if (MUS_Funcs->PitchBend != NULL)
        {
            int bend;

            bend = MUS_Parameter * 64;
            MUS_Funcs->PitchBend(channel, bend & 0x7f,
                                 (bend >> 7) & 0x7f);
        }
        break;

    case MUS_SYSTEMEVENT:
        /* System events are the valueless controllers 10-14 */
        if (MUS_Parameter >= 10 && MUS_Parameter < 15 &&
            MUS_Funcs->ControlChange != NULL)
        {
            MUS_Funcs->ControlChange(channel,
                                     MUS_ControllerMap[MUS_Parameter],
                                     0);
        }
        break;

    case MUS_CONTROLLER:
        if (MUS_Parameter == 0)
        {
            /* Instrument change */
            if (MUS_Funcs->ProgramChange != NULL)
            {
                MUS_Funcs->ProgramChange(channel, MUS_Vel & 0x7f);
            }
        }
        else if (MUS_Parameter < 15)
        {
            if (MUS_Funcs->ControlChange != NULL)
            {
                /* DMX stores 8-bit controller values, clamp the
                   value to the legal 7-bit MIDI range */
                value = (MUS_Vel & 0x80) ? 0x7f : (MUS_Vel & 0x7f);
                MUS_Funcs->ControlChange(channel,
                                         MUS_ControllerMap[MUS_Parameter],
                                         value);
            }
        }
        break;

    case MUS_ENDOFMEASURE:
    case MUS_SCOREEND:
        /* Nothing to do here */
        break;
    }
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
   the music device.  Run once per MUS time unit (140 times per
   second).
---------------------------------------------------------------------*/

static void
MUS_ServiceRoutine(
    task *Task)
{
    if (!MUS_SongActive)
    {
        return;
    }

    if (MUS_Delay > 0)
    {
        MUS_Delay--;
        return;
    }

    /* All the events of a block are due at the same time, play
       the whole block before waiting for the next one */
    for (;;)
    {
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

        MUS_PlayEvent();

        MUS_HaveEvent = FALSE;

        if (MUS_Event & 0x80)
        {
            /* Last event of the block was played */
            break;
        }
    }
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
    int channel;

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

    for (channel = 0; channel < NUM_MIDI_CHANNELS; channel++)
    {
        MUS_ChannelVel[channel] = 127;
    }

    /* The MUS time division of the Doom engine is 140 Hz: one MUS
       tick lasts 1/140th of a second, so the score is serviced
       140 times per second. */
    MUS_Task = TS_ScheduleTask(MUS_ServiceRoutine, MUS_TIME_DIVISION, 1,
                               NULL);

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

   Note that MUS scores have a fixed time division (140 Hz in the
   Doom engine) and the tempo is baked into the score delay
   values, so this only updates the stored tempo and does not
   change the score service rate.
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

   Pauses playback of a MUS module.
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
