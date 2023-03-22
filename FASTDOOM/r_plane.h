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
//	Refresh, visplane stuff (floor, ceilings).
//

#ifndef __R_PLANE__
#define __R_PLANE__

#include "r_data.h"

// Visplane related.
extern short *lastopening;

extern short floorclip[SCREENWIDTH];
extern short ceilingclip[SCREENWIDTH];

extern fixed_t yslope[SCREENHEIGHT];
extern fixed_t distscale[SCREENWIDTH];

void R_ClearPlanes(void);

void R_MapPlane(int y, int x1);

void R_DrawPlanes(void);
void R_DrawPlanesflatVisplanes(void);
void R_DrawPlanesflatVisplanesLow(void);
void R_DrawPlanesflatVisplanesPotato(void);
void R_DrawPlanesflatVisplanesText80100(void);
void R_DrawPlanesflatVisplanesText8050(void);
void R_DrawPlanesflatVisplanesText8025(void);
void R_DrawPlanesflatVisplanesText4025(void);
void R_DrawPlanesflatVisplanesText4050(void);
void R_DrawPlanesflatVisplanesTextMDA(void);
void R_DrawPlanesflatVisplanes_13h(void);
void R_DrawPlanesflatVisplanesVBE2(void);
void R_DrawSky(visplane_t *pl);

visplane_t * R_FindPlane(fixed_t height, int picnum, int lightlevel);
visplane_t * R_CheckPlane(visplane_t *pl, int start, int stop);

#endif
