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
//	Row (span) rendering of the flat modes: the walls and the sky.
//

#ifndef __R_FLATWALL__
#define __R_FLATWALL__

#include "options.h"
#include "r_defs.h"

//
// The walls of the flat modes have one color per column, so a row of a
// wall is a run of pixels of the same color, and the wall is painted
// with the flat span renderers of the video mode, one span per row.
//
// The walls are painted directly with spans: the seg loops walk the
// columns of the segment as usual and hand every column to the sweep of
// r_flatwall.c, which opens one span per row at the first column
// reaching the row and paints it as soon as the columns stop reaching
// it. The only work added to the column loop is to open and to close
// the rows.
//
// A span never crosses a change of the color of the wall, and the seg
// loop is who works that color out, so it says so with R_FlatWallColor:
// the spans the columns before left open are painted with the color
// that opened them, and the rows of the columns from there take the new
// one.
//
// A mode without a flat span renderer of its own leaves
// flatwallspanfunc null (the MDA and the VT100, whose walls are
// outlines of the character cells), and so do the modes of the setup
// menu that draw the walls column by column: their columns go to the
// column renderer of the mode as the seg loop walks them, the same
// moment the rows take them.
//

// Span renderer used for the wall rows, set up per video mode.
extern void (*flatwallspanfunc)(void);

// Non zero when the walls of the view are painted row by row, zero
// when they are painted column by column. R_SetupFlatWall sets it from
// the mode of the walls of the setup menu and the flat span renderer of
// the video mode, once per view.
extern int flatwallrows;

// Non zero when the flat sky is painted row by row, zero when it is
// painted column by column. R_ExecuteSetViewSize sets it from the sky
// rendering mode and flatwallspanfunc, once per view.
extern int flatskyrows;

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
// The sky of the flat sky modes is one color, so the sky of every
// column of the sky plane is a range of rows and the sky of every row
// is one span of that color: the sky is painted with the same sweep of
// the rows as the walls, with one span per row of the plane.
//
// The column renderers of the sky of the planar modes set the map mask
// of the plane of the pixel of the column on every column, and the
// renderers of the other modes walk the rows of one column at a time.
// The sweep walks every column of the plane once to open and to close
// its rows, and paints one span per row of the sky.
//

// Chooses the renderer of the sky of the view, row by row or column by
// column, the same way as R_SetupFlatWall does with the walls. The mode
// without a flat span renderer (the MDA) keeps its own column renderer
// of the sky. Called by R_ExecuteSetViewSize once the render functions
// of the video mode are set up.
void R_SetupFlatSky(void);

// Paints the sky plane pl row by row, one span per row, with the flat
// span renderer of the mode. The flat sky renderers of r_plane.c call
// it when flatskyrows is set, and their own loops paint the columns
// when it is not.
void R_DrawSkyFlatRows(visplane_t *pl);

//
// The wall of a segment is made of up to two tiers: the single sided
// line or the top wall of a two sided one, and the bottom wall of a
// two sided one. The seg loops of the flat walls open the wall of the
// segment, hand every column they walk to it and paint it at the end.
//
// A column with yl > yh is not visible: it closes the rows of the wall
// it does not reach, which is what makes the spans of the rows end.
//

// Opens the wall of a segment. The wall has the color the caller left
// in dc_color, which the caller of R_FlatWallColor changes when the
// light along the seg gives the columns another one. A wall whose color
// does not change, as the flatter walls, never calls it.
void R_FlatWallStart(void);

// Adds one column, from the row yl to the row yh both included, to the
// first or to the second tier of the wall of the segment.
void R_FlatWallColumn1(int x, int yl, int yh);
void R_FlatWallColumn2(int x, int yl, int yh);

// The color of the wall of the columns from x on is color: the spans the
// columns before it left open are painted with the color that opened
// them, and the rows of the columns from there take the new one. Called
// by the seg loops that light their wall per column, before the first
// column of the new color.
void R_FlatWallColor(int x, int color);

// Closes the tiers of the wall of the segment: the spans the last
// column left open end with it, at the column x2. Both tiers are closed
// together: a tier without a wall of its own has nothing open.
void R_FlatWallFinish(int x2);

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
