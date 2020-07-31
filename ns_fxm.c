#include <stdio.h>
#include <stdlib.h>
#include "ns_cards.h"
#include "ns_multi.h"
#include "ns_sb.h"
#include "ns_pas16.h"
#include "ns_scape.h"
#include "ns_gusau.h"
#include "ns_dsney.h"
#include "ns_llm.h"
#include "ns_user.h"
#include "ns_fxm.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

static unsigned FX_MixRate;

int FX_SoundDevice = -1;
int FX_ErrorCode = FX_Ok;
int FX_Installed = FALSE;

void TextMode(void);
#pragma aux TextMode =  \
    "mov    ax, 0003h", \
            "int    10h" modify[ax];

#define FX_SetErrorCode(status) \
    FX_ErrorCode = (status);

/*---------------------------------------------------------------------
   Function: FX_SetupCard

   Sets the configuration of a sound device.
---------------------------------------------------------------------*/

int FX_SetupCard(
    int SoundCard,
    fx_device *device)

{
    int status;
    int DeviceStatus;

    FX_SoundDevice = SoundCard;

    status = FX_Ok;
    FX_SetErrorCode(FX_Ok);

    switch (SoundCard)
    {
    case SoundBlaster:
    case Awe32:
        DeviceStatus = BLASTER_Init();
        if (DeviceStatus != BLASTER_Ok)
        {
            FX_SetErrorCode(FX_SoundCardError);
            status = FX_Error;
            break;
        }

        device->MaxVoices = 32;
        BLASTER_GetCardInfo(&device->MaxSampleBits, &device->MaxChannels);
        break;

    case ProAudioSpectrum:
    case SoundMan16:
        DeviceStatus = PAS_Init();
        if (DeviceStatus != PAS_Ok)
        {
            FX_SetErrorCode(FX_SoundCardError);
            status = FX_Error;
            break;
        }

        device->MaxVoices = 32;
        PAS_GetCardInfo(&device->MaxSampleBits, &device->MaxChannels);
        break;

    case GenMidi:
    case SoundCanvas:
    case WaveBlaster:
        device->MaxVoices = 0;
        device->MaxSampleBits = 0;
        device->MaxChannels = 0;
        break;

    case SoundScape:
        device->MaxVoices = 32;
        DeviceStatus = SOUNDSCAPE_GetCardInfo(&device->MaxSampleBits,
                                              &device->MaxChannels);
        if (DeviceStatus != SOUNDSCAPE_Ok)
        {
            FX_SetErrorCode(FX_SoundCardError);
            status = FX_Error;
        }
        break;

    case UltraSound:
        if (GUSWAVE_Init(8) != GUSWAVE_Ok)
        {
            FX_SetErrorCode(FX_SoundCardError);
            status = FX_Error;
            break;
        }

        device->MaxVoices = 8;
        device->MaxSampleBits = 0;
        device->MaxChannels = 0;
        break;

    case SoundSource:
    case TandySoundSource:
        DeviceStatus = SS_Init(SoundCard);
        if (DeviceStatus != SS_Ok)
        {
            FX_SetErrorCode(FX_SoundCardError);
            status = FX_Error;
            break;
        }
        SS_Shutdown();
        device->MaxVoices = 32;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;

    default:
        FX_SetErrorCode(FX_InvalidCard);
        status = FX_Error;
    }

    return (status);
}

/*---------------------------------------------------------------------
   Function: FX_GetBlasterSettings

   Returns the current BLASTER environment variable settings.
---------------------------------------------------------------------*/

int FX_GetBlasterSettings(
    fx_blaster_config *blaster)

{
    int status;
    BLASTER_CONFIG Blaster;

    FX_SetErrorCode(FX_Ok);

    status = BLASTER_GetEnv(&Blaster);
    if (status != BLASTER_Ok)
    {
        FX_SetErrorCode(FX_BlasterError);
        return (FX_Error);
    }

    blaster->Type = Blaster.Type;
    blaster->Address = Blaster.Address;
    blaster->Interrupt = Blaster.Interrupt;
    blaster->Dma8 = Blaster.Dma8;
    blaster->Dma16 = Blaster.Dma16;
    blaster->Midi = Blaster.Midi;
    blaster->Emu = Blaster.Emu;

    return (FX_Ok);
}

