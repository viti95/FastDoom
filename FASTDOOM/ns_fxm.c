#include <stdlib.h>
#include "ns_cards.h"
#include "ns_multi.h"
#include "ns_sb.h"
#include "ns_pas16.h"
#include "ns_scape.h"
#include "ns_gusau.h"
#include "ns_dsney.h"
#include "ns_speak.h"
#include "ns_pwm.h"
#include "ns_cms.h"
#include "ns_lpt.h"
#include "ns_sbdm.h"
#include "ns_llm.h"
#include "ns_user.h"
#include "ns_fxm.h"
#include "ns_adbfx.h"
#include "ns_tandy.h"
#include "options.h"
#include "doomstat.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

unsigned int FX_MixRate;

int FX_SoundDevice = -1;
int FX_Installed = FALSE;

/*---------------------------------------------------------------------
   Function: FX_SetupCard

   Sets the configuration of a sound device.
---------------------------------------------------------------------*/

int FX_SetupCard(int SoundCard, fx_device *device, int port)
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
        if (DeviceStatus != BLASTER_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }

        device->MaxVoices = 9;
        BLASTER_GetCardInfo(&device->MaxSampleBits, &device->MaxChannels);
        break;

    case ProAudioSpectrum:
    case SoundMan16:
        DeviceStatus = PAS_Init();
        if (DeviceStatus != PAS_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }

        device->MaxVoices = 9;
        PAS_GetCardInfo(&device->MaxSampleBits, &device->MaxChannels);
        break;

    case GenMidi:
    case SBMIDI:
    case RS232MIDI:
    case LPTMIDI:
        device->MaxVoices = 0;
        device->MaxSampleBits = 0;
        device->MaxChannels = 0;
        break;

    case SoundScape:
        device->MaxVoices = 9;
        DeviceStatus = SOUNDSCAPE_GetCardInfo(&device->MaxSampleBits,
                                              &device->MaxChannels);
        if (DeviceStatus != SOUNDSCAPE_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
        }
        break;

    case UltraSound:
        if (GUSWAVE_Init() != GUSWAVE_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }

        device->MaxVoices = 9;
        device->MaxSampleBits = 0;
        device->MaxChannels = 0;
        break;

    case SoundSource:
        DeviceStatus = SS_Init(SoundCard, port);
        if (DeviceStatus != SS_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }
        SS_Shutdown();
        device->MaxVoices = 9;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;
    case PC1bit:
        DeviceStatus = PCSpeaker_Init(SoundCard);
        if (DeviceStatus != PCSpeaker_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }
        device->MaxVoices = 9;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;
    case PCPWM:
        DeviceStatus = PCSpeaker_PWM_Init(SoundCard);
        if (DeviceStatus != PCSpeaker_PWM_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }
        device->MaxVoices = 9;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;
    case CMS:
        DeviceStatus = CMS_Init(SoundCard, port);
        if (DeviceStatus != CMS_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }
        device->MaxVoices = 9;
        device->MaxSampleBits = 8;
        device->MaxChannels = 2;
    case LPTDAC:
        DeviceStatus = LPT_Init(SoundCard, port);
        if (DeviceStatus != LPT_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }
        device->MaxVoices = 9;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;
    case SoundBlasterDirect:
        DeviceStatus = SBDM_Init(SoundCard);
        if (DeviceStatus != SBDM_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }
        device->MaxVoices = 9;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;
    case Adlib:
    case OPL2LPT:
    case OPL3LPT:
        DeviceStatus = ADBFX_Init(SoundCard, port);
        if (DeviceStatus != ADBFX_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }
        device->MaxVoices = 9;
        device->MaxSampleBits = 8;
        device->MaxChannels = 1;
        break;
    case Tandy3Voice:
        DeviceStatus = TANDY_Init(SoundCard);
        if (DeviceStatus != TANDY_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
            break;
        }
        device->MaxVoices = 9;
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

int FX_SetupSoundBlaster(fx_blaster_config blaster)
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
    if (DeviceStatus != BLASTER_Ok && !ignoreSoundChecks)
    {
        return (FX_Error);
    }

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
    case Tandy3Voice:
    case UltraSound:
    case PC1bit:
    case PCPWM:
    case CMS:
    case LPTDAC:
    case Adlib:
    case OPL2LPT:
    case OPL3LPT:
    case SoundBlasterDirect:
        devicestatus = MV_Init(SoundCard, FX_MixRate, numvoices, numchannels, samplebits);
        if (devicestatus != MV_Ok && !ignoreSoundChecks)
        {
            status = FX_Error;
        }
        break;

    default:
        status = FX_Error;
    }

    if (status == FX_Ok)
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
    case Tandy3Voice:
    case UltraSound:
    case PC1bit:
    case PCPWM:
    case CMS:
    case LPTDAC:
    case Adlib:
    case OPL2LPT:
    case OPL3LPT:
    case SoundBlasterDirect:
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
   Function: FX_SetVolume

   Sets the volume of the current sound device.
---------------------------------------------------------------------*/

void FX_SetVolume(int volume)
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
    case SBMIDI:
    case RS232MIDI:
    case LPTMIDI:
        break;

    case UltraSound:
        GUSWAVE_SetVolume(volume);
        break;

    case SoundScape:
    case SoundSource:
    case Tandy3Voice:
    case Adlib:
    case OPL2LPT:
    case OPL3LPT:
    case PC1bit:
    case PCPWM:
    case LPTDAC:
    case SoundBlasterDirect:
    case CMS:
        MV_SetVolume(volume);
        break;
    }
}
