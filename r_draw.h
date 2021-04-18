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
//	System specific interface stuff.
//

#ifndef __R_DRAW__
#define __R_DRAW__

#define FLATPIXELCOLOR 1850

extern lighttable_t *dc_colormap;
extern int dc_x;
extern int dc_yl;
extern int dc_yh;
extern fixed_t dc_iscale;
extern fixed_t dc_texturemid;

// first pixel in a column
extern byte *dc_source;

// The span blitting interface.
// Hook in assembler or system specific BLT
//  here.

void R_DrawColumn(void);
void R_DrawSkyFlat(void);
void R_DrawColumnLow(void);
void R_DrawSkyFlatLow(void);

// The Spectre/Invisibility effect.
void R_DrawFuzzColumn(void);
void R_DrawFuzzColumnSaturn(void);
void R_DrawFuzzColumnFast(void);

void R_DrawFuzzColumnLow(void);
void R_DrawFuzzColumnSaturnLow(void);
void R_DrawFuzzColumnFastLow(void);

void R_DrawFuzzColumnPotato(void);
void R_DrawFuzzColumnSaturnPotato(void);
void R_DrawFuzzColumnFastPotato(void);

void R_DrawColumnPotato(void);
void R_DrawSkyFlatPotato(void);

void R_DrawColumnText8050(void);
void R_DrawFuzzColumnText8050(void);
void R_DrawFuzzColumnSaturnText8050(void);
void R_DrawSkyFlatText8050(void);
void R_DrawSpanText8050(void);
void R_DrawFuzzColumnFastText8050(void);

void R_DrawColumnText8025(void);
void R_DrawSpanText8025(void);
void R_DrawSkyFlatText8025(void);
void R_DrawFuzzColumnText8025(void);
void R_DrawFuzzColumnFastText8025(void);
void R_DrawFuzzColumnSaturnText8025(void);

void R_DrawColumn_13h(void);
void R_DrawSpan_13h(void);
void R_DrawFuzzColumn_13h(void);

void R_VideoErase(unsigned ofs,int count);

extern int ds_y;
extern int ds_x1;
extern int ds_x2;

extern lighttable_t *ds_colormap;

extern fixed_t ds_xfrac;
extern fixed_t ds_yfrac;
extern fixed_t ds_xstep;
extern fixed_t ds_ystep;

// start of a 64*64 tile image
extern byte *ds_source;

// Span blitting for rows, floor/ceiling.
// No Sepctre effect needed.
void R_DrawSpan(void);
void R_DrawSpanFlat (void);

// Low resolution mode, 160x200?
void R_DrawSpanLow(void);
void R_DrawSpanFlatLow(void);

void R_DrawSpanPotato(void);
void R_DrawSpanFlatPotato(void);

void R_DrawSpanFlatText8050(void);
void R_DrawSpanFlatText8025(void);

void R_InitBuffer(int width, int height);

// Rendering function.
void R_FillBackScreen(void);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder(void);

#endif
