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
//	Wall renderer of r_col2.c, which maps two columns at a time of the
//	detail of the low.
//

*/

#ifndef __R_COL2__
#define __R_COL2__

/* the macros of the mode of the video of the guard below, for the header of
   r_draw.h includes this one before the seg loops include the mode of their
   own, as of the header of the four columns above */
#include "options.h"

/*
 * The renderer keeps the wall columns the seg loops ask for, of the two of a
 * group, and draws the group with one 32 bit store per row: the detail of the
 * low keeps two bytes of the view for one pixel of it, so the two columns of a
 * group are the four bytes of a row of the view, which is what
 * R_SetupColumn2() tells, the mode of the same detail drawing one column with
 * two stores of 16 bits of them.
 *
 * The renderer is of the detail of the low, the detail of one pixel of each
 * row of the four columns of r_col4.c being of it of its own, and of the same
 * modes of the four of them: the buffered modes and MODE_VBE2_DIRECT, of the
 * CPUs which have no column renderer tuned for them.  The scaled views, the
 * planar modes, the text modes and the modes of CGA of sixteen colours, of CVB
 * and of 512 columns are not, for two of their columns are not four bytes of a
 * row.
 *
 * The columns kept point into the WAD cache, which the seg loops purge when
 * they take a lump, so every such loop hands the columns kept to the view
 * first with R_FLUSH_COLUMNS2(), which tests col2pending, the number of
 * columns kept, of its own and calls nothing when none is kept.  Of the modes
 * without the renderer the macro is of nothing, of the shape of a call still.
 *
 * The texture position of a row is kept in the format of the assembly column
 * renderers of the modes of the detail of the low, and the renderer is
 * compared with them byte for byte over the shapes of the columns of the seg
 * loops.
 *
 * The middle rows of a group of one light table and of one step, which is what
 * a seg of one texture gives, and the groups of more than one light table, of
 * more than one step and of sources of more than one stride, of every column
 * of the group of the source, of the table and of the step of its own, are of
 * the loops of the file, which walk the two positions of a row and write the
 * four bytes of it with one store of 32 bits.  The loops are kept in
 * col2midfunc and in col2midallfunc, of the two of the file by default, as the
 * loops of the four columns are of col4.asm, so the loop of a CPU of the file
 * of asm of them is chosen by r_main.c of its own switch of the CPU, as it
 * chooses the renderer of the four of them and the renderers of a mode.
 */

/* The modes of the renderer, of the reason of the paragraph above: of the
   macro below are the files which keep the renderer out of their build, and of
   it the switch of r_main.c which chooses the column function of the detail of
   the low. */
#if (defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)) \
    && !defined(MODE_CGA16) && !defined(MODE_CVB) && !defined(MODE_CGA512)
#define USE_COLUMNS2

void R_DrawColumn2(void);
void R_FlushColumn2(void);

/* The middle rows of a group of two columns of one light table and of one
   step: the group of the columns kept in col2slots, of the x of them in
   col2x, of the first of their rows in col2y and of their number in col2rows,
   which r_col2.c writes before the call.  The renderer of them is kept in
   col2midfunc, of the loop of the file by default, and of the loop of a CPU of
   the file of asm of them, which r_main.c chooses of its own switch of the
   CPU, as it chooses the renderers of a mode. */
void R_DrawColumns2Mid(void);
void R_DrawColumns2MidMMX(void);
void R_DrawColumns2MidSSE2(void);
void R_DrawColumns2MidAll(void);
void R_DrawColumns2MidAllMMX(void);
void R_DrawColumns2MidAllSSE2(void);
extern void (*col2midfunc)(void);
extern void (*col2midallfunc)(void);

/* The middle rows of the same two columns kept, of a group which is not of one
   light table, of one step, or of sources of one stride, so of every column of
   the group the source, the table, the step and the position of a row are
   walked on their own: of the loop of the file, and of the loop of a CPU of the
   file of asm of them, of the same x, of the same first row and of the same
   number of rows of the data of the file.  Every one of them writes the four
   bytes of a row of the view with one store of 32 bits; the loop of them is
   kept in col2midallfunc, of the loop of the file by default, and r_main.c
   chooses it of its own switch of the CPU, as it chooses the loop of the
   columns of one light table and of one step above. */

/* the number of the columns of the seg loops kept of the view */
extern int col2pending;

/* the column function of the frame: R_DrawColumn2 of the views of the detail
   of the low drawn two columns at a time, the fallback of the others */
void (*R_SetupColumn2(void (*fallback)(void)))(void);

#define R_CHOOSE_COLUMNS2(fallback) R_SetupColumn2(fallback)
#define R_FLUSH_COLUMNS2() do { if (col2pending) R_FlushColumn2(); } while (0)

#else

#define R_CHOOSE_COLUMNS2(fallback) (fallback)
#define R_FLUSH_COLUMNS2() ((void)0)

#endif

#endif
