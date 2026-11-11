/*
 * Copyright (C) 1993-1996 Id Software, Inc.
 * Copyright (C) 2016-2017 Alexey Khokholov (Nuke.YKT)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *	Row (span) rendering of the flat modes: the walls and the sky.
 */

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

#if defined(MODE_MDA) || defined(MODE_VT100)
/* the walls of the MDA and of the VT100 are outlines of the character
   cells, drawn only at the edges of the wall of the seg */
extern int rw_x;
extern int rw_stopx;

static int flatwallfirstx;
static int flatwalllastx;
#endif

/* the columns of the masked mid walls still to draw, of the drawseg being
   painted, and the lighting of the seg loop drawing it */
extern short *maskedtexturecol;
extern lighttable_t **walllights;

void (*flatwallspanfunc)(void);

void (*flatwallcolumn1)(void);
void (*flatwallcolumn2)(void);

/* the renderers installed by R_SetupFlatWall: the sweep of the rows of a
   tier, or the column renderer of the mode, with its version skipping the
   columns the screen update of the mode does not paint */
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

/*
 * The span renderers translate the color with ds_colormap and the flat wall
 * column renderers write dc_color, already lit, so the walls are painted
 * through an identity map to get the same pixels. The text modes take the
 * lookuptable of the 16 colors of the mode, which their column renderers
 * use, and need no identity map.
 *
 * The span renderers of the planar modes read the color of the row by
 * overwriting the low byte of the pointer of the colormap, as the light
 * tables of those modes all start at a multiple of 256. The identity map
 * has to start at a multiple of 256 too, which the compiler does not do
 * for an array: the table is then aligned inside an array 255 bytes longer
 * (see I_ProcessPalette of i_cgacvb.c, which aligns the lookuptable of the
 * 16 color modes the same way).
 */
#if !defined(TEXT_MODE)
static byte flatwallbright[256 + 255];
static byte *flatwallbrightmap;
#endif

#if defined(TEXT_MODE)
/*
 * The sky of the text modes is a fixed attribute of the cells, not a color
 * of the palette translated by the lookuptable, which changes with the
 * gamma. The map of the sky of the rows is then the identity with the
 * attribute of the cells of the sky where the color of the flat sky, so
 * that the row renderer writes that attribute. It has to start at a
 * multiple of 256 as the light tables of the modes.
 */
static byte flatwallskyattr[256 + 255];
static byte *flatwallskyattrmap;
#endif /* TEXT_MODE */

/*
 * The sweep of the rows of one tier of a wall.
 *
 * The columns of the wall are walked from left to right and the wall of a
 * column is a range of rows: the span of a row opens at the first column
 * reaching it and is known to be finished at the first one that stops
 * reaching it, so it is painted then.
 *
 * The columns reaching a row are consecutive, as the edges of a wall are
 * straight lines and the clips of the seg only cut columns out, so the
 * rows with a span open are a range lo to hi, the wall of the last column
 * that was visible, and the rows of the wall of a column out of that range
 * open a span. Every column only walks the rows of its wall that open and
 * that close spans, one span per row of the wall.
 */
typedef struct
{
	/* column where the span of each row of the open range started. Rows
	   of the view, never over SCREENHEIGHT */
	short start[SCREENHEIGHT];

	int lo;
	int hi;

	/* color of the spans open at the last column, the color of the wall
	   of its columns: a span never crosses a change of it, which the
	   caller announces with R_FlatWallColor */
	int color;
} flatwallsweep_t;

static flatwallsweep_t flatwallsweep;
static flatwallsweep_t flatwallsweep2;

void R_InitFlatWall(void)
{
	int i;

#if defined(TEXT_MODE)
	flatwallskyattrmap = (byte *)(((int)flatwallskyattr + 255) & ~0xff);
	for (i = 0; i < 256; i++)
		flatwallskyattrmap[i] = i;
	flatwallskyattrmap[SKYFLATCOLOR] = SKYTEXTATTR;
#else
	flatwallbrightmap = (byte *)(((int)flatwallbright + 255) & ~0xff);

	for (i = 0; i < 256; i++)
		flatwallbrightmap[i] = i;
#endif

	flatwallrows = 0;
	flatskyrows = 0;

	/* the columns of the walls go to the column renderer of the mode
	   until R_SetupFlatWall chooses */
	flatwallcolumn1 = R_FlatWallColumnCols;
	flatwallcolumn2 = R_FlatWallColumnCols;
}

