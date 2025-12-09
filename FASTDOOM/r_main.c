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
//	Rendering main loop and setup functions,
//	 utility functions (BSP, geometry, trigonometry).
//	See tables.c, too.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include "options.h"
#include "doomdef.h"
#include "doomstat.h"
#include "d_net.h"
#include "i_debug.h"
#include "m_misc.h"

#include "r_local.h"

#include "std_func.h"

#include "sizeopt.h"

#include "m_menu.h"

#include "st_stuff.h"

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW 2048

#define SC_INDEX 0x3C4

// increment every time a check is made
int validcount = 1;

lighttable_t *fixedcolormap;
extern lighttable_t **walllights;

#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA) && !defined(MODE_COLOR_MDA)
int centerx;
int centery;

fixed_t centerxfrac;
fixed_t centeryfrac;
fixed_t centeryfracshifted;
fixed_t projection;
#endif

fixed_t viewx;
fixed_t viewxs;
fixed_t viewy;
fixed_t viewyneg;
fixed_t viewys;
fixed_t viewz;

angle_t viewangle;
angle_t viewangle90;

fixed_t viewcos;
fixed_t viewsin;

// 0 = high, 1 = low, 2 = potato
int detailshift;

fixed_t interpolation_weight = 0x10000;
unsigned frametime_hrticks = 17;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int viewangletox[FINEANGLES / 2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t xtoviewangle[SCREENWIDTH + 1];
angle_t xtoviewangle90[SCREENWIDTH + 1];
fixed_t sinextoviewangle90[SCREENWIDTH + 1];

const fixed_t *finecosine = &finesine[FINEANGLES / 4];

lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *scalelightfixed[MAXLIGHTSCALE];
lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

// bumped light from gun blasts
int extralight;

void (*colfunc)(void);
void (*basespritefunc)(void);
void (*fuzzcolfunc)(void);
void (*spanfunc)(void);
void (*skyfunc)(void);
void (*spritefunc)(void);
void (*pspritefunc)(void);
void (*basepspritefunc)(void);

byte R_PointOnSegSide(fixed_t x,
                      fixed_t y,
                      seg_t *line)
{
    fixed_t lx;
    fixed_t ly;
    fixed_t ldx;
    fixed_t ldy;
    fixed_t dx;
    fixed_t dy;
    fixed_t left;
    fixed_t right;

    lx = line->v1->x;
    ly = line->v1->y;

    ldx = line->v2->x - lx;
    ldy = line->v2->y - ly;

    if (!ldx)
    {
        return (x <= lx) ^ (ldy <= 0);
    }
    if (!ldy)
    {
        return (y <= ly) ^ (ldx >= 0);
    }

    dx = (x - lx);
    dy = (y - ly);

    // Try to quickly decide by looking at sign bits.
    if ((ldy ^ ldx ^ dx ^ dy) & 0x80000000)
        return ROLAND1(ldy ^ dx);

    left = FixedMulEDX(ldy >> FRACBITS, dx);
    right = FixedMulEDX(dy, ldx >> FRACBITS);

    // returns 0/1 front/back side
    return right >= left;
}

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table.

//

angle_t
R_PointToAngle(fixed_t x,
               fixed_t y)
{
    fixed_t tempDivision;

    x -= viewx;
    y -= viewy;

    if ((!x) && (!y))
        return 0;

    if (x >= 0)
    {
        // x >=0
        if (y >= 0)
        {
            // y>= 0
            if (x > y)
            {
                // octant 0
                if (x < 512)
                    return 536870912;
                else
                {
                    tempDivision = FixedDivDBITS(y, x);
                    if (tempDivision < SLOPERANGE)
                        return tantoangle[tempDivision];
                    else
                        return 536870912;
                }
            }
            else
            {
                // octant 1
                if (y < 512)
                    return ANG90 - 1 - 536870912;
                else
                {
                    tempDivision = FixedDivDBITS(x, y);
                    if (tempDivision < SLOPERANGE)
                        return ANG90 - 1 - tantoangle[tempDivision];
                    else
                        return ANG90 - 1 - 536870912;
                }
            }
        }
        else
        {
            // y<0
            y = -y;

            if (x > y)
            {
                // octant 8
                if (x < 512)
                    return -536870912;
                else
                {
                    tempDivision = FixedDivDBITS(y, x);
                    if (tempDivision < SLOPERANGE)
                        return -tantoangle[tempDivision];
                    else
                        return -536870912;
                }
            }
            else
            {
                // octant 7
                if (y < 512)
                    return ANG270 + 536870912;
                else
                {
                    tempDivision = FixedDivDBITS(x, y);
                    if (tempDivision < SLOPERANGE)
                        return ANG270 + tantoangle[tempDivision];
                    else
                        return ANG270 + 536870912;
                }
            }
        }
    }
    else
    {
        // x<0
        x = -x;

        if (y >= 0)
        {
            // y>= 0
            if (x > y)
            {
                // octant 3
                if (x < 512)
                    return ANG180 - 1 - 536870912;
                else
                {
                    tempDivision = FixedDivDBITS(y, x);
                    if (tempDivision < SLOPERANGE)
                        return ANG180 - 1 - tantoangle[tempDivision];
                    else
                        return ANG180 - 1 - 536870912;
                }
            }
            else
            {
                // octant 2
                if (y < 512)
                    return ANG90 + 536870912;
                else
                {
                    tempDivision = FixedDivDBITS(x, y);
                    if (tempDivision < SLOPERANGE)
                        return ANG90 + tantoangle[tempDivision];
                    else
                        return ANG90 + 536870912;
                };
            }
        }
        else
        {
            // y<0
            y = -y;

            if (x > y)
            {
                // octant 4
                if (x < 512)
                    return ANG180 + 536870912;
                else
                {
                    tempDivision = FixedDivDBITS(y, x);
                    if (tempDivision < SLOPERANGE)
                        return ANG180 + tantoangle[tempDivision];
                    else
                        return ANG180 + 536870912;
                }
            }
            else
            {
                // octant 5
                if (y < 512)
                    return ANG270 - 1 - 536870912;
                else
                {
                    tempDivision = FixedDivDBITS(x, y);
                    if (tempDivision < SLOPERANGE)
                        return ANG270 - 1 - tantoangle[tempDivision];
                    else
                        return ANG270 - 1 - 536870912;
                }
            }
        }
    }
    return 0;
}

angle_t
R_PointToAngle2(fixed_t x1,
                fixed_t y1,
                fixed_t x2,
                fixed_t y2)
{
    fixed_t tempDivision;

    x2 -= x1;
    y2 -= y1;

    if ((!x2) && (!y2))
        return 0;

    if (x2 >= 0)
    {
        // x >=0
        if (y2 >= 0)
        {
            // y>= 0
            if (x2 > y2)
            {
                // octant 0
                if (x2 < 512)
                    return 536870912;
                else
                {
                    tempDivision = (y2 << 3) / (x2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return tantoangle[tempDivision];
                    else
                        return 536870912;
                }
            }
            else
            {
                // octant 1
                if (y2 < 512)
                    return ANG90 - 1 - 536870912;
                else
                {
                    tempDivision = (x2 << 3) / (y2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG90 - 1 - tantoangle[tempDivision];
                    else
                        return ANG90 - 1 - 536870912;
                }
            }
        }
        else
        {
            // y<0
            y2 = -y2;

            if (x2 > y2)
            {
                // octant 8
                if (x2 < 512)
                    return -536870912;
                else
                {
                    tempDivision = (y2 << 3) / (x2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return -tantoangle[tempDivision];
                    else
                        return -536870912;
                }
            }
            else
            {
                // octant 7

                if (y2 < 512)
                    return ANG270 + 536870912;
                else
                {
                    tempDivision = (x2 << 3) / (y2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG270 + tantoangle[tempDivision];
                    else
                        return ANG270 + 536870912;
                }
            }
        }
    }
    else
    {
        // x<0
        x2 = -x2;

        if (y2 >= 0)
        {
            // y>= 0
            if (x2 > y2)
            {
                // octant 3
                if (x2 < 512)
                    return ANG180 - 1 - 536870912;
                else
                {
                    tempDivision = (y2 << 3) / (x2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG180 - 1 - tantoangle[tempDivision];
                    else
                        return ANG180 - 1 - 536870912;
                }
            }
            else
            {
                // octant 2

                if (y2 < 512)
                    return ANG90 + 536870912;
                else
                {
                    tempDivision = (x2 << 3) / (y2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG90 + tantoangle[tempDivision];
                    else
                        return ANG90 + 536870912;
                }
            }
        }
        else
        {
            // y<0
            y2 = -y2;

            if (x2 > y2)
            {
                // octant 4
                if (x2 < 512)
                    return ANG180 + 536870912;
                else
                {
                    tempDivision = (y2 << 3) / (x2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG180 + tantoangle[tempDivision];
                    else
                        return ANG180 + 536870912;
                }
            }
            else
            {
                // octant 5

                if (y2 < 512)
                    return ANG270 - 1 - 536870912;
                else
                {
                    tempDivision = (x2 << 3) / (y2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG270 - 1 - tantoangle[tempDivision];
                    else
                        return ANG270 - 1 - 536870912;
                }
            }
        }
    }
    return 0;
}

angle_t
R_PointToAngle00(fixed_t x2,
                 fixed_t y2)
{
    fixed_t tempDivision;

    if ((!x2) && (!y2))
        return 0;

    if (x2 >= 0)
    {
        // x >=0
        if (y2 >= 0)
        {
            // y>= 0
            if (x2 > y2)
            {
                // octant 0
                if (x2 < 512)
                    return 536870912;
                else
                {
                    tempDivision = (y2 << 3) / (x2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return tantoangle[tempDivision];
                    else
                        return 536870912;
                }
            }
            else
            {
                // octant 1
                if (y2 < 512)
                    return ANG90 - 1 - 536870912;
                else
                {
                    tempDivision = (x2 << 3) / (y2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG90 - 1 - tantoangle[tempDivision];
                    else
                        return ANG90 - 1 - 536870912;
                }
            }
        }
        else
        {
            // y<0
            y2 = -y2;

            if (x2 > y2)
            {
                // octant 8
                if (x2 < 512)
                    return -536870912;
                else
                {
                    tempDivision = (y2 << 3) / (x2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return -tantoangle[tempDivision];
                    else
                        return -536870912;
                }
            }
            else
            {
                // octant 7

                if (y2 < 512)
                    return ANG270 + 536870912;
                else
                {
                    tempDivision = (x2 << 3) / (y2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG270 + tantoangle[tempDivision];
                    else
                        return ANG270 + 536870912;
                }
            }
        }
    }
    else
    {
        // x<0
        x2 = -x2;

        if (y2 >= 0)
        {
            // y>= 0
            if (x2 > y2)
            {
                // octant 3
                if (x2 < 512)
                    return ANG180 - 1 - 536870912;
                else
                {
                    tempDivision = (y2 << 3) / (x2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG180 - 1 - tantoangle[tempDivision];
                    else
                        return ANG180 - 1 - 536870912;
                }
            }
            else
            {
                // octant 2

                if (y2 < 512)
                    return ANG90 + 536870912;
                else
                {
                    tempDivision = (x2 << 3) / (y2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG90 + tantoangle[tempDivision];
                    else
                        return ANG90 + 536870912;
                }
            }
        }
        else
        {
            // y<0
            y2 = -y2;

            if (x2 > y2)
            {
                // octant 4
                if (x2 < 512)
                    return ANG180 + 536870912;
                else
                {
                    tempDivision = (y2 << 3) / (x2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG180 + tantoangle[tempDivision];
                    else
                        return ANG180 + 536870912;
                }
            }
            else
            {
                // octant 5

                if (y2 < 512)
                    return ANG270 - 1 - 536870912;
                else
                {
                    tempDivision = (x2 << 3) / (y2 >> 8);
                    if (tempDivision < SLOPERANGE)
                        return ANG270 - 1 - tantoangle[tempDivision];
                    else
                        return ANG270 - 1 - 536870912;
                }
            }
        }
    }
    return 0;
}

fixed_t lutsineangle[SLOPERANGE + 1];

fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
    int angle;
    fixed_t dx;
    fixed_t dy;
    fixed_t temp;
    fixed_t dist;

    dx = abs(x - viewx);
    dy = abs(y - viewy);

    if (dy > dx)
    {
        fixed_t temp_var = dx;
        dx = dy;
        dy = temp_var;
    }

    if (dy >> 14 >= dx)
    {
        return 0;
    }
    else
    {
        temp = lutsineangle[FixedDivDBITS(dy, dx)];
        dist = ((dx >> 14) >= temp) ? ((dx ^ temp) >> 31) ^ MAXINT : FixedDiv2(dx, temp);
    }

    return dist;
}

//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
fixed_t R_ScaleFromGlobalAngle(int position)
{
    fixed_t scale;
    int angleb;
    int sinea;
    int sineb;
    fixed_t num;
    int den;

    // both sines are allways positive
    sinea = sinextoviewangle90[position];

    angleb = xtoviewangle90[position] + viewangle - rw_normalangle;
    angleb >>= ANGLETOFINESHIFT;

    sineb = finesine[angleb];
#if defined(MODE_T4050)
    num = FixedMulEDX(projection, sineb) << 1;
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    num = FixedMulEDX(projection, sineb) << detailshift;
#endif
#if defined(MODE_Y_HALF)
    num = FixedMulEDXHalf(projection, sineb) << detailshift;
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    num = FixedMulEDX(projection, sineb);
#endif
    den = FixedMulEDX(rw_distance, sinea);

    if (den > num >> 16)
    {
        scale = FixedDiv(num, den);

        // SCALE BETWEEN 256 AND 64 * FRACUNIT ???
        if (scale > 64 * FRACUNIT)
            scale = 64 * FRACUNIT;
        else if (scale < 256)
            scale = 256;
    }
    else
        scale = 64 * FRACUNIT;

    return scale;
}

//
// R_InitTextureMapping
//
void R_InitTextureMapping(void)
{
    int i;
    int x;
    int t;
    fixed_t focallength;

    // Use tangent table to generate viewangletox:
    //  viewangletox will give the next greatest x
    //  after the view angle.
    //
    // Calc focallength
    //  so FIELDOFVIEW angles covers SCREENWIDTH.
    focallength = FixedDiv(centerxfrac, finetangent[FINEANGLES / 4 + FIELDOFVIEW / 2]);

    for (i = 0; i < FINEANGLES / 2; i++)
    {
        if (finetangent[i] > FRACUNIT * 2)
            t = -1;
        else if (finetangent[i] < -FRACUNIT * 2)
            t = viewwidth + 1;
        else
        {
            t = FixedMul(finetangent[i], focallength);
            t = (centerxfrac - t + FRACUNIT - 1) >> FRACBITS;

            if (t < -1)
                t = -1;
            else if (t > viewwidth + 1)
                t = viewwidth + 1;
        }
        viewangletox[i] = t;
    }

    // Scan viewangletox[] to generate xtoviewangle[]:
    //  xtoviewangle will give the smallest view angle
    //  that maps to x.
    for (x = 0; x <= viewwidth; x++)
    {
        i = 0;
        while (viewangletox[i] > x)
            i++;
        xtoviewangle90[x] = (i << ANGLETOFINESHIFT);
        sinextoviewangle90[x] = finesine[xtoviewangle90[x] >> ANGLETOFINESHIFT];
        xtoviewangle[x] = xtoviewangle90[x] - ANG90;
    }

    // Take out the fencepost cases from viewangletox.
    for (i = 0; i < FINEANGLES / 2; i++)
    {
        if (viewangletox[i] == -1)
            viewangletox[i] = 0;
        else if (viewangletox[i] == viewwidth + 1)
            viewangletox[i] = viewwidth;
    }

    // Initialize LUT angle sine
    for (i = 0; i < SLOPERANGE + 1; i++)
    {
        int angle = (tantoangle[i] + ANG90) >> ANGLETOFINESHIFT;
        lutsineangle[i] = finesine[angle];
    }
}

//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP 2

void R_InitLightTables(void)
{
    int i;
    int j;
    int level;
    int startmap;
    int scale;

    // Calculate the light levels to use
    //  for each level / distance combination.
    for (i = 0; i < LIGHTLEVELS; i++)
    {
        startmap = ((LIGHTLEVELS - 1 - i) * 2) * NUMCOLORMAPS / LIGHTLEVELS;
        for (j = 0; j < MAXLIGHTZ; j++)
        {
            scale = FixedDiv((ORIGINAL_SCREENWIDTH / 2 * FRACUNIT), (j + 1) << LIGHTZSHIFT);
            scale >>= LIGHTSCALESHIFT;
            level = startmap - scale / DISTMAP;

            if (level < 0)
                level = 0;
            else if (level >= NUMCOLORMAPS)
                level = NUMCOLORMAPS - 1;

            zlight[i][j] = colormaps + level * 256;
        }
    }
}

//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
byte setsizeneeded;
int setblocks;
int setdetail;

void R_SetViewSize(int blocks, int detail)
{
    setsizeneeded = 1;
    setblocks = blocks;
    setdetail = detail;

    if (blocks == 11 && gamestate == GS_LEVEL)
    {
        ST_createWidgets_mini();
    }
    else
    {
        ST_createWidgets();
    }
}

void R_PatchCode(void)
{
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF)
    R_PatchCenteryPlanar();
    R_PatchCenteryPlanarKN();
    R_PatchCenteryPlanarDirect();
    R_PatchFuzzColumn();
#endif

#if defined(USE_BACKBUFFER)
    R_PatchLinearHigh();
    R_PatchLinearLow();
    R_PatchLinearPotato();

    R_PatchCenteryLinearHighKN();
    R_PatchCenteryLinearLowKN();

    R_PatchCenteryLinearDirect();
    R_PatchCenteryLinearLowDirect();
    R_PatchCenteryLinearPotatoDirect();

    R_PatchColumnofsHighPentium();
    R_PatchColumnofsLowPentium();
    R_PatchColumnofsPotatoPentium();

    R_PatchFuzzColumnLinearHigh();
    R_PatchFuzzColumnLinearLow();
    R_PatchFuzzColumnLinearPotato();
#endif

#if defined(MODE_VBE2_DIRECT)
    R_PatchCenteryVBE2High();
    R_PatchCenteryVBE2Low();
    R_PatchCenteryVBE2Potato();

    R_PatchCenteryVBE2Direct();
    R_PatchCenteryVBE2LowDirect();
    R_PatchCenteryVBE2PotatoDirect();

    R_PatchFuzzColumnHighVBE2();
    R_PatchFuzzColumnLowVBE2();
    R_PatchFuzzColumnPotatoVBE2();
#endif
}

//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize(void)
{
    fixed_t cosadj;
    fixed_t dy;
    int i;
    int j;
    int level;
    int startmap;

    setsizeneeded = 0;

    // TODO Add more granular detection
    if (selectedCPU == AUTO_CPU)
    {
        switch (I_GetCPUModel())
        {
        case 386:
            selectedCPU = INTEL_386DX;
            break;
        case 486:
        case 586:
            selectedCPU = INTEL_486;
            break;
        case 686:
            selectedCPU = INTEL_PENTIUM;
            break;
        default:
            selectedCPU = INTEL_486;
            break;
        }
    }
#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA) && !defined(MODE_COLOR_MDA)
    if (setblocks >= 11)
    {
        scaledviewwidth = SCREENWIDTH;
        viewheight = SCREENHEIGHT;
        viewheightminusone = SCREENHEIGHT - 1;
        viewheightshift = SCREENHEIGHT << FRACBITS;
        viewheightopt = (SCREENHEIGHT << FRACBITS) - SCREENHEIGHT;
        viewheight32 = SCREENHEIGHT << 16 | SCREENHEIGHT;
        automapheight = SCREENHEIGHT;
    }
    else
    {
        // Since (SCREENWDITH / 10) may have a remainder, check 10 explcitly
        if (setblocks == 10)
        {
            scaledviewwidth = SCREENWIDTH;
        }
        else
        {
            scaledviewwidth = setblocks * (SCREENWIDTH / 10);
            // Stay multiple of 4
            scaledviewwidth &= ~0x3;
        }

        if (setblocks == 10)
            viewheight = SCREENHEIGHT - SBARHEIGHT;
        else
            viewheight = ((setblocks * (SCREENHEIGHT - SBARHEIGHT)) / 10) & ~7;

        viewheightminusone = viewheight - 1;
        viewheightshift = viewheight << FRACBITS;
        viewheightopt = (viewheight << FRACBITS) - viewheight;
        viewheight32 = viewheight << 16 | viewheight;
        automapheight = SCREENHEIGHT - SBARHEIGHT;
    }
#endif

#if defined(MODE_13H) || defined(MODE_VBE2)
    endscreen = MulScreenWidth(viewwindowy + viewheight);
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    if (forcePotatoDetail || forceLowDetail || forceHighDetail)
    {
        if (forceHighDetail)
            detailshift = DETAIL_HIGH;
        else if (forceLowDetail)
            detailshift = DETAIL_LOW;
        else
            detailshift = DETAIL_POTATO;
    }
    else
        detailshift = setdetail;
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    viewwidth = scaledviewwidth >> detailshift;
    viewwidthhalf = viewwidth / 2;
#endif

#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA) && !defined(MODE_COLOR_MDA)
    viewwidthlimit = viewwidth - 1;
    centery = viewheight / 2;
    centerx = viewwidth / 2;
    centerxfrac = centerx << FRACBITS;
    centeryfrac = centery << FRACBITS;
    centeryfracshifted = centeryfrac >> 4;
    projection = centerxfrac;
#endif

    switch (wallRender)
    {
    case WALL_NORMAL:
        renderSegLoop = R_RenderSegLoop;
        renderMaskedSegRange = R_RenderMaskedSegRange;
        renderMaskedSegRange2 = R_RenderMaskedSegRange2;
        break;
    case WALL_FLAT:
        renderSegLoop = R_RenderSegLoopFlat;
        renderMaskedSegRange = R_RenderMaskedSegRangeFlat;
        renderMaskedSegRange2 = R_RenderMaskedSegRange2Flat;
        break;
    case WALL_FLATTER:
        renderSegLoop = R_RenderSegLoopFlatter;
        renderMaskedSegRange = R_RenderMaskedSegRangeFlatter;
        renderMaskedSegRange2 = R_RenderMaskedSegRange2Flatter;
        break;
    }

    switch (spriteRender)
    {
    case SPRITE_NORMAL:
        drawVisSprite = R_DrawVisSprite;
        break;
    case SPRITE_FLAT:
        drawVisSprite = R_DrawVisSpriteFlat;
        break;
    case SPRITE_FLATTER:
        drawVisSprite = R_DrawVisSpriteFlatter;
        break;
    }

    switch (pspriteRender)
    {
    case PSPRITE_NORMAL:
        drawPlayerSprite = R_DrawVisPSprite;
        break;
    case PSPRITE_FLAT:
        drawPlayerSprite = R_DrawVisPSpriteFlat;
        break;
    case PSPRITE_FLATTER:
        drawPlayerSprite = R_DrawVisPSpriteFlatter;
        break;
    }

#if defined(MODE_T4050)

    switch (wallRender)
    {
    case WALL_NORMAL:
        colfunc = R_DrawColumnText4050;
        break;
    case WALL_FLAT:
    case WALL_FLATTER:
        colfunc = R_DrawColumnText4050Flat;
        break;
    }

    switch (spriteRender)
    {
    case SPRITE_NORMAL:
        spritefunc = basespritefunc = R_DrawColumnText4050;
        break;
    case SPRITE_FLAT:
    case SPRITE_FLATTER:
        spritefunc = basespritefunc = R_DrawColumnText4050Flat;
        break;
    }

    switch (pspriteRender)
    {
    case SPRITE_NORMAL:
        pspritefunc = basepspritefunc = R_DrawColumnText4050;
        break;
    case SPRITE_FLAT:
    case SPRITE_FLATTER:
        pspritefunc = basepspritefunc = R_DrawColumnText4050Flat;
        break;
    }

    switch (visplaneRender)
    {
    case VISPLANES_NORMAL:
        drawPlanes = R_DrawPlanes;
        mapPlane = R_MapPlane;
        clearPlanes = R_ClearPlanes;
        spanfunc = R_DrawSpanText4050;
        break;
    case VISPLANES_FLAT:
        drawPlanes = R_DrawPlanes;
        mapPlane = R_MapPlaneFlat;
        clearPlanes = R_ClearPlanesFlat;
        spanfunc = R_DrawSpanFlatText4050;
        break;
    case VISPLANES_FLATTER:
        clearPlanes = R_ClearPlanesFlat;
        drawPlanes = R_DrawPlanesFlatterText4050;
        break;
    }

    if (flatSky)
    {
        drawSky = R_DrawSkyFlat;
        skyfunc = R_DrawSkyFlatText4050;
    }
    else
    {
        drawSky = R_DrawSky;
        skyfunc = R_DrawColumnText4050;
    }

    switch (invisibleRender)
    {
    case INVISIBLE_NORMAL:
        fuzzcolfunc = R_DrawFuzzColumnText4050;
        break;
    case INVISIBLE_FLAT:
        fuzzcolfunc = R_DrawFuzzColumnFlatText4050;
        break;
    case INVISIBLE_FLAT_SATURN:
        fuzzcolfunc = R_DrawFuzzColumnFlatSaturnText4050;
        break;
    case INVISIBLE_SATURN:
        fuzzcolfunc = R_DrawFuzzColumnSaturnText4050;
        break;
    case INVISIBLE_TRANSLUCENT:
        fuzzcolfunc = R_DrawFuzzColumnTransText4050;
        break;
    }

#endif
#if defined(MODE_T4025)

    switch (wallRender)
    {
    case WALL_NORMAL:
        colfunc = R_DrawColumnText4025;
        break;
    case WALL_FLAT:
    case WALL_FLATTER:
        colfunc = R_DrawColumnText4025Flat;
        break;
    }

    switch (spriteRender)
    {
    case SPRITE_NORMAL:
        spritefunc = basespritefunc = R_DrawColumnText4025;
        break;
    case SPRITE_FLAT:
    case SPRITE_FLATTER:
        spritefunc = basespritefunc = R_DrawColumnText4025Flat;
        break;
    }

    switch (pspriteRender)
    {
    case SPRITE_NORMAL:
        pspritefunc = basepspritefunc = R_DrawColumnText4025;
        break;
    case SPRITE_FLAT:
    case SPRITE_FLATTER:
        pspritefunc = basepspritefunc = R_DrawColumnText4025Flat;
        break;
    }

    switch (visplaneRender)
    {
    case VISPLANES_NORMAL:
        drawPlanes = R_DrawPlanes;
        mapPlane = R_MapPlane;
        clearPlanes = R_ClearPlanes;
        spanfunc = R_DrawSpanText4025;
        break;
    case VISPLANES_FLAT:
        drawPlanes = R_DrawPlanes;
        mapPlane = R_MapPlaneFlat;
        clearPlanes = R_ClearPlanesFlat;
        spanfunc = R_DrawSpanFlatText4025;
        break;
    case VISPLANES_FLATTER:
        clearPlanes = R_ClearPlanesFlat;
        drawPlanes = R_DrawPlanesFlatterText4025;
        break;
    }

    if (flatSky)
    {
        drawSky = R_DrawSkyFlat;
        skyfunc = R_DrawSkyFlatText4025;
    }
    else
    {
        drawSky = R_DrawSky;
        skyfunc = R_DrawColumnText4025;
    }

    switch (invisibleRender)
    {
    case INVISIBLE_NORMAL:
        fuzzcolfunc = R_DrawFuzzColumnText4025;
        break;
    case INVISIBLE_FLAT:
        fuzzcolfunc = R_DrawFuzzColumnFlatText4025;
        break;
    case INVISIBLE_FLAT_SATURN:
        fuzzcolfunc = R_DrawFuzzColumnFlatSaturnText4025;
        break;
    case INVISIBLE_SATURN:
        fuzzcolfunc = R_DrawFuzzColumnSaturnText4025;
        break;
    case INVISIBLE_TRANSLUCENT:
        fuzzcolfunc = R_DrawFuzzColumnTransText4025;
        break;
    }

#endif
#if defined(MODE_T8025) || defined(MODE_COLOR_MDA)
    switch (wallRender)
    {
    case WALL_NORMAL:
        colfunc = R_DrawColumnText8025;
        break;
    case WALL_FLAT:
    case WALL_FLATTER:
        colfunc = R_DrawColumnText8025Flat;
        break;
    }

    switch (spriteRender)
    {
    case SPRITE_NORMAL:
        spritefunc = basespritefunc = R_DrawColumnText8025;
        break;
    case SPRITE_FLAT:
    case SPRITE_FLATTER:
        spritefunc = basespritefunc = R_DrawColumnText8025Flat;
        break;
    }

    switch (pspriteRender)
    {
    case SPRITE_NORMAL:
        pspritefunc = basepspritefunc = R_DrawColumnText8025;
        break;
    case SPRITE_FLAT:
    case SPRITE_FLATTER:
        pspritefunc = basepspritefunc = R_DrawColumnText8025Flat;
        break;
    }

    switch (visplaneRender)
    {
    case VISPLANES_NORMAL:
        drawPlanes = R_DrawPlanes;
        mapPlane = R_MapPlane;
        clearPlanes = R_ClearPlanes;
        spanfunc = R_DrawSpanText8025;
        break;
    case VISPLANES_FLAT:
        drawPlanes = R_DrawPlanes;
        mapPlane = R_MapPlaneFlat;
        clearPlanes = R_ClearPlanesFlat;
        spanfunc = R_DrawSpanFlatText8025;
        break;
    case VISPLANES_FLATTER:
        clearPlanes = R_ClearPlanesFlat;
        drawPlanes = R_DrawPlanesFlatterText8025;
        break;
    }

    if (flatSky)
    {
        drawSky = R_DrawSkyFlat;
        skyfunc = R_DrawSkyFlatText8025;
    }
    else
    {
        drawSky = R_DrawSky;
        skyfunc = R_DrawColumnText8025;
    }

    switch (invisibleRender)
    {
    case INVISIBLE_NORMAL:
        fuzzcolfunc = R_DrawFuzzColumnText8025;
        break;
    case INVISIBLE_FLAT:
        fuzzcolfunc = R_DrawFuzzColumnFlatText8025;
        break;
    case INVISIBLE_FLAT_SATURN:
        fuzzcolfunc = R_DrawFuzzColumnFlatSaturnText8025;
        break;
    case INVISIBLE_SATURN:
        fuzzcolfunc = R_DrawFuzzColumnSaturnText8025;
        break;
    case INVISIBLE_TRANSLUCENT:
        fuzzcolfunc = R_DrawFuzzColumnTransText8025;
        break;
    }

#endif
#if defined(MODE_MDA)

    drawPlanes = R_DrawPlanesFlatterTextMDA;
    mapPlane = R_MapPlaneFlat;
    clearPlanes = R_ClearPlanesFlat;
    colfunc = spritefunc = pspritefunc = basespritefunc = basepspritefunc = R_DrawLineColumnTextMDA;

    spanfunc = R_DrawSpanTextMDA;

    drawSky = R_DrawSkyFlat;
    skyfunc = R_DrawSkyTextMDA;

    fuzzcolfunc = R_DrawLineColumnTextMDA;
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)

    switch (wallRender)
    {
    case WALL_NORMAL:
        colfunc = R_DrawColumnText8050;
        break;
    case WALL_FLAT:
    case WALL_FLATTER:
        colfunc = R_DrawColumnText8050Flat;
        break;
    }

    switch (spriteRender)
    {
    case SPRITE_NORMAL:
        spritefunc = basespritefunc = R_DrawColumnText8050;
        break;
    case SPRITE_FLAT:
    case SPRITE_FLATTER:
        spritefunc = basespritefunc = R_DrawColumnText8050Flat;
        break;
    }

    switch (pspriteRender)
    {
    case PSPRITE_NORMAL:
        pspritefunc = basepspritefunc = R_DrawColumnText8050;
        break;
    case PSPRITE_FLAT:
    case PSPRITE_FLATTER:
        pspritefunc = basepspritefunc = R_DrawColumnText8050Flat;
        break;
    }

    switch (visplaneRender)
    {
    case VISPLANES_NORMAL:
        drawPlanes = R_DrawPlanes;
        mapPlane = R_MapPlane;
        clearPlanes = R_ClearPlanes;
        spanfunc = R_DrawSpanText8050;
        break;
    case VISPLANES_FLAT:
        drawPlanes = R_DrawPlanes;
        mapPlane = R_MapPlaneFlat;
        clearPlanes = R_ClearPlanesFlat;
        spanfunc = R_DrawSpanFlatText8050;
        break;
    case VISPLANES_FLATTER:
        clearPlanes = R_ClearPlanesFlat;
        drawPlanes = R_DrawPlanesFlatterText8050;
        break;
    }

    if (flatSky)
    {
        drawSky = R_DrawSkyFlat;
        skyfunc = R_DrawSkyFlatText8050;
    }
    else
    {
        drawSky = R_DrawSky;
        skyfunc = R_DrawColumnText8050;
    }

    switch (invisibleRender)
    {
    case INVISIBLE_NORMAL:
        fuzzcolfunc = R_DrawFuzzColumnText8050;
        break;
    case INVISIBLE_FLAT:
        fuzzcolfunc = R_DrawFuzzColumnFlatText8050;
        break;
    case INVISIBLE_FLAT_SATURN:
        fuzzcolfunc = R_DrawFuzzColumnFlatSaturnText8050;
        break;
    case INVISIBLE_SATURN:
        fuzzcolfunc = R_DrawFuzzColumnSaturnText8050;
        break;
    case INVISIBLE_TRANSLUCENT:
        fuzzcolfunc = R_DrawFuzzColumnTransText8050;
        break;
    }

#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF)
    switch (detailshift)
    {
    case DETAIL_HIGH:

        switch (wallRender)
        {
        case WALL_NORMAL:
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                colfunc = R_DrawColumnFastLEA;
                break;
            default:
                colfunc = R_DrawColumn;
                break;
            }
            break;
        case WALL_FLAT:
        case WALL_FLATTER:
            colfunc = R_DrawColumnFlat;
            break;
        }

        switch (spriteRender)
        {
        case SPRITE_NORMAL:
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                spritefunc = basespritefunc = R_DrawColumnFastLEA;
                break;
            default:
                spritefunc = basespritefunc = R_DrawColumn;
                break;
            }
            break;
        case SPRITE_FLAT:
        case SPRITE_FLATTER:
            spritefunc = basespritefunc = R_DrawColumnFlat;
            break;
        }

        switch (pspriteRender)
        {
        case PSPRITE_NORMAL:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            if (screenblocks >= 10)
            {
                pspritefunc = basepspritefunc = R_DrawColumnDirect;
            }
            else
            {
                switch (selectedCPU)
                {
                case UMC_GREEN_486:
                case CYRIX_5X86:
                case CYRIX_6X86:
                case CYRIX_6X86MX:
                case AMD_K5:
                case AMD_K6:
                case RISE_MP6:
                    pspritefunc = basepspritefunc = R_DrawColumnFastLEA;
                    break;
                default:
                    pspritefunc = basepspritefunc = R_DrawColumn;
                    break;
                }
            }
#else
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                pspritefunc = basepspritefunc = R_DrawColumnFastLEA;
                break;
            default:
                pspritefunc = basepspritefunc = R_DrawColumn;
                break;
            }
#endif
            break;
        case PSPRITE_FLAT:
        case PSPRITE_FLATTER:
            pspritefunc = basepspritefunc = R_DrawColumnFlat;
            break;
        }

        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlane;
            clearPlanes = R_ClearPlanes;
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpan386SX;
                break;
            default:
                spanfunc = R_DrawSpan;
                break;
            }
            break;
        case VISPLANES_FLAT:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlaneFlat;
            clearPlanes = R_ClearPlanesFlat;
            spanfunc = R_DrawSpanFlat;
            break;
        case VISPLANES_FLATTER:
            clearPlanes = R_ClearPlanesFlat;
            drawPlanes = R_DrawPlanesFlatter;
            spanfunc = R_DrawColumnPlaneFlat;
            break;
        }

        if (flatSky)
        {
            drawSky = R_DrawSkyFlatPlanar;
            skyfunc = R_DrawColumnPlaneFlat;
        }
        else
        {
            drawSky = R_DrawSky;
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnSkyFullDirect : R_DrawColumnFastLEA;
#else
                skyfunc = R_DrawColumnFastLEA;
#endif
                break;
            default:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnSkyFullDirect : R_DrawColumn;
#else
                skyfunc = R_DrawColumn;
#endif
            }
        }

        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fuzzcolfunc = R_DrawFuzzColumn;
            break;
        case INVISIBLE_FLAT:
            fuzzcolfunc = R_DrawFuzzColumnFlat;
            break;
        case INVISIBLE_FLAT_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnFlatSaturn;
            break;
        case INVISIBLE_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnSaturn;
            break;
        case INVISIBLE_TRANSLUCENT:
            fuzzcolfunc = R_DrawFuzzColumnTrans;
            break;
        }

        break;
    case DETAIL_LOW:

        switch (wallRender)
        {
        case WALL_NORMAL:
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                colfunc = R_DrawColumnLowFastLEA;
                break;
            default:
                colfunc = R_DrawColumnLow;
                break;
            }
            break;
        case WALL_FLAT:
        case WALL_FLATTER:
            colfunc = R_DrawColumnFlatLow;
            break;
        }

        switch (spriteRender)
        {
        case SPRITE_NORMAL:
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                spritefunc = basespritefunc = R_DrawColumnLowFastLEA;
                break;
            default:
                spritefunc = basespritefunc = R_DrawColumnLow;
                break;
            }
            break;
        case SPRITE_FLAT:
        case SPRITE_FLATTER:
            spritefunc = basespritefunc = R_DrawColumnFlatLow;
            break;
        }

        switch (pspriteRender)
        {
        case PSPRITE_NORMAL:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            if (screenblocks >= 10)
            {
                pspritefunc = basepspritefunc = R_DrawColumnLowDirect;
            }
            else
            {
                switch (selectedCPU)
                {
                case UMC_GREEN_486:
                case CYRIX_5X86:
                case CYRIX_6X86:
                case CYRIX_6X86MX:
                case AMD_K5:
                case AMD_K6:
                case RISE_MP6:
                    pspritefunc = basepspritefunc = R_DrawColumnLowFastLEA;
                    break;
                default:
                    pspritefunc = basepspritefunc = R_DrawColumnLow;
                    break;
                }
            }
#else
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                pspritefunc = basepspritefunc = R_DrawColumnLowFastLEA;
                break;
            default:
                pspritefunc = basepspritefunc = R_DrawColumnLow;
                break;
            }
#endif
            break;
        case PSPRITE_FLAT:
        case PSPRITE_FLATTER:
            pspritefunc = basepspritefunc = R_DrawColumnFlatLow;
            break;
        }

        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlane;
            clearPlanes = R_ClearPlanes;
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanLow386SX;
                break;
            default:
                spanfunc = R_DrawSpanLow;
                break;
            }
            break;
        case VISPLANES_FLAT:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlaneFlat;
            clearPlanes = R_ClearPlanesFlat;
            spanfunc = R_DrawSpanFlatLow;
            break;
        case VISPLANES_FLATTER:
            clearPlanes = R_ClearPlanesFlat;
            drawPlanes = R_DrawPlanesFlatterLow;
            spanfunc = R_DrawColumnPlaneFlatLow;
            break;
        }

        if (flatSky)
        {
            drawSky = R_DrawSkyFlatPlanarLow;
            skyfunc = R_DrawColumnPlaneFlatLow;
        }
        else
        {
            drawSky = R_DrawSky;
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnLowSkyFullDirect : R_DrawColumnLowFastLEA;
#else
                skyfunc = R_DrawColumnLowFastLEA;
#endif
                break;
            default:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnLowSkyFullDirect : R_DrawColumnLow;
#else
                skyfunc = R_DrawColumnLow;
#endif
                break;
            }
        }

        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fuzzcolfunc = R_DrawFuzzColumnLow;
            break;
        case INVISIBLE_FLAT:
            fuzzcolfunc = R_DrawFuzzColumnFlatLow;
            break;
        case INVISIBLE_FLAT_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnFlatSaturnLow;
            break;
        case INVISIBLE_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnSaturnLow;
            break;
        case INVISIBLE_TRANSLUCENT:
            fuzzcolfunc = R_DrawFuzzColumnTransLow;
            break;
        }

        break;
    case DETAIL_POTATO:

        switch (wallRender)
        {
        case WALL_NORMAL:
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                colfunc = R_DrawColumnPotatoFastLEA;
                break;
            default:
                colfunc = R_DrawColumnPotato;
                break;
            }
            break;
        case WALL_FLAT:
        case WALL_FLATTER:
            colfunc = R_DrawColumnFlatPotato;
            break;
        }

        switch (spriteRender)
        {
        case SPRITE_NORMAL:
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                spritefunc = basespritefunc = R_DrawColumnPotatoFastLEA;
                break;
            default:
                spritefunc = basespritefunc = R_DrawColumnPotato;
                break;
            }
            break;
        case SPRITE_FLAT:
        case SPRITE_FLATTER:
            spritefunc = basespritefunc = R_DrawColumnFlatPotato;
            break;
        }

        switch (pspriteRender)
        {
        case PSPRITE_NORMAL:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            if (screenblocks >= 10)
            {
                pspritefunc = basepspritefunc = R_DrawColumnPotatoDirect;
            }
            else
            {
                switch (selectedCPU)
                {
                case UMC_GREEN_486:
                case CYRIX_5X86:
                case CYRIX_6X86:
                case CYRIX_6X86MX:
                case AMD_K5:
                case AMD_K6:
                case RISE_MP6:
                    pspritefunc = basepspritefunc = R_DrawColumnPotatoFastLEA;
                    break;
                default:
                    pspritefunc = basepspritefunc = R_DrawColumnPotato;
                    break;
                }
            }
#else
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                pspritefunc = basepspritefunc = R_DrawColumnPotatoFastLEA;
                break;
            default:
                pspritefunc = basepspritefunc = R_DrawColumnPotato;
                break;
            }
