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
// walk to the renderer of the column of the view, which the setup of the
// view installs once: the sweep of the rows, which paints the spans of
// the rows that the column closes, when the walls are painted row by
// row, or the column renderer of the mode when they are not. Both take
// the column from dc_x, dc_yl and dc_yh, as the renderers of the mode
// do, so that the seg loop does the same work for every column of its
// wall in either of them. A column painted by the mode takes the color
// the seg loop has for it at that moment, which is the color of the
// wall of that column.
//
#if defined(MODE_MDA) || defined(MODE_VT100)
// the edges of the wall of the seg: of the MDA and of the VT100 the
// walls are outlines of the character cells, drawn only at the edges
//
extern int rw_x;
extern int rw_stopx;

static int flatwallfirstx;
static int flatwalllastx;
#endif

// the columns of the masked mid walls still to draw, of the drawseg
// being painted, and the lighting of the seg loop that draws it
extern short *maskedtexturecol;
extern lighttable_t **walllights;

void (*flatwallspanfunc)(void);

// the renderer of one column of the wall of the seg loops of the flat
// walls, of the first and of the second tier of it, installed by
// R_SetupFlatWall with the other renderers of the view
void (*flatwallcolumn1)(void);
void (*flatwallcolumn2)(void);

// the renderers of one column of the wall of the seg loops, installed
// by R_SetupFlatWall: the sweep of the rows of a tier, and the column
// renderers of the modes of the walls left to the columns
static void R_FlatWallColumnRows1(void);
static void R_FlatWallColumnRows2(void);
static void R_FlatWallColumnCols(void);
#if !defined(MODE_MDA) && !defined(MODE_VT100)
static void R_FlatWallColumnColsStride(void);
#endif
#if defined(MODE_MDA) || defined(MODE_VT100)
static void R_FlatWallColumnColsMDA(void);
#endif

int flatwallrows;
int flatskyrows;
int flatwallcolscale = 1;

//
// The span renderers translate the color with ds_colormap, while the
// flat wall column renderers write dc_color directly (they already
// applied the light level), so the walls are painted through an
// identity colormap to get exactly the same pixels.
//
// The text modes take the 16 color attribute lookuptable of the mode,
// which is what their flat wall column renderers use, and so need no
// identity map of their own.
//
// The span renderers of the planar modes read the color of the row with
// a lookup that overwrites the low byte of the pointer of the colormap
// with the color, as the light tables of those modes all start at a
// multiple of 256. The table of the identity is then only read right
// when it starts at a multiple of 256 too, which the compiler does not
// do for an array: the array is then 255 bytes longer than the table,
// and the table starts at the multiple of 256 inside the array (see
// I_ProcessPalette of i_cgacvb.c, which aligns the lookuptable of the
// 16 color modes in the same way).
//
#if !defined(TEXT_MODE)
static byte flatwallbright[256 + 255];
static byte *flatwallbrightmap;
#endif

#if defined(TEXT_MODE)
//
// The sky of the text modes is drawn with a fixed attribute of the
// cells, and not with a color of the palette translated by the
// lookuptable of the palette, which changes with the gamma. The map of
// the sky of the rows is then the identity, with the attribute of the
// cells of the sky where the color of the flat sky, so the row renderer
// of the mode writes that attribute for the sky.
//
// It must start at a multiple of 256 as the light tables of the modes,
// for the same reason.
//
static byte flatwallskyattr[256 + 255];
static byte *flatwallskyattrmap;
#endif // TEXT_MODE

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

	// color of the spans open at the last column, the color of the
	// wall of the columns: a span never crosses a change of it, and
	// the caller of the sweep knows when the color changes and hands
	// the new one over with R_FlatWallColor.
	int color;
} flatwallsweep_t;

static flatwallsweep_t flatwallsweep;
static flatwallsweep_t flatwallsweep2;

void R_InitFlatWall(void)
{
	int i;

#if defined(TEXT_MODE)
	// the map of the sky of the rows of the text modes, at a multiple
	// of 256
	flatwallskyattrmap = (byte *)(((int)flatwallskyattr + 255) & ~0xff);
	for (i = 0; i < 256; i++)
		flatwallskyattrmap[i] = i;
	flatwallskyattrmap[SKYFLATCOLOR] = SKYTEXTATTR;
#else
	// the identity table of the walls, at a multiple of 256
	flatwallbrightmap = (byte *)(((int)flatwallbright + 255) & ~0xff);

	for (i = 0; i < 256; i++)
		flatwallbrightmap[i] = i;
#endif

	flatwallrows = 0;
	flatskyrows = 0;

	// the columns of the walls go to the column renderer of the mode,
	// until R_SetupFlatWall chooses between the two ways of painting
	// them
	flatwallcolumn1 = R_FlatWallColumnCols;
	flatwallcolumn2 = R_FlatWallColumnCols;
}

