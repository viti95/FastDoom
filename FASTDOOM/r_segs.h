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
//	Refresh module, drawing LineSegs from BSP.
//

#ifndef __R_SEGS__
#define __R_SEGS__

void R_RenderMaskedSegRange(drawseg_t *ds, int x1, int x2);
void R_RenderMaskedSegRange2(drawseg_t *ds);

extern void R_RenderSegLoop(void);
extern void R_RenderSegLoopFlat(void);
extern void R_RenderSegLoopFlatter(void);

extern void R_RenderMaskedSegRange(drawseg_t *ds, int x1, int x2);
extern void R_RenderMaskedSegRangeFlat(drawseg_t *ds, int x1, int x2);
extern void R_RenderMaskedSegRangeFlatter(drawseg_t *ds, int x1, int x2);

extern void R_RenderMaskedSegRange2(drawseg_t *ds);
extern void R_RenderMaskedSegRange2Flat(drawseg_t *ds);
extern void R_RenderMaskedSegRange2Flatter(drawseg_t *ds);


#endif