#endif
            break;
        case PSPRITE_FLAT:
        case PSPRITE_FLATTER:
            pspritefunc = basepspritefunc = R_DrawColumnFlatPotato;
            break;
        }

        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlane;
            clearPlanes = R_ClearPlanes;
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanPotato386SX;
                break;
            default:
                spanfunc = R_DrawSpanPotato;
                break;
            }
            break;
        case VISPLANES_FLAT:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlaneFlat;
            clearPlanes = R_ClearPlanesFlat;
            spanfunc = R_DrawSpanFlatPotato;
            break;
        case VISPLANES_FLATTER:
            clearPlanes = R_ClearPlanesFlat;
            drawPlanes = R_DrawPlanesFlatterPotato;
            spanfunc = R_DrawColumnFlatPotato;
            break;
        }

        if (flatSky)
        {
            drawSky = R_DrawSkyFlat;
            skyfunc = R_DrawColumnFlatPotato;
        }
        else
        {
            drawSky = R_DrawSky;
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnPotatoSkyFullDirect : R_DrawColumnPotatoFastLEA;
#else
                skyfunc = R_DrawColumnPotatoFastLEA;
#endif
                break;
            default:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnPotatoSkyFullDirect : R_DrawColumnPotato;
#else
                skyfunc = R_DrawColumnPotato;
#endif
                break;
            }
        }

        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fuzzcolfunc = R_DrawFuzzColumnPotato;
            break;
        case INVISIBLE_FLAT:
            fuzzcolfunc = R_DrawFuzzColumnFlatPotato;
            break;
        case INVISIBLE_FLAT_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnFlatSaturnPotato;
            break;
        case INVISIBLE_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnSaturnPotato;
            break;
        case INVISIBLE_TRANSLUCENT:
            fuzzcolfunc = R_DrawFuzzColumnTransPotato;
            break;
        }

        break;
    }