void R_SetupFlatWall(void)
{
	// the walls are painted row by row with the flat span renderer of
	// the mode, when the mode has one and the walls of the setup menu
	// are set to the rendering of the rows. The MDA has no flat span
	// renderer, and the walls of the MDA and of the VT100 are outlines
	// of the character cells, drawn by the two renderers of the cells of
	// the mode: those modes never take the rows.
	flatwallrows = flatwallspanfunc
		&& (wallRender == WALL_FLAT_ROWS || wallRender == WALL_FLATTER_ROWS);

	// the renderer of the column of the walls: the sweep of the rows
	// when they are painted row by row, the column renderer of the mode
	// when they are not. Chosen once for the view, so that the seg loops
	// of the walls pay nothing for the renderer they do not use.
	if (flatwallrows)
	{
		flatwallcolumn1 = R_FlatWallColumnRows1;
		flatwallcolumn2 = R_FlatWallColumnRows2;
	}
	else
	{
#if defined(MODE_MDA) || defined(MODE_VT100)
		flatwallcolumn1 = R_FlatWallColumnColsMDA;
		flatwallcolumn2 = R_FlatWallColumnColsMDA;
#else
		if (flatwallcolscale > 1)
		{
			flatwallcolumn1 = R_FlatWallColumnColsStride;
			flatwallcolumn2 = R_FlatWallColumnColsStride;
		}
		else
		{
			flatwallcolumn1 = R_FlatWallColumnCols;
			flatwallcolumn2 = R_FlatWallColumnCols;
		}
#endif
	}
}

void R_SetupFlatSky(void)
{
	// the sky is painted row by row when it is set to the flat sky with
	// the row renderer and the mode has a flat span renderer. The mode
	// of the MDA has none: the sky of it is drawn by its own column
	// renderer, as its walls.
	flatskyrows = skyRender == SKY_FLAT_ROWS && flatwallspanfunc;
}

//
// Paints the spans of the rows y1 to y2 of the sweep, both included, all
// of them ending at the column end, with the color of the spans open at
// the last column. An empty range (y1 > y2) does nothing.
//
// What is the same for every row of the flush, the renderer of the
// mode, the column where the spans end and the color, is taken once.
//
static void R_FlatWallSweepRows(flatwallsweep_t *s, int y1, int y2, int end)
{
	short *start;
	void (*spanfunc)(void);
	int y;

	if (y1 > y2)
		return;

	start = s->start;
	spanfunc = flatwallspanfunc;

	ds_x2 = end;
	ds_flatcolor = s->color;

	for (y = y1; y <= y2; y++)
	{
		ds_x1 = start[y];
		ds_y = y;

		spanfunc();
	}
}

//
// Adds one column of the wall to the sweep: the wall of the column is
// the rows yl to yh, and a column with yl > yh is not visible. The
// spans that the column closes end at the previous column, and the
// spans left open by the last column of the wall end with it, which
// the caller of the sweep of the rows paints.
//
// The color of the wall does not enter here: the caller tells the sweep
// of its changes with R_FlatWallColor.
//
static void R_FlatWallSweepColumn(flatwallsweep_t *s, int x, int yl, int yh)
{
	short *start = s->start;
	int lo = s->lo;
	int hi = s->hi;

	// the spans that finish here end at the previous column
	int end = x - 1;

	int y;

	if (yl > yh)
	{
		// the column is not visible, it closes every span open. A
		// sweep with nothing open is already in the state the
		// closing would leave, and it has nothing to paint.
		if (lo <= hi)
		{
			R_FlatWallSweepRows(s, lo, hi, end);
			s->lo = 1;
			s->hi = 0;
		}
		return;
	}

	if (lo <= hi)
	{
		// the rows above and the rows below the wall of this column
		// stop being reached, their spans end here. A column whose
		// wall is inside the open range closes none of them.
		if (yl > lo)
		{
			y = yl - 1;
			R_FlatWallSweepRows(s, lo, y < hi ? y : hi, end);
		}

		if (yh < hi)
		{
			y = yh + 1;
			R_FlatWallSweepRows(s, y > lo ? y : lo, hi, end);
		}
	}

	// the rows of this column out of the range of the open rows start
	// a span at this column, the rest of them keep the span they have
	// open, and the rows of the open range that this column does not
	// reach were closed above
	for (y = yl; y < lo && y <= yh; y++)
		start[y] = x;

	for (y = yh; y > hi && y >= yl; y--)
		start[y] = x;

	// every row of the wall of this column has a span open after it:
	// the rows that keep the span they had and the rows whose span
	// starts here, so the wall of this column is the range of the
	// open rows of the next one
	s->lo = yl;
	s->hi = yh;
}

