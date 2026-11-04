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
// The wall tiers are recorded column by column by the seg loops and
// painted row by row here.
//
short wallrowyl[SCREENWIDTH];
short wallrowyh[SCREENWIDTH];
short wallrow2yl[SCREENWIDTH];
short wallrow2yh[SCREENWIDTH];
byte wallrowcol[SCREENWIDTH];

// the columns of the masked mid walls still to draw, of the drawseg
// being painted, and the lighting of the seg loop that draws it
extern short *maskedtexturecol;
extern lighttable_t **walllights;

void (*flatwallspanfunc)(void);

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

// flatwallmode comes from the wall rendering mode of the setup menu,
// see flatwallmode_t: FLATWALL_COLS only draws columns, FLATWALL_AUTO
// and FLATWALL_ROWS only draw rows.
int flatwallmode;

// The column renderers of the low resolution CGA modes are called for
// every flatwallcolscale column, as the screen update throws the other
// columns away. One by default.
int flatwallcolscale = 1;

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
}

//
// Paints one row of a wall. The color of the row and the colormap of
// the tier are already in the globals of the span renderer, set once
// per tier by R_FillFlatWallRows: every row of a tier shares them, and
// setting them per row costs more than the shortest rows are worth.
//
static void R_FlatWallRow(int x1, int x2, int y)
{
	if (x1 > x2)
		return;

	ds_x1 = x1;
	ds_x2 = x2;
	ds_y = y;

	flatwallspanfunc();
}

//
// Paints the rows of a tier that has a single color.
//
// The edges of a wall tier are straight lines, so yl[x] never grows and
// yh[x] never shrinks while x grows. Every row is then one contiguous
// span and both edges can be followed with two moving indexes, without
// scanning the columns of the row.
//
static void R_FillFlatWallRowsSolid(int x1,
				    int x2,
				    const short *yl,
				    const short *yh,
				    byte color)
{
	int x;
	int y;
	int ymin;
	int ymax;
	int rowtop;
	int rowbot;
	int rowstart;
	int rowend;
	int endl;
	int endh;
	int monoright;
	int monoleft;

	// skip the columns that are completely clipped away
	while (x1 <= x2 && yl[x1] > yh[x1])
		x1++;

	while (x2 >= x1 && yl[x2] > yh[x2])
		x2--;

	if (x1 > x2)
		return;

	ymin = 32767;
	ymax = -32768;
	monoright = 1;
	monoleft = 1;

	for (x = x1; x <= x2; x++)
	{
		if (yl[x] > yh[x])
		{
			// a hole in the middle, the rows are not contiguous
			monoright = 0;
			monoleft = 0;
			continue;
		}

		if (yl[x] < ymin)
			ymin = yl[x];

		if (yh[x] > ymax)
			ymax = yh[x];

		if (x > x1)
		{
			// the rows are suffixes when the top of the wall only
			// goes up and its bottom only goes down to the right
			if (yl[x] > yl[x - 1] || yh[x] < yh[x - 1])
				monoright = 0;

			// and the rows are prefixes the other way around
			if (yl[x] < yl[x - 1] || yh[x] > yh[x - 1])
				monoleft = 0;
		}
	}

	if (ymin > ymax)
		return;

	ds_flatcolor = color;

	if (monoright)
	{
		// the columns reaching a row from the top and from the bottom
		// are both suffixes of the segment, so a row is a single span.
		// Walking the rows from the bottom to the top, the top edge of
		// the wall only moves to the right and its bottom edge only
		// moves to the left, so each edge is followed by one index.
		rowtop = x1;
		rowbot = x2;

		for (y = ymax; y >= ymin; y--)
		{
			while (rowtop <= x2 && yl[rowtop] > y)
				rowtop++;

			while (rowbot > x1 && yh[rowbot - 1] >= y)
				rowbot--;

			rowstart = rowtop > rowbot ? rowtop : rowbot;

			if (rowstart <= x2)
				R_FlatWallRow(rowstart, x2, y);
		}

		return;
	}

	if (monoleft)
	{
		// the mirror of the case above: the columns that reach a row
		// are the first ones of the segment, so walking the rows from
		// the bottom of the wall to its top the end of the row only
		// moves to the left, one index for each of the two edges.
		endl = x2;
		endh = x1 - 1;

		for (y = ymax; y >= ymin; y--)
		{
			while (endl >= x1 && yl[endl] > y)
				endl--;

			while (endh + 1 <= x2 && yh[endh + 1] >= y)
				endh++;

			rowend = endl < endh ? endl : endh;

			if (rowend >= x1)
				R_FlatWallRow(x1, rowend, y);
		}

		return;
	}

	// general case, a row can be made of several spans
	for (y = ymin; y <= ymax; y++)
	{
		x = x1;

		while (x <= x2)
		{
			while (x <= x2 && (yl[x] > y || yh[x] < y))
				x++;

			if (x > x2)
				break;

			rowstart = x;

			while (x <= x2 && yl[x] <= y && yh[x] >= y)
				x++;

			R_FlatWallRow(rowstart, x - 1, y);
		}
	}
}