#endif
#if defined(USE_BACKBUFFER)
    switch (detailshift)
    {
    case DETAIL_HIGH:
        switch (wallRender)
        {
        case WALL_NORMAL:
            switch (selectedCPU)
            {
            case CYRIX_6X86:
                colfunc = R_DrawColumnBackbufferRoll;
                break;
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
                colfunc = R_DrawColumnBackbufferFastLEA;
                break;
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
                colfunc = R_DrawColumnBackbufferMMX;
                break;
            default:
                colfunc = R_DrawColumnBackbuffer;
                break;
            }
            break;
        case WALL_FLAT:
        case WALL_FLATTER:
            colfunc = R_DrawColumnBackbufferFlat;
            break;
        }

        switch (spriteRender)
        {
        case SPRITE_NORMAL:
            switch (selectedCPU)
            {
            case CYRIX_6X86:
                spritefunc = basespritefunc = R_DrawColumnBackbufferRoll;
                break;
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
                spritefunc = basespritefunc = R_DrawColumnBackbufferFastLEA;
                break;
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
                spritefunc = basespritefunc = R_DrawColumnBackbufferMMX;
                break;
            default:
                spritefunc = basespritefunc = R_DrawColumnBackbuffer;
                break;
            }
            break;
        case SPRITE_FLAT:
        case SPRITE_FLATTER:
            spritefunc = basespritefunc = R_DrawColumnBackbufferFlat;
            break;
        }

        switch (pspriteRender)
        {
        case PSPRITE_NORMAL:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            if (screenblocks >= 10)
            {
                pspritefunc = basepspritefunc = R_DrawColumnBackbufferDirect;
            }
            else
            {
                switch (selectedCPU)
                {
                case CYRIX_6X86:
                    pspritefunc = basepspritefunc = R_DrawColumnBackbufferRoll;
                    break;
                case UMC_GREEN_486:
                case CYRIX_5X86:
                case CYRIX_6X86MX:
                case AMD_K5:
                case AMD_K6:
                    pspritefunc = basepspritefunc = R_DrawColumnBackbufferFastLEA;
                    break;
                case RISE_MP6:
                case INTEL_PENTIUM_MMX:
                    pspritefunc = basepspritefunc = R_DrawColumnBackbufferMMX;
                    break;
                default:
                    pspritefunc = basepspritefunc = R_DrawColumnBackbuffer;
                    break;
                }
            }
#else
            switch (selectedCPU)
            {
            case CYRIX_6X86:
                pspritefunc = basepspritefunc = R_DrawColumnBackbufferRoll;
                break;
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
                pspritefunc = basepspritefunc = R_DrawColumnBackbufferFastLEA;
                break;
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
                pspritefunc = basepspritefunc = R_DrawColumnBackbufferMMX;
                break;
            default:
                pspritefunc = basepspritefunc = R_DrawColumnBackbuffer;
                break;
            }
#endif
            break;
        case PSPRITE_FLAT:
        case PSPRITE_FLATTER:
            pspritefunc = basepspritefunc = R_DrawColumnBackbufferFlat;
            break;
        }

        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlane;
            clearPlanes = R_ClearPlanes;
            switch (selectedCPU)
            {
            case INTEL_PENTIUM:
            case IDT_WINCHIP:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
                spanfunc = R_DrawSpanBackbufferPentium;
                break;
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
                spanfunc = R_DrawSpanBackbufferMMX;
                break;
            default:
                spanfunc = R_DrawSpanBackbuffer;
                break;
            }
            break;
        case VISPLANES_FLAT:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlaneFlat;
            clearPlanes = R_ClearPlanesFlat;
            spanfunc = R_DrawSpanFlatBackbuffer;
            break;
        case VISPLANES_FLATTER:
            clearPlanes = R_ClearPlanesFlat;
            drawPlanes = R_DrawPlanesFlatterBackbuffer;
            spanfunc = R_DrawColumnBackbufferFlat;
            break;
        }

        if (flatSky)
        {
            drawSky = R_DrawSkyFlat;
            skyfunc = R_DrawColumnBackbufferFlat;
        }
        else
        {
            drawSky = R_DrawSky;
            switch (selectedCPU)
            {
            case CYRIX_6X86:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnBackbufferSkyFullDirect : R_DrawColumnBackbufferRoll;
#else
                skyfunc = R_DrawColumnBackbufferRoll;
#endif
                break;
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnBackbufferSkyFullDirect : R_DrawColumnBackbufferFastLEA;
#else
                skyfunc = R_DrawColumnBackbufferFastLEA;
#endif
                break;
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnBackbufferSkyFullDirect : R_DrawColumnBackbufferMMX;
#else
                skyfunc = R_DrawColumnBackbufferMMX;
#endif
                break;
            default:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnBackbufferSkyFullDirect : R_DrawColumnBackbuffer;
#else
                skyfunc = R_DrawColumnBackbuffer;
#endif
                break;
            }
        }

        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fuzzcolfunc = R_DrawFuzzColumnBackbuffer;
            break;
        case INVISIBLE_FLAT:
            fuzzcolfunc = R_DrawFuzzColumnFlatBackbuffer;
            break;
        case INVISIBLE_FLAT_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnFlatSaturnBackbuffer;
            break;
        case INVISIBLE_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnSaturnBackbuffer;
            break;
        case INVISIBLE_TRANSLUCENT:
            fuzzcolfunc = R_DrawFuzzColumnTransBackbuffer;
            break;
        }

        break;
    case DETAIL_LOW:
        switch (wallRender)
        {
        case WALL_NORMAL:
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                colfunc = R_DrawColumnLowBackbufferFastLEA;
                break;
            default:
                colfunc = R_DrawColumnLowBackbuffer;
                break;
            }
            break;
        case WALL_FLAT:
        case WALL_FLATTER:
            colfunc = R_DrawColumnLowBackbufferFlat;
            break;
        }

        switch (spriteRender)
        {
        case SPRITE_NORMAL:
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                spritefunc = basespritefunc = R_DrawColumnLowBackbufferFastLEA;
                break;
            default:
                spritefunc = basespritefunc = R_DrawColumnLowBackbuffer;
                break;
            }
            break;
        case SPRITE_FLAT:
        case SPRITE_FLATTER:
            spritefunc = basespritefunc = R_DrawColumnLowBackbufferFlat;
            break;
        }

        switch (pspriteRender)
        {
        case PSPRITE_NORMAL:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            if (screenblocks >= 10)
            {
                pspritefunc = basepspritefunc = R_DrawColumnLowBackbufferDirect;
            }
            else
            {
                switch (selectedCPU)
                {
                case UMC_GREEN_486:
                case CYRIX_5X86:
                case CYRIX_6X86:
                case CYRIX_6X86MX:
                case AMD_K5:
                case AMD_K6:
                case RISE_MP6:
                    pspritefunc = basepspritefunc = R_DrawColumnLowBackbufferFastLEA;
                    break;
                default:
                    pspritefunc = basepspritefunc = R_DrawColumnLowBackbuffer;
                    break;
                }
            }
#else
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
                pspritefunc = basepspritefunc = R_DrawColumnLowBackbufferFastLEA;
                break;
            default:
                pspritefunc = basepspritefunc = R_DrawColumnLowBackbuffer;
                break;
            }
