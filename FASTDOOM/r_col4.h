/*
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
//	Wall renderer of r_col4.c, which maps four columns at a time.
//

*/

#ifndef __R_COL4__
#define __R_COL4__

/* the macros of the mode of the video of the guard of the renderer, for the
   header of r_draw.h includes this one before the seg loops of the game have
   included the mode of the video of their own */
#include "options.h"

/*
 * R_DrawColumn4 is a column function of the seg loops, so it keeps a few
 * wall columns waiting for the three which complete their group.  Four
 * columns of the view are the four bytes of an aligned 32 bit store of a
 * row only of the mode keep the view one byte per pixel and of the view be
 * of the 1:1 detail level, which is what R_SetupColumn4() tells.  So the
 * buffered modes (USE_BACKBUFFER, of which MODE_13H and MODE_VBE2) and the
 * direct modes (MODE_VBE2_DIRECT) are drawn with it, of the CPUs which have
 * no column renderer of their own; the CPUs with a renderer tuned for them
 * (the MMX renderers, the renderers of the K5 and of the Pentium P54CS, ...)
 * keep it, as they do for the planes and the sprites.
 *
 * The other modes are not drawn with it, for four of their columns are not
 * four bytes of a row: a pixel of the planar modes (MODE_X, MODE_Y,
 * MODE_Y_HALF, MODE_PGC) is a byte of every one of the four planes, a cell
 * of the text modes (MODE_T4050, MODE_T4025, MODE_T8025, MODE_T8050) is a
 * character and its colour, and of the modes of CGA (MODE_CGA16, MODE_CGA512,
 * MODE_CVB) the seg loops ask for the column function of every second and
 * of every fourth column only, so four calls are not four columns of a row.
 * The DETAIL_LOW and the DETAIL_POTATO level is not drawn with it either:
 * its renderers write a pixel of two and of four bytes and they skip the
 * rows of the view which a pixel of their size does not reach, which the
 * renderer of a group of four columns can not do with the rows of its edges.
 *
 * The columns kept are of four slots, of which the first is the leftmost of
 * them: the column asked for, when it is the next one of the group kept, goes
 * into the slot after the ones kept, and the group of four is drawn whole and
 * leaves no column kept.  The seg loops ask for the columns of a seg one
 * after the other, so the column after a group drawn starts a new group, of
 * an address aligned to four.  The group of an address which is not aligned
 * to four gives its leftmost column to the view on its own and keeps the
 * three of its right, which move to the first of the slots; the columns kept
 * which go to the view are taken of the slots in their order, of the leftmost
 * one first.
 *
 * The columns kept point into the WAD cache, and taking a lump into the
 * cache, as the seg loops do for the column of a texture whose patches are
 * in more than one lump, purges other lumps.  So every loop which takes a
 * texture column from the cache hands the columns kept to the view first,
 * with R_FLUSH_COLUMNS4(), and it does so when W_CachedLumpBase() tells
 * that the lump is not in the cache any more, which is the same test the
 * loop makes for its own column.  The columns kept go to the view before
 * the planes and the masked pass draw over it, for the same reason.
 *
 * The macro tests col4pending, the number of columns kept, of its own, so
 * the seg loops of the renderers of the modes, which keep no columns, pay
 * the test of one word and no call.  Of the modes without the renderer of
 * four columns the macro is of nothing, of the shape of a call still, so the
 * seg loops of them pay nothing at all and an if of a flush of them keeps its
 * body.
 *
 * The texture position of a row is kept in the format of the assembly
 * renderers of the modes, of 25 fraction bits and of seven significant bits,
 * and not in the fixed_t format of the C renderers of r_draw.c of the text
 * modes, for the walls of a large step (the walls close to the view) and of a
 * large position (the tall walls and the masked walls) fall out of the seven
 * significant bits and are drawn from the texel of the shifted value, so the
 * renderer has to shift the same way to draw a wall at its place.  The shift
 * of the position to the texel is not signed, "shr ebx,25" of the assembly
 * source, so the texel is one of 0 to 127 and a texture wraps vertically at
 * 128 texels, as the comment of the assembly source says.  A pixel of a light
 * table is named by the low byte of the address of the table written over,
 * "mov al,[eax]" of the assembly source, so the tables of the game are of 256
 * bytes and the renderer masks the low byte of the address of a table to name
 * the pixel of it the way the renderers of the mode name it.
 *
 * The renderer is compared byte for byte with the assembly renderers of the
 * modes, R_DrawColumnBackbuffer() of linearh.asm of the buffered modes and
 * R_DrawColumnVBE2() of vbe2dh.asm of the direct mode, over the shapes of the
 * columns the seg loops make: the walls of equal and of ragged heights, the
 * top and the bottom of a two sided line, the posts of a masked wall, the
 * walls of a large step and of a large position, and the view placed at every
 * byte of the alignment of a group of four columns.
 */
/* The modes drawn with the renderer are the ones which keep the view a byte
   per pixel and which ask for the column function of every column of the
   view.  The modes of CGA of sixteen colours, of CVB and of 512 columns are
   left out: their seg loops ask for the column function of every second and
   of every fourth column of the 1:1 view only, for a pixel of them is two
   and four columns wide, so four calls are not the four columns of a row. */
#if (defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)) \
    && !defined(MODE_CGA16) && !defined(MODE_CVB) && !defined(MODE_CGA512)

void R_DrawColumn4(void);
void R_FlushColumn4(void);

/* the number of the columns of the seg loops kept of the view */
extern int col4pending;

/* the column function of the frame: R_DrawColumn4 of the views drawn four
   columns at a time, the fallback of the others */
void (*R_SetupColumn4(void (*fallback)(void)))(void);

#define R_CHOOSE_COLUMNS4(fallback) R_SetupColumn4(fallback)
#define R_FLUSH_COLUMNS4() do { if (col4pending) R_FlushColumn4(); } while (0)

#else

#define R_CHOOSE_COLUMNS4(fallback) (fallback)
/* the macro is of the shape of a call, for a seg loop of a mode of the
   renderer tests the columns kept before the flush of its own, which leaves
   the flush of a mode without the renderer as the body of an if of its own */
#define R_FLUSH_COLUMNS4() ((void)0)

#endif

#endif
