#ifndef __SNDCARDS_H
#define __SNDCARDS_H

#define ASS_VERSION_STRING "1.1"

typedef enum
{
   SoundBlaster,
   ProAudioSpectrum,
   SoundMan16,
   Adlib,
   GenMidi,
   Awe32,
   SoundScape,
   UltraSound,
   SoundSource,
   Tandy3Voice,
   PC,
   PC1bit,
   LPTDAC,
   SoundBlasterDirect,
   PCPWM,
   CMS,
   OPL2LPT,
   OPL3LPT,
   AudioCD,
   FileWAV,
   NumSoundCards
} soundcardnames;

#endif