#endif
            break;
        case PSPRITE_FLAT:
        case PSPRITE_FLATTER:
            pspritefunc = basepspritefunc = R_DrawColumnLowBackbufferFlat;
            break;
        }

        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlane;
            clearPlanes = R_ClearPlanes;
            switch (selectedCPU)
            {
            case INTEL_PENTIUM:
            case INTEL_PENTIUM_MMX:
            case IDT_WINCHIP:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
                spanfunc = R_DrawSpanLowBackbufferPentium;
                break;
            default:
                spanfunc = R_DrawSpanLowBackbuffer;
                break;
            }
            break;
        case VISPLANES_FLAT:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlaneFlat;
            clearPlanes = R_ClearPlanesFlat;
            spanfunc = R_DrawSpanFlatLowBackbuffer;
            break;
        case VISPLANES_FLATTER:
            clearPlanes = R_ClearPlanesFlat;
            drawPlanes = R_DrawPlanesFlatterLowBackbuffer;
            spanfunc = R_DrawColumnLowBackbufferFlat;
            break;
        }

        if (flatSky)
        {
            drawSky = R_DrawSkyFlat;
            skyfunc = R_DrawColumnLowBackbufferFlat;
        }
        else
        {
            drawSky = R_DrawSky;
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
            case AMD_K5:
            case AMD_K6:
            case RISE_MP6:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnLowBackbufferSkyFullDirect : R_DrawColumnLowBackbufferFastLEA;
#else
                skyfunc = R_DrawColumnLowBackbufferFastLEA;
#endif
                break;
            default:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnLowBackbufferSkyFullDirect : R_DrawColumnLowBackbuffer;
#else
                skyfunc = R_DrawColumnLowBackbuffer;
#endif
                break;
            }
        }

        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fuzzcolfunc = R_DrawFuzzColumnLowBackbuffer;
            break;
        case INVISIBLE_FLAT:
            fuzzcolfunc = R_DrawFuzzColumnFlatLowBackbuffer;
            break;
        case INVISIBLE_FLAT_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnFlatSaturnLowBackbuffer;
            break;
        case INVISIBLE_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnSaturnLowBackbuffer;
            break;
        case INVISIBLE_TRANSLUCENT:
            fuzzcolfunc = R_DrawFuzzColumnTransLowBackbuffer;
            break;
        }

        break;
    case DETAIL_POTATO:
        switch (wallRender)
        {
        case WALL_NORMAL:
            colfunc = R_DrawColumnPotatoBackbuffer;
            break;
        case WALL_FLAT:
        case WALL_FLATTER:
            colfunc = R_DrawColumnPotatoBackbufferFlat;
            break;
        }

        switch (spriteRender)
        {
        case SPRITE_NORMAL:
            spritefunc = basespritefunc = R_DrawColumnPotatoBackbuffer;
            break;
        case SPRITE_FLAT:
        case SPRITE_FLATTER:
            spritefunc = basespritefunc = R_DrawColumnPotatoBackbufferFlat;
            break;
        }

        switch (pspriteRender)
        {
        case PSPRITE_NORMAL:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            if (screenblocks >= 10)
            {
                pspritefunc = basepspritefunc = R_DrawColumnPotatoBackbufferDirect;
            }
            else
            {
                pspritefunc = basepspritefunc = R_DrawColumnPotatoBackbuffer;
            }
#else
            pspritefunc = basepspritefunc = R_DrawColumnPotatoBackbuffer;
#endif
            break;
        case PSPRITE_FLAT:
        case PSPRITE_FLATTER:
            pspritefunc = basepspritefunc = R_DrawColumnPotatoBackbufferFlat;
            break;
        }

        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlane;
            clearPlanes = R_ClearPlanes;
            switch (selectedCPU)
            {
            case INTEL_PENTIUM:
            case INTEL_PENTIUM_MMX:
            case IDT_WINCHIP:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
                spanfunc = R_DrawSpanPotatoBackbufferPentium;
                break;
            default:
                spanfunc = R_DrawSpanPotatoBackbuffer;
                break;
            }
            break;
        case VISPLANES_FLAT:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlaneFlat;
            clearPlanes = R_ClearPlanesFlat;
            spanfunc = R_DrawSpanFlatPotatoBackbuffer;
            break;
        case VISPLANES_FLATTER:
            clearPlanes = R_ClearPlanesFlat;
            drawPlanes = R_DrawPlanesFlatterPotatoBackbuffer;
            spanfunc = spanfunc = R_DrawColumnPotatoBackbufferFlat;
            break;
        }

        if (flatSky)
        {
            drawSky = R_DrawSkyFlat;
            skyfunc = R_DrawColumnPotatoBackbufferFlat;
        }
        else
        {
            drawSky = R_DrawSky;
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            skyfunc = (screenblocks >= 10) ? R_DrawColumnPotatoBackbufferSkyFullDirect : R_DrawColumnPotatoBackbuffer;
#else
            skyfunc = R_DrawColumnPotatoBackbuffer;
#endif
        }

        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fuzzcolfunc = R_DrawFuzzColumnPotatoBackbuffer;
            break;
        case INVISIBLE_FLAT:
            fuzzcolfunc = R_DrawFuzzColumnFlatPotatoBackbuffer;
            break;
        case INVISIBLE_FLAT_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnFlatSaturnPotatoBackbuffer;
            break;
        case INVISIBLE_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnSaturnPotatoBackbuffer;
            break;
        case INVISIBLE_TRANSLUCENT:
            fuzzcolfunc = R_DrawFuzzColumnTransPotatoBackbuffer;
            break;
        }

        break;
    }
