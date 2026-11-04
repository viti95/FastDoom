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

#include "doomdef.h"
#include "options.h"
#include "z_zone.h"
#include "r_data.h"
#include "r_local.h"
#include "r_flatwall.h"
#include "doomstat.h"
#include "i_debug.h"

#include "sizeopt.h"

#if defined(TEXT_MODE)
#include "i_text.h"
#endif

//
// The seg loops of the flat walls hand every column of the wall they
// walk to the sweep of the rows, which paints the spans of the wall as
// they finish, without recording the columns of it.
//
// Only the walls that are not painted row by row need their columns:
// the walls drawn with the column renderer of the setup menu and the
// walls of the modes without a flat span renderer, which record the
// geometry of every column here and paint it at the end of the
// segment.
//
static short wallrowyl[SCREENWIDTH];
static short wallrowyh[SCREENWIDTH];
static short wallrow2yl[SCREENWIDTH];
static short wallrow2yh[SCREENWIDTH];
static byte wallrowcol[SCREENWIDTH];

// the columns of the masked mid walls still to draw, of the drawseg
// being painted, and the lighting of the seg loop that draws it
extern short *maskedtexturecol;
extern lighttable_t **walllights;

void (*flatwallspanfunc)(void);

int flatwallmode;
int flatwallrows;
int flatwallcolscale = 1;

//
// The span renderers translate the color with ds_colormap, while the
// flat wall column renderers write dc_color directly (they already
// applied the light level), so the walls are painted through an
// identity colormap to get exactly the same pixels.
//
// Text modes use the 16 color attribute lookuptable, which is what the
// flat wall column renderers of those modes use.
//
static byte *flatwallcolormap;
static byte flatwallbright[256];

// non zero while the wall of a segment with the color of every column
// recorded, see R_FlatWallStart
static int flatwalllit;

//
// The sweep of the rows of one tier of a wall.
//
// The columns of the wall are walked from left to right, and the wall
// of every column is a range of rows: a row of the wall is a span that
// opens at the first column reaching it and ends at the last one
// reaching it, so the span of a row is known to be finished at the
// first column that stops reaching the row, and it is painted then.
//
// The columns reaching a row are consecutive, as the edges of a wall
// are straight lines and the clips of the seg only cut columns out, so
// the rows with a span open at a column are a range of rows lo to hi,
// which is the wall of the last column that was visible, and the rows
// of the wall of a column out of that range open a span. Each column
// then only walks the rows of its wall that open and that close, one
// span per row of the wall.
//
typedef struct
{
	// column where the span open at each row started, only valid for
	// the rows of the open range. The rows of the view, which never
	// go over SCREENHEIGHT.
	short start[SCREENHEIGHT];

	int lo;
	int hi;

	// color of the spans open at the last column, -1 when the sweep
	// did not open a span yet. All the open spans share it: a span
	// never crosses a change of the color of the columns.
	int color;
} flatwallsweep_t;

static flatwallsweep_t flatwallsweep;
static flatwallsweep_t flatwallsweep2;

void R_InitFlatWall(void)
{
	int i;

	for (i = 0; i < 256; i++)
		flatwallbright[i] = i;

#if defined(TEXT_MODE)
	flatwallcolormap = ptrlut16colors;
#else
	flatwallcolormap = flatwallbright;
#endif

	flatwallmode = FLATWALL_AUTO;
	flatwallrows = 0;
}

void R_SetupFlatWall(void)
{
	// the walls are painted row by row with the flat span renderer of
	// the mode, when the mode has one and the walls of the setup menu
	// are set to the row renderer. The modes of the MDA and of the
	// VT100 have no flat span renderer: their walls are outlines of
	// the character cells, drawn by their own column renderer.
	flatwallrows = flatwallspanfunc && flatwallmode != FLATWALL_COLS;
}

//
// Paints the span of the row y of the sweep, which ends at the column
// x2, with the color of the spans open at the last column.
//
static void R_FlatWallSweepSpan(flatwallsweep_t *s, int y, int x2)
{
	ds_x1 = s->start[y];
	ds_x2 = x2;
	ds_y = y;
	ds_flatcolor = s->color;

	flatwallspanfunc();
}

//
// Paints the spans of the rows y1 to y2 of the sweep, both included,
// all of them ending at the column x2. An empty range (y1 > y2) does
// nothing.
//
static void R_FlatWallSweepRows(flatwallsweep_t *s, int y1, int y2, int x2)
{
	int y;

	for (y = y1; y <= y2; y++)
		R_FlatWallSweepSpan(s, y, x2);
}

