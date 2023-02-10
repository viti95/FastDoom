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
#include "ns_cms.h"
#include "ns_scape.h"
#include "ns_llm.h"
#include "ns_user.h"
#include "options.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

int MUSIC_SoundDevice = -1;

static midifuncs MUSIC_MidiFunctions;


int MUSIC_InitAWE32(midifuncs *Funcs);
int MUSIC_InitFM(int card, midifuncs *Funcs, int Address);
int MUSIC_InitMidi(int card, midifuncs *Funcs, int Address);
int MUSIC_InitGUS(midifuncs *Funcs);
int MUSIC_InitCMS(midifuncs *Funcs, int Address);

/*---------------------------------------------------------------------
   Function: MUSIC_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int MUSIC_Init(int SoundCard, int Address)
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
    case OPL2LPT:
    case OPL3LPT:
        status = MUSIC_InitFM(SoundCard, &MUSIC_MidiFunctions, Address);
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

    case CMS:
        status = MUSIC_InitCMS(&MUSIC_MidiFunctions, Address);
        break;

    case AudioCD:
    case FileWAV:
        status = MUSIC_Ok;
        break;

    case SoundSource:
    case TandySoundSource:
    case PC:
    case PC1bit:
    case PCPWM:
    case LPTDAC:
    case SoundBlasterDirect:
    case AdlibFX:
    default:
        status = MUSIC_Error;
        break;
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
    case OPL2LPT:
    case OPL3LPT:
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
    case CMS:
	CMS_MIDI_Shutdown();
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
    case OPL2LPT:
    case OPL3LPT:
    case ProAudioSpectrum:
    case SoundMan16:
    case GenMidi:
    case SoundCanvas:
    case WaveBlaster:
    case SoundScape:
    case Awe32:
    case UltraSound:
    case CMS:
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

int MUSIC_InitFM(int card, midifuncs *Funcs, int Address)
{
    int status;
    int passtatus;

    status = MIDI_Ok;

    if (card != OPL2LPT && card != OPL3LPT)
    {
        if (!AL_DetectFM())
        {
            return (MUSIC_Error);
        }
    }
    
    // Init the fm routines
    AL_Init(card, Address);

    Funcs->NoteOff = AL_NoteOff;
    Funcs->NoteOn = AL_NoteOn;
    Funcs->PolyAftertouch = NULL;
    Funcs->ControlChange = AL_ControlChange;
    Funcs->ProgramChange = AL_ProgramChange;
    Funcs->ChannelAftertouch = NULL;
    Funcs->PitchBend = AL_SetPitchBend;
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
    case OPL2LPT:
    case OPL3LPT:
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
    Funcs->SetVolume = GUSMIDI_SetVolume;
    Funcs->GetVolume = GUSMIDI_GetVolume;

    MIDI_SetMidiFuncs(Funcs);

    return (status);
}

int MUSIC_InitCMS(
    midifuncs *Funcs,
    int Address)

{
    int status;

    status = MUSIC_Ok;

    if (CMS_MIDI_Init(Address) != CMS_Ok)
    {
        return (MUSIC_Error);
    }

    Funcs->NoteOff = CMS_NoteOff;
    Funcs->NoteOn = CMS_NoteOn;
    Funcs->PolyAftertouch = NULL;
    Funcs->ControlChange = CMS_ControlChange;
    Funcs->ProgramChange = CMS_ProgramChange;
    Funcs->ChannelAftertouch = NULL;
    Funcs->PitchBend = CMS_PitchBend;
    Funcs->SetVolume = NULL;
    Funcs->GetVolume = NULL;

    MIDI_SetMidiFuncs(Funcs);

    return (status);
}


