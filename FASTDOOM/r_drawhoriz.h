//
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
//	Horizontal (two-pass, cache-friendly) column rendering
//	(r_drawhoriz.c).
//

#ifndef __R_DRAWHORIZ__
#define __R_DRAWHORIZ__

#include "options.h"
#include "doomtype.h"
#include "doomdef.h"
#include "r_defs.h" // vissprite_t

#if defined(SUPPORTS_HORIZ_RENDER)

// ---------------------------------------------------------------------------
// Shared staging state (r_drawhoriz.c). Also referenced by the
// assembly post-pass in linearhz.asm as _horiztemp32, _horizrowmask,
// _horizymin, _horizymax.
// ---------------------------------------------------------------------------

// Interleaved staging buffer: pixel y of column c of the current
// four-column group is at byte ((byte *)horiztemp32)[(y << 2) + c];
// horiztemp32[y] is the same row as one aligned dword.
extern unsigned horiztemp32[SCREENHEIGHT];

// Bit c set on row y: column c of the group has a staged pixel on
// that row. Consumed and cleared by the post-pass.
// Declared as dwords so the pre-pass marks four screen rows of a
// span with one dword write; it is the same memory the assembly
// walks byte by byte (the C code uses ((byte *)horizrowmask)[y]).
extern unsigned horizrowmask[(SCREENHEIGHT + 3) / 4];

// Row range that can carry mask bits for the current group.
extern int horizymin;
extern int horizymax;

// ---------------------------------------------------------------------------
// Sprite drawers (pre-pass + flush in one call)
// ---------------------------------------------------------------------------

// Replacements for the classic sprite column drawers, selected
// through the spriteRender/pspriteRender tables (r_main.c).
void R_DrawVisSpriteHoriz(vissprite_t *vis);
void R_DrawVisPSpriteHoriz(vissprite_t *vis);

// ---------------------------------------------------------------------------
// Post-pass (flush)
// ---------------------------------------------------------------------------

// Assembly implementation (linearhz.asm) of the post-pass, used for
// the groups the C fast path of r_drawhoriz.c does not handle (the
// partial group at the view edge). Watcom register calling
// convention (EAX = xstart, EDX = group, ECX = colormap). Every
// staged byte is already translated through dc_colormap by the
// pre-pass (walls, sky and sprites alike), so NULL is passed: the
// staged bytes are screen colors and get copied verbatim (routed
// internally through an identity table). See linearhz.asm for the
// full interface spec.
void R_FlushHorizGroupAsm(int xstart, int group, byte *colormap);

// Wall colfunc replacement: stage the dc_* column into the current
// four-column group instead of drawing it to the screen directly.
// No arguments, reads only the classic dc_* globals plus viewwidth
// boundaries via dc_x/dc_yl/dc_yh, so it is a drop-in shape for an
// asm replacement like the R_DrawColumn* family.
extern void R_StageColumnHoriz(void);

// Flush any pending wall columns to the screen. Must be called
// whenever wall drawing switches context (after a seg loop or a
// masked seg range), so a later seg can still overwrite columns.
extern void R_FlushWallHoriz(void);

#else

// The horizontal wall renderer is not available in this video
// mode, so nothing ever gets staged and flushing is a no-op.
#define R_FlushWallHoriz() ((void)0)

#endif

#endif