//
// Adds one column of the wall to the sweep: the wall of the column is
// the rows yl to yh, and a column with yl > yh is not visible.
//
static void R_FlatWallSweepColumnSweep(flatwallsweep_t *s, int x, int yl,
				       int yh)
{
	int lo = s->lo;
	int hi = s->hi;

	// the spans that finish here end at the previous column
	int end = x - 1;

	int y;

	if (dc_color != s->color)
	{
		// the spans of the wall never cross a change of the light
		// level: the spans open at the last column are painted
		// with the color of the columns that opened them, and the
		// spans of the columns from here take the new color
		R_FlatWallSweepRows(s, lo, hi, end);
		s->color = dc_color;
		lo = 1;
		hi = 0;
	}

	if (yl > yh)
	{
		// the column is not visible, it closes every span open
		R_FlatWallSweepRows(s, lo, hi, end);
		s->lo = 1;
		s->hi = 0;
		return;
	}

	if (lo <= hi)
	{
		// the rows above and the rows below the wall of this
		// column stop being reached, their spans end here
		R_FlatWallSweepRows(s, lo, yl - 1 < hi ? yl - 1 : hi, end);
		R_FlatWallSweepRows(s, yh + 1 > lo ? yh + 1 : lo, hi, end);
	}

	// the rows of this column out of the range of the open rows start
	// a span at this column, the rest of them keep the span they have
	// open, and the rows of the open range that this column does not
	// reach were closed above
	for (y = yl; y <= (yh < lo ? yh : lo - 1); y++)
		s->start[y] = x;

	for (y = (yl > hi + 1 ? yl : hi + 1); y <= yh; y++)
		s->start[y] = x;

	// every row of the wall of this column has a span open after it:
	// the rows that keep the span they had and the rows whose span
	// starts here, so the wall of this column is the range of the
	// open rows of the next one
	s->lo = yl;
	s->hi = yh;
}

//
// Records one column of a wall for the column renderers.
//
static void R_FlatWallRecordColumn(short *yl, short *yh, int x, int ylval,
				   int yhval)
{
	yl[x] = ylval;
	yh[x] = yhval;
	wallrowcol[x] = dc_color;
}

#if defined(MODE_MDA) || defined(MODE_VT100)
//
// The walls of the MDA are the outlines of the character cells: only
// the first and the last column of the wall draw the block of the
// wall, the inside of it is cleared. Row rendering does not fit this,
// as every row of cells would need its edges too.
//
static void R_DrawFlatWallTextMDA(int x1, int x2, const short *yl,
				  const short *yh)
{
	int x;

	for (x = x1; x <= x2; x++)
	{
		if (yl[x] > yh[x])
			continue;

		dc_x = x;
		dc_yl = yl[x];
		dc_yh = yh[x];

		if (x == x1 || x == x2)
			R_DrawLineColumnTextMDA();
		else
			R_DrawEmptyColumnTextMDA();
	}
}
#endif

//
// Paints the recorded columns of a tier with the column renderer of the
// mode, the fall back of the sweep when the walls are not painted row
// by row. The modes of the MDA and of the VT100 have their own column
// renderer of the walls, above.
//
#if !defined(MODE_MDA) && !defined(MODE_VT100)
static void R_DrawFlatWallColumns(int x1, int x2, const short *yl,
				  const short *yh)
{
	int x;

	for (x = x1; x <= x2; x++)
	{
		if (flatwallcolscale > 1 && (x & (flatwallcolscale - 1)))
			continue;

		if (yl[x] > yh[x])
			continue;

		dc_x = x;
		dc_yl = yl[x];
		dc_yh = yh[x];

		// the flat walls are lit per column, the flatter ones
		// keep the color the seg loop left in dc_color
		if (flatwalllit)
			dc_color = wallrowcol[x];

		colfunc();
	}
}
#endif

static void R_FlatWallDrawColumns(const short *yl, const short *yh, int x1,
				  int x2)
{
#if defined(MODE_MDA) || defined(MODE_VT100)
	R_DrawFlatWallTextMDA(x1, x2, yl, yh);
#else
	R_DrawFlatWallColumns(x1, x2, yl, yh);
#endif
}

void R_FlatWallStart(int lit)
{
	flatwalllit = lit;

	if (!flatwallrows)
		return;

	flatwallsweep.lo = 1;
	flatwallsweep.hi = 0;
	flatwallsweep.color = -1;

	flatwallsweep2.lo = 1;
	flatwallsweep2.hi = 0;
	flatwallsweep2.color = -1;

#if defined(TEXT_MODE)
	// the palette lookuptable can change between the levels
	flatwallcolormap = ptrlut16colors;
#endif

	// the span renderers translate the color of the rows with
	// ds_colormap, and the colors of the walls are already lit: the
	// walls are painted with the identity colormap of the mode
	ds_colormap = (lighttable_t *)flatwallcolormap;
}

void R_FlatWallColumn1(int x, int yl, int yh)
{
	if (flatwallrows)
		R_FlatWallSweepColumnSweep(&flatwallsweep, x, yl, yh);
	else
		R_FlatWallRecordColumn(wallrowyl, wallrowyh, x, yl, yh);
}

