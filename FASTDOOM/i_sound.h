//
// Copyright (C) 1993-1996 id Software, Inc.
// Copyright (C) 1993-2008 Raven Software
// Copyright (C) 2016-2017 Alexey Khokholov (Nuke.YKT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  System interface for sound.
//

#ifndef __I_SOUND__
#define __I_SOUND__

#define SND_TICRATE 140      // tic rate for updating sound

typedef enum
{
    snd_none,
    snd_PC,
    snd_Adlib,
    snd_SB,
    snd_PAS,
    snd_GUS,
    snd_MPU,
    snd_AWE,
    snd_ENSONIQ,
    snd_DISNEY,
    snd_TANDY,
    snd_PC1BIT,
    snd_LPTDAC,
    snd_SBDirect,
    snd_PCPWM,
    snd_CMS,
    snd_OPL2LPT,
    snd_OPL3LPT,
    snd_CD,
    snd_WAV,
    snd_SBMIDI,
    snd_RS232MIDI,
    snd_LPTMIDI,
    NUM_SCARDS
} cardenum_t;

extern int snd_Rate;
extern int snd_PCMRate;

#endif
