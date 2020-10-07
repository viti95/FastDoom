#include <stdio.h>
#include <stdlib.h>
#include "ns_task.h"
#include "ns_cards.h"
#include "ns_music.h"
#include "ns_midi.h"
#include "ns_sbmus.h"
#include "ns_pas16.h"
#include "ns_sb.h"
#include "ns_gusmi.h"
#include "ns_mp401.h"
#include "ns_awe32.h"
#include "ns_scape.h"
#include "ns_llm.h"
#include "ns_user.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

void TextMode(void);
#pragma aux TextMode =  \
    "mov    ax, 0003h", \
            "int    10h" modify[ax];

int MUSIC_SoundDevice = -1;

static midifuncs MUSIC_MidiFunctions;


int MUSIC_InitAWE32(midifuncs *Funcs);
int MUSIC_InitFM(int card, midifuncs *Funcs);
int MUSIC_InitMidi(int card, midifuncs *Funcs, int Address);
int MUSIC_InitGUS(midifuncs *Funcs);

/*---------------------------------------------------------------------
   Function: MUSIC_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int MUSIC_Init(
    int SoundCard,
    int Address)

{
    int i;
    int status;

    for (i = 0; i < 128; i++)
    {
        MIDI_PatchMap[i] = i;
    }

    status = MUSIC_Ok;
    MUSIC_SoundDevice = SoundCard;

    switch (SoundCard)
    {
    case SoundBlaster:
    case Adlib:
    case ProAudioSpectrum:
    case SoundMan16:
        status = MUSIC_InitFM(SoundCard, &MUSIC_MidiFunctions);
        break;

    case GenMidi:
    case SoundCanvas:
    case WaveBlaster:
    case SoundScape:
        status = MUSIC_InitMidi(SoundCard, &MUSIC_MidiFunctions, Address);
        break;

    case Awe32:
        status = MUSIC_InitAWE32(&MUSIC_MidiFunctions);
        break;

    case UltraSound:
        status = MUSIC_InitGUS(&MUSIC_MidiFunctions);
        break;

    case SoundSource:
    case TandySoundSource:
    case PC:
    default:
        status = MUSIC_Error;
    }

    return (status);
}

/*---------------------------------------------------------------------
   Function: MUSIC_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

int MUSIC_Shutdown(
    void)

{
    int status;

    status = MUSIC_Ok;

    MIDI_StopSong();

    switch (MUSIC_SoundDevice)
    {
    case Adlib:
        AL_Shutdown();
        break;

    case SoundBlaster:
        AL_Shutdown();
        BLASTER_RestoreMidiVolume();
        break;

    case GenMidi:
    case SoundCanvas:
    case SoundScape:
        MPU_Reset();
        break;

    case WaveBlaster:
        BLASTER_ShutdownWaveBlaster();
        MPU_Reset();
        BLASTER_RestoreMidiVolume();
        break;

    case Awe32:
        AWE32_Shutdown();
        BLASTER_RestoreMidiVolume();
        break;

    case ProAudioSpectrum:
    case SoundMan16:
        AL_Shutdown();
        PAS_RestoreMusicVolume();
        break;

    case UltraSound:
        GUSMIDI_Shutdown();
        break;
    }

    return (status);
}

/*---------------------------------------------------------------------
   Function: MUSIC_SetVolume

   Sets the volume of music playback.
---------------------------------------------------------------------*/

void MUSIC_SetVolume(
    int volume)

{
    volume = max(0, volume);
    volume = min(volume, 255);

    if (MUSIC_SoundDevice != -1)
    {
        MIDI_SetVolume(volume);
    }
}

/*---------------------------------------------------------------------
   Function: MUSIC_SetMidiChannelVolume

   Sets the volume of music playback on the specified MIDI channel.
---------------------------------------------------------------------*/

void MUSIC_SetMidiChannelVolume(
    int channel,
    int volume)

{
    MIDI_SetUserChannelVolume(channel, volume);
}

/*---------------------------------------------------------------------
   Function: MUSIC_ResetMidiChannelVolumes

   Sets the volume of music playback on all MIDI channels to full volume.
---------------------------------------------------------------------*/

void MUSIC_ResetMidiChannelVolumes(
    void)

