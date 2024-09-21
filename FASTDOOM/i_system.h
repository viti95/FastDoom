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
//	System specific interface stuff.
//

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include "d_ticcmd.h"
#include "d_event.h"
#include "sounds.h"

// Called by DoomMain.
void I_Init(void);

// Called by startup code
// to get the ammount of memory to malloc
// for the zone management.
byte *I_ZoneBase(int *size);

//
// Called by D_DoomLoop,
// called before processing each tic in a frame.
// Quick syncronous operations are performed here.
// Can call D_PostEvent.
void I_StartTic(void);

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Called by M_Responder when quit is selected.
// Clean exit, displays sell blurb.
void I_Quit(void);

// Allocates from low memory under dos,
// just mallocs under unix
byte *I_AllocLow(int length);
int I_GetCPUModel(void);

void I_Error(char *error, ...);

//
//  MUSIC I/O
//

void I_LoopSong(int handle);
// called by anything that wishes to start music.
// plays a song, and when the song is done, starts playing it again in
// an endless loop.  the start is faded in over three seconds.

void I_ResumeSong(int handle);

//  SFX I/O
//

int I_GetSfxLumpNum(sfxinfo_t *sfx);
// called by routines which wish to play a sound effect at some later
// time.  Pass it the lump name of a sound effect WITHOUT the sfx
// prefix.  This means the maximum name length is 7 letters/digits.
// The prefixes for different sound cards are 'S','M','A', and 'P'.
// They refer to the card type.  The routine will cache in the
// appropriate sound effect when it is played.

// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode
void I_InitGraphics(void);

void I_ShutdownGraphics(void);

// Takes full 8 bit values.
void I_ProcessPalette(byte *palette);
void I_SetPalette(int numpalette);

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
void I_UpdateNoBlit(void);
#endif
void I_FinishUpdate(void);
#if defined(MODE_13H)
void I_FinishUpdateDifferential(void);
void I_FinishUpdateDirect(void);
#endif
void I_CalculateFPS(void);

#ifndef MAC

// Wait for vertical retrace or pause a bit.
void I_WaitSingleVBL(void);

// Wait for CGA to be available
void I_WaitCGA(void);

void I_DisableCGABlink(void);

void I_DisableMDABlink(void);

#endif

#endif
