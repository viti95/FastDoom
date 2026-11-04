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
// The flat wall modes paint the walls with a single color per column,
// so a wall of one color can be drawn with the flat span renderers row
// by row instead of the column renderers. Painting rows is cheaper for
// walls that are wide compared to how tall they are, which is the case
// for most of the walls of a level once the view is scaled up.
//
// The seg loops record the geometry of every column of a wall tier and
// R_FillFlatWallRows paints the tier with the span renderer, which also
// handles the light levels of the segment by splitting the rows in
// runs of columns of the same color.
//
// Every video mode sets the span renderer of its flat walls, and the
// cost model of R_FlatWallUseRows decides for every wall which of the
// two ways is cheaper. A mode without a flat span renderer of its own
// leaves flatwallspanfunc null, and its walls are always drawn with
// columns.
//

// Span renderer used for the wall rows, set up per video mode.
extern void (*flatwallspanfunc)(void);

// Rendering quality in use, selects the cost model of R_FlatWallUseRows.
extern int flatwallmodel;

// One of FLATWALL_COLS, FLATWALL_AUTO, FLATWALL_ROWS (doomdef.h): the
// columns only, the cost model, the rows only. R_ExecuteSetViewSize sets it
// from the wall rendering mode.
extern int flatwallmode;

// How many columns the column renderer draws: the low resolution modes
// of the CGA composite cards only update every n-th column, so their
// columns are n times cheaper than they look.
extern int flatwallcolscale;

#define FLATWALL_MODEL_HIGH 0
#define FLATWALL_MODEL_LOW 1
#define FLATWALL_MODEL_POTATO 2

// Geometry of the tier being recorded, per column. A column with
// yl > yh is not visible.
extern short wallrowyl[];
extern short wallrowyh[];
extern short wallrow2yl[];
extern short wallrow2yh[];
extern byte wallrowcol[];

void R_InitFlatWall(void);

// Paints the tier with rows, col may be null when all the columns
// have the color in dc_color.
void R_FillFlatWallRows(int x1,
			int x2,
			const short *yl,
			const short *yh,
			const byte *col);

// Paints a tier of a wall with the cheapest way for it, the entry
// point of the seg loops. col may be null when all the columns have
// the color in dc_color.
void R_DrawFlatWallTier(int x1,
			int x2,
			const short *yl,
			const short *yh,
			const byte *col);

// Paints the tier with columns, the fall back of the row renderer.
void R_DrawFlatWallColumns(int x1, int x2, const short *yl, const short *yh);
void R_DrawFlatWallColumnsCol(int x1,
			      int x2,
			      const short *yl,
			      const short *yh,
			      const byte *col);

//
// The masked mid walls (the windows, the fences and the like) drawn by
// the seg loops of the flat walls are a single color per column, as
// every other wall of the flat modes, so the rows can paint them too.
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

// Compares the cost of the two ways of painting the tier.
int R_FlatWallUseRows(int x1, int x2, const short *yl, const short *yh);

#define R_FLATWALL_RECORD(x, ylval, yhval) \
	do { \
		wallrowyl[(x)] = (ylval); \
		wallrowyh[(x)] = (yhval); \
		wallrowcol[(x)] = dc_color; \
	} while (0)

#define R_FLATWALL_RECORD2(x, ylval, yhval) \
	do { \
		wallrow2yl[(x)] = (ylval); \
		wallrow2yh[(x)] = (yhval); \
		wallrowcol[(x)] = dc_color; \
	} while (0)

// Marks a tier as not visible at that column.
#define R_FLATWALL_EMPTY(x) \
	do { \
		wallrowyl[(x)] = 1; \
		wallrowyh[(x)] = 0; \
	} while (0)

#define R_FLATWALL_EMPTY2(x) \
	do { \
		wallrow2yl[(x)] = 1; \
		wallrow2yh[(x)] = 0; \
	} while (0)

#endif