/*---------------------------------------------------------------------
   Function: FX_SetupSoundBlaster

   Handles manual setup of the Sound Blaster information.
---------------------------------------------------------------------*/

int FX_SetupSoundBlaster(
    fx_blaster_config blaster,
    int *MaxVoices,
    int *MaxSampleBits,
    int *MaxChannels)

{
    int DeviceStatus;
    BLASTER_CONFIG Blaster;

    FX_SetErrorCode(FX_Ok);

    FX_SoundDevice = SoundBlaster;

    Blaster.Type = blaster.Type;
    Blaster.Address = blaster.Address;
    Blaster.Interrupt = blaster.Interrupt;
    Blaster.Dma8 = blaster.Dma8;
    Blaster.Dma16 = blaster.Dma16;
    Blaster.Midi = blaster.Midi;
    Blaster.Emu = blaster.Emu;

    BLASTER_SetCardSettings(Blaster);

    DeviceStatus = BLASTER_Init();
    if (DeviceStatus != BLASTER_Ok)
    {
        FX_SetErrorCode(FX_SoundCardError);
        return (FX_Error);
    }

    *MaxVoices = 8;
    BLASTER_GetCardInfo(MaxSampleBits, MaxChannels);

    return (FX_Ok);
}

/*---------------------------------------------------------------------
   Function: FX_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int FX_Init(
    int SoundCard,
    int numvoices,
    int numchannels,
    int samplebits,
    unsigned int mixrate)

{
    int status;
    int devicestatus;

    if (FX_Installed)
    {
        FX_Shutdown();
    }

    status = LL_LockMemory();
    if (status != LL_Ok)
    {
        FX_SetErrorCode(FX_DPMI_Error);
        return (FX_Error);
    }

    FX_MixRate = mixrate;

    status = FX_Ok;
    FX_SoundDevice = SoundCard;
    switch (SoundCard)
    {
    case SoundBlaster:
    case Awe32:
    case ProAudioSpectrum:
    case SoundMan16:
    case SoundScape:
    case SoundSource:
    case TandySoundSource:
    case UltraSound:
        devicestatus = MV_Init(SoundCard, FX_MixRate, numvoices,
                               numchannels, samplebits);
        if (devicestatus != MV_Ok)
        {
            FX_SetErrorCode(FX_MultiVocError);
            status = FX_Error;
        }
        break;

    default:
        FX_SetErrorCode(FX_InvalidCard);
        status = FX_Error;
    }

    if (status != FX_Ok)
    {
        LL_UnlockMemory();
    }
    else
    {
        FX_Installed = TRUE;
    }

    return (status);
}

/*---------------------------------------------------------------------
   Function: FX_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

int FX_Shutdown(
    void)

{
    int status;

    if (!FX_Installed)
    {
        return (FX_Ok);
    }

    status = FX_Ok;
    switch (FX_SoundDevice)
    {
    case SoundBlaster:
    case Awe32:
    case ProAudioSpectrum:
    case SoundMan16:
    case SoundScape:
    case SoundSource:
    case TandySoundSource:
    case UltraSound:
        status = MV_Shutdown();
        if (status != MV_Ok)
        {
            FX_SetErrorCode(FX_MultiVocError);
            status = FX_Error;
        }
        break;

    default:
        FX_SetErrorCode(FX_InvalidCard);
        status = FX_Error;
    }

    FX_Installed = FALSE;
    LL_UnlockMemory();

    return (status);
}

/*---------------------------------------------------------------------
   Function: FX_SetCallback

   Sets the function to call when a voice is done.
---------------------------------------------------------------------*/

int FX_SetCallBack(
    void (*function)(unsigned long))

{
    int status;

    status = FX_Ok;

    switch (FX_SoundDevice)
    {
    case SoundBlaster:
    case Awe32:
    case ProAudioSpectrum:
    case SoundMan16:
    case SoundScape:
    case SoundSource:
    case TandySoundSource:
    case UltraSound:
        MV_SetCallBack(function);
        break;

    default:
        FX_SetErrorCode(FX_InvalidCard);
        status = FX_Error;
    }

    return (status);
}

