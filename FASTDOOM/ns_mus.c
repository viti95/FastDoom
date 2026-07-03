//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2006 Ben Ryves
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Native DOOM MUS music format player for FastDoom.
// Directly interprets MUS event streams and dispatches MIDI messages
// through the midifuncs interface. No live conversion to MIDI needed.
//

#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "ns_task.h"
#include "ns_midi.h"
#include "ns_mus.h"
#include "z_zone.h"
#include "doomtype.h"
#include "fastmath.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

// MUS runs at a fixed 70-tick-per-second clock
#define MUS_TICK_RATE 70

#define MUS_NUM_CHANNELS 16
#define MUS_PERCUSSION_CHAN 15
#define MIDI_PERCUSSION_CHAN 9

// MUS event types
#define MUS_RELEASE_KEY   0x00
#define MUS_PRESS_KEY     0x10
#define MUS_PITCH_WHEEL   0x20
#define MUS_SYSTEM_EVENT  0x30
#define MUS_CTRL_CHANGE   0x40
#define MUS_SCORE_END     0x60

// MUS header
typedef struct
{
    byte  id[4];
    short scorelength;
    short scorestart;
    short primarychannels;
    short secondarychannels;
    short instrumentcount;
} musheader_t;

// MUS -> MIDI controller mapping (for controllers 1-14)
static const byte mus_controller_map[] =
{
    /*  0  */ 0x00,   /* (program change, handled separately) */
    /*  1  */ 0x20,   /* Bank Select */
    /*  2  */ 0x01,   /* Modulation */
    /*  3  */ 0x07,   /* Volume */
    /*  4  */ 0x0A,   /* Pan */
    /*  5  */ 0x0B,   /* Expression */
    /*  6  */ 0x5B,   /* Portamento Time */
    /*  7  */ 0x5D,   /* Release Time */
    /*  8  */ 0x40,   /* Decay Time */
    /*  9  */ 0x43,   /* Vibrato Rate */
    /* 10  */ 0x78,   /* All Notes Off */
    /* 11  */ 0x7B,   /* All Sound Off */
    /* 12  */ 0x7E,   /* Mono Mode On */
    /* 13  */ 0x7F,   /* Poly Mode On */
    /* 14  */ 0x79    /* All Controllers Off */
};

//
// Player state
//

// Raw MUS data pointer (owned by caller, we don't free it)
static byte *MUS_ScoreData = NULL;

// Pointer into the raw MUS data for current read position
static byte *MUS_ScorePos = NULL;

// Start of score data within MUS_ScoreData
static byte *MUS_ScoreStart = NULL;
static unsigned int MUS_ScoreLength;

// MIDI functions table (fetched from MIDI layer at play time)
static midifuncs *MUS_Funcs = NULL;

// Channel mapping: MUS channel -> MIDI channel (-1 = not yet allocated)
static int MUS_ChannelMap[MUS_NUM_CHANNELS];

// Per-channel cached velocities (for note-on events)
static byte MUS_ChannelVelocity[MUS_NUM_CHANNELS];

// Delay counter (ticks until next event block)
static long MUS_Delay = 0;

// Song state
static int MUS_SongLoaded = FALSE;
static int MUS_SongActive = FALSE;
static int MUS_Loop = FALSE;

// Task handle for playback
static task *MUS_PlayRoutine = NULL;


//
// Helper: allocate a free MIDI channel for a MUS channel
//

static int MUS_AllocMIDIChannel(void)
{
    int i;
    int max = -1;

    for (i = 0; i < MUS_NUM_CHANNELS; i++)
    {
        if (MUS_ChannelMap[i] > max)
        {
            max = MUS_ChannelMap[i];
        }
    }

    // Next channel after the highest allocated one
    i = max + 1;

    /* Skip the MIDI percussion channel for non-percussion MUS channels */
    if (i == MIDI_PERCUSSION_CHAN)
    {
        i++;
    }

    return i;
}

//
// Helper: get MIDI channel for a given MUS channel.
// MUS channel 15 (percussion) maps to MIDI channel 9.
// Other channels are allocated sequentially, skipping channel 9.
// On first use of a channel, send "All Sound Off" to prevent
// stuck notes (the "D_DDTBLU disease" fix).
//

static int MUS_GetMIDIChannel(int mus_channel)
{
    if (mus_channel == MUS_PERCUSSION_CHAN)
    {
        return MIDI_PERCUSSION_CHAN;
    }

    if (MUS_ChannelMap[mus_channel] == -1)
    {
        MUS_ChannelMap[mus_channel] = MUS_AllocMIDIChannel();
        /* All Sound Off on first use of this channel */
        MUS_Funcs->ControlChange(MUS_ChannelMap[mus_channel], 0x7B, 0);
    }

    return MUS_ChannelMap[mus_channel];
}

