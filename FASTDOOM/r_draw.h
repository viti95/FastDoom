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
extern byte dc_color;

extern int columnofs[SCREENWIDTH];

#if defined(USE_BACKBUFFER)
extern byte *ylookup[SCREENHEIGHT];
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF)
extern byte *ylookup[SCREENHEIGHT];
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_VBE2_DIRECT) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
extern byte **ylookup;
#endif

// The span blitting interface.
// Hook in assembler or system specific BLT
//  here.

void R_DrawColumnFlat(void);
void R_DrawColumnFlatLow(void);
void R_DrawColumnFlatPotato(void);

void R_DrawColumnPlaneFlat(void);
void R_DrawColumnPlaneFlatLow(void);

void R_DrawColumnBackbufferFlat(void);
void R_DrawColumnLowBackbufferFlat(void);
void R_DrawColumnPotatoBackbufferFlat(void);

void R_DrawColumnVBE2Flat(void);
void R_DrawColumnLowVBE2Flat(void);
void R_DrawColumnPotatoVBE2Flat(void);

void R_DrawColumnFastLEA(void);
void R_DrawColumn(void);
void R_DrawColumnDirect(void);
void R_DrawColumnLowFastLEA(void);
void R_DrawColumnLow(void);
void R_DrawColumnLowDirect(void);

void R_DrawColumnBackbufferRoll(void);

void R_DrawColumnBackbufferMMX(void);
void R_DrawSpanBackbufferMMX(void);

void R_DrawColumnSkyFullDirect(void);
void R_DrawColumnLowSkyFullDirect(void);
void R_DrawColumnPotatoSkyFullDirect(void);

void R_DrawColumnBackbufferSkyFullDirect(void);
void R_DrawColumnLowBackbufferSkyFullDirect(void);
void R_DrawColumnPotatoBackbufferSkyFullDirect(void);

void R_DrawColumnVBE2SkyFullDirect(void);
void R_DrawColumnLowVBE2SkyFullDirect(void);
void R_DrawColumnPotatoVBE2SkyFullDirect(void);

// The Spectre/Invisibility effect.
void R_DrawFuzzColumn(void);
void R_DrawFuzzColumnSaturn(void);
void R_DrawFuzzColumnFlatSaturn(void);
void R_DrawFuzzColumnFlat(void);
void R_DrawFuzzColumnTrans(void);

void R_DrawFuzzColumnLow(void);
void R_DrawFuzzColumnSaturnLow(void);
void R_DrawFuzzColumnFlatSaturnLow(void);
void R_DrawFuzzColumnFlatLow(void);
void R_DrawFuzzColumnTransLow(void);

void R_DrawFuzzColumnPotato(void);
void R_DrawFuzzColumnSaturnPotato(void);
void R_DrawFuzzColumnFlatSaturnPotato(void);
void R_DrawFuzzColumnFlatPotato(void);
void R_DrawFuzzColumnTransPotato(void);

void R_DrawColumnPotatoFastLEA(void);
void R_DrawColumnPotato(void);
void R_DrawColumnPotatoDirect(void);

void R_DrawColumnText8050(void);
void R_DrawColumnText8050Flat(void);
void R_DrawFuzzColumnText8050(void);
void R_DrawFuzzColumnSaturnText8050(void);
void R_DrawFuzzColumnFlatSaturnText8050(void);
void R_DrawSkyFlatText8050(void);
void R_DrawSpanText8050(void);
void R_DrawFuzzColumnFlatText8050(void);
void R_DrawFuzzColumnTransText8050(void);

void R_DrawLineColumnTextMDA(void);
void R_DrawEmptyColumnTextMDA(void);
void R_DrawSpanTextMDA(void);
void R_DrawSkyTextMDA(void);
void R_DrawSpriteTextMDA(void);

void R_DrawColumnText8025(void);
void R_DrawColumnText8025Flat(void);
void R_DrawSpanText8025(void);
void R_DrawSkyFlatText8025(void);
void R_DrawFuzzColumnText8025(void);
void R_DrawFuzzColumnFlatText8025(void);
void R_DrawFuzzColumnSaturnText8025(void);
void R_DrawFuzzColumnFlatSaturnText8025(void);
void R_DrawFuzzColumnTransText8025(void);

void R_DrawColumnText4025(void);
void R_DrawColumnText4025Flat(void);
void R_DrawSpanText4025(void);
void R_DrawSkyFlatText4025(void);
void R_DrawFuzzColumnText4025(void);
void R_DrawFuzzColumnFlatText4025(void);
void R_DrawFuzzColumnSaturnText4025(void);
void R_DrawFuzzColumnFlatSaturnText4025(void);
void R_DrawFuzzColumnTransText4025(void);

void R_DrawColumnText4050(void);
void R_DrawColumnText4050Flat(void);
void R_DrawSpanText4050(void);
void R_DrawSkyFlatText4050(void);
void R_DrawFuzzColumnText4050(void);
void R_DrawFuzzColumnFlatText4050(void);
void R_DrawFuzzColumnSaturnText4050(void);
void R_DrawFuzzColumnFlatSaturnText4050(void);
void R_DrawFuzzColumnTransText4050(void);

