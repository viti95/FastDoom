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
   played back at 120 BPM, i.e. 140 ticks per second.)

   MUS score format:
     "MUS\x1a" header followed by the score.  The score is a
     sequence of event blocks.  Each block holds one or more
     events; all events except the last one in a block have bit 7
     of the event descriptor clear.  After the last event of a
     block follows a variable-length (base-128) time value
     giving the number of time units to wait before the next
     block is played.  All events of a block are due at the same
     time.  The time value is stored most significant byte
     first: each byte holds 7 data bits and its high bit is set
     if another byte follows, the value being built up as
     "value = value * 128 + (b & 0x7f)".  For example, the byte
     pair 82 05 decodes to 2 * 128 + 5 = 261.

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

/* MUS controller number that carries the channel volume (mapped
   to MIDI controller 7). */

#define MUS_CONTROLLER_VOLUME 3

/* Mapping table from MUS controller numbers to MIDI controller */
/* numbers.  Controller 0 (instrument) is handled separately as a */
/* program change. */

static const unsigned char MUS_ControllerMap[15] =
{
    0,      /* 0  instrument (handled as a program change) */
    0x20,   /* 1  bank select */
    0x01,   /* 2  modulation wheel */
    0x07,   /* 3  volume */
    0x0a,   /* 4  pan */
    0x0b,   /* 5  expression */
    0x5b,   /* 6  reverb depth */
    0x5d,   /* 7  chorus depth */
    0x40,   /* 8  sustain pedal */
    0x43,   /* 9  soft pedal */
    0x78,   /* 10 all sounds off */
    0x7b,   /* 11 all notes off */
    0x7e,   /* 12 mono mode */
    0x7f,   /* 13 poly mode */
    0x79    /* 14 reset all controllers */
};

/* Mapping table from MUS channels to MIDI channels.  MUS channel
   15 carries the percussion and is mapped to the MIDI percussion
   channel (channel 9, zero based).  All the other channels are
   mapped 1:1; MUS scores never use MUS channel 9. */

static const unsigned char MUS_ChannelMap[NUM_MIDI_CHANNELS] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 9
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

/* Total volume of the music, 0..MIDI_MaxVolume.  Devices with a
   hardware mixer of their own get the volume through their
   SetVolume function; all other devices have the channel volume
   controller values scaled by this total volume in software, the
   same way the MIDI file player does it. */

static int MUS_TotalVolume = MIDI_MaxVolume;

/* Last score volume used on each MUS channel.  Used to re-send the
   (scaled) channel volumes when the total volume changes. */

static unsigned char MUS_ChannelVol[NUM_MIDI_CHANNELS];

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
static void MUS_SendChannelVolumes(void);
static void MUS_ResetScore(void);

/*---------------------------------------------------------------------
   Function: MUS_IsMusFile

   Returns TRUE if the given data is a MUS module.
---------------------------------------------------------------------*/