//
// Helper: read a variable-length delta time from the score.
// Returns the number of ticks.
//

static unsigned long MUS_ReadDeltaTime(void)
{
    unsigned long timedelay = 0;
    byte c;

    for (;;)
    {
        c = *MUS_ScorePos++;
        timedelay = timedelay * 128 + (c & 0x7F);
        if ((c & 0x80) == 0)
        {
            break;
        }
    }

    return timedelay;
}

//
// Helper: process a single block of MUS events (no time delay).
// Events in a block share the same timestamp.
// The high bit (0x80) of an event descriptor signals end of block.
//
// Returns non-zero if score ended, 0 otherwise.
//

static int MUS_ProcessEventBlock(void)
{
    byte descriptor;
    int channel;
    int mus_channel;
    byte event_type;

    for (;;)
    {
        if (MUS_ScorePos >= MUS_ScoreStart + MUS_ScoreLength)
        {
            /* Premature end of score */
            return TRUE;
        }

        descriptor = *MUS_ScorePos++;
        mus_channel = descriptor & 0x0F;
        event_type = (descriptor >> 4) & 0x07;

        channel = MUS_GetMIDIChannel(mus_channel);

        switch (event_type)
        {
        case MUS_RELEASE_KEY >> 4:
            {
                byte key = *MUS_ScorePos++;
                MUS_Funcs->NoteOff(channel, key, 0);
                break;
            }

        case MUS_PRESS_KEY >> 4:
            {
                byte key = *MUS_ScorePos++;
                byte velocity;

                /* Key with high bit set means velocity follows */
                if (key & 0x80)
                {
                    key &= 0x7F;
                    velocity = *MUS_ScorePos++;
                    MUS_ChannelVelocity[mus_channel] = velocity;
                }
                else
                {
                    velocity = MUS_ChannelVelocity[mus_channel];
                }

                MUS_Funcs->NoteOn(channel, key, velocity);
                break;
            }

        case MUS_PITCH_WHEEL >> 4:
            {
                byte raw = *MUS_ScorePos++;
                /* MUS pitch is 0-127, MIDI is 0-16383 centered at 8192.
                   Map: raw -> (raw - 64) * 128  gives range -8192..8191 */
                short wheel = (short)((raw - 64) * 128);
                byte lsb = wheel & 0x7F;
                byte msb = (wheel >> 7) & 0x7F;
                /* Fix sign for 7-bit values */
                if (lsb & 0x80)
                {
                    lsb &= 0x7F;
                    msb++;
                }
                MUS_Funcs->PitchBend(channel, lsb, msb);
                break;
            }

        case MUS_SYSTEM_EVENT >> 4:
            {
                byte ctrl = *MUS_ScorePos++;
                if (ctrl >= 10 && ctrl <= 14)
                {
                    MUS_Funcs->ControlChange(channel,
                                             mus_controller_map[ctrl], 0);
                }
                break;
            }

        case MUS_CTRL_CHANGE >> 4:
            {
                byte ctrl = *MUS_ScorePos++;
                byte value = *MUS_ScorePos++;

                if (ctrl == 0)
                {
                    /* Program change */
                    MUS_Funcs->ProgramChange(channel, value & 0x7F);
                }
                else if (ctrl >= 1 && ctrl <= 9)
                {
                    if (value & 0x80)
                    {
                        value = 0x7F;
                    }
                    MUS_Funcs->ControlChange(channel,
                                             mus_controller_map[ctrl],
                                             value);
                }
                break;
            }

        case MUS_SCORE_END >> 4:
            /* End of score */
            return TRUE;

        default:
            /* Unknown event type - skip */
            break;
        }

        /* High bit set means end of event block (time delay follows) */
        if (descriptor & 0x80)
        {
            break;
        }
    }

    return FALSE;
}

//
// Service routine: called at 70 Hz to process MUS events.
//