void R_FlatWallColumn2(int x, int yl, int yh)
{
	if (flatwallrows)
		R_FlatWallSweepColumnSweep(&flatwallsweep2, x, yl, yh);
	else
		R_FlatWallRecordColumn(wallrow2yl, wallrow2yh, x, yl, yh);
}

//
// Closes a tier of the wall: the spans left open by the last column
// end with it, at the column x2.
//
void R_FlatWallFinish1(int x1, int x2)
{
	if (flatwallrows)
	{
		R_FlatWallSweepRows(&flatwallsweep, flatwallsweep.lo,
				    flatwallsweep.hi, x2);
		flatwallsweep.lo = 1;
		flatwallsweep.hi = 0;
		return;
	}

	R_FlatWallDrawColumns(wallrowyl, wallrowyh, x1, x2);
}

void R_FlatWallFinish2(int x1, int x2)
{
	if (flatwallrows)
	{
		R_FlatWallSweepRows(&flatwallsweep2, flatwallsweep2.lo,
				    flatwallsweep2.hi, x2);
		flatwallsweep2.lo = 1;
		flatwallsweep2.hi = 0;
		return;
	}

	R_FlatWallDrawColumns(wallrow2yl, wallrow2yh, x1, x2);
}

//
// The masked mid walls of the flat modes (the windows, the fences and
// the like) are a single color per column as every other wall of those
// modes, so the rows paint them too, one tier for the whole wall.
//
// A masked texture is drawn with the posts of its columns, and the gaps
// of the posts must stay empty, which a row of one span would paint.
// The rows are exact for the textures whose columns are a single post
// of the whole height of the texture: their wall is one span per
// column, and the rows draw the same pixels as the posts with the same
// clipping of the seg. The other textures, and they are the majority,
// keep the column renderer.
//
// R_TextureIsSolidMasked of r_data.c answers that per texture, and
// caches the answer, as the posts of a texture never change.
//
int R_DrawMaskedWallRows(drawseg_t *ds,
			 int texnum,
			 int x1,
			 int x2,
			 byte color,
			 int lit)
{
	unsigned index;

	int height;
	int x;
	int yl;
	int yh;

	fixed_t scale;
	fixed_t topscreen;
	fixed_t bottomscreen;

	// the modes drawing the walls with columns, the modes without a
	// flat span renderer of their own and the modes of the MDA and
	// of the VT100 keep the columns
	if (!flatwallrows)
		return 0;

	// the textures with gaps between the posts of their columns keep
	// the columns too, the rows would paint the gaps
	if (!R_TextureIsSolidMasked(texnum))
		return 0;

	height = textureheight[texnum] >> FRACBITS;

	scale = ds->scale1 + (x1 - ds->x1) * ds->scalestep;

	R_FlatWallStart(lit);

	for (x = x1; x <= x2; x++)
	{
		if (maskedtexturecol[x] == MAXSHORT)
		{
			// the wall is completely covered by the opaque
			// walls, a column with no rows of wall
			yl = 1;
			yh = 0;
		}
		else
		{
			if (lit && !fixedcolormap)
			{
				index = scale >> LIGHTSCALESHIFT;
#if PIXEL_SCALING != 1
				index /= PIXEL_SCALING;
#endif
				if (index >= MAXLIGHTSCALE)
					index = MAXLIGHTSCALE - 1;

				dc_colormap = walllights[index];
				dc_color = dc_colormap[color];
			}

			// the rows of the wall of the column are the
			// rows of the post of the column, worked out as
			// both renderers of the columns work them out,
			// with the post of a whole texture. A post
			// clipped away whole, and the post of a column
			// behind the viewer, which stands upside down,
			// leave no rows.
			topscreen = centeryfrac
				    - FixedMulEDX(scale, dc_texturemid);

			bottomscreen = topscreen + scale * height;

			yh = (bottomscreen - 1) >> FRACBITS;

			if (yh >= mfloorclip[x])
				yh = mfloorclip[x] - 1;

			if (yh >= viewheight)
			{
				// a post ending below the bottom of the
				// view is skipped whole by the seg, its
				// top is not clipped into the view
				yl = 1;
				yh = 0;
			}
			else
			{
				yl = (topscreen + FRACUNIT - 1) >> FRACBITS;

				if (yl <= mceilingclip[x])
					yl = mceilingclip[x] + 1;

				if (yl > yh)
				{
					yl = 1;
					yh = 0;
				}
			}

			maskedtexturecol[x] = MAXSHORT;
		}

		R_FlatWallColumn1(x, yl, yh);

		scale += ds->scalestep;
	}

	R_FlatWallFinish1(x1, x2);

	return 1;
}