#endif

#if defined(MODE_VBE2_DIRECT)
    switch (detailshift)
    {
    case DETAIL_HIGH:

        switch (wallRender)
        {
        case WALL_NORMAL:
            switch (selectedCPU)
            {
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
                colfunc = R_DrawColumnVBE2MMX;
                break;
            default:
                colfunc = R_DrawColumnVBE2;
                break;
            }
            break;
        case WALL_FLAT:
        case WALL_FLATTER:
            colfunc = R_DrawColumnVBE2Flat;
            break;
        }

        switch (spriteRender)
        {
        case SPRITE_NORMAL:
            switch (selectedCPU)
            {
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
                spritefunc = basespritefunc = R_DrawColumnVBE2MMX;
                break;
            default:
                spritefunc = basespritefunc = R_DrawColumnVBE2;
                break;
            }
            break;
        case SPRITE_FLAT:
        case SPRITE_FLATTER:
            spritefunc = basespritefunc = R_DrawColumnVBE2Flat;
            break;
        }

        switch (pspriteRender)
        {
        case PSPRITE_NORMAL:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            if (screenblocks >= 10)
            {
                pspritefunc = basepspritefunc = R_DrawColumnVBE2Direct;
            }
            else
            {
                switch (selectedCPU)
                {
                case RISE_MP6:
                case INTEL_PENTIUM_MMX:
                    pspritefunc = basepspritefunc = R_DrawColumnVBE2MMX;
                    break;
                default:
                    pspritefunc = basepspritefunc = R_DrawColumnVBE2;
                    break;
                }
            }
#else
            switch (selectedCPU)
            {
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
                pspritefunc = basepspritefunc = R_DrawColumnVBE2MMX;
                break;
            default:
                pspritefunc = basepspritefunc = R_DrawColumnVBE2;
                break;
            }
#endif
            break;
        case PSPRITE_FLAT:
        case PSPRITE_FLATTER:
            pspritefunc = basepspritefunc = R_DrawColumnVBE2Flat;
            break;
        }

        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlane;
            clearPlanes = R_ClearPlanes;
            switch (selectedCPU)
            {
            case INTEL_PENTIUM:
            case CYRIX_6X86:
                spanfunc = R_DrawSpanVBE2Pentium;
                break;
            case IDT_WINCHIP:
            case CYRIX_6X86MX:
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
            case AMD_K6:
                spanfunc = R_DrawSpanVBE2MMX;
                break;
            default:
                spanfunc = R_DrawSpanVBE2;
                break;
            }
            break;
        case VISPLANES_FLAT:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlaneFlat;
            clearPlanes = R_ClearPlanesFlat;
            spanfunc = R_DrawSpanFlatVBE2;
            break;
        case VISPLANES_FLATTER:
            clearPlanes = R_ClearPlanesFlat;
            drawPlanes = R_DrawPlanesFlatterVBE2;
            spanfunc = R_DrawColumnVBE2Flat;
            break;
        }

        if (flatSky)
        {
            drawSky = R_DrawSkyFlat;
            skyfunc = R_DrawColumnVBE2Flat;
        }
        else
        {
            drawSky = R_DrawSky;

            switch (selectedCPU)
            {
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnVBE2SkyFullDirect : R_DrawColumnVBE2MMX;
#else
                skyfunc = R_DrawColumnVBE2MMX;
#endif
                break;
            default:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
                skyfunc = (screenblocks >= 10) ? R_DrawColumnVBE2SkyFullDirect : R_DrawColumnVBE2;
#else
                skyfunc = R_DrawColumnVBE2;
#endif
                break;
            }
        }

        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fuzzcolfunc = R_DrawFuzzColumnVBE2;
            break;
        case INVISIBLE_FLAT:
            fuzzcolfunc = R_DrawFuzzColumnFlatVBE2;
            break;
        case INVISIBLE_FLAT_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnFlatSaturnVBE2;
            break;
        case INVISIBLE_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnSaturnVBE2;
            break;
        case INVISIBLE_TRANSLUCENT:
            fuzzcolfunc = R_DrawFuzzColumnTransVBE2;
            break;
        }

        break;
    case DETAIL_LOW:
        switch (wallRender)
        {
        case WALL_NORMAL:
            colfunc = R_DrawColumnLowVBE2;
            break;
        case WALL_FLAT:
        case WALL_FLATTER:
            colfunc = R_DrawColumnLowVBE2Flat;
            break;
        }

        switch (spriteRender)
        {
        case SPRITE_NORMAL:
            spritefunc = basespritefunc = R_DrawColumnLowVBE2;
            break;
        case SPRITE_FLAT:
        case SPRITE_FLATTER:
            spritefunc = basespritefunc = R_DrawColumnLowVBE2Flat;
            break;
        }

        switch (pspriteRender)
        {
        case PSPRITE_NORMAL:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            if (screenblocks >= 10)
            {
                pspritefunc = basepspritefunc = R_DrawColumnLowVBE2Direct;
            }
            else
            {
                pspritefunc = basepspritefunc = R_DrawColumnLowVBE2;
            }
#else
            pspritefunc = basepspritefunc = R_DrawColumnLowVBE2;
#endif
            break;
        case PSPRITE_FLAT:
        case PSPRITE_FLATTER:
            pspritefunc = basepspritefunc = R_DrawColumnLowVBE2Flat;
            break;
        }

        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlane;
            clearPlanes = R_ClearPlanes;
            switch (selectedCPU)
            {
            case INTEL_PENTIUM:
            case INTEL_PENTIUM_MMX:
            case IDT_WINCHIP:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
                spanfunc = R_DrawSpanLowVBE2Pentium;
                break;
            default:
                spanfunc = R_DrawSpanLowVBE2;
                break;
            }
            break;
        case VISPLANES_FLAT:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlaneFlat;
            clearPlanes = R_ClearPlanesFlat;
            spanfunc = R_DrawSpanFlatLowVBE2;
            break;
        case VISPLANES_FLATTER:
            clearPlanes = R_ClearPlanesFlat;
            drawPlanes = R_DrawPlanesFlatterVBE2;
            spanfunc = R_DrawColumnLowVBE2Flat;
            break;
        }

        if (flatSky)
        {
            drawSky = R_DrawSkyFlat;
            skyfunc = R_DrawColumnLowVBE2Flat;
        }
        else
        {
            drawSky = R_DrawSky;
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            skyfunc = (screenblocks >= 10) ? R_DrawColumnLowVBE2SkyFullDirect : R_DrawColumnLowVBE2;
#else
            skyfunc = R_DrawColumnLowVBE2;
#endif
        }

        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fuzzcolfunc = R_DrawFuzzColumnLowVBE2;
            break;
        case INVISIBLE_FLAT:
            fuzzcolfunc = R_DrawFuzzColumnFlatLowVBE2;
            break;
        case INVISIBLE_FLAT_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnFlatSaturnLowVBE2;
            break;
        case INVISIBLE_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnSaturnLowVBE2;
            break;
        case INVISIBLE_TRANSLUCENT:
            fuzzcolfunc = R_DrawFuzzColumnTransLowVBE2;
            break;
        }

        break;
    case DETAIL_POTATO:
        switch (wallRender)
        {
        case WALL_NORMAL:
            colfunc = R_DrawColumnPotatoVBE2;
            break;
        case WALL_FLAT:
        case WALL_FLATTER:
            colfunc = R_DrawColumnPotatoVBE2Flat;
            break;
        }

        switch (spriteRender)
        {
        case SPRITE_NORMAL:
            spritefunc = basespritefunc = R_DrawColumnPotatoVBE2;
            break;
        case SPRITE_FLAT:
        case SPRITE_FLATTER:
            spritefunc = basespritefunc = R_DrawColumnPotatoVBE2Flat;
            break;
        }

        switch (pspriteRender)
        {
        case PSPRITE_NORMAL:
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            if (screenblocks >= 10)
            {
                pspritefunc = basepspritefunc = R_DrawColumnPotatoVBE2Direct;
            }
            else
            {
                pspritefunc = basepspritefunc = R_DrawColumnPotatoVBE2;
            }
#else
            pspritefunc = basepspritefunc = R_DrawColumnPotatoVBE2;
#endif
            break;
        case PSPRITE_FLAT:
        case PSPRITE_FLATTER:
            pspritefunc = basepspritefunc = R_DrawColumnPotatoVBE2Flat;
            break;
        }

        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlane;
            clearPlanes = R_ClearPlanes;
            switch (selectedCPU)
            {
            case INTEL_PENTIUM:
            case INTEL_PENTIUM_MMX:
            case IDT_WINCHIP:
            case CYRIX_6X86:
            case CYRIX_6X86MX:
                spanfunc = R_DrawSpanPotatoVBE2Pentium;
                break;
            default:
                spanfunc = R_DrawSpanPotatoVBE2;
                break;
            }
            break;
        case VISPLANES_FLAT:
            drawPlanes = R_DrawPlanes;
            mapPlane = R_MapPlaneFlat;
            clearPlanes = R_ClearPlanesFlat;
            spanfunc = R_DrawSpanFlatPotatoVBE2;
            break;
        case VISPLANES_FLATTER:
            clearPlanes = R_ClearPlanesFlat;
            drawPlanes = R_DrawPlanesFlatterVBE2;
            spanfunc = R_DrawColumnPotatoVBE2Flat;
            break;
        }

        if (flatSky)
        {
            drawSky = R_DrawSkyFlat;
            skyfunc = R_DrawColumnPotatoVBE2Flat;
        }
        else
        {
            drawSky = R_DrawSky;
#if SCREENHEIGHT == 200 || SCREENHEIGHT == 240
            skyfunc = (screenblocks >= 10) ? R_DrawColumnPotatoVBE2SkyFullDirect : R_DrawColumnPotatoVBE2;
#else
            skyfunc = R_DrawColumnPotatoVBE2;
#endif
        }

        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fuzzcolfunc = R_DrawFuzzColumnPotatoVBE2;
            break;
        case INVISIBLE_FLAT:
            fuzzcolfunc = R_DrawFuzzColumnFlatPotatoVBE2;
            break;
        case INVISIBLE_FLAT_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnFlatSaturnPotatoVBE2;
            break;
        case INVISIBLE_SATURN:
            fuzzcolfunc = R_DrawFuzzColumnSaturnPotatoVBE2;
            break;
        case INVISIBLE_TRANSLUCENT:
            fuzzcolfunc = R_DrawFuzzColumnTransPotatoVBE2;
            break;
        }

        break;
    }
