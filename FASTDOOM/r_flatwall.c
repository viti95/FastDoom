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

// flatwallmodel is set by the video mode setup and selects the cost
// model of the row renderer, see the table below.
int flatwallmodel;

// flatwallmode comes from the wall rendering mode of the setup menu,
// see flatwallmode_t: FLATWALL_COLS only draws columns, FLATWALL_AUTO lets the
// cost model decide and FLATWALL_ROWS only draws rows.
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

	flatwallmodel = FLATWALL_MODEL_HIGH;
	flatwallmode = FLATWALL_AUTO;
}

static void R_FlatWallRow(int x1, int x2, int y, byte color)
{
	if (x1 > x2)
		return;

	ds_x1 = x1;
	ds_x2 = x2;
	ds_y = y;
	ds_colormap = (lighttable_t *)flatwallcolormap;
	ds_flatcolor = color;

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
	int monotonic;

	// skip the columns that are completely clipped away
	while (x1 <= x2 && yl[x1] > yh[x1])
		x1++;

	while (x2 >= x1 && yl[x2] > yh[x2])
		x2--;

	if (x1 > x2)
		return;

	ymin = 32767;
	ymax = -32768;
	monotonic = 1;

	for (x = x1; x <= x2; x++)
	{
		if (yl[x] > yh[x])
		{
			// a hole in the middle, the rows are not contiguous
			monotonic = 0;
			continue;
		}

		if (yl[x] < ymin)
			ymin = yl[x];

		if (yh[x] > ymax)
			ymax = yh[x];

		if (x > x1 && (yl[x] > yl[x - 1] || yh[x] < yh[x - 1]))
			monotonic = 0;
	}

	if (ymin > ymax)
		return;

	if (monotonic)
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
				R_FlatWallRow(rowstart, x2, y, color);
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

			R_FlatWallRow(rowstart, x - 1, y, color);
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
// Paints a tier of a wall with the cheapest way for it.
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
	if (R_FlatWallUseRows(x1, x2, yl, yh))
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
// Cost of the two ways of painting a wall, in eigths of a cycle:
//
// columns : width * colsetup + pixels * colpix
// rows    : height * (rowsetup * 8 + width * rowpix) + scan
//
// rowsetup / colsetup are the per row / per column setup costs,
// rowpix / colpix the cost of a single wall pixel and scan the cost of
// walking the tier (a fixed number of eigths of a cycle per column and
// per row).
//
// The costs depend on the video mode and on the rendering quality, as
// the pixel writes are of a different size and the span renderers use
// string operations.
//
static const int flatwallcosts[3][4] = {
#if defined(MODE_Y) || defined(MODE_X) || defined(MODE_Y_HALF)
	// planar VGA: the columns write one byte per pixel with a stride
	// of one plane row, the spans write two bytes at a time
	{30, 22, 8, 16},
	{30, 22, 4, 14},
	{26, 15, 6, 4}
#elif defined(MODE_VBE2_DIRECT)
	// VBE2 direct: 32 bit writes, the spans use rep stosl
	{24, 22, 4, 10},
	{24, 22, 3, 8},
	{22, 20, 2, 4}
#elif defined(TEXT_MODE)
#if defined(MODE_T4050) || defined(MODE_T8025)
	// interlaced text modes, the spans have to read the cell first
	{26, 25, 24, 12},
	{26, 25, 24, 12},
	{26, 25, 24, 12}
#else
	{24, 22, 8, 12},
	{24, 22, 8, 12},
	{24, 22, 8, 12}
#endif
#else
	// linear backbuffer modes
	{22, 20, 6, 10},
	{22, 20, 4, 8},
	{20, 18, 3, 4}
#endif
};

// Fixed cost of walking the tier, in eigths of a cycle per column and
// per row.
#define FLATWALL_SCAN 24

int R_FlatWallUseRows(int x1, int x2, const short *yl, const short *yh)
{
	const int *cost;
	int x;
	int ymin;
	int ymax;
	int pixels;
	int columns;
	int width;
	int height;

	if (flatwallmode != FLATWALL_AUTO)
		return flatwallmode == FLATWALL_ROWS && flatwallspanfunc;

	// the modes without a flat span renderer always use columns
	if (!flatwallspanfunc)
		return 0;

	width = x2 - x1 + 1;

	if (width < 4)
		return 0;

	ymin = 32767;
	ymax = -32768;
	pixels = 0;

	for (x = x1; x <= x2; x++)
	{
		if (yl[x] > yh[x])
			continue;

		if (yl[x] < ymin)
			ymin = yl[x];

		if (yh[x] > ymax)
			ymax = yh[x];

		pixels += yh[x] - yl[x] + 1;
	}

	if (ymax < ymin)
		return 0;

	height = ymax - ymin + 1;

	cost = flatwallcosts[flatwallmodel];

	// the cheap columns of the modes that skip columns
	pixels /= flatwallcolscale;
	columns = width / flatwallcolscale;

	return (height * (8 * cost[0] + width * cost[2])
	    + FLATWALL_SCAN * (width + height)
	    < pixels * cost[3] + columns * (8 * cost[1]));
}
