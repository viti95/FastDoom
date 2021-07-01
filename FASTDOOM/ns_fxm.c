#include <stdlib.h>
#include "ns_cards.h"
#include "ns_multi.h"
#include "ns_sb.h"
#include "ns_pas16.h"
#include "ns_scape.h"
#include "ns_gusau.h"
#include "ns_dsney.h"
#include "ns_speak.h"
#include "ns_lpt.h"
#include "ns_llm.h"
#include "ns_user.h"
#include "ns_fxm.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

static unsigned FX_MixRate;

int FX_SoundDevice = -1;
int FX_Installed = FALSE;

/*---------------------------------------------------------------------
   Function: FX_SetupCard

   Sets the configuration of a sound device.
---------------------------------------------------------------------*/

int FX_SetupCard(int SoundCard, fx_device *device)
{
    int status;
    int DeviceStatus;

    FX_SoundDevice = SoundCard;

    status = FX_Ok;

    switch (SoundCard)
    {
    case SoundBlaster:
    case Awe32:
        DeviceStatus = BLASTER_Init();
        if (DeviceStatus != BLASTER_Ok)
        {
            status = FX_Error;
            break;
        }

        device->MaxVoices = 8;
        BLASTER_GetCardInfo(&device->MaxSampleBits, &device->MaxChannels);
        break;

    case ProAudioSpectrum:
    case SoundMan16:
        DeviceStatus = PAS_Init();
        if (DeviceStatus != PAS_Ok)
        {
            status = FX_Error;
            break;
        }

        device->MaxVoices = 8;
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
        device->MaxVoices = 8;
        DeviceStatus = SOUNDSCAPE_GetCardInfo(&device->MaxSampleBits,
                                              &device->MaxChannels);
        if (DeviceStatus != SOUNDSCAPE_Ok)
        {
            status = FX_Error;
        }
        break;

    case UltraSound:
        if (GUSWAVE_Init(8) != GUSWAVE_Ok)
        {
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
            status = FX_Error;
            break;
        }
        SS_Shutdown();
        device->MaxVoices = 8;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;
    case PC1bit:
        DeviceStatus = PCSpeaker_Init(SoundCard);
        if (DeviceStatus != PCSpeaker_Ok)
        {
            status = FX_Error;
            break;
        }
        device->MaxVoices = 8;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;
    case LPTDAC:
        DeviceStatus = LPT_Init(SoundCard);
        if (DeviceStatus != LPT_Ok)
        {
            status = FX_Error;
            break;
        }
        device->MaxVoices = 8;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;
    default:
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

    status = BLASTER_GetEnv(&Blaster);
    if (status != BLASTER_Ok)
    {
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
    case PC1bit:
    case LPTDAC:
        devicestatus = MV_Init(SoundCard, FX_MixRate, numvoices, numchannels, samplebits);
        if (devicestatus != MV_Ok)
        {
            status = FX_Error;
        }
        break;

    default:
        status = FX_Error;
    }

    if (status != FX_Ok)
    {
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
    case PC1bit:
    case LPTDAC:
        status = MV_Shutdown();
        if (status != MV_Ok)
        {
            status = FX_Error;
        }
        break;

    default:
        status = FX_Error;
    }

    FX_Installed = FALSE;

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
    case PC1bit:
    case LPTDAC:
        MV_SetCallBack(function);
        break;

    default:
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
   Function: FX_PlayRaw

   Begin playback of raw sound data with the given volume and priority.
---------------------------------------------------------------------*/

int FX_PlayRaw(
    unsigned char *ptr,
    unsigned long length,
    unsigned rate,
    int vol,
    int left,
    int right,
    int priority,
    unsigned long callbackval)

{
    int handle;

    handle = MV_PlayRaw(ptr, length, rate, vol, left, right, priority, callbackval);
    if (handle < MV_Ok)
    {
        handle = FX_Warning;
    }

    return (handle);
}