//
// Paints a wall tier. The columns of a tier can have different colors
// when the segment is long enough to cross several light levels, the
// columns are then grouped in runs of the same color.
//
void R_FillFlatWallRows(int x1,
			int x2,
			const short *yl,
			const short *yh,
			const byte *col)
{
	int x;
	int rowstart;
	byte color;

	if (x1 > x2)
		return;

#if defined(TEXT_MODE)
	// the palette lookuptable can change between the levels
	flatwallcolormap = ptrlut16colors;
#endif

	// the span renderers translate the color of the row with ds_colormap,
	// the whole tier with the same one
	ds_colormap = (lighttable_t *)flatwallcolormap;

	if (!col)
	{
		R_FillFlatWallRowsSolid(x1, x2, yl, yh, dc_color);
		return;
	}

	x = x1;

	while (x <= x2)
	{
		color = col[x];
		rowstart = x++;

		while (x <= x2 && col[x] == color)
			x++;

		R_FillFlatWallRowsSolid(rowstart, x - 1, yl, yh, color);
	}
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
// Paints a tier of a wall with rows, the entry point of the seg loops.
//
void R_DrawFlatWallTier(int x1,
			int x2,
			const short *yl,
			const short *yh,
			const byte *col)
{
#if defined(MODE_MDA) || defined(MODE_VT100)
	R_DrawFlatWallTextMDA(x1, x2, yl, yh);
#else
	// the flat wall modes draw the rows, the columns only mode (and the
	// modes without a flat span renderer) draw the columns
	if (flatwallmode != FLATWALL_COLS && flatwallspanfunc)
		R_FillFlatWallRows(x1, x2, yl, yh, col);
	else if (col)
		R_DrawFlatWallColumnsCol(x1, x2, yl, yh, col);
	else
		R_DrawFlatWallColumns(x1, x2, yl, yh);
#endif
}

//
// Paints a tier column by column, used when the rows are not cheaper.
//
void R_DrawFlatWallColumns(int x1, int x2, const short *yl, const short *yh)
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

		colfunc();
	}
}

void R_DrawFlatWallColumnsCol(int x1,
			      int x2,
			      const short *yl,
			      const short *yh,
			      const byte *col)
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
		dc_color = col[x];

		colfunc();
	}
}