#endif

    R_InitBuffer(scaledviewwidth, viewheight);

    R_InitTextureMapping();

    // psprite scales
#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA) && !defined(MODE_COLOR_MDA)
    pspritescale = FRACUNIT * viewwidth / 320;
#if defined(MODE_Y_HALF)
    pspritescaleds = (pspritescale << detailshift) / 2;
#else
    pspritescaleds = pspritescale << detailshift;
#endif

    pspriteiscale = FRACUNIT * 320 / viewwidth;
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    pspriteiscaleshifted = pspriteiscale >> detailshift;
    pspriteiscaleshifted_sky = (pspriteiscaleshifted * SKY_SCALE) / 100;
#endif

    // thing clipping
    SetWords(screenheightarray, viewheight, viewwidth);

    // planes
    for (i = 0; i < viewheight; i++)
    {
        dy = ((i - viewheight / 2) << FRACBITS) + FRACUNIT / 2;
        dy = abs(dy);

#if defined(MODE_T4050)
        yslope[i] = FixedDiv((viewwidth << 1) / 2 * FRACUNIT, dy);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
        yslope[i] = FixedDiv((viewwidth << detailshift) / 2 * FRACUNIT, dy);
#endif

#if defined(MODE_Y_HALF)
        yslope[i] = FixedDiv((viewwidth << detailshift) / 4 * FRACUNIT, dy);
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
        yslope[i] = FixedDiv((viewwidth) / 2 * FRACUNIT, dy);
#endif
    }

    for (i = 0; i < viewwidth; i++)
    {
        cosadj = abs(finecosine[xtoviewangle[i] >> ANGLETOFINESHIFT]);
        distscale[i] = (4 >= cosadj) ? (65536 ^ cosadj >> 31) ^ MAXINT : FixedDiv65536(cosadj);
    }

    // Calculate the light levels to use
    //  for each level / scale combination.
    for (i = 0; i < LIGHTLEVELS; i++)
    {
        startmap = ((LIGHTLEVELS - 1 - i) * 2) * NUMCOLORMAPS / LIGHTLEVELS;
        for (j = 0; j < MAXLIGHTSCALE; j++)
        {
#if defined(MODE_T4050)
            level = startmap - MulScreenWidth(j) / (viewwidth << 1) / DISTMAP;
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
            level = startmap - MulScreenWidth(j) / (viewwidth << detailshift) / DISTMAP;
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
            level = startmap - MulScreenWidth(j) / (viewwidth) / DISTMAP;
#endif
            if (level < 0)
                level = 0;
            else if (level > NUMCOLORMAPS - 1)
                level = NUMCOLORMAPS - 1;

            scalelight[i][j] = colormaps + level * 256;
        }
    }

    R_PatchCode();

    // I put this here so I could see which functions were having rendering
    // problems. Leaving it here for demonstration purposes.
