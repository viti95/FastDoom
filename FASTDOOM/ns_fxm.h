#ifndef __FX_MAN_H
#define __FX_MAN_H

#include "ns_cards.h"

typedef struct
{
       int MaxVoices;
       int MaxSampleBits;
       int MaxChannels;
} fx_device;

#define MonoFx 1
#define StereoFx 2

typedef struct
{
       unsigned long Address;
       unsigned long Type;
       unsigned long Interrupt;
       unsigned long Dma8;
       unsigned long Dma16;
       unsigned long Midi;
       unsigned long Emu;
} fx_blaster_config;

enum FX_ERRORS
{
       FX_Warning = -2,
       FX_Error = -1,
       FX_Ok = 0,
       FX_ASSVersion,
       FX_BlasterError,
       FX_SoundCardError,
       FX_InvalidCard,
       FX_MultiVocError,
       FX_DPMI_Error
};

enum fx_BLASTER_Types
{
       fx_SB = 1,
       fx_SBPro = 2,
       fx_SB20 = 3,
       fx_SBPro2 = 4,
       fx_SB16 = 6
};

extern unsigned int FX_MixRate;

int FX_SetupCard(int SoundCard, fx_device *device, int port);
int FX_GetBlasterSettings(fx_blaster_config *blaster);
int FX_SetupSoundBlaster(fx_blaster_config blaster);
int FX_Init(int SoundCard, int numvoices, int numchannels, int samplebits, unsigned int mixrate);
int FX_Shutdown(void);
void FX_SetVolume(int volume);

#endif
