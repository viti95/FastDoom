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
 * The renderer is drawn with the CPUs of r_main.c which pay for the four
 * byte stores of a row of four pixels, and it is left out of a build made
 * with -DNO_WALL4, which is there to compare the two of them on a machine.
 * The renderers of a column of r_draw.c are used in its place.
 */

/*
 * R_DrawColumn4 is a column function of the seg loops, so it keeps a few
 * wall columns waiting for the three which complete their group.  The
 * columns kept point to the texture lumps, which the cache can move, and
 * to the view, which the planes overwrite after the walls.  So the code
 * which draws the view and the cache have to hand the columns kept to the
 * view with R_FLUSH_COLUMNS4() before they do so, and the setup of the
 * view starts the frame with R_SETUP_COLUMNS4().
 */
#if defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)

void R_DrawColumn4(void);
void R_SetupColumn4(void);
void R_FlushColumn4(void);

#define R_FLUSH_COLUMNS4() R_FlushColumn4()
#define R_SETUP_COLUMNS4() R_SetupColumn4()

#else

#define R_FLUSH_COLUMNS4()
#define R_SETUP_COLUMNS4()

#endif

#endif