//
// The column of the sweep of the rows of the first and of the second
// tier of the wall of the seg, taking it from dc_x, dc_yl and dc_yh as
// the column renderers of the modes do. A column with dc_yl over dc_yh
// has no wall, and it is handed to the sweep, which closes with it the
// rows of the wall it does not reach.
//
static void R_FlatWallColumnRows1(void)
{
	R_FlatWallSweepColumn(&flatwallsweep, dc_x, dc_yl, dc_yh);
}

static void R_FlatWallColumnRows2(void)
{
	R_FlatWallSweepColumn(&flatwallsweep2, dc_x, dc_yl, dc_yh);
}

//
// The color of the wall of the columns from x on is color: the spans the
// columns before left open are painted with the color that opened them,
// and the spans of the columns from here take the new one. Both tiers
// take it at once: a tier without a wall of its own has nothing open.
//
void R_FlatWallColor(int x, int color)
{
	if (!flatwallrows)
		return;

	// the spans open at the last column end with it
	R_FlatWallSweepRows(&flatwallsweep, flatwallsweep.lo,
			    flatwallsweep.hi, x - 1);
	flatwallsweep.lo = 1;
	flatwallsweep.hi = 0;
	flatwallsweep.color = color;

	R_FlatWallSweepRows(&flatwallsweep2, flatwallsweep2.lo,
			    flatwallsweep2.hi, x - 1);
	flatwallsweep2.lo = 1;
	flatwallsweep2.hi = 0;
	flatwallsweep2.color = color;
}

//
// The renderers of one column of the walls that are not painted row by
// row, installed in flatwallcolumn1 and flatwallcolumn2 by
// R_SetupFlatWall. The seg loop keeps dc_color at the color of the wall
// of the column of the moment, which is the color painted here, whether
// the light changed with this column or not.
//
// A column with dc_yl over dc_yh is not visible and paints nothing, as
// the column renderers of the modes of the walls left to the columns do.
//
static void R_FlatWallColumnCols(void)
{
	if (dc_yl > dc_yh)
		return;

	colfunc();
}

#if !defined(MODE_MDA) && !defined(MODE_VT100)
// the modes of the CGA composite cards update only every
// flatwallcolscale-th column of the screen and leave the rest of them
// alone, so the columns between them are not painted
static void R_FlatWallColumnColsStride(void)
{
	if (dc_yl > dc_yh)
		return;

	if (flatwallcolscale > 1 && (dc_x & (flatwallcolscale - 1)))
		return;

	colfunc();
}
#endif

#if defined(MODE_MDA) || defined(MODE_VT100)
// the walls of the MDA and of the VT100 are the outlines of the
// character cells: only the first and the last column of the wall draw
// the block of it, the inside of it is cleared. Row rendering does not
// fit this, as every row of cells would need its edges too.
static void R_FlatWallColumnColsMDA(void)
{
	if (dc_yl > dc_yh)
		return;

	if (dc_x == flatwallfirstx || dc_x == flatwalllastx)
		R_DrawLineColumnTextMDA();
	else
		R_DrawEmptyColumnTextMDA();
}
#endif

void R_FlatWallStart(void)
{
#if defined(MODE_MDA) || defined(MODE_VT100)
	// the edges of the wall of the segment, for the outlines of the
	// character cells
	flatwallfirstx = rw_x;
	flatwalllastx = rw_stopx - 1;
#endif

	if (!flatwallrows)
		return;

	flatwallsweep.lo = 1;
	flatwallsweep.hi = 0;

	// the color of the wall is what the caller left in dc_color,
	// until the caller says otherwise with R_FlatWallColor
	flatwallsweep.color = dc_color;

	flatwallsweep2.lo = 1;
	flatwallsweep2.hi = 0;
	flatwallsweep2.color = dc_color;

	// the span renderers translate the color of the rows with
	// ds_colormap, and the colors of the walls are already lit: the
	// walls are painted through a map that gives the color back. The
	// text modes take the lookuptable of their palette, which can
	// change between the levels and so is read once per segment
#if defined(TEXT_MODE)
	ds_colormap = (lighttable_t *)ptrlut16colors;
#else
	ds_colormap = (lighttable_t *)flatwallbrightmap;
#endif
}

