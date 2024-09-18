//
// Copyright (C) 1993-1996 Id Software, Inc.
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

#include <stdio.h>

#include "dmx.h"

#include "i_ibm.h"
#include "i_system.h"
#include "s_sound.h"
#include "i_sound.h"
#include "sounds.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#include "doomdef.h"
#include "doomstat.h"

#include "options.h"

//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t *sfx)
{
    if (snd_SfxDevice > snd_PC)
    {
        char namebuf[9] = "DS";
        strcpy(namebuf+2, sfx->name);
        return W_GetNumForName(namebuf);
    }
    else
    {
        char namebuf[9] = "DP";
        strcpy(namebuf+2, sfx->name);
        return W_GetNumForName(namebuf);
    }
}

//
// Sound startup stuff
//

void I_sndArbitrateCards(void)
{

}

//
// I_StartupSound
// Inits all sound stuff
//
void I_StartupSound(void)
{

}
//
// I_ShutdownSound
// Shuts down all sound stuff
//
void I_ShutdownSound(void)
{
    S_PauseMusic();
}
