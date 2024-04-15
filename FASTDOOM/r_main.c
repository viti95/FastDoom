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

#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA)
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

    // angle = (tantoangle[FixedDiv(dy, dx) >> DBITS] + ANG90) >> ANGLETOFINESHIFT;
    angle = (tantoangle[((dy >> 14 >= dx) ? ((dy ^ dx) >> 31) ^ MAXINT : FixedDiv2(dy, dx)) >> DBITS] + ANG90) >> ANGLETOFINESHIFT;

    // use as cosine
    // dist = FixedDiv(dx, finesine[angle]);
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
fixed_t R_ScaleFromGlobalAngle(int position)
{
    fixed_t scale;
    int anglea;
    int angleb;
    int sinea;
    int sineb;
    fixed_t num;
    int den;

    anglea = xtoviewangle90[position];
    angleb = anglea + viewangle - rw_normalangle;
    anglea >>= ANGLETOFINESHIFT;
    angleb >>= ANGLETOFINESHIFT;

    // both sines are allways positive
    sinea = finesine[anglea];
    sineb = finesine[angleb];
#if defined(MODE_T4050)
    num = FixedMulEDX(projection, sineb) << 1;
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    num = FixedMulEDX(projection, sineb) << detailshift;
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_MDA)
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

    if (blocks == 11 && gamestate == GS_LEVEL)
    {
        ST_createWidgets_mini();
    }
    else
    {
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

    // TODO Add more granular detection
    if (selectedCPU == AUTO_CPU) {
      switch(I_GetCPUModel()) {
        case 386:
          selectedCPU = INTEL_386SX;
          break;
        case 486:
          selectedCPU = INTEL_486;
          break;
        case 586:
        case 686:
          selectedCPU = INTEL_PENTIUM;
          break;
        default:
          selectedCPU = INTEL_386SX;
          break;
      }
    }
#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA)
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
        if (setblocks == 10) {
            scaledviewwidth = SCREENWIDTH;
        } else {
            scaledviewwidth = setblocks * (SCREENWIDTH / 10);
            // Stay multiple of 4
            scaledviewwidth &=~0x3;
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

#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
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

#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    viewwidth = scaledviewwidth >> detailshift;
    viewwidthhalf = viewwidth / 2;
#endif

#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA)
    viewwidthlimit = viewwidth - 1;
    centery = viewheight / 2;
    centerx = viewwidth / 2;
    centerxfrac = centerx << FRACBITS;
    centeryfrac = centery << FRACBITS;
    centeryfracshifted = centeryfrac >> 4;
    projection = centerxfrac;
#endif

#if defined(MODE_T4050)
    colfunc = basecolfunc = R_DrawColumnText4050;

    if (visplaneRender == VISPLANES_FLAT)
        spanfunc = R_DrawSpanFlatText4050;
    else
        spanfunc = R_DrawSpanText4050;

    if (flatSky)
        skyfunc = R_DrawSkyFlatText4050;
    else
        skyfunc = R_DrawColumnText4050;

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
    colfunc = basecolfunc = R_DrawColumnText4025;

    if (visplaneRender == VISPLANES_FLAT)
        spanfunc = R_DrawSpanFlatText4025;
    else
        spanfunc = R_DrawSpanText4025;

    if (flatSky)
        skyfunc = R_DrawSkyFlatText4025;
    else
        skyfunc = R_DrawColumnText4025;

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
#if defined(MODE_T8025)
    colfunc = basecolfunc = R_DrawColumnText8025;

    if (visplaneRender == VISPLANES_FLAT)
        spanfunc = R_DrawSpanFlatText8025;
    else
        spanfunc = R_DrawSpanText8025;

    if (flatSky)
        skyfunc = R_DrawSkyFlatText8025;
    else
        skyfunc = R_DrawColumnText8025;

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
    colfunc = basecolfunc = R_DrawLineColumnTextMDA;

    spanfunc = R_DrawSpanTextMDA;

    skyfunc = R_DrawSkyTextMDA;

    fuzzcolfunc = R_DrawLineColumnTextMDA;
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    colfunc = basecolfunc = R_DrawColumnText8050;

    if (visplaneRender == VISPLANES_FLAT)
        spanfunc = R_DrawSpanFlatText8050;
    else
        spanfunc = R_DrawSpanText8050;

    if (flatSky)
        skyfunc = R_DrawSkyFlatText8050;
    else
        skyfunc = R_DrawColumnText8050;

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

#if defined(MODE_X) || defined(MODE_Y)
    switch (detailshift)
    {
    case DETAIL_HIGH:
        switch (selectedCPU)
        {
        case UMC_GREEN_486:
        case CYRIX_5X86:
        case AMD_K5:
            colfunc = basecolfunc = R_DrawColumnFastLEA;
            break;
        default:
            colfunc = basecolfunc = R_DrawColumn;
            break;
        }

        if (visplaneRender == VISPLANES_FLAT)
            spanfunc = R_DrawSpanFlat;
        else
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case UMC_GREEN_486:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpan386SX;
                break;
            default:
                spanfunc = R_DrawSpan;
                break;
            }

        if (flatSky)
            skyfunc = R_DrawSkyFlat;
        else
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case AMD_K5:
                skyfunc = R_DrawColumnFastLEA;
                break;
            default:
                skyfunc = R_DrawColumn;
                break;
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
        switch (selectedCPU)
        {
        case UMC_GREEN_486:
        case CYRIX_5X86:
        case AMD_K5:
            colfunc = basecolfunc = R_DrawColumnLowFastLEA;
            break;
        default:
            colfunc = basecolfunc = R_DrawColumnLow;
            break;
        }

        if (visplaneRender == VISPLANES_FLAT)
            spanfunc = R_DrawSpanFlatLow;
        else
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case UMC_GREEN_486:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanLow386SX;
                break;
            default:
                spanfunc = R_DrawSpanLow;
                break;
            }

        if (flatSky)
            skyfunc = R_DrawSkyFlatLow;
        else
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case AMD_K5:
                skyfunc = R_DrawColumnLowFastLEA;
                break;
            default:
                skyfunc = R_DrawColumnLow;
                break;
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
        switch (selectedCPU)
        {
        case UMC_GREEN_486:
        case CYRIX_5X86:
        case AMD_K5:
            colfunc = basecolfunc = R_DrawColumnPotatoFastLEA;
            break;
        default:
            colfunc = basecolfunc = R_DrawColumnPotato;
            break;
        }

        if (visplaneRender == VISPLANES_FLAT)
            spanfunc = R_DrawSpanFlatPotato;
        else
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case UMC_GREEN_486:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanPotato386SX;
                break;
            default:
                spanfunc = R_DrawSpanPotato;
                break;
            }

        if (flatSky)
            skyfunc = R_DrawSkyFlatPotato;
        else
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case AMD_K5:
                skyfunc = R_DrawColumnPotatoFastLEA;
                break;
            default:
                skyfunc = R_DrawColumnPotato;
                break;
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
        switch (selectedCPU)
        {
        case UMC_GREEN_486:
        case CYRIX_5X86:
        case AMD_K5:
            colfunc = basecolfunc = R_DrawColumnBackbufferFastLEA;
            break;
        default:
            colfunc = basecolfunc = R_DrawColumnBackbuffer;
            break;
        }

        if (visplaneRender == VISPLANES_FLAT)
            spanfunc = R_DrawSpanFlatBackbuffer;
        else
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case UMC_GREEN_486:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanBackbuffer386SX;
                break;
            default:
                spanfunc = R_DrawSpanBackbuffer;
                break;
            }

        if (flatSky)
            skyfunc = R_DrawSkyFlatBackbuffer;
        else
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case AMD_K5:
                skyfunc = R_DrawColumnBackbufferFastLEA;
                break;
            default:
                skyfunc = R_DrawColumnBackbuffer;
                break;
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
        switch (selectedCPU)
        {
        case UMC_GREEN_486:
        case CYRIX_5X86:
        case AMD_K5:
            colfunc = basecolfunc = R_DrawColumnLowBackbufferFastLEA;
            break;
        default:
            colfunc = basecolfunc = R_DrawColumnLowBackbuffer;
            break;
        }

        if (visplaneRender == VISPLANES_FLAT)
            spanfunc = R_DrawSpanFlatLowBackbuffer;
        else
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case UMC_GREEN_486:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanLowBackbuffer386SX;
                break;
            default:
                spanfunc = R_DrawSpanLowBackbuffer;
                break;
            }

        if (flatSky)
            skyfunc = R_DrawSkyFlatLowBackbuffer;
        else
            switch (selectedCPU)
            {
            case UMC_GREEN_486:
            case CYRIX_5X86:
            case AMD_K5:
                skyfunc = R_DrawColumnLowBackbufferFastLEA;
                break;
            default:
                skyfunc = R_DrawColumnLowBackbuffer;
                break;
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
        colfunc = basecolfunc = R_DrawColumnPotatoBackbuffer;

        if (visplaneRender == VISPLANES_FLAT)
            spanfunc = R_DrawSpanFlatPotatoBackbuffer;
        else
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case UMC_GREEN_486:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanPotatoBackbuffer386SX;
                break;
            default:
                spanfunc = R_DrawSpanPotatoBackbuffer;
                break;
            }

        if (flatSky)
            skyfunc = R_DrawSkyFlatPotatoBackbuffer;
        else
            skyfunc = R_DrawColumnPotatoBackbuffer;

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
        colfunc = basecolfunc = R_DrawColumnVBE2;

        if (visplaneRender == VISPLANES_FLAT)
            spanfunc = R_DrawSpanFlatVBE2;
        else
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case UMC_GREEN_486:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanVBE2_386SX;
                break;
            default:
                spanfunc = R_DrawSpanVBE2;
                break;
            }

        if (flatSky)
            skyfunc = R_DrawSkyFlatVBE2;
        else
            skyfunc = R_DrawColumnVBE2;

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
        colfunc = basecolfunc = R_DrawColumnLowVBE2;

        if (visplaneRender == VISPLANES_FLAT)
            spanfunc = R_DrawSpanFlatLowVBE2;
        else
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case UMC_GREEN_486:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanLowVBE2_386SX;
                break;
            default:
                spanfunc = R_DrawSpanLowVBE2;
                break;
            }

        if (flatSky)
            skyfunc = R_DrawSkyFlatLowVBE2;
        else
            skyfunc = R_DrawColumnLowVBE2;

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
        colfunc = basecolfunc = R_DrawColumnPotatoVBE2;

        if (visplaneRender == VISPLANES_FLAT)
            spanfunc = R_DrawSpanFlatPotatoVBE2;
        else
            switch (selectedCPU)
            {
            case INTEL_386SX:
            case INTEL_386DX:
            case UMC_GREEN_486:
            case CYRIX_386DLC:
            case CYRIX_486:
                spanfunc = R_DrawSpanPotatoVBE2_386SX;
                break;
            default:
                spanfunc = R_DrawSpanPotatoVBE2;
                break;
            }

        if (flatSky)
            skyfunc = R_DrawSkyFlatPotatoVBE2;
        else
            skyfunc = R_DrawColumnPotatoVBE2;

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
#if !defined(MODE_T8050) && !defined(MODE_T8043) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_MDA)
    pspritescale = FRACUNIT * viewwidth / 320;
    pspriteiscale = FRACUNIT * 320 / viewwidth;
    pspriteiscaleneg = -pspriteiscale;
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    pspriteiscaleshifted = pspriteiscale >> detailshift;
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
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_MDA)
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
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
            level = startmap - MulScreenWidth(j) / (viewwidth << detailshift) / DISTMAP;
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_MDA)
            level = startmap - MulScreenWidth(j) / (viewwidth) / DISTMAP;