static void MUS_ServiceRoutine(task *Task)
{
    (void)Task;

    if (!MUS_SongActive)
    {
        return;
    }

    /* Process events until we have a delay or the score ends */
    while (MUS_Delay == 0)
    {
        int scoreEnded = MUS_ProcessEventBlock();

        if (scoreEnded)
        {
            /* Score is done */
            if (MUS_Loop)
            {
                /* Restart from the beginning */
                MUS_ScorePos = MUS_ScoreStart;
                SetDWords(MUS_ChannelMap, -1, MUS_NUM_CHANNELS);
                SetBytes(MUS_ChannelVelocity, 127, MUS_NUM_CHANNELS);
                MUS_Delay = 0;
            }
            else
            {
                MUS_SongActive = FALSE;
                return;
            }
        }
        else
        {
            /* Read the time delay for the next event block */
            unsigned long delta;
            if (MUS_ScorePos >= MUS_ScoreStart + MUS_ScoreLength)
            {
                /* Premature end */
                if (MUS_Loop)
                {
                    MUS_ScorePos = MUS_ScoreStart;
                    SetDWords(MUS_ChannelMap, -1, MUS_NUM_CHANNELS);
                    SetBytes(MUS_ChannelVelocity, 127, MUS_NUM_CHANNELS);
                }
                else
                {
                    MUS_SongActive = FALSE;
                    return;
                }
            }
            else
            {
                delta = MUS_ReadDeltaTime();
                MUS_Delay = delta;
            }
        }
    }

    MUS_Delay--;
}

//
// Send all notes off on all MIDI channels.
//

static void MUS_AllNotesOff(void)
{
    int i;
    for (i = 0; i < MUS_NUM_CHANNELS; i++)
    {
        if (MUS_ChannelMap[i] >= 0)
        {
            MUS_Funcs->ControlChange(MUS_ChannelMap[i], 0x40, 0);
            MUS_Funcs->ControlChange(MUS_ChannelMap[i], 0x7B, 0);
            MUS_Funcs->ControlChange(MUS_ChannelMap[i], 0x78, 0);
        }
    }
}

//
// Public API
//

int MUS_InitPlayer(void *data)
{
    musheader_t header;

    if (data == NULL)
    {
        return -1;
    }

    /* Stop any currently playing song first */
    if (MUS_SongLoaded)
    {
        MUS_Stop();
    }

    /* Copy header from data */
    CopyBytes(data, &header, sizeof(musheader_t));

    /* Validate MUS magic: "MUS" + 0x1A */
    if (header.id[0] != 'M' || header.id[1] != 'U' ||
        header.id[2] != 'S' || header.id[3] != 0x1A)
    {
        return -1;
    }

    /* Score data starts at header.scorestart bytes from start of data */
    MUS_ScoreData = (byte *)data;
    MUS_ScoreStart = MUS_ScoreData + header.scorestart;
    MUS_ScoreLength = header.scorelength;

    /* Validate score boundaries */
    if (MUS_ScoreStart + MUS_ScoreLength > MUS_ScoreData +
        (header.scorestart + header.scorelength))
    {
        return -1;
    }

    MUS_ScorePos = MUS_ScoreStart;

    MUS_SongLoaded = TRUE;

    return 0;
}

void MUS_Play(void)
{
    if (!MUS_SongLoaded)
    {
        return;
    }

    /* If a task is already running, just resume it */
    if (MUS_PlayRoutine != NULL)
    {
        MUS_SongActive = TRUE;
        return;
    }

    /* Get midi function pointers from the MIDI subsystem */
    MUS_Funcs = MIDI_GetMidiFuncs();
    if (MUS_Funcs == NULL)
    {
        return;
    }

    /* Reset channel state */
    SetDWords(MUS_ChannelMap, -1, MUS_NUM_CHANNELS);
    SetBytes(MUS_ChannelVelocity, 127, MUS_NUM_CHANNELS);

    /* Reset score position */
    MUS_ScorePos = MUS_ScoreStart;
    MUS_Delay = 0;

    /* All notes off before starting */
    MUS_AllNotesOff();

    /* Schedule playback task at 70 Hz */
    MUS_PlayRoutine = TS_ScheduleTask(MUS_ServiceRoutine,
                                      MUS_TICK_RATE, 100, NULL);
    TS_Dispatch();

    MUS_SongActive = TRUE;
}

void MUS_Pause(void)
{
    if (MUS_SongLoaded && MUS_SongActive)
    {
        MUS_SongActive = FALSE;
        MUS_AllNotesOff();
    }
}

void MUS_Stop(void)
{
    if (MUS_PlayRoutine != NULL)
    {
        TS_Terminate(MUS_PlayRoutine);
        MUS_PlayRoutine = NULL;
    }

    MUS_SongActive = FALSE;
    MUS_SongLoaded = FALSE;
    MUS_ScoreData = NULL;
    MUS_ScoreStart = NULL;
    MUS_ScorePos = NULL;
    MUS_Delay = 0;

    SetDWords(MUS_ChannelMap, -1, MUS_NUM_CHANNELS);
    SetBytes(MUS_ChannelVelocity, 127, MUS_NUM_CHANNELS);
}

int MUS_IsPlaying(void)
{
    return MUS_SongActive;
}

void MUS_PlayerSetLoop(int loopflag)
{
    MUS_Loop = loopflag ? TRUE : FALSE;
}
