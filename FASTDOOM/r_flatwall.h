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
//	Row (span) rendering of the flat / flatter wall modes.
//

#ifndef __R_FLATWALL__
#define __R_FLATWALL__

#include "options.h"
#include "r_defs.h"

//
// The walls of the flat modes have one color per column, so a row of a
// wall is a run of pixels of the same color and can be painted with the
// flat span renderers of the video mode, which write a whole row at a
// time. Painting rows is cheaper than painting columns for the walls
// that are wide compared to how tall they are, which is the case for
// most of the walls of a level once the view is scaled up.
//
// The walls are painted directly with spans, without recording their
// columns and converting them to spans afterwards: the seg loops walk
// the columns of the segment as usual and hand every column to the
// sweep of r_flatwall.c, which opens one span per row at the first
// column reaching the row and paints it as soon as the columns stop
// reaching it. A row of a wall is then one span, and the only work
// added to the column loop is to open and close the rows.
//
// A span never crosses a light level: the color of the columns is
// checked per column, and a color change paints the spans opened with
// the previous color.
//
// Every video mode sets the span renderer of its flat walls, and the
// modes of the setup menu that draw the walls row by row use it. A mode
// without a flat span renderer of its own leaves flatwallspanfunc null
// (the MDA and the VT100, whose walls are outlines of the character
// cells) and its walls are drawn column by column, as the seg loops of
// the walls drawn with the column renderer of the setup menu. Those
// paths record the columns of the wall and paint them at the end of the
// segment, which is the only case where the columns are recorded.
//

// Span renderer used for the wall rows, set up per video mode.
extern void (*flatwallspanfunc)(void);

// One of FLATWALL_COLS, FLATWALL_AUTO, FLATWALL_ROWS (doomdef.h): the
// columns only or the rows only. R_ExecuteSetViewSize sets it from the
// wall rendering mode.
extern int flatwallmode;

// Non zero when the walls of the view are painted row by row, zero
// when they are painted column by column. R_ExecuteSetViewSize sets it
// from flatwallmode and flatwallspanfunc, once per view.
extern int flatwallrows;

// How many columns the column renderer draws: the low resolution modes
// of the CGA composite cards only update every n-th column, so their
// columns are n times cheaper than they look.
extern int flatwallcolscale;

void R_InitFlatWall(void);

// Chooses the renderer of the walls of the view, row by row or column
// by column. Called by R_ExecuteSetViewSize once the render functions
// of the video mode are set up.
void R_SetupFlatWall(void);

//
// The wall of a segment is made of up to two tiers: the single sided
// line or the top wall of a two sided one, and the bottom wall of a
// two sided one. The seg loops of the flat walls open the wall of the
// segment, hand every column they walk to it and paint it at the end.
//
// A column with yl > yh is not visible: it closes the rows of the wall
// it does not reach, which is what makes the spans of the rows end.
//

// Opens the wall of a segment. lit is zero when the whole wall has the
// color the caller left in dc_color (the flatter walls) and non zero
// when the color of the columns changes along the wall (the flat
// walls, lit per column).
void R_FlatWallStart(int lit);

// Adds one column, from the row yl to the row yh both included, to the
// first or to the second tier of the wall of the segment.
void R_FlatWallColumn1(int x, int yl, int yh);
void R_FlatWallColumn2(int x, int yl, int yh);

// Paints what is left of the wall of a tier, the columns x1 to x2 of
// it, and closes the tier.
void R_FlatWallFinish1(int x1, int x2);
void R_FlatWallFinish2(int x1, int x2);

//
// The masked mid walls (the windows, the fences and the like) drawn by
// the seg loops of the flat modes are a single color per column, as
// every other wall of the flat modes, so they are painted row by row
// too.
//
// A masked texture is drawn with several posts per column, and the gaps
// between the posts must stay empty, which the rows of a single span
// would paint. The rows are exact for the textures whose columns are a
// single post of the whole height of the texture, and the rest of the
// textures keep the columns.
//

// Paints the masked mid wall of a flat seg loop with rows: the
// drawseg ds, the columns x1 to x2 of it, the mid texture texnum, the
// color of the texture and lit when its columns are lit one by one.
// Returns true when the rows painted the wall, so the caller can skip
// its column loop. The clips (mfloorclip, mceilingclip), the position
// of the texture (dc_texturemid) and the lighting (walllights) of the
// caller must be set up, as for its column loop.
int R_DrawMaskedWallRows(drawseg_t *ds,
			 int texnum,
			 int x1,
			 int x2,
			 byte color,
			 int lit);

#endif