void R_DrawColumnBackbuffer(void);
void R_DrawColumnLowBackbuffer(void);
void R_DrawColumnPotatoBackbuffer(void);
void R_DrawColumnBackbufferFastLEA(void);
void R_DrawColumnLowBackbufferFastLEA(void);
void R_DrawColumnPotatoBackbufferFastLEA(void);
void R_DrawSpanBackbuffer(void);
void R_DrawSpanLowBackbuffer(void);
void R_DrawSpanPotatoBackbuffer(void);
void R_DrawSpanBackbufferPentium(void);
void R_DrawSpanLowBackbufferPentium(void);
void R_DrawSpanPotatoBackbufferPentium(void);
void R_DrawFuzzColumnBackbuffer(void);
void R_DrawFuzzColumnLowBackbuffer(void);
void R_DrawFuzzColumnPotatoBackbuffer(void);
void R_DrawFuzzColumnFlatBackbuffer(void);
void R_DrawFuzzColumnFlatLowBackbuffer(void);
void R_DrawFuzzColumnFlatPotatoBackbuffer(void);
void R_DrawSkyFlatBackbuffer(void);
void R_DrawSkyFlatLowBackbuffer(void);
void R_DrawSkyFlatPotatoBackbuffer(void);
void R_DrawSpanFlatBackbuffer(void);
void R_DrawSpanFlatLowBackbuffer(void);
void R_DrawSpanFlatPotatoBackbuffer(void);
void R_DrawFuzzColumnSaturnBackbuffer(void);
void R_DrawFuzzColumnSaturnLowBackbuffer(void);
void R_DrawFuzzColumnSaturnPotatoBackbuffer(void);
void R_DrawFuzzColumnFlatSaturnBackbuffer(void);
void R_DrawFuzzColumnFlatSaturnLowBackbuffer(void);
void R_DrawFuzzColumnFlatSaturnPotatoBackbuffer(void);
void R_DrawFuzzColumnTransBackbuffer(void);
void R_DrawFuzzColumnTransLowBackbuffer(void);
void R_DrawFuzzColumnTransPotatoBackbuffer(void);

void R_DrawColumnBackbufferDirect(void);
void R_DrawColumnLowBackbufferDirect(void);
void R_DrawColumnPotatoBackbufferDirect(void);

void R_DrawColumnVBE2MMX(void);
void R_DrawColumnVBE2(void);
void R_DrawColumnLowVBE2(void);
void R_DrawColumnPotatoVBE2(void);
void R_DrawSpanVBE2MMX(void);
void R_DrawSpanVBE2(void);
void R_DrawSpanLowVBE2(void);
void R_DrawSpanPotatoVBE2(void);
void R_DrawSpanVBE2Pentium(void);
void R_DrawSpanLowVBE2Pentium(void);
void R_DrawSpanPotatoVBE2Pentium(void);
void R_DrawSkyFlatVBE2(void);
void R_DrawSkyFlatLowVBE2(void);
void R_DrawSkyFlatPotatoVBE2(void);
void R_DrawFuzzColumnFlatVBE2(void);
void R_DrawFuzzColumnFlatLowVBE2(void);
void R_DrawFuzzColumnFlatPotatoVBE2(void);
void R_DrawFuzzColumnSaturnVBE2(void);
void R_DrawFuzzColumnSaturnLowVBE2(void);
void R_DrawFuzzColumnSaturnPotatoVBE2(void);
void R_DrawFuzzColumnFlatSaturnVBE2(void);
void R_DrawFuzzColumnFlatSaturnLowVBE2(void);
void R_DrawFuzzColumnFlatSaturnPotatoVBE2(void);
void R_DrawFuzzColumnVBE2(void);
void R_DrawFuzzColumnLowVBE2(void);
void R_DrawFuzzColumnPotatoVBE2(void);
void R_DrawFuzzColumnTransVBE2(void);
void R_DrawFuzzColumnTransLowVBE2(void);
void R_DrawFuzzColumnTransPotatoVBE2(void);

void R_DrawColumnVBE2Direct(void);
void R_DrawColumnLowVBE2Direct(void);
void R_DrawColumnPotatoVBE2Direct(void);

void R_VideoErase(unsigned ofs, int count);

extern int ds_y;
extern int ds_x1;
extern int ds_x2;

extern lighttable_t *ds_colormap;

extern fixed_t ds_frac;
extern fixed_t ds_step;

// start of a 64*64 tile image
extern byte *ds_source;

// Span blitting for rows, floor/ceiling.
// No Sepctre effect needed.
void R_DrawSpan(void);
void R_DrawSpanLow(void);
void R_DrawSpanPotato(void);
void R_DrawSpan386SX(void);
void R_DrawSpanLow386SX(void);
void R_DrawSpanPotato386SX(void);

void R_DrawSpanFlat(void);
void R_DrawSpanFlatLow(void);
void R_DrawSpanFlatPotato(void);

void R_DrawSpanFlatText80100(void);
void R_DrawSpanFlatText8050(void);
void R_DrawSpanFlatText8025(void);
void R_DrawSpanFlatText4025(void);
void R_DrawSpanFlatText4050(void);
void R_DrawSpanFlatTextMDA(void);

void R_DrawSpanFlatVBE2(void);
void R_DrawSpanFlatLowVBE2(void);
void R_DrawSpanFlatPotatoVBE2(void);

void R_InitBuffer(int width, int height);

// Rendering function.
void R_FillBackScreen(void);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder(void);

#endif
