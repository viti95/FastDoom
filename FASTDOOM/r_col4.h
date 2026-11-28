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
//	Wall renderer of r_col.c, which maps four columns at a time of the
//	detail of high, the two columns of the detail of the low being of the
//	same file, r_col2.h being of it.
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
 * The renderer is of the file of r_col.c, which is of the two of the renderers
 * at once, of the two columns of the detail of the low below it, for the
 * bookkeeping of the columns kept of the two of them is the same, of the state
 * of it being one: R_SetupColumn4() and R_SetupColumn2() of the file give the
 * shape of the renderer of the frame, and the columns kept of the frame before
 * are dropped by the setup of it.  The header of the two columns is of the file
 * as well, the macros of the two of the headers of them being of the seg loops
 * of their own.
 *
 * The columns kept point into the WAD cache, which the seg loops purge when
 * they take a lump, so every such loop hands the columns kept to the view
 * first with R_FLUSH_COLUMNS4(), which tests colp, of the number of the columns
 * kept of the renderer of the frame being, of its own and calls nothing when
 * none is kept.  Of the modes without the renderer the macro is of nothing, of
 * the shape of a call still.
 *
 * The texture position of a row is kept in the format of the assembly column
 * renderers of the modes, and the renderer is compared with them byte for
 * byte over the shapes of the columns of the seg loops.
 *
 * The middle rows of a group of one light table and of one step, which is
 * what a seg of one texture gives, go to the loop of col4.asm, of the four
 * bytes of a row: of the positions of the four columns of the registers of
 * the machine, and of the registers of MMX for the CPUs of MMX.  The choice
 * between the two of them is of r_main.c, which writes col4midfunc of its
 * own switch of the CPU, as the choice of colfunc and of skyfunc is of it;
 * the groups of columns of more than one light table, of more than one step
 * or of sources of more than one stride are of the loop of the registers of
 * the machine of col4.asm, of the source, of the table and of the step of
 * every column of its own, of the same choice of r_main.c of the pointer of
 * them, col4midallfunc, the loop of the CPUs of MMX and of the CPUs of SSE2
 * walking the four positions of the row and taking the four texels of them
 * of their registers, the loads of the bytes of the sources and of the
 * pixels of the light tables being of the registers of the machine of the
 * three of them.
 */

/* The modes of the renderer, of the reason of the paragraph above: of the
   macro below are the files which keep the renderer out of their build, and
   of it the switch of r_main.c which chooses the loop of the middle rows. */
#if (defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)) \
    && !defined(MODE_CGA16) && !defined(MODE_CVB) && !defined(MODE_CGA512)
#define USE_COLUMNS4

/* The columns kept of the renderer of the frame handed over to the view, of the
   two of the renderers of the one function of them being. */
void R_FlushColumn(void);

/* The middle rows of a group of four columns of one light table and of one
   step: the group of the columns kept in col4slots, of the x of them in
   col4x, of the first of their rows in col4y and of their number in col4rows,
   which r_col.c writes before the call.  The renderer of them is kept in
   col4midfunc, of the scalar loop by default, and r_main.c chooses the loop
   of the CPUs of MMX of its own switch of the CPU, as it chooses the other
   renderers of a mode. */
void R_DrawColumns4Mid(void);
void R_DrawColumns4MidMMX(void);
void R_DrawColumns4MidSSE2(void);
extern void (*col4midfunc)(void);

/* The middle rows of the same four columns kept, of a group which is not of
   one light table, of one step, or of sources of one stride, so of every
   column of the group the source, the table, the step and the position of a
   row are walked on their own: of the loop of the registers of the machine of
   col4.asm, of the loop of the registers of MMX of the CPUs of them, or of
   the loop of the instructions of SSE2 of the Pentium 4, of the same x, of
   the same first row and of the same number of rows of the data of the file.
   Every one of the three of them writes the four bytes of a row of the view
   with one store of 32 bits; the loop of them is kept in col4midallfunc, of
   the loop of the registers of the machine by default, and r_main.c chooses
   it of its own switch of the CPU, as it chooses the loop of the columns of
   one light table and of one step above. */
void R_DrawColumns4MidAll(void);
void R_DrawColumns4MidAllMMX(void);
void R_DrawColumns4MidAllSSE2(void);
extern void (*col4midallfunc)(void);

/* the number of the columns of the seg loops kept of the view, of the two of
   the renderers of the file being one, the renderer of the frame one of them
   being and the columns kept of the other of none, the macro of the flush of the
   renderer of the frame of none of them being */
extern int colp;

/* the column function of the frame of the columns kept, of the views drawn four
   columns at a time, the fallback of the others */
void (*R_SetupColumn4(void (*fallback)(void)))(void);

#define R_CHOOSE_COLUMNS4(fallback) R_SetupColumn4(fallback)
#define R_FLUSH_COLUMNS4() do { if (colp) R_FlushColumn(); } while (0)

#else

#define R_CHOOSE_COLUMNS4(fallback) (fallback)
#define R_FLUSH_COLUMNS4() ((void)0)

#endif

#endif
