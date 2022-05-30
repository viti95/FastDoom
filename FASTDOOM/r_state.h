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
//	Refresh/render internal state variables (global).
//

#ifndef __R_STATE__
#define __R_STATE__

// Need data structure definitions.
#include "d_player.h"
#include "r_data.h"

//
// Refresh internal data structures,
//  for rendering.
//

// needed for texture pegging
extern fixed_t *textureheight;

// needed for pre rendering (fracs)
extern fixed_t *spritewidth;

extern fixed_t *spriteoffset;
extern fixed_t *spritetopoffset;

extern lighttable_t datacolormaps[34 * 256 + 255];
extern lighttable_t *colormaps;

extern int viewwidth;
extern int viewwidthhalf;
extern int viewwidthlimit;
extern int scaledviewwidth;
extern int viewheight;
extern int viewheightshift;
extern int viewheightopt;
extern int viewheight32;

extern int firstflat;

// for global animation
extern int *flattranslation;
extern int *texturetranslation;

// Sprite....
extern int firstspritelump;
extern int lastspritelump;
extern int numspritelumps;

//
// Lookup tables for map data.
//
extern spritedef_t sprites[NUMSPRITES];

extern int numvertexes;
extern vertex_t *vertexes;

extern seg_t *segs;

extern int numsectors;
extern sector_t *sectors;

extern subsector_t *subsectors;

extern int firstnode;
extern node_t *nodes;

extern int numlines;
extern line_t *lines;

extern int numsides;
extern side_t *sides;

//
// POV data.
//
extern fixed_t viewx;
extern fixed_t viewxs;
extern fixed_t viewy;
extern fixed_t viewys;
extern fixed_t viewz;

extern angle_t viewangle;

// ?
extern angle_t clipangle;
extern angle_t fieldofview;

extern int viewangletox[FINEANGLES / 2];
extern angle_t xtoviewangle[SCREENWIDTH + 1];

extern fixed_t rw_distance;
extern angle_t rw_normalangle;

// angle to line origin
extern int rw_angle1;

extern visplane_t *floorplane;
extern visplane_t *ceilingplane;

#endif