#if (DEBUG_ENABLED == 1)
    I_Printf("Render Functions:\n");
    I_Printf("\tcolfunc: %s\n", I_LookupSymbolName(colfunc));
    I_Printf("\tbasespritefunc: %s\n", I_LookupSymbolName(basespritefunc));
    I_Printf("\tfuzzcolfunc: %s\n", I_LookupSymbolName(fuzzcolfunc));
    I_Printf("\tspanfunc: %s\n", I_LookupSymbolName(spanfunc));
    I_Printf("\tskyfunc: %s\n", I_LookupSymbolName(skyfunc));
    I_Printf("\tpspritefunc: %s\n", I_LookupSymbolName(pspritefunc));
    I_Printf("\tbasepspritefunc: %s\n", I_LookupSymbolName(basepspritefunc));
#endif
}

//
// R_Init
//
extern int detailLevel;

#define SKYFLATNAME "F_SKY1"

void R_Init(void)
{
    R_InitData();
    // viewwidth / viewheight / detailLevel are set by the defaults
    printf(".");

    R_SetViewSize(screenblocks, detailLevel);
    printf(".");
    R_InitLightTables();
    printf(".");
    skyflatnum = R_FlatNumForName(SKYFLATNAME);
    printf(".");
}

//
// R_PointInSubsector
//
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y)
{
    int nodenum = firstnode;

    while (!(nodenum & NF_SUBSECTOR))
    {
        node_t *node = &nodes[nodenum];
        byte side;
        fixed_t dx, dy;

        switch (node->type)
        {
        case 0:
            side = (x <= node->x) ? (node->dy > 0) : (node->dy < 0);
            break;
        case 1:
            side = (y <= node->y) ? (node->dx < 0) : (node->dx > 0);
            break;
        default:
            dx = (x - node->x);
            dy = (y - node->y);

            // Try to quickly decide by looking at sign bits.
            if ((node->dy ^ node->dx ^ dx ^ dy) & 0x80000000)
            {
                side = ROLAND1(node->dy ^ dx);
            }
            else
            {
                fixed_t left = FixedMulEDX(node->dy >> FRACBITS, dx);
                fixed_t right = FixedMulEDX(dy, node->dx >> FRACBITS);

                side = right >= left;
            }
            break;
        }

        nodenum = node->children[side];
    }

    return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// R_SetupFrame
//
void R_SetupFrame(void)
{
    int i;

    if (highResTimer)
    {
        viewx = FixedInterpolate(players_mo->prevx, players_mo->x, interpolation_weight);
        viewy = FixedInterpolate(players_mo->prevy, players_mo->y, interpolation_weight);
        viewangle = (players_mo)->prevangle + ((((signed)(players_mo)->angle - (signed)(players_mo)->prevangle) * ((signed)(interpolation_weight >> 12)) / 16));
        viewz = FixedInterpolate(players.prevviewz, players.viewz, interpolation_weight);
    }
    else
    {
        // No interpolation
        viewx = (players_mo)->x;
        viewy = (players_mo)->y;
        viewangle = (players_mo)->angle;
        viewz = players.viewz;
    }
    viewxs = viewx >> FRACBITS;
    viewyneg = -viewy;
    viewys = viewy >> FRACBITS;
    viewangle90 = viewangle + ANG90;
    extralight = players.extralight;

    viewsin = finesine[viewangle >> ANGLETOFINESHIFT];
    viewcos = finecosine[viewangle >> ANGLETOFINESHIFT];

    if (players.fixedcolormap)
    {
        fixedcolormap = colormaps + players.fixedcolormap * 256 * sizeof(lighttable_t);

        walllights = scalelightfixed;

        for (i = 0; i < MAXLIGHTSCALE; i++)
            scalelightfixed[i] = fixedcolormap;
    }
    else
        fixedcolormap = 0;

    validcount++;

#if defined(MODE_VBE2_DIRECT)
    destview = destscreen + MulScreenWidth(viewwindowy) + viewwindowx;
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF)
    destview = destscreen + MulScreenWidthQuarter(viewwindowy) + (viewwindowx >> 2);
#endif
}

//
// R_RenderView
//
void R_RenderPlayerView(void)
{
    R_SetupFrame();

    // Clear buffers.
    R_ClearClipSegs();
    R_ClearDrawSegs();
    clearPlanes();
    R_ClearSprites();

    // check for new console commands.
    NetUpdate();

// Set potato mode VGA plane
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF)
    if (detailshift == DETAIL_POTATO)
    {
        outp(SC_INDEX + 1, 15);
    }
#endif

    // The head node is the last node output.
    R_RenderBSPNode(firstnode);

    // Check for new console commands.
    NetUpdate();

    drawPlanes();

    // Check for new console commands.
    NetUpdate();

    R_DrawMasked();

    // Check for new console commands.
    NetUpdate();
}

void R_UpdateAutomap(void)
{
    R_SetupFrame();

    // Clear buffers.
    R_ClearClipSegs();

    NetUpdate();

    R_UpdateBSPNode(firstnode);

    NetUpdate();
}