{
    MIDI_ResetUserChannelVolume();
}

/*---------------------------------------------------------------------
   Function: MUSIC_GetVolume

   Returns the volume of music playback.
---------------------------------------------------------------------*/

int MUSIC_GetVolume(
    void)

{
    if (MUSIC_SoundDevice == -1)
    {
        return (0);
    }
    return (MIDI_GetVolume());
}

/*---------------------------------------------------------------------
   Function: MUSIC_SetLoopFlag

   Set whether the music will loop or end when it reaches the end of
   the song.
---------------------------------------------------------------------*/

void MUSIC_SetLoopFlag(
    int loopflag)

{
    MIDI_SetLoopFlag(loopflag);
}

/*---------------------------------------------------------------------
   Function: MUSIC_SongPlaying

   Returns whether there is a song playing.
---------------------------------------------------------------------*/

int MUSIC_SongPlaying(
    void)

{
    return (MIDI_SongPlaying());
}

/*---------------------------------------------------------------------
   Function: MUSIC_Continue

   Continues playback of a paused song.
---------------------------------------------------------------------*/

void MUSIC_Continue(
    void)

{
    MIDI_ContinueSong();
}

/*---------------------------------------------------------------------
   Function: MUSIC_Pause

   Pauses playback of a song.
---------------------------------------------------------------------*/

void MUSIC_Pause(
    void)

{
    MIDI_PauseSong();
}

/*---------------------------------------------------------------------
   Function: MUSIC_StopSong

   Stops playback of current song.
---------------------------------------------------------------------*/

int MUSIC_StopSong(
    void)

{
    MIDI_StopSong();
    return (MUSIC_Ok);
}

/*---------------------------------------------------------------------
   Function: MUSIC_PlaySong

   Begins playback of MIDI song.
---------------------------------------------------------------------*/

int MUSIC_PlaySong(
    unsigned char *song,
    int loopflag)

{
    int status;

    switch (MUSIC_SoundDevice)
    {
    case SoundBlaster:
    case Adlib:
    case ProAudioSpectrum:
    case SoundMan16:
    case GenMidi:
    case SoundCanvas:
    case WaveBlaster:
    case SoundScape:
    case Awe32:
    case UltraSound:
        MIDI_StopSong();
        status = MIDI_PlaySong(song, loopflag);
        if (status != MIDI_Ok)
        {
            return (MUSIC_Warning);
        }
        break;

    case SoundSource:
    case PC:
    default:
        return (MUSIC_Warning);
        break;
    }

    return (MUSIC_Ok);
}

int MUSIC_InitAWE32(
    midifuncs *Funcs)

{
    int status;

    status = AWE32_Init();
    if (status != AWE32_Ok)
    {
        return (MUSIC_Error);
    }

    Funcs->NoteOff = AWE32_NoteOff;
    Funcs->NoteOn = AWE32_NoteOn;
    Funcs->PolyAftertouch = AWE32_PolyAftertouch;
    Funcs->ControlChange = AWE32_ControlChange;
    Funcs->ProgramChange = AWE32_ProgramChange;
    Funcs->ChannelAftertouch = AWE32_ChannelAftertouch;
    Funcs->PitchBend = AWE32_PitchBend;
    Funcs->ReleasePatches = NULL;
    Funcs->LoadPatch = NULL;
    Funcs->SetVolume = NULL;
    Funcs->GetVolume = NULL;

    if (BLASTER_CardHasMixer())
    {
        BLASTER_SaveMidiVolume();
        Funcs->SetVolume = BLASTER_SetMidiVolume;
        Funcs->GetVolume = BLASTER_GetMidiVolume;
    }

    status = MUSIC_Ok;
    MIDI_SetMidiFuncs(Funcs);

    return (status);
}

int MUSIC_InitFM(
    int card,
    midifuncs *Funcs)

