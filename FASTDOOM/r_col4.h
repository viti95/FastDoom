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
 * The columns kept point into the WAD cache, and taking a lump into the
 * cache, as the seg loops do for the column of a texture whose patches are
 * in more than one lump, purges other lumps.  So every loop which takes a
 * texture column from the cache hands the columns kept to the view first,
 * with R_FLUSH_COLUMNS4(), and it does so when W_CachedLumpBase() tells
 * that the lump is not in the cache any more, which is the same test the
 * loop makes for its own column.  The columns kept go to the view before
 * the planes and the masked pass draw over it, for the same reason.
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
 *
 * -DWALL4_VERIFY draws every group of columns a second time, column by
 * column, into a shadow of the view and compares the two of them, printing
 * the column, its rows and the position of its texture of a byte which
 * differs, which is how a wall type which does not go with the renderer is
 * found on the machine of the player.
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

/* the column function of the frame: R_DrawColumn4 of the views drawn four
   columns at a time, the fallback of the others */
void (*R_SetupColumn4(void (*fallback)(void)))(void);

#define R_CHOOSE_COLUMNS4(fallback) R_SetupColumn4(fallback)
#define R_FLUSH_COLUMNS4() R_FlushColumn4()

#else

#define R_CHOOSE_COLUMNS4(fallback) (fallback)
#define R_FLUSH_COLUMNS4()

#endif

#endif
