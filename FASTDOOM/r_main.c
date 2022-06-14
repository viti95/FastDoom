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

#include "doomdef.h"
#include "doomstat.h"
#include "d_net.h"

#include "m_misc.h"

#include "r_local.h"
#include "r_sky.h"

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

#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8086) && !defined(MODE_T80100) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA)
int centerx;
int centery;

fixed_t centerxfrac;
fixed_t centeryfrac;
fixed_t centeryfracshifted;
fixed_t projection;
fixed_t iprojection;
#endif

fixed_t viewx;
fixed_t viewxs;
fixed_t viewy;
fixed_t viewyneg;
fixed_t viewys;
fixed_t viewz;

angle_t viewangle;

fixed_t viewcos;
fixed_t viewsin;

// 0 = high, 1 = low
int detailshift;


// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int viewangletox[FINEANGLES / 2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t xtoviewangle[SCREENWIDTH + 1];

fixed_t *finecosine = &finesine[FINEANGLES / 4];

lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *scalelightfixed[MAXLIGHTSCALE];
lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

// bumped light from gun blasts
int extralight;

void (*colfunc)(void);
void (*basecolfunc)(void);
void (*fuzzcolfunc)(void);
void (*spanfunc)(void);
void (*skyfunc)(void);

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
        return ((ldy ^ dx) & 0x80000000) != 0;

    left = FixedMulEDX(ldy >> FRACBITS, dx);
    right = FixedMulEDX(dy, ldx >> FRACBITS);

    // returns 0/1 front/back side
    return right >= left;
}

#define SlopeDiv(num, den) ((den < 512) ? SLOPERANGE : min((num << 3) / (den >> 8), SLOPERANGE))

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
                    tempDivision = (y << 3) / (x >> 8);
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
                    tempDivision = (x << 3) / (y >> 8);
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
                    tempDivision = (y << 3) / (x >> 8);
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
                    tempDivision = (x << 3) / (y >> 8);
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
                    tempDivision = (y << 3) / (x >> 8);
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
                    tempDivision = (x << 3) / (y >> 8);
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
                    tempDivision = (y << 3) / (x >> 8);
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
                    tempDivision = (x << 3) / (y >> 8);
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

    //angle = (tantoangle[FixedDiv(dy, dx) >> DBITS] + ANG90) >> ANGLETOFINESHIFT;
    angle = (tantoangle[((dy >> 14 >= dx) ? ((dy ^ dx) >> 31) ^ MAXINT : FixedDiv2(dy, dx)) >> DBITS] + ANG90) >> ANGLETOFINESHIFT;

    // use as cosine
    //dist = FixedDiv(dx, finesine[angle]);
    temp = finesine[angle];
    dist = ((dx >> 14) >= abs(temp)) ? ((dx ^ temp) >> 31) ^ MAXINT : FixedDiv2(dx, temp);

    return dist;
}