{
    int status;
    int passtatus;

    status = MIDI_Ok;

    if (!AL_DetectFM())
    {
        return (MUSIC_Error);
    }

    // Init the fm routines
    AL_Init(card);

    Funcs->NoteOff = AL_NoteOff;
    Funcs->NoteOn = AL_NoteOn;
    Funcs->PolyAftertouch = NULL;
    Funcs->ControlChange = AL_ControlChange;
    Funcs->ProgramChange = AL_ProgramChange;
    Funcs->ChannelAftertouch = NULL;
    Funcs->PitchBend = AL_SetPitchBend;
    Funcs->ReleasePatches = NULL;
    Funcs->LoadPatch = NULL;
    Funcs->SetVolume = NULL;
    Funcs->GetVolume = NULL;

    switch (card)
    {
    case SoundBlaster:
        if (BLASTER_CardHasMixer())
        {
            BLASTER_SaveMidiVolume();
            Funcs->SetVolume = BLASTER_SetMidiVolume;
            Funcs->GetVolume = BLASTER_GetMidiVolume;
        }
        else
        {
            Funcs->SetVolume = NULL;
            Funcs->GetVolume = NULL;
        }
        break;

    case Adlib:
        Funcs->SetVolume = NULL;
        Funcs->GetVolume = NULL;
        break;

    case ProAudioSpectrum:
    case SoundMan16:
        Funcs->SetVolume = NULL;
        Funcs->GetVolume = NULL;

        passtatus = PAS_SaveMusicVolume();
        if (passtatus == PAS_Ok)
        {
            Funcs->SetVolume = PAS_SetFMVolume;
            Funcs->GetVolume = PAS_GetFMVolume;
        }
        break;
    }

    MIDI_SetMidiFuncs(Funcs);

    return (status);
}

int MUSIC_InitMidi(
    int card,
    midifuncs *Funcs,
    int Address)

{
    int status;

    status = MUSIC_Ok;

    if ((card == WaveBlaster) || (card == SoundCanvas) ||
        (card == GenMidi))
    {
        // Setup WaveBlaster Daughterboard clone
        // (ie. SoundCanvas DB, TurtleBeach Rio)
        BLASTER_SetupWaveBlaster();
    }

    if (card == SoundScape)
    {
        Address = SOUNDSCAPE_GetMIDIPort();
        if (Address < SOUNDSCAPE_Ok)
        {
            return (MUSIC_Error);
        }
    }

    if (MPU_Init(Address) != MPU_Ok)
    {
        return (MUSIC_Error);
    }

    Funcs->NoteOff = MPU_NoteOff;
    Funcs->NoteOn = MPU_NoteOn;
    Funcs->PolyAftertouch = MPU_PolyAftertouch;
    Funcs->ControlChange = MPU_ControlChange;
    Funcs->ProgramChange = MPU_ProgramChange;
    Funcs->ChannelAftertouch = MPU_ChannelAftertouch;
    Funcs->PitchBend = MPU_PitchBend;
    Funcs->ReleasePatches = NULL;
    Funcs->LoadPatch = NULL;
    Funcs->SetVolume = NULL;
    Funcs->GetVolume = NULL;

    if (card == WaveBlaster)
    {
        if (BLASTER_CardHasMixer())
        {
            BLASTER_SaveMidiVolume();
            Funcs->SetVolume = BLASTER_SetMidiVolume;
            Funcs->GetVolume = BLASTER_GetMidiVolume;
        }
    }

    MIDI_SetMidiFuncs(Funcs);

    return (status);
}

int MUSIC_InitGUS(
    midifuncs *Funcs)

{
    int status;

    status = MUSIC_Ok;

    if (GUSMIDI_Init() != GUS_Ok)
    {
        return (MUSIC_Error);
    }

    Funcs->NoteOff = GUSMIDI_NoteOff;
    Funcs->NoteOn = GUSMIDI_NoteOn;
    Funcs->PolyAftertouch = NULL;
    Funcs->ControlChange = GUSMIDI_ControlChange;
    Funcs->ProgramChange = GUSMIDI_ProgramChange;
    Funcs->ChannelAftertouch = NULL;
    Funcs->PitchBend = GUSMIDI_PitchBend;
    Funcs->ReleasePatches = NULL; //GUSMIDI_ReleasePatches;
    Funcs->LoadPatch = NULL;      //GUSMIDI_LoadPatch;
    Funcs->SetVolume = GUSMIDI_SetVolume;
    Funcs->GetVolume = GUSMIDI_GetVolume;

    MIDI_SetMidiFuncs(Funcs);

    return (status);
}

