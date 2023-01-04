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
// DESCRIPTION:
//	The not so system specific sound interface.
//

#ifndef __S_SOUND__
#define __S_SOUND__

#include "doomtype.h"
#include "p_mobj.h"

#define S_MAX_VOLUME 127

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
#define S_CLOSE_DIST (200 * 0x10000)

#define NORM_SEP 128

#define S_STEREO_SWING (96 * 0x10000)

// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST (1200 * 0x10000)
#define S_CLIPPING_DIST_BOSS (3600 * 0x10000)

extern int snd_clipping;

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int musicVolume);

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void);

//
// Start sound for thing at <origin>
//  using <sound_id> from sounds.h
//
void S_StartSound(mobj_t *origin, byte sound_id);

// Stop sound for thing at <origin>
void S_StopSound(void *origin);

// Start music using <music_id> from sounds.h
void S_StartMusic(int music_id);

// Start music using <music_id> from sounds.h,
//  and set whether looping
void S_ChangeMusic(int music_id,
                   int looping);

// Stops the music fer sure.
void S_StopMusic(void);

// Stop and resume music, during game PAUSE.
void S_PauseMusic(void);
void S_ResumeMusic(void);
void S_CheckCD(void);

//
// Updates music & sounds
//
void S_UpdateSounds(void);

void S_SetMusicVolume(int volume);
void S_SetSfxVolume(int volume);

void S_ClearSounds(void);

#endif