//
// The masked mid walls of the flat modes (the windows, the fences and
// the like) are a single color per column as every other wall of those
// modes, so the rows can paint them too. Their seg loops record the
// wall of every column here and paint the tier with R_DrawFlatWallTier.
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
#if !defined(MODE_MDA) && !defined(MODE_VT100)
//
// Records one column of a masked mid wall of a texture without gaps:
// the wall of the column is the whole texture of the column, so it is
// the texture clipped by the masks of the drawseg, which is what the
// posts of the column renderer add up to.
//
// scale and topscreen are the position of the texture at the column x,
// height its height in pixels, topclip and botclip the masked clips of
// the drawseg and viewh the height of its view in pixels. The name of
// that argument is not viewheight, which sizeopt.h turns into a macro
// of a constant in the text modes.
//
// The span is the whole column of the texture, the first post of the
// column of the post loop of the seg, which covers the column.
//
static void R_RecordMaskedWallColumn(int x,
				     fixed_t scale,
				     fixed_t topscreen,
				     int height,
				     const short *topclip,
				     const short *botclip,
				     int viewh)
{
	int yl;
	int yh;

	// the walls behind the viewer have their posts upside down, the
	// bottom of the post above its top, and nothing is drawn
	if (scale <= 0)
	{
		R_FLATWALL_EMPTY(x);
		return;
	}

	yh = (topscreen + scale * height - 1) >> FRACBITS;

	if (yh > botclip[x] - 1)
		yh = botclip[x] - 1;

	// a post ending below the bottom of the view is skipped whole
	// by the seg, its top is not clipped into the view
	if (yh >= viewh)
	{
		R_FLATWALL_EMPTY(x);
		return;
	}

	yl = (topscreen + FRACUNIT - 1) >> FRACBITS;

	if (yl < topclip[x] + 1)
		yl = topclip[x] + 1;

	if (yl > yh)
	{
		R_FLATWALL_EMPTY(x);
		return;
	}

	R_FLATWALL_RECORD(x, yl, yh);
}
#endif

//
// Paints the masked mid wall of a seg loop of the flat walls with the
// rows, returning true when it did the whole wall and the caller can
// skip its column loop.
//
// ds is the drawseg of the wall and x1 to x2 the columns of it to
// paint, which the sprite splits can cut in pieces. color is the color
// of the texture and lit tells whether its columns are lit one by one
// (the flat walls) or all of them with the color the caller left in
// dc_color (the flatter walls).
//
// The clips, the texture position and the lighting globals are already
// set up by the caller, exactly as for its column loop.
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

	fixed_t scale;
	fixed_t scalestep;
	fixed_t topscreen;

#if defined(MODE_MDA) || defined(MODE_VT100)
	// the walls of those modes are outlines of the character cells,
	// drawn by their own renderer, the rows do not fit them
	(void)ds;
	(void)texnum;
	(void)x1;
	(void)x2;
	(void)color;
	(void)lit;
	return 0;
#else
	// the modes drawing the flat walls with columns, and the modes
	// without a flat span renderer of their own, keep the columns
	if (flatwallmode == FLATWALL_COLS || !flatwallspanfunc)
		return 0;

	// the textures with gaps between the posts of their columns keep
	// the columns too, the rows would paint the gaps
	if (!R_TextureIsSolidMasked(texnum))
		return 0;

	height = textureheight[texnum] >> FRACBITS;

	scale = ds->scale1 + (x1 - ds->x1) * ds->scalestep;
	scalestep = ds->scalestep;

	x = x1;

	while (x <= x2)
	{
		if (maskedtexturecol[x] == MAXSHORT)
		{
			// the wall is completely covered by the opaque walls
			R_FLATWALL_EMPTY(x);
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

			topscreen = centeryfrac
				    - FixedMulEDX(scale, dc_texturemid);

			R_RecordMaskedWallColumn(x, scale, topscreen, height,
						   mceilingclip, mfloorclip,
						   viewheight);

			maskedtexturecol[x] = MAXSHORT;
		}

		scale += scalestep;
		x++;
	}

	// the colors of the columns of the flat walls change with the
	// light levels the wall crosses, the flatter ones are one color
	if (lit)
		R_DrawFlatWallTier(x1, x2, wallrowyl, wallrowyh, wallrowcol);
	else
		R_DrawFlatWallTier(x1, x2, wallrowyl, wallrowyh, (byte *)0);

	return 1;
#endif
}
