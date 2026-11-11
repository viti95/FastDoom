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

#ifndef __R_FLATWALL__
#define __R_FLATWALL__

#include "options.h"
#include "r_defs.h"

/*
 * The walls of the flat modes have one color per column, so every row of a
 * wall is a run of one color and the wall is painted with the flat span
 * renderer of the video mode, one span per row.
 *
 * The seg loops keep walking their columns and hand each one to the sweep
 * of r_flatwall.c, which opens the span of a row at the first column
 * reaching it and paints it at the first column that stops reaching it. A
 * span never crosses a change of the color of the wall, which only the seg
 * loop knows, so it announces it with R_FlatWallColor.
 *
 * The modes with no flat span renderer of their own leave
 * flatwallspanfunc null (the MDA and the VT100, whose walls are outlines
 * of the character cells), and so do the modes of the setup menu that draw
 * the walls column by column: their columns go to the column renderer of
 * the mode.
 */

/* flat span renderer of the video mode, null when it has none */
extern void (*flatwallspanfunc)(void);

/*
 * Renderer of one column of the wall of the seg loops, of the first and of
 * the second tier. R_SetupFlatWall installs the sweep of the rows of the
 * tier when the walls are painted row by row, and the column renderer of
 * the mode when they are not. Both take the column from dc_x, dc_yl and
 * dc_yh, as the renderers of the mode do, and a column with dc_yl over
 * dc_yh has no wall.
 */
extern void (*flatwallcolumn1)(void);
extern void (*flatwallcolumn2)(void);

/* non zero when the walls of the view are painted row by row */
extern int flatwallrows;

/* non zero when the flat sky is painted row by row */
extern int flatskyrows;

/* how many columns of the screen one update covers: the composite modes
   of the CGA cards update every n-th column only, the rest are not painted */
extern int flatwallcolscale;

void R_InitFlatWall(void);

/* choose the renderer of the walls of the view, row by row or column by
   column. Called by R_ExecuteSetViewSize once the render functions of the
   video mode are set up */
void R_SetupFlatWall(void);

/* choose the renderer of the sky of the view, row by row or column by
   column, as R_SetupFlatWall does with the walls. The modes with no flat
   span renderer keep their column renderer of the sky */
void R_SetupFlatSky(void);

/* paint the sky plane pl row by row, one span per row: called by the flat
   sky renderers of r_plane.c when flatskyrows is set */
void R_DrawSkyFlatRows(visplane_t *pl);

/*
 * The wall of a segment has up to two tiers: the single sided line or the
 * top wall of a two sided one, and the bottom wall of the latter. The seg
 * loops of the flat walls open the wall of the segment, hand over every
 * column they walk and close it at the end. A column with yl over yh is not
 * visible: it closes the rows of the wall it does not reach.
 */

/* open the wall of a segment with the color the caller left in dc_color:
   the callers lighting per column change it with R_FlatWallColor */
void R_FlatWallStart(void);

/* the wall of the columns from x on has color: the spans the previous
   columns left open are painted with the color that opened them */
void R_FlatWallColor(int x, int color);

/* close the tiers of the wall of the segment: the spans the last column
   left open end at the column x2 */
void R_FlatWallFinish(int x2);

/*
 * The masked mid walls of the flat modes (the windows, the fences) are one
 * color per column as the rest of their walls, so the rows paint them too.
 *
 * A masked texture is drawn with several posts per column and the gaps
 * between them must stay empty, which a row of one span would paint: the
 * rows are only used for the textures whose columns are a single post of
 * their whole height (see R_TextureIsSolidMasked), the rest keep the
 * columns.
 */

/* paint the masked mid wall of the drawseg ds, from the column x1 to the
   column x2, of the texture texnum and the color color, row by row. lit
   tells whether its columns are lighted one by one. Returns 0 when the
   caller has to paint the columns itself. dc_texturemid, walllights,
   mfloorclip and mceilingclip must be set up as for its column loop */
int R_DrawMaskedWallRows(drawseg_t *ds,
			 int texnum,
			 int x1,
			 int x2,
			 byte color,
			 int lit);

#endif