//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
fixed_t R_ScaleFromGlobalAngle(angle_t visangle)
{
    fixed_t scale;
    int anglea;
    int angleb;
    int sinea;
    int sineb;
    fixed_t num;
    int den;

    anglea = ANG90 + (visangle - viewangle);
    angleb = ANG90 + (visangle - rw_normalangle);

    // both sines are allways positive
    sinea = finesine[anglea >> ANGLETOFINESHIFT];
    sineb = finesine[angleb >> ANGLETOFINESHIFT];
#if defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_T8086)
    num = FixedMulEDX(projection, sineb) << 1;
#endif
#ifdef MODE_Y
    num = FixedMulEDX(projection, sineb) << detailshift;
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(USE_BACKBUFFER) || defined(MODE_T4025) || defined(MODE_VBE2_DIRECT) || defined(MODE_MDA)
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
        xtoviewangle[x] = (i << ANGLETOFINESHIFT) - ANG90;
    }

    // Take out the fencepost cases from viewangletox.
    for (i = 0; i < FINEANGLES / 2; i++)
    {
        t = FixedMul(finetangent[i], focallength);
        t = centerx - t;

        if (viewangletox[i] == -1)
            viewangletox[i] = 0;
        else if (viewangletox[i] == viewwidth + 1)
            viewangletox[i] = viewwidth;
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
            scale = FixedDiv((SCREENWIDTH / 2 * FRACUNIT), (j + 1) << LIGHTZSHIFT);
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
    
    if (blocks == 11 && gamestate == GS_LEVEL){
        ST_createWidgets_mini();
    }else{
        ST_createWidgets();
    }
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

    if (forceScreenSize)
        setblocks = forceScreenSize;

#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8086) && !defined(MODE_T80100) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA)
    if (setblocks >= 11)
    {
        scaledviewwidth = SCREENWIDTH;
        viewheight = SCREENHEIGHT;
        viewheightshift = SCREENHEIGHT << FRACBITS;
        viewheightopt = (SCREENHEIGHT << FRACBITS) - SCREENHEIGHT;
        viewheight32 = SCREENHEIGHT << 16 | SCREENHEIGHT;
        automapheight = SCREENHEIGHT;
    }
    else
    {
        scaledviewwidth = setblocks * 32;
        viewheight = (setblocks * 168 / 10) & ~7;
        viewheightshift = viewheight << FRACBITS;
        viewheightopt = (viewheight << FRACBITS) - viewheight;
        viewheight32 = viewheight << 16 | viewheight;
        automapheight = SCREENHEIGHT - 32;
    }

    #if defined(MODE_13H) || defined(MODE_VBE2)
    endscreen = Mul320(viewwindowy + viewheight);
    #endif
#endif

#ifdef MODE_Y
    if (forcePotatoDetail || forceLowDetail || forceHighDetail)
    {
        if (forceHighDetail)
            detailshift = 0;
        else if (forceLowDetail)
            detailshift = 1;
        else
            detailshift = 2;
    }
    else
        detailshift = setdetail;
#endif

#ifdef MODE_Y
    viewwidth = scaledviewwidth >> detailshift;
    viewwidthhalf = viewwidth / 2;
#endif
#if defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    viewwidth = scaledviewwidth;
    viewwidthhalf = viewwidth / 2;
#endif

#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8086) && !defined(MODE_T80100) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA)
    viewwidthlimit = viewwidth - 1;
    centery = viewheight / 2;
    centerx = viewwidth / 2;
    centerxfrac = centerx << FRACBITS;
    centeryfrac = centery << FRACBITS;
    centeryfracshifted = centeryfrac >> 4;
    projection = centerxfrac;
    iprojection = FixedDiv(FRACUNIT << 8, projection);
#endif

#ifdef MODE_T4050
    colfunc = basecolfunc = R_DrawColumnText4050;

    if (untexturedSurfaces)
    {
        spanfunc = R_DrawSpanFlatText4050;
    }
    else
    {
        spanfunc = R_DrawSpanText4050;
    }

    if (flatSky)
        skyfunc = R_DrawSkyFlatText4050;
    else
        skyfunc = R_DrawColumnText4050;

    if (flatShadows)
        fuzzcolfunc = R_DrawFuzzColumnFastText4050;
    else if (saturnShadows)
        fuzzcolfunc = R_DrawFuzzColumnSaturnText4050;
    else
        fuzzcolfunc = R_DrawFuzzColumnText4050;
#endif
#ifdef MODE_T4025
    colfunc = basecolfunc = R_DrawColumnText4025;

    if (untexturedSurfaces)
    {
        spanfunc = R_DrawSpanFlatText4025;
    }
    else
    {
        spanfunc = R_DrawSpanText4025;
    }

    if (flatSky)
        skyfunc = R_DrawSkyFlatText4025;
    else
        skyfunc = R_DrawColumnText4025;

    if (flatShadows)
        fuzzcolfunc = R_DrawFuzzColumnFastText4025;
    else if (saturnShadows)
        fuzzcolfunc = R_DrawFuzzColumnSaturnText4025;
    else
        fuzzcolfunc = R_DrawFuzzColumnText4025;
#endif
#ifdef MODE_T8025
    colfunc = basecolfunc = R_DrawColumnText8025;

    if (untexturedSurfaces)
    {
        spanfunc = R_DrawSpanFlatText8025;
    }
    else
    {
        spanfunc = R_DrawSpanText8025;
    }

    if (flatSky)
        skyfunc = R_DrawSkyFlatText8025;
    else
        skyfunc = R_DrawColumnText8025;

    if (flatShadows)
        fuzzcolfunc = R_DrawFuzzColumnFastText8025;
    else if (saturnShadows)
        fuzzcolfunc = R_DrawFuzzColumnSaturnText8025;
    else
        fuzzcolfunc = R_DrawFuzzColumnText8025;
#endif
#ifdef MODE_MDA
    colfunc = basecolfunc = R_DrawLineColumnTextMDA;

    spanfunc = R_DrawSpanTextMDA;

    skyfunc = R_DrawSkyTextMDA;

    fuzzcolfunc = R_DrawLineColumnTextMDA;
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    colfunc = basecolfunc = R_DrawColumnText8050;

    if (untexturedSurfaces)
    {
        spanfunc = R_DrawSpanFlatText8050;
    }
    else
    {
        spanfunc = R_DrawSpanText8050;
    }

    if (flatSky)
        skyfunc = R_DrawSkyFlatText8050;
    else
        skyfunc = R_DrawColumnText8050;

    if (flatShadows)
        fuzzcolfunc = R_DrawFuzzColumnFastText8050;
    else if (saturnShadows)
        fuzzcolfunc = R_DrawFuzzColumnSaturnText8050;
    else
        fuzzcolfunc = R_DrawFuzzColumnText8050;
#endif

#if defined(MODE_T80100) || defined(MODE_T8086)
    colfunc = basecolfunc = R_DrawColumnText80100;

    if (untexturedSurfaces)
    {
        spanfunc = R_DrawSpanFlatText80100;
    }
    else
    {
        spanfunc = R_DrawSpanText80100;
    }

    if (flatSky)
        skyfunc = R_DrawSkyFlatText80100;
    else
        skyfunc = R_DrawColumnText80100;

    if (flatShadows)
        fuzzcolfunc = R_DrawFuzzColumnFastText80100;
    else if (saturnShadows)
        fuzzcolfunc = R_DrawFuzzColumnSaturnText80100;
    else
        fuzzcolfunc = R_DrawFuzzColumnText80100;
#endif

#ifdef MODE_Y
    switch (detailshift)
    {
    case 0:
        colfunc = basecolfunc = R_DrawColumn;

        if (untexturedSurfaces)
            spanfunc = R_DrawSpanFlat;
        else
            spanfunc = R_DrawSpan;

        if (flatSky)
            skyfunc = R_DrawSkyFlat;
        else
            skyfunc = R_DrawColumn;

        if (flatShadows)
            fuzzcolfunc = R_DrawFuzzColumnFast;
        else if (saturnShadows)
            fuzzcolfunc = R_DrawFuzzColumnSaturn;
        else
            fuzzcolfunc = R_DrawFuzzColumn;
        break;
    case 1:
        colfunc = basecolfunc = R_DrawColumnLow;

        if (untexturedSurfaces)
            spanfunc = R_DrawSpanFlatLow;
        else
            spanfunc = R_DrawSpanLow;

        if (flatSky)
            skyfunc = R_DrawSkyFlatLow;
        else
            skyfunc = R_DrawColumnLow;

        if (flatShadows)
            fuzzcolfunc = R_DrawFuzzColumnFastLow;
        else if (saturnShadows)
            fuzzcolfunc = R_DrawFuzzColumnSaturnLow;
        else
            fuzzcolfunc = R_DrawFuzzColumnLow;
        break;
    case 2:
        colfunc = basecolfunc = R_DrawColumnPotato;

        if (untexturedSurfaces)
            spanfunc = R_DrawSpanFlatPotato;
        else
            spanfunc = R_DrawSpanPotato;

        if (flatSky)
            skyfunc = R_DrawSkyFlatPotato;
        else
            skyfunc = R_DrawColumnPotato;

        if (flatShadows)
            fuzzcolfunc = R_DrawFuzzColumnFastPotato;
        else if (saturnShadows)
            fuzzcolfunc = R_DrawFuzzColumnSaturnPotato;
        else
            fuzzcolfunc = R_DrawFuzzColumnPotato;
        break;
    }
#endif
#if defined(USE_BACKBUFFER)
    colfunc = basecolfunc = R_DrawColumn_13h;

    if (untexturedSurfaces)
        spanfunc = R_DrawSpanFlat_13h;
    else
        spanfunc = R_DrawSpan_13h;

    if (flatSky)
        skyfunc = R_DrawSkyFlat_13h;
    else
        skyfunc = R_DrawColumn_13h;

    if (flatShadows)
        fuzzcolfunc = R_DrawFuzzColumnFast_13h;
    else if (saturnShadows)
        fuzzcolfunc = R_DrawFuzzColumnSaturn_13h;
    else
        fuzzcolfunc = R_DrawFuzzColumn_13h;
#endif

#ifdef MODE_VBE2_DIRECT
    colfunc = basecolfunc = R_DrawColumnVBE2;

    if (untexturedSurfaces)
        spanfunc = R_DrawSpanFlatVBE2;
    else
        spanfunc = R_DrawSpanVBE2;

    if (flatSky)
        skyfunc = R_DrawSkyFlatVBE2;
    else
        skyfunc = R_DrawColumnVBE2;

    if (flatShadows)
        fuzzcolfunc = R_DrawFuzzColumnFastVBE2;
    else if (saturnShadows)
        fuzzcolfunc = R_DrawFuzzColumnSaturnVBE2;
    else
        fuzzcolfunc = R_DrawFuzzColumnVBE2;
#endif

    R_InitBuffer(scaledviewwidth, viewheight);

    R_InitTextureMapping();

    // psprite scales
#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8086) && !defined(MODE_T80100) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA)
    pspritescale = FRACUNIT * viewwidth / SCREENWIDTH;
    pspriteiscale = FRACUNIT * SCREENWIDTH / viewwidth;
    pspriteiscaleneg = -pspriteiscale;
#endif

#ifdef MODE_Y
    pspriteiscaleshifted = pspriteiscale >> detailshift;
#endif
#if defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    pspriteiscaleshifted = pspriteiscale;
#endif

    // thing clipping
    SetWords(screenheightarray, viewheight, viewwidth);

    // planes
    for (i = 0; i < viewheight; i++)
    {
        dy = ((i - viewheight / 2) << FRACBITS) + FRACUNIT / 2;
        dy = abs(dy);

#if defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_T8086)
        yslope[i] = FixedDiv((viewwidth << 1) / 2 * FRACUNIT, dy);
#endif
#ifdef MODE_Y
        yslope[i] = FixedDiv((viewwidth << detailshift) / 2 * FRACUNIT, dy);
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(USE_BACKBUFFER) || defined(MODE_T4025) || defined(MODE_VBE2_DIRECT) || defined(MODE_MDA)
        yslope[i] = FixedDiv((viewwidth) / 2 * FRACUNIT, dy);
#endif
    }

    for (i = 0; i < viewwidth; i++)
    {
        cosadj = abs(finecosine[xtoviewangle[i] >> ANGLETOFINESHIFT]);
        distscale[i] = FixedDiv(FRACUNIT, cosadj);
    }

    // Calculate the light levels to use
    //  for each level / scale combination.
    for (i = 0; i < LIGHTLEVELS; i++)
    {
        startmap = ((LIGHTLEVELS - 1 - i) * 2) * NUMCOLORMAPS / LIGHTLEVELS;
        for (j = 0; j < MAXLIGHTSCALE; j++)
        {
#if defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_T8086)
            level = startmap - Mul320(j) / (viewwidth << 1) / DISTMAP;
#endif
#ifdef MODE_Y
            level = startmap - Mul320(j) / (viewwidth << detailshift) / DISTMAP;
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(USE_BACKBUFFER) || defined(MODE_T4025) || defined(MODE_VBE2_DIRECT) || defined(MODE_MDA)
            level = startmap - Mul320(j) / (viewwidth) / DISTMAP;
#endif
            if (level < 0)
                level = 0;
            else if (level > NUMCOLORMAPS - 1)
                level = NUMCOLORMAPS - 1;

            scalelight[i][j] = colormaps + level * 256;
        }
    }
}