/*---------------------------------------------------------------------
   Function: FX_SetVolume

   Sets the volume of the current sound device.
---------------------------------------------------------------------*/

void FX_SetVolume(
    int volume)

{
    int status;

    switch (FX_SoundDevice)
    {
    case SoundBlaster:
    case Awe32:
        if (BLASTER_CardHasMixer())
        {
            BLASTER_SetVoiceVolume(volume);
        }
        else
        {
            MV_SetVolume(volume);
        }
        break;

    case ProAudioSpectrum:
    case SoundMan16:
        status = PAS_SetPCMVolume(volume);
        if (status != PAS_Ok)
        {
            MV_SetVolume(volume);
        }
        break;

    case GenMidi:
    case SoundCanvas:
    case WaveBlaster:
        break;

    case SoundScape:
        MV_SetVolume(volume);
        break;

    case UltraSound:
        GUSWAVE_SetVolume(volume);
        break;

    case SoundSource:
    case TandySoundSource:
        MV_SetVolume(volume);
        break;
    }
}

/*---------------------------------------------------------------------
   Function: FX_SetReverseStereo

   Set the orientation of the left and right channels.
---------------------------------------------------------------------*/

void FX_SetReverseStereo(
    int setting)

{
    MV_SetReverseStereo(setting);
}

/*---------------------------------------------------------------------
   Function: FX_GetReverseStereo

   Returns the orientation of the left and right channels.
---------------------------------------------------------------------*/

int FX_GetReverseStereo(
    void)

{
    return MV_GetReverseStereo();
}

/*---------------------------------------------------------------------
   Function: FX_VoiceAvailable

   Checks if a voice can be play at the specified priority.
---------------------------------------------------------------------*/

int FX_VoiceAvailable(
    int priority)

{
    return MV_VoiceAvailable(priority);
}

/*---------------------------------------------------------------------
   Function: FX_SetPan

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int FX_SetPan(
    int handle,
    int vol,
    int left,
    int right)

{
    int status;

    status = MV_SetPan(handle, vol, left, right);
    if (status == MV_Error)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return (status);
}

/*---------------------------------------------------------------------
   Function: FX_SetPitch

   Sets the pitch of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int FX_SetPitch(
    int handle,
    int pitchoffset)

{
    int status;

    status = MV_SetPitch(handle, pitchoffset);
    if (status == MV_Error)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return (status);
}

/*---------------------------------------------------------------------
   Function: FX_SetFrequency

   Sets the frequency of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int FX_SetFrequency(
    int handle,
    int frequency)

{
    int status;

    status = MV_SetFrequency(handle, frequency);
    if (status == MV_Error)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Warning;
    }

    return (status);
}

/*---------------------------------------------------------------------
   Function: FX_PlayRaw

   Begin playback of raw sound data with the given volume and priority.
---------------------------------------------------------------------*/

int FX_PlayRaw(
    char *ptr,
    unsigned long length,
    unsigned rate,
    int pitchoffset,
    int vol,
    int left,
    int right,
    int priority,
    unsigned long callbackval)

{
    int handle;

    handle = MV_PlayRaw(ptr, length, rate, pitchoffset,
                        vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return (handle);
}

/*---------------------------------------------------------------------
   Function: FX_SoundActive

   Tests if the specified sound is currently playing.
---------------------------------------------------------------------*/

int FX_SoundActive(
    int handle)

{
    return (MV_VoicePlaying(handle));
}

/*---------------------------------------------------------------------
   Function: FX_SoundsPlaying

   Reports the number of voices playing.
---------------------------------------------------------------------*/

int FX_SoundsPlaying(
    void)

{
    return (MV_VoicesPlaying());
}

/*---------------------------------------------------------------------
   Function: FX_StopSound

   Halts playback of a specific voice
---------------------------------------------------------------------*/

int FX_StopSound(
    int handle)

{
    int status;

    status = MV_Kill(handle);
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        return (FX_Warning);
    }

    return (FX_Ok);
}

/*---------------------------------------------------------------------
   Function: FX_StopAllSounds

   Halts playback of all sounds.
---------------------------------------------------------------------*/

int FX_StopAllSounds(
    void)

{
    int status;

    status = MV_KillAllVoices();
    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        return (FX_Warning);
    }

    return (FX_Ok);
}