int
MUS_IsMusFile(
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

   The time value is stored most significant byte first: the
   accumulated value is multiplied by 128 and the 7 low bits of
   the next byte are added to it.  The high bit of each byte is
   set if another byte follows.

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
        value = value * 128 + (b & 0x7f);

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
        /* Unused command (7).  DMX still consumes one data byte */
        /* for it, so skip the byte and keep playing */
        if (MUS_Ptr >= MUS_ScoreEnd)
        {
            return (FALSE);
        }
        MUS_Parameter = *MUS_Ptr++;
        break;
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

    channel = MUS_ChannelMap[MUS_Event & 0x0f];
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
        /* The velocity cache is kept per MUS channel (the score
           refers to the previous velocity of the raw MUS
           channel), while the events are emitted on the mapped
           MIDI channel */
        MUS_ChannelVel[MUS_Event & 0x0f] = (unsigned char)(MUS_Vel & 0x7f);
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
        else if (MUS_Parameter >= 1 && MUS_Parameter <= 9)
        {
            if (MUS_Funcs->ControlChange != NULL)
            {
                /* DMX stores 8-bit controller values, clamp the
                   value to the legal 7-bit MIDI range */
                value = MUS_Vel & 0x7f;

                if (MUS_Parameter == MUS_CONTROLLER_VOLUME)
                {
                    /* Keep the score volume per channel so the
                       channel volumes can be re-sent when the
                       total volume changes, and scale it by the
                       total volume on devices without a mixer */
                    MUS_ChannelVol[MUS_Event & 0x0f] =
                        (unsigned char)value;
                    if (MUS_Funcs->SetVolume == NULL)
                    {
                        value = (value * MUS_TotalVolume + 127) / 255;
                    }
                }

                MUS_Funcs->ControlChange(channel,
                                         MUS_ControllerMap[MUS_Parameter],
                                         value);
            }
        }
        break;

    case MUS_ENDOFMEASURE:
    case MUS_SCOREEND:
    default:
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
   Function: MUS_SendChannelVolumes

   Re-sends the last score volume on every channel, scaled by the
   total volume on devices without a hardware mixer.
---------------------------------------------------------------------*/

static void
MUS_SendChannelVolumes(
    void)
{
    int channel;
    int volume;

    if (MUS_Funcs == NULL || MUS_Funcs->ControlChange == NULL)
    {
        return;
    }

    for (channel = 0; channel < NUM_MIDI_CHANNELS; channel++)
    {
        volume = MUS_ChannelVol[channel];
        if (MUS_Funcs->SetVolume == NULL)
        {
            volume = (volume * MUS_TotalVolume + 127) / 255;
        }
        MUS_Funcs->ControlChange(MUS_ChannelMap[channel], MIDI_VOLUME,
                                 volume);
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

    /* All the events of a block are due at the same time, play
       the whole block before waiting for the next one.  A block
       with delay d must be followed by exactly d ticks: the
       counter is only decremented on ticks where the next block
       is not due yet, so the next block is played on the tick
       the counter reaches zero (and a zero-delay block is played
       on the very same tick). */
    for (;;)
    {
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

        MUS_PlayEvent();

        MUS_HaveEvent = FALSE;

        if (MUS_Event & 0x80)
        {
            /* Last event of the block was played.  If the next
               block has a non-zero delay, start counting it on
               this very tick so that exactly d ticks elapse
               before it is played; if its delay is zero it is
               due now and the loop plays it on this same tick */
            if (MUS_Delay > 0)
            {
                MUS_Delay--;
                return;
            }
        }
    }
}

/*---------------------------------------------------------------------
   Function: MUS_PlaySong

   Begins playback of a MUS module.
---------------------------------------------------------------------*/

int
MUS_PlaySong(
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

    if (!MUS_IsMusFile(song))
    {
        return (MUSIC_Warning);
    }

    if (MUS_SongLoaded)
    {
        MUS_StopSong();
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
        MUS_ChannelVol[channel] = 127;
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
   Function: MUS_SetTempo

   Sets the tempo of the current MUS module.

   Note that MUS scores have a fixed time division (140 Hz in the
   Doom engine) and the tempo is baked into the score delay
   values, so this only updates the stored tempo and does not
   change the score service rate.
---------------------------------------------------------------------*/

void
MUS_SetTempo(
    int tempo)
{
    if (tempo < 5)
    {
        tempo = 5;
    }

    MUS_Tempo = tempo;
}

/*---------------------------------------------------------------------
   Function: MUS_SetVolume

   Sets the total volume of the MUS music, 0..MIDI_MaxVolume.
   Devices with a hardware mixer get the volume through their
   SetVolume function; all other devices have the channel volume
   controller values scaled in software and are re-sent on every
   channel, the same way the MIDI file player does it.
---------------------------------------------------------------------*/

void
MUS_SetVolume(
    int volume)
{
    if (MUS_Funcs == NULL)
    {
        return;
    }

    if (volume > MIDI_MaxVolume)
    {
        volume = MIDI_MaxVolume;
    }

    if (volume < 0)
    {
        volume = 0;
    }

    MUS_TotalVolume = volume;

    if (MUS_Funcs->SetVolume != NULL)
    {
        MUS_Funcs->SetVolume(volume);
    }
    else
    {
        MUS_SendChannelVolumes();
    }
}

/*---------------------------------------------------------------------
   Function: MUS_StopSong

   Stops playback of the current MUS module.
---------------------------------------------------------------------*/

void
MUS_StopSong(
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
   Function: MUS_PauseSong

   Pauses playback of a MUS module.
---------------------------------------------------------------------*/

void
MUS_PauseSong(
    void)
{
    if (MUS_SongLoaded)
    {
        MUS_SongActive = FALSE;
        MUS_AllNotesOff();
    }
}

/*---------------------------------------------------------------------
   Function: MUS_ContinueSong

   Continues playback of a paused MUS module.
---------------------------------------------------------------------*/

void
MUS_ContinueSong(
    void)
{
    if (MUS_SongLoaded)
    {
        MUS_SongActive = TRUE;
    }
}

/*---------------------------------------------------------------------
   Function: MUS_SongPlaying

   Returns whether a MUS module is playing.
---------------------------------------------------------------------*/

int
MUS_SongPlaying(
    void)
{
    return (MUS_SongActive);
}