//
// R_Init
//
extern int detailLevel;

void R_Init(void)
{
    R_InitData();
    // viewwidth / viewheight / detailLevel are set by the defaults
    printf(".");

    R_SetViewSize(screenblocks, detailLevel);
    printf(".");
    R_InitLightTables();
    printf(".");
    R_InitSkyMap();
    printf(".");
}

//
// R_PointInSubsector
//
subsector_t *
R_PointInSubsector(fixed_t x,
                   fixed_t y)
{
    node_t *node;
    byte side;
    int nodenum;
    fixed_t dx;
    fixed_t dy;
    fixed_t left;
    fixed_t right;

    nodenum = firstnode;

    while (!(nodenum & NF_SUBSECTOR))
    {
        node = &nodes[nodenum];

        if (!node->dx)
        {
            if (x <= node->x)
            {
                side = node->dy > 0;
            }
            else
            {
                side = node->dy < 0;
            }
        }
        else
        {
            if (!node->dy)
            {
                if (y <= node->y)
                {
                    side = node->dx < 0;
                }
                else
                {
                    side = node->dx > 0;
                }
            }
            else
            {
                dx = (x - node->x);
                dy = (y - node->y);

                // Try to quickly decide by looking at sign bits.
                if ((node->dy ^ node->dx ^ dx ^ dy) & 0x80000000)
                {
                    side = ((node->dy ^ dx) & 0x80000000) != 0;
                }
                else
                {
                    left = FixedMul(node->dys, dx);
                    right = FixedMul(dy, node->dxs);

                    side = right >= left;
                }
            }
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

    viewx = (players_mo)->x;
    viewxs = viewx >> FRACBITS;
    viewy = (players_mo)->y;
    viewyneg = -viewy;
    viewys = viewy >> FRACBITS;
    viewangle = (players_mo)->angle;
    extralight = players.extralight;

    viewz = players.viewz;

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
    
    #ifdef MODE_VBE2_DIRECT
    destview = destscreen + Mul320(viewwindowy) + viewwindowx;
    #endif

    #ifdef MODE_Y
    destview = destscreen + Mul80(viewwindowy) + (viewwindowx >> 2);
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
    R_ClearPlanes();
    R_ClearSprites();

    // check for new console commands.
    NetUpdate();

// Set potato mode VGA plane
#ifdef MODE_Y
    if (detailshift == 2)
    {
        outp(SC_INDEX + 1, 15);
    }
#endif

    // The head node is the last node output.
    R_RenderBSPNode(firstnode);

    // Check for new console commands.
    NetUpdate();

#ifdef MODE_T4050
    if (flatSurfaces)
        R_DrawPlanesFlatSurfacesText4050();
    else
        R_DrawPlanes();
#endif
#ifdef MODE_T4025
    if (flatSurfaces)
        R_DrawPlanesFlatSurfacesText4025();
    else
        R_DrawPlanes();
#endif
#ifdef MODE_T8025
    if (flatSurfaces)
        R_DrawPlanesFlatSurfacesText8025();
    else
        R_DrawPlanes();
#endif
#ifdef MODE_MDA
    R_DrawPlanesFlatSurfacesTextMDA();
#endif

#if defined(MODE_T80100) || defined(MODE_T8086)
    if (flatSurfaces)
        R_DrawPlanesFlatSurfacesText80100();
    else
        R_DrawPlanes();
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)
    if (flatSurfaces)
        R_DrawPlanesFlatSurfacesText8050();
    else
        R_DrawPlanes();
#endif
#ifdef MODE_Y
    if (flatSurfaces)
        switch (detailshift)
        {
        case 0:
            R_DrawPlanesFlatSurfaces();
            break;
        case 1:
            R_DrawPlanesFlatSurfacesLow();
            break;
        case 2:
            R_DrawPlanesFlatSurfacesPotato();
            break;
        }
    else
        R_DrawPlanes();
#endif
#if defined(USE_BACKBUFFER)
    if (flatSurfaces)
        R_DrawPlanesFlatSurfaces_13h();
    else
        R_DrawPlanes();
#endif
#ifdef MODE_VBE2_DIRECT
    if (flatSurfaces)
        R_DrawPlanesFlatSurfacesVBE2();
    else
        R_DrawPlanes();
#endif

    // Check for new console commands.
    NetUpdate();

    R_DrawMasked();

    // Check for new console commands.
    NetUpdate();
}
