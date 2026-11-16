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

/* the macros of the mode of the video of the guard below, for the header of
   r_draw.h includes this one before the seg loops include the mode of their
   own */
#include "options.h"

/*
 * The renderer keeps the wall columns the seg loops ask for, of the four of
 * a group, and draws the group with one 32 bit store per row: four columns of
 * the view are the four bytes of a row only of the mode keep the view one
 * byte per pixel and of the view be of the 1:1 detail level, which is what
 * R_SetupColumn4() tells.  The buffered modes and MODE_VBE2_DIRECT are drawn
 * with it, of the CPUs which have no column renderer tuned for them; the
 * scaled views, the planar modes, the text modes and the modes of CGA of
 * sixteen colours, of CVB and of 512 columns are not, for four of their
 * columns are not four bytes of a row.
 *
 * The columns kept point into the WAD cache, which the seg loops purge when
 * they take a lump, so every such loop hands the columns kept to the view
 * first with R_FLUSH_COLUMNS4(), which tests col4pending, the number of
 * columns kept, of its own and calls nothing when none is kept.  Of the modes
 * without the renderer the macro is of nothing, of the shape of a call still.
 *
 * The texture position of a row is kept in the format of the assembly column
 * renderers of the modes, and the renderer is compared with them byte for
 * byte over the shapes of the columns of the seg loops.
 *
 * The middle rows of a group of one light table and of one step, which is
 * what a seg of one texture gives, go to the loop of col4.asm, of the four
 * bytes of a row: of the positions of the four columns of the registers of
 * the machine, and of the registers of MMX for the CPUs of MMX.  The choice
 * between the two of them is of R_SET_COLUMNS4_MID(), which r_main.c calls
 * of its switch of the CPU, as the choice of colfunc and of spanfunc is of
 * it; the groups of columns of more than one light table, or of more than
 * one step, are drawn of the C.
 */

/* The modes of the renderer, of the reason of the paragraph above. */
#if (defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)) \
    && !defined(MODE_CGA16) && !defined(MODE_CVB) && !defined(MODE_CGA512)

void R_DrawColumn4(void);
void R_FlushColumn4(void);

/* The middle rows of a group of four columns of one light table and of one
   step: the group of the columns kept in col4slots, of the x of them in
   col4x, of the first of their rows in col4y and of their number in col4rows,
   which r_col4.c writes before the call.  The renderer of them is kept in
   col4midfunc, of the scalar loop by default, and r_main.c chooses the loop
   of the CPUs of MMX with the macro below, as it chooses the other renderers
   of a mode. */
void R_DrawColumns4Mid(void);
void R_DrawColumns4MidMMX(void);
extern void (*col4midfunc)(void);

#define R_SET_COLUMNS4_MID(fn) (col4midfunc = (fn))

/* the number of the columns of the seg loops kept of the view */
extern int col4pending;

/* the column function of the frame: R_DrawColumn4 of the views drawn four
   columns at a time, the fallback of the others */
void (*R_SetupColumn4(void (*fallback)(void)))(void);

#define R_CHOOSE_COLUMNS4(fallback) R_SetupColumn4(fallback)
#define R_FLUSH_COLUMNS4() do { if (col4pending) R_FlushColumn4(); } while (0)

#else

#define R_CHOOSE_COLUMNS4(fallback) (fallback)
#define R_SET_COLUMNS4_MID(fn) ((void)0)
#define R_FLUSH_COLUMNS4() ((void)0)

#endif

#endif
