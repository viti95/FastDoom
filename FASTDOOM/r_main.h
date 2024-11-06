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

#ifndef __R_MAIN__
#define __R_MAIN__

#include "d_player.h"
#include "r_data.h"

//
// POV related.
//
extern fixed_t viewcos;
extern fixed_t viewsin;

extern int viewwidth;
extern int viewwidthhalf;
extern int viewwidthlimit;
extern int viewheight;
extern int viewheightminusone;
extern int viewheightshift;
extern int viewheightopt;
extern int viewheight32;
extern int viewwindowx;
extern int viewwindowy;

extern int centerx;
extern int centery;

extern fixed_t centerxfrac;
extern fixed_t centeryfrac;
extern fixed_t projection;

extern int validcount;

// When interpolation is enabled (uncappedFPS), this is the weight of the
// current frame. It is 1.0 (IE 0x10000) when the frame is not interpolated.
extern fixed_t interpolation_weight;
// The last frame render time in units of 1/560ths of a second
extern unsigned frametime_hrticks;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.
// Now why not 32 levels here?
#define LIGHTLEVELS 16
#define LIGHTSEGSHIFT 4

#define MAXLIGHTSCALE 48
#define LIGHTSCALESHIFT 12
#define MAXLIGHTZ 128
#define LIGHTZSHIFT 20

extern lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern lighttable_t *scalelightfixed[MAXLIGHTSCALE];
extern lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

extern int extralight;
extern lighttable_t *fixedcolormap;

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS 32

// Blocky/low detail mode.
// B remove this?
//  0 = high, 1 = low
extern int detailshift;

//
// Function pointers to switch refresh/drawing functions.
// Used to select shadow mode etc.
//
extern void (*colfunc)(void);
extern void (*basespritefunc)(void);
extern void (*fuzzcolfunc)(void);
// No shadow effects on floors.
extern void (*spanfunc)(void);
extern void (*skyfunc)(void);
extern void (*spritefunc)(void);
extern void (*pspritefunc)(void);
extern void (*basepspritefunc)(void);

extern void (*renderSegLoop)(void);
extern void (*renderMaskedSegRange)(drawseg_t *ds, int x1, int x2);
extern void (*renderMaskedSegRange2)(drawseg_t *ds);
extern void (*drawVisSprite)(vissprite_t *vis);
extern void (*mapPlane)(int y, int x1);

extern void (*drawPlayerSprite)(vissprite_t *vis);

extern void (*drawPlanes)(void);
extern void (*mapPlane)(int y, int x1);
extern void (*clearPlanes)(void);

extern void (*drawSky)(visplane_t *pl);

//
// Utility functions.
byte R_PointOnSegSide(fixed_t x,
                      fixed_t y,
                      seg_t *line);

angle_t
R_PointToAngle(fixed_t x,
               fixed_t y);

angle_t
R_PointToAngle2(fixed_t x1,
                fixed_t y1,
                fixed_t x2,
                fixed_t y2);

angle_t
R_PointToAngle00(fixed_t x2,
                 fixed_t y2);

fixed_t
R_PointToDist(fixed_t x,
              fixed_t y);

fixed_t R_ScaleFromGlobalAngle(int position);

subsector_t *
R_PointInSubsector(fixed_t x,
                   fixed_t y);

void R_ExecuteSetViewSize(void);

//
// REFRESH - the actual rendering functions.
//

// Called by G_Drawer.
void R_RenderPlayerView(void);

// Called by startup code.
void R_Init(void);

// Called by M_Responder.
void R_SetViewSize(int blocks, int detail);

void R_PatchCenteryPlanar(void);
void R_PatchCenteryPlanarKN(void);
void R_PatchCenteryPlanarDirect(void);

void R_PatchLinearHigh(void);
void R_PatchLinearLow(void);
void R_PatchLinearPotato(void);

void R_PatchCenteryLinearHighKN(void);
void R_PatchCenteryLinearLowKN(void);

void R_PatchCenteryLinearDirect(void);
void R_PatchCenteryLinearLowDirect(void);
void R_PatchCenteryLinearPotatoDirect(void);

void R_PatchColumnofsHigh386SX(void);
void R_PatchColumnofsLow386SX(void);
void R_PatchColumnofsPotato386SX(void);

void R_PatchCenteryVBE2High(void);
void R_PatchCenteryVBE2Low(void);
void R_PatchCenteryVBE2Potato(void);

void R_PatchCenteryVBE2Direct(void);
void R_PatchCenteryVBE2LowDirect(void);
void R_PatchCenteryVBE2PotatoDirect(void);

void R_PatchFuzzColumn(void);

void R_PatchFuzzColumnLinearHigh(void);
void R_PatchFuzzColumnLinearLow(void);
void R_PatchFuzzColumnLinearPotato(void);

void R_PatchFuzzColumnHighVBE2(void);
void R_PatchFuzzColumnLowVBE2(void);
void R_PatchFuzzColumnPotatoVBE2(void);
#endif
