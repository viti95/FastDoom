//
// Copyright (C) 1993-1996 Id Software, Inc.
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
// DESCRIPTION:  none
//

#include <stdio.h>
#include <stdlib.h>
#include "options.h"
#include "i_system.h"
#include "i_sound.h"
#include "sounds.h"
#include "s_sound.h"

#include "z_zone.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"
#include "p_local.h"

#include "doomstat.h"
#include "dmx.h"
#include "ns_music.h"

#include "p_mobj.h"

#include "std_func.h"

#ifndef MAC
#include "ns_cd.h"
#include "ns_multi.h"
#include "ns_muldf.h"
#endif

// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
extern int snd_MusicDevice;
extern int snd_SfxDevice;
// Config file? Same disclaimer as above.
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;

int cdlooping = 0;
int cdmusicnum = 0;

int wavhandle = -1;
int wavmusicnum = 0;
int wavlooping = 0;
unsigned char *wavfileptr = NULL;

typedef struct
{
    // sound information (if null, channel avail.)
    sfxinfo_t *sfxinfo;

    // origin of sound
    void *origin;

    // handle of the sound being played
    int handle;

} channel_t;

// the set of channels available
static channel_t *channels;

// These are not used, but should be (menu).
// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
static int snd_SfxVolume;

// Maximum volume of music. Useless so far.
static int snd_MusicVolume;

extern int sfxVolume;
extern int musicVolume;

// whether songs are mus_paused
static byte mus_paused;

// music currently being played
static musicinfo_t *mus_playing = 0;

int snd_clipping = S_CLIPPING_DIST;

//
// Internals.
//
int S_getChannel(void *origin, sfxinfo_t *sfxinfo);
byte S_AdjustSoundParams(mobj_t *source, int *vol, int *sep);
void S_StopChannel(int cnum);

void S_SetMusicVolumeCD(int volume)
{

}

void S_SetMusicVolumeWAV(int volume)
{

}

void S_SetMusicVolumeMIDI(int volume)
{

}

void S_SetMusicVolume(int volume)
{

}

void S_StopMusicMIDI(void)
{

}

unsigned char S_MapMusicCD(int musicnum)
{

}

void S_ChangeMusicCD(int musicnum, int looping)
{

}

void S_CheckCD(void)
{

}

void S_ChangeMusicWAV(int musicnum, int looping)
{

}

void S_CheckWAV(void)
{

}

void S_ChangeMusicMIDI(int musicnum, int looping)
{
    
}

void S_ChangeMusic(int musicnum, int looping)
{
    
}

void S_StopChannel(int cnum)
{

}

//
// Changes volume, stereo-separation, and pitch variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
byte S_AdjustSoundParams(mobj_t *source, int *vol, int *sep)
{
    fixed_t approx_dist;
    fixed_t adx;
    fixed_t ady;
    angle_t angle;

    fixed_t optSine;

    // calculate the distance to sound origin
    //  and clip it if necessary
    adx = abs(players_mo->x - source->x);
    ady = abs(players_mo->y - source->y);

    // From _GG1_ p.428. Appox. eucledian distance fast.
    approx_dist = adx + ady - ((adx < ady ? adx : ady) >> 1);

    if (approx_dist > snd_clipping)
    {
        return 1; // Non audible
    }

    // volume calculation
    if (approx_dist < S_CLOSE_DIST)
    {
        *vol = snd_SfxVolume;
    }
    else
    {
        // distance effect
        *vol = Div1000(snd_SfxVolume * ((snd_clipping - approx_dist) >> FRACBITS));
    }

    if (*vol == 0)
    {
        return 1; // Non audible
    }

    if (monoSound)
    {
        *sep = NORM_SEP;
    }
    else
    {
        // angle of source to listener
        angle = R_PointToAngle2(players_mo->x, players_mo->y, source->x, source->y);

        angle -= players_mo->angle;
        if (angle <= players_mo->angle)
            angle += 0xffffffff;

        angle >>= ANGLETOFINESHIFT;

        // stereo separation (S_STEREO_SWING == 2^21 + 2^22)
        optSine = finesine[angle];
        *sep = 128 - (((optSine << 6) + (optSine << 5)) >> FRACBITS);
    }

    return 0; // Audible
}

void S_SetSfxVolume(int volume)
{
    snd_SfxVolume = volume;
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseMusic(void)
{
    
}

void S_ResumeMusic(void)
{

}

void S_StopSound(void *origin)
{
 
}

//
// S_getChannel :
//   If none available, return -1.  Otherwise channel #.
//
int S_getChannel(void *origin, sfxinfo_t *sfxinfo)
{
    
}

void S_StartSound(mobj_t *origin, byte sfx_id)
{
   
}

//
// Updates music & sounds
//
void S_UpdateSounds(void)
{
    
}

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int musicVolume)
{
    
}

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
    
}

void S_ClearSounds(void)
{

}
