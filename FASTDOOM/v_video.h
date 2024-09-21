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
//	Gamma correction LUT.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//

#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomtype.h"
#include "options.h"
#include "doomdef.h"

// Needed because we are refering to patches.
#include "r_data.h"



//
// VIDEO
//

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
extern byte screen0[SCREENWIDTH * SCREENHEIGHT];
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
// Note must be screenwdith because the draw functions assume this.
// We can also expand the staus bar with this to go the full width.
extern byte screen4[SCREENWIDTH * SBARHEIGHT];
#endif

#if defined(USE_BACKBUFFER)
extern byte backbuffer[SCREENWIDTH * SCREENHEIGHT];
#endif

#if defined(MODE_MDA)
extern unsigned short backbuffer[80 * 25];
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
extern int dirtybox[4];
#endif

extern int usegamma;

// Allocates buffer screens, call before R_Init.
void V_Init(void);

void V_CopyRect(int srcx, int srcy, byte *srcscrn, int width, int height, int destx, int desty, byte *destscrn);

void V_DrawPatch(int x, int y, byte *scrn, patch_t *patch);

#if PIXEL_SCALING!=1
// If we are pixel scaling, we need a version of V_DrawPatch that draws to the
// native resolution. This is currently only used for the border texture.
void V_DrawPatchNativeRes(int x, int y, byte *scrn, patch_t *patch);
#else
// V_DrawPatchNativeRes is just an alias for V_DrawPatch.
#define V_DrawPatchNativeRes(x, y, scrn, patch) V_DrawPatch(x, y, scrn, patch)
#endif

// Begin mode specific functions.

#if defined(MODE_T4050)
void V_DrawPatchDirectText4050(int x, int y, patch_t *patch);
#define V_DrawPatchMode V_DrawPatchDirectText4050
#endif

#if defined(MODE_T4025)
void V_DrawPatchDirectText4025(int x, int y, patch_t *patch);
#define V_DrawPatchMode V_DrawPatchDirectText4025
#endif

#if defined(MODE_T8025)
void V_DrawPatchDirectText8025(int x, int y, patch_t *patch);
#define V_DrawPatchMode V_DrawPatchDirectText8025
#endif

#if defined(MODE_T8043)
void V_DrawPatchDirectText8043(int x, int y, patch_t *patch);
#define V_DrawPatchMode V_DrawPatchDirectText8043
#endif

#if defined(MODE_T8050)
void V_DrawPatchDirectText8050(int x, int y, patch_t *patch);
#define V_DrawPatchMode V_DrawPatchDirectText8050
#endif

#if defined(MODE_MDA)
void V_DrawPatchDirectTextMDA(int x, int y, patch_t *patch);
#define V_DrawPatchMode V_DrawPatchDirectTextMDA
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
void V_DrawPatchDirect(int x, int y, patch_t *patch);
#define V_DrawPatchDirectCentered(x, y, patch) \
  V_DrawPatchDirect(CENTERING_OFFSET_X + (x), CENTERING_OFFSET_Y + (y), patch)
void V_DrawPatchFlippedDirect(int x, int y, patch_t *patch);
#endif

#if defined(MODE_Y_HALF)
void V_DrawPatchDirect(int x, int y, patch_t *patch);
#define V_DrawPatchDirectCentered(x, y, patch) \
  V_DrawPatchDirect(CENTERING_OFFSET_X + (x), CENTERING_OFFSET_Y + (y/2), patch)
void V_DrawPatchFlippedDirect(int x, int y, patch_t *patch);
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
void V_DrawPatchScreen0(int x, int y, patch_t *patch);
void V_DrawPatchFlippedScreen0(int x, int y, patch_t *patch);
#define V_DrawPatchMode V_DrawPatchScreen0
#define V_DrawPatchFlippedMode V_DrawPatchFlippedScreen0
#endif

#if defined(USE_BACKBUFFER)
#define V_DrawPatchMode V_DrawPatchDirect
// TODO SHould there be a flipped mode PatchDirect?
#define V_DrawPatchFlippedMode V_DrawPatchDirect
#endif

#define V_DrawPatchModeCentered(x, y, patch) \
  V_DrawPatchMode(CENTERING_OFFSET_X + (x), CENTERING_OFFSET_Y + (y), patch)
#define V_DrawPatchFlippedModeCentered(x, y, patch) \
  V_DrawPatchFlippedMode(CENTERING_OFFSET_X + (x), CENTERING_OFFSET_Y + (y), patch)

// End mode specific functions.
//
void V_WriteTextDirect(int x, int y, char *string);
void V_WriteCharDirect(int x, int y, unsigned char c);

void V_WriteTextColorDirect(int x, int y, char *string, unsigned short color);
void V_WriteCharColorDirect(int x, int y, unsigned char c, unsigned short color);

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
void V_MarkRect(int x, int y, int width, int height);
#endif

#endif