#endif
            if (level < 0)
                level = 0;
            else if (level > NUMCOLORMAPS - 1)
                level = NUMCOLORMAPS - 1;

            scalelight[i][j] = colormaps + level * 256;
        }
    }
    // I put this here so I could see which functions were having rendering
    // problems. Leaving it here for demonstration purposes.
#if (DEBUG_ENABLED==1)
    I_Printf("R_InitData: %s", "test");
    I_Printf("Render Functions:\n");
    I_Printf("\tcolfunc: %s\n", I_LookupSymbolName(colfunc));
    I_Printf("\tbasecolfunc: %s\n", I_LookupSymbolName(basecolfunc));
    I_Printf("\tfuzzcolfunc: %s\n", I_LookupSymbolName(fuzzcolfunc));
    I_Printf("\tspanfunc: %s\n", I_LookupSymbolName(spanfunc));
    I_Printf("\tskyfunc: %s\n", I_LookupSymbolName(skyfunc));
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
subsector_t * R_PointInSubsector(fixed_t x, fixed_t y)
{
    int nodenum = firstnode;

    while (!(nodenum & NF_SUBSECTOR))
    {        
        node_t *node = &nodes[nodenum];
        byte side;
        fixed_t dx, dy;

        switch(node->dxySelector)
        {
            case 0:
                side = (x > node->x) ^ (node->dyGT0);
                break;
            case 1:
                side = (y > node->y) ^ (node->dxLT0);
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
                    fixed_t left = FixedMulEDX(node->dys, dx);
                    fixed_t right = FixedMulEDX(dy, node->dxs);

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

    viewx = (players_mo)->x;
    viewxs = viewx >> FRACBITS;
    viewy = (players_mo)->y;
    viewyneg = -viewy;
    viewys = viewy >> FRACBITS;
    viewangle = (players_mo)->angle;
    viewangle90 = viewangle + ANG90;
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

#if defined(MODE_VBE2_DIRECT)
    destview = destscreen + MulScreenWidth(viewwindowy) + viewwindowx;
#endif

#if defined(MODE_X) || defined(MODE_Y)
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
    R_ClearPlanes();
    R_ClearSprites();

    // check for new console commands.
    NetUpdate();

// Set potato mode VGA plane
#if defined(MODE_X) || defined(MODE_Y)
    if (detailshift == DETAIL_POTATO)
    {
        outp(SC_INDEX + 1, 15);
    }
#endif

    // The head node is the last node output.
    R_RenderBSPNode(firstnode);

    // Check for new console commands.
    NetUpdate();

#if defined(MODE_T4050)
    if (visplaneRender == VISPLANES_FLATTER)
        R_DrawPlanesFlatterText4050();
    else
        R_DrawPlanes();
#endif
#if defined(MODE_T4025)
    if (visplaneRender == VISPLANES_FLATTER)
        R_DrawPlanesFlatterText4025();
    else
        R_DrawPlanes();
#endif
#if defined(MODE_T8025)
    if (visplaneRender == VISPLANES_FLATTER)
        R_DrawPlanesFlatterText8025();
    else
        R_DrawPlanes();
#endif
#if defined(MODE_MDA)
    R_DrawPlanesFlatterTextMDA();
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)
    if (visplaneRender == VISPLANES_FLATTER)
        R_DrawPlanesFlatterText8050();
    else
        R_DrawPlanes();
#endif
#if defined(MODE_X) || defined(MODE_Y)
    if (visplaneRender == VISPLANES_FLATTER)
        switch (detailshift)
        {
        case DETAIL_HIGH:
            R_DrawPlanesFlatter();
            break;
        case DETAIL_LOW:
            R_DrawPlanesFlatterLow();
            break;
        case DETAIL_POTATO:
            R_DrawPlanesFlatterPotato();
            break;
        }
    else
        R_DrawPlanes();
#endif
#if defined(USE_BACKBUFFER)
    if (visplaneRender == VISPLANES_FLATTER)
        switch (detailshift)
        {
        case DETAIL_HIGH:
            R_DrawPlanesFlatterBackbuffer();
            break;
        case DETAIL_LOW:
            R_DrawPlanesFlatterLowBackbuffer();
            break;
        case DETAIL_POTATO:
            R_DrawPlanesFlatterPotatoBackbuffer();
            break;
        }
    else
        R_DrawPlanes();
#endif
#if defined(MODE_VBE2_DIRECT)
    if (visplaneRender == VISPLANES_FLATTER)
        switch (detailshift)
        {
        case DETAIL_HIGH:
            R_DrawPlanesFlatterVBE2();
            break;
        case DETAIL_LOW:
            R_DrawPlanesFlatterLowVBE2();
            break;
        case DETAIL_POTATO:
            R_DrawPlanesFlatterPotatoVBE2();
            break;
        }
    else
        R_DrawPlanes();
#endif

    // Check for new console commands.
    NetUpdate();

    R_DrawMasked();

    // Check for new console commands.
    NetUpdate();
}