//
// Closes the wall of the segment: the spans the last column left open
// end with it, at the column x2.
//
// Both tiers are closed together, whichever of them the seg has: a tier
// without a wall of its own has nothing open.
//
void R_FlatWallFinish(int x2)
{
	if (!flatwallrows)
		return;

	R_FlatWallSweepRows(&flatwallsweep, flatwallsweep.lo,
			    flatwallsweep.hi, x2);
	flatwallsweep.lo = 1;
	flatwallsweep.hi = 0;

	R_FlatWallSweepRows(&flatwallsweep2, flatwallsweep2.lo,
			    flatwallsweep2.hi, x2);
	flatwallsweep2.lo = 1;
	flatwallsweep2.hi = 0;
}

//
// The sky of the flat sky modes is one color, and the sky of every
// column of the sky plane is a range of rows: the sky of a row is then
// one span of that color, so the same sweep of the rows of the walls
// paints the plane, with one span per row of the sky.
//
// The column renderers of the sky pay the work of a column per column
// of the sky of the view: the planar ones set the map mask of the plane
// of the pixel of the column on every column, and the others walk the
// rows of one column at a time. The sweep walks every column once to
// open and to close its rows, and paints one span per row, which is the
// work of the rows of the plane and no more.
//
void R_DrawSkyFlatRows(visplane_t *pl)
{
	// the sweep of the rows of the walls: the sky is painted by the
	// renderers of the planes, once the seg loops of the view are
	// done, so no wall has a span open here, and the sky opens and
	// closes its own rows below
	flatwallsweep_t *s = &flatwallsweep;

	// the last row of the view, the sky of the plane is not drawn
	// below it
	int last = viewheight - 1;

	int x;

	int yl;
	int yh;

	dc_color = SKYFLATCOLOR;

#if defined(TEXT_MODE)
	// the sky of the text modes keeps its fixed attribute of the
	// cells: the map of the sky gives it for the color of the flat sky
	ds_colormap = (lighttable_t *)flatwallskyattrmap;
#else
	// the span renderers translate the color of the rows with
	// ds_colormap, and the color of the sky is a color of the palette
	// to write as it is: the sky is painted with the identity colormap
	// of the walls
	ds_colormap = (lighttable_t *)flatwallbrightmap;
#endif

	s->lo = 1;
	s->hi = 0;

	// the sky is one color for all its columns: the sweep takes it
	// once, and no column changes it
	s->color = dc_color;

	// a column of the plane without sky has top over bottom, and the
	// columns the plane does not reach keep the highest pixel of them,
	// a marker out of the rows of the view: the sky of such a column
	// is not visible, as the wall of a column that is not visible, and
	// it closes the spans that are open
	for (x = pl->minx; x <= pl->maxx; x++)
	{
		yl = pl->top[x];
		yh = pl->bottom[x];

		if (yh > last)
			yh = last;

		if (yl > yh)
			yl = 1, yh = 0;

		R_FlatWallSweepColumn(s, x, yl, yh);
	}

	// the spans of the rows the last column of the plane reaches end
	// with that column
	R_FlatWallSweepRows(s, s->lo, s->hi, pl->maxx);
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
	// the sweep of the rows of the wall, kept in a variable of the
	// function
	flatwallsweep_t *s = &flatwallsweep;

	unsigned index;

	// the color of the wall of the columns walked, and the color of
	// the column of the moment, to know when the light of the seg
	// changes the color of a span
	byte wallcolor;
	byte newcolor;

	int height;
	int x;
	int yl;
	int yh;

	fixed_t scale;
	fixed_t topscreen;
	fixed_t bottomscreen;

	// the modes drawing the walls with columns, which are the modes
	// without a flat span renderer of their own, keep the columns
	if (!flatwallrows)
		return 0;

	// the textures with gaps between the posts of their columns keep
	// the columns too, the rows would paint the gaps
	if (!R_TextureIsSolidMasked(texnum))
		return 0;

	height = textureheight[texnum] >> FRACBITS;

	scale = ds->scale1 + (x1 - ds->x1) * ds->scalestep;

	R_FlatWallStart();

	// the wall takes the color the caller left in dc_color, until the
	// light of a column gives it another one
	wallcolor = dc_color;

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

				// the renderers of the flat walls write the
				// color themselves: only the color of the
				// column is needed, not the colormap of it
				newcolor = walllights[index][color];

				// a span never crosses a change of the color
				// of the wall: the spans the columns before
				// this one left open are painted with the
				// color that opened them, and only then does
				// the wall take the new one
				if (newcolor != wallcolor)
				{
					wallcolor = newcolor;
					dc_color = newcolor;

					R_FlatWallColor(x, newcolor);
				}
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

		R_FlatWallSweepColumn(s, x, yl, yh);

		scale += ds->scalestep;
	}

	R_FlatWallFinish(x2);

	return 1;
}