void R_SetupFlatWall(void)
{
	flatwallrows = flatwallspanfunc
		&& (wallRender == WALL_FLAT_ROWS || wallRender == WALL_FLATTER_ROWS);

	/* chosen once for the view, so that the seg loops of the walls pay
	   nothing for the renderer they do not use */
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
	/* the MDA has no flat span renderer: its sky is drawn by its own
	   column renderer */
	flatskyrows = skyRender == SKY_FLAT_ROWS && flatwallspanfunc;
}

/*
 * Paints the spans of the rows from y1 to y2, both included, all of them
 * ending at the column end, with the color of the spans open at the last
 * column. An empty range (y1 over y2) does nothing. What is the same for
 * every row of the flush, the renderer of the mode, the column where the
 * spans end and the color, is taken once.
 */
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

/*
 * Adds one column of the wall to the sweep: its wall is the rows yl to yh,
 * and a column with yl over yh is not visible. The spans the column closes
 * end at the previous column. The color of the wall does not enter here,
 * the caller announces its changes with R_FlatWallColor.
 */
static void R_FlatWallSweepColumn(flatwallsweep_t *s, int x, int yl, int yh)
{
	short *start = s->start;
	int lo = s->lo;
	int hi = s->hi;

	/* the spans ending here end at the previous column */
	int end = x - 1;

	int y;

	if (yl > yh)
	{
		/* the column is not visible: it closes every span open. A
		   sweep with nothing open is already closed, and has
		   nothing to paint */
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
		/* the rows above and below the wall of this column stop
		   being reached, their spans end here. A column whose wall
		   is inside the open range closes none of them */
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

	/* the rows of this column out of the range of the open rows start a
	   span here, the rest keep the span they had */
	for (y = yl; y < lo && y <= yh; y++)
		start[y] = x;

	for (y = yh; y > hi && y >= yl; y--)
		start[y] = x;

	/* every row of the wall of this column has a span open after it, so
	   the wall of the next column is the range of the open rows */
	s->lo = yl;
	s->hi = yh;
}

static void R_FlatWallColumnRows1(void)
{
	R_FlatWallSweepColumn(&flatwallsweep, dc_x, dc_yl, dc_yh);
}

static void R_FlatWallColumnRows2(void)
{
	R_FlatWallSweepColumn(&flatwallsweep2, dc_x, dc_yl, dc_yh);
}

void R_FlatWallColor(int x, int color)
{
	if (!flatwallrows)
		return;

	/* the spans open at the last column end with it */
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

/*
 * The renderers of one column of the walls that are not painted row by row.
 * The seg loop keeps dc_color at the color of the wall of the column of the
 * moment, which is the color painted here, whether the light changed with
 * this column or not.
 */
static void R_FlatWallColumnCols(void)
{
	if (dc_yl > dc_yh)
		return;

	colfunc();
}

#if !defined(MODE_MDA) && !defined(MODE_VT100)
/* the composite modes of the CGA cards update only every
   flatwallcolscale-th column of the screen, so the rest of them are not
   painted */
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
/* the walls of the MDA and of the VT100 are the outlines of the character
   cells: only the first and the last column of the wall draw the block, the
   inside is cleared. The rows do not fit this: every row of cells would
   need its edges too */
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
	/* the edges of the wall of the segment, for the outlines of the
	   character cells */
	flatwallfirstx = rw_x;
	flatwalllastx = rw_stopx - 1;
#endif

	if (!flatwallrows)
		return;

	flatwallsweep.lo = 1;
	flatwallsweep.hi = 0;

	flatwallsweep.color = dc_color;

	flatwallsweep2.lo = 1;
	flatwallsweep2.hi = 0;
	flatwallsweep2.color = dc_color;

	/* the text modes take the lookuptable of their palette, which can
	   change between the levels, so it is read once per segment */
#if defined(TEXT_MODE)
	ds_colormap = (lighttable_t *)ptrlut16colors;
#else
	ds_colormap = (lighttable_t *)flatwallbrightmap;
#endif
}

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

/*
 * The sky of the flat sky modes is one color and the sky of every column of
 * the sky plane is a range of rows, so the sky of a row is one span of that
 * color and the same sweep of the rows of the walls paints the plane.
 *
 * The column renderers of the sky pay a column per column of the sky of the
 * view, setting the map mask of the plane on each of them or walking its rows
 * one column at a time. The sweep paints one span per row of the plane.
 */
void R_DrawSkyFlatRows(visplane_t *pl)
{
	/* the sweep of the rows of the walls: the sky is painted by the
	   renderers of the planes, once the seg loops of the view are done,
	   so no wall has a span open here */
	flatwallsweep_t *s = &flatwallsweep;

	/* last row of the view, the sky of the plane is not drawn below it */
	int last = viewheight - 1;

	int x;

	int yl;
	int yh;

	dc_color = SKYFLATCOLOR;

#if defined(TEXT_MODE)
	ds_colormap = (lighttable_t *)flatwallskyattrmap;
#else
	ds_colormap = (lighttable_t *)flatwallbrightmap;
#endif

	s->lo = 1;
	s->hi = 0;

	/* one color for all the columns: no column changes it */
	s->color = dc_color;

	/* a column of the plane without sky has top over bottom, as the
	   columns the plane does not reach keep their marker of them: its sky
	   is not visible, as the wall of a column that is not visible, and it
	   closes the spans that are open */
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

	/* the spans of the rows the last column of the plane reaches end
	   with that column */
	R_FlatWallSweepRows(s, s->lo, s->hi, pl->maxx);
}

int R_DrawMaskedWallRows(drawseg_t *ds,
			 int texnum,
			 int x1,
			 int x2,
			 byte color,
			 int lit)
{
	flatwallsweep_t *s = &flatwallsweep;

	unsigned index;

	/* the color of the wall of the columns walked and of the column of
	   the moment: it marks when the light changes the color of a span */
	byte wallcolor;
	byte newcolor;

	int height;
	int x;
	int yl;
	int yh;

	fixed_t scale;
	fixed_t topscreen;
	fixed_t bottomscreen;

	if (!flatwallrows)
		return 0;

	if (!R_TextureIsSolidMasked(texnum))
		return 0;

	height = textureheight[texnum] >> FRACBITS;

	scale = ds->scale1 + (x1 - ds->x1) * ds->scalestep;

	R_FlatWallStart();

	/* the wall starts with the color of the caller */
	wallcolor = dc_color;

	for (x = x1; x <= x2; x++)
	{
		if (maskedtexturecol[x] == MAXSHORT)
		{
			/* the wall is completely covered by the opaque
			   walls: a column with no rows of wall */
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

				/* the renderers of the flat walls write the color themselves:
				   only the color is needed, not the colormap of it */
				newcolor = walllights[index][color];

				/* a span never crosses a change of the color: the spans of the
				   columns before take the color that opened them */
				if (newcolor != wallcolor)
				{
					wallcolor = newcolor;
					dc_color = newcolor;

					R_FlatWallColor(x, newcolor);
				}
			}

			/* the rows of the wall of the column are the rows of its post,
			   worked out as the renderers of the columns work them. A post
			   clipped away whole, and a post of a column behind the viewer,
			   standing upside down, leave no rows */
			topscreen = centeryfrac
				    - FixedMulEDX(scale, dc_texturemid);

			bottomscreen = topscreen + scale * height;

			yh = (bottomscreen - 1) >> FRACBITS;

			if (yh >= mfloorclip[x])
				yh = mfloorclip[x] - 1;

			if (yh >= viewheight)
			{
				/* a post ending below the bottom of the view is
				   skipped whole by the seg: its top is not clipped
				   into the view */
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
