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

#include <stdlib.h>
#include <math.h>
#include <conio.h>

#include "doomdef.h"
#include "doomstat.h"
#include "d_net.h"

#include "m_misc.h"

#include "r_local.h"
#include "r_sky.h"

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW 2048

#define SC_INDEX 0x3C4

// increment every time a check is made
int validcount = 1;

lighttable_t *fixedcolormap;
extern lighttable_t **walllights;

int centerx;
int centery;

fixed_t centerxfrac;
fixed_t centeryfrac;
fixed_t centeryfracshifted;
fixed_t projection;
fixed_t iprojection;

// just for profiling purposes
int framecount;

int linecount;
int loopcount;

fixed_t viewx;
fixed_t viewy;
fixed_t viewz;

angle_t viewangle;

fixed_t viewcos;
fixed_t viewsin;

player_t *viewplayer;

// 0 = high, 1 = low
int detailshift;

//
// precalculated math tables
//
angle_t clipangle;
angle_t fieldofview;

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

int R_PointOnSegSide(fixed_t x,
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
        if (x <= lx)
            return ldy > 0;

        return ldy < 0;
    }
    if (!ldy)
    {
        if (y <= ly)
            return ldx < 0;

        return ldx > 0;
    }

    dx = (x - lx);
    dy = (y - ly);

    // Try to quickly decide by looking at sign bits.
    if ((ldy ^ ldx ^ dx ^ dy) & 0x80000000)
    {
        if ((ldy ^ dx) & 0x80000000)
        {
            // (left is negative)
            return 1;
        }
        return 0;
    }

    left = FixedMul(ldy >> FRACBITS, dx);
    right = FixedMul(dy, ldx >> FRACBITS);

    if (right < left)
    {
        // front side
        return 0;
    }
    // back side
    return 1;
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
    fixed_t tempDivision, slopeDiv;

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
    fixed_t tempDivision, slopeDiv;

    viewx = x1;
    viewy = y1;

    x2 -= viewx;
    y2 -= viewy;

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

fixed_t
R_PointToDist(fixed_t x,
              fixed_t y)
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
        temp = dx;
        dx = dy;
        dy = temp;
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
    num = FixedMul(projection, sineb) << detailshift;
    den = FixedMul(rw_distance, sinea);

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

    clipangle = xtoviewangle[0];
    fieldofview = 2 * clipangle;
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

            if (level >= NUMCOLORMAPS)
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
boolean setsizeneeded;
int setblocks;
int setdetail;

void R_SetViewSize(int blocks,
                   int detail)
{
    setsizeneeded = true;
    setblocks = blocks;
    setdetail = detail;
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

    setsizeneeded = false;

    if (forceScreenSize)
        setblocks = forceScreenSize;

    if (setblocks == 11)
    {
        scaledviewwidth = SCREENWIDTH;
        viewheight = SCREENHEIGHT;
    }
    else
    {
        scaledviewwidth = setblocks * 32;
        viewheight = (setblocks * 168 / 10) & ~7;
    }

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

    viewwidth = scaledviewwidth >> detailshift;
    viewwidthlimit = viewwidth - 1;

    centery = viewheight / 2;
    centerx = viewwidth / 2;
    centerxfrac = centerx << FRACBITS;
    centeryfrac = centery << FRACBITS;
    centeryfracshifted = centeryfrac >> 4;
    projection = centerxfrac;
    iprojection = FixedDiv(FRACUNIT << 8, projection);

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

    R_InitBuffer(scaledviewwidth, viewheight);

    R_InitTextureMapping();

    // psprite scales
    pspritescale = FRACUNIT * viewwidth / SCREENWIDTH;
    pspriteiscale = FRACUNIT * SCREENWIDTH / viewwidth;
    pspriteiscaleshifted = pspriteiscale >> detailshift;

    // thing clipping
    for (i = 0; i < viewwidth; i++)
        screenheightarray[i] = viewheight;

    // planes
    for (i = 0; i < viewheight; i++)
    {
        dy = ((i - viewheight / 2) << FRACBITS) + FRACUNIT / 2;
        dy = abs(dy);
        yslope[i] = FixedDiv((viewwidth << detailshift) / 2 * FRACUNIT, dy);
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
            level = startmap - Mul320(j) / (viewwidth << detailshift) / DISTMAP;

            if (level < 0)
                level = 0;

            if (level > NUMCOLORMAPS - 1)
                level = NUMCOLORMAPS - 1;

            scalelight[i][j] = colormaps + level * 256;
        }
    }
}

//
// R_Init
//
extern int detailLevel;
extern int screenblocks;

void R_Init(void)
{
    R_InitData();
    // viewwidth / viewheight / detailLevel are set by the defaults
    printf(".");

    R_SetViewSize(screenblocks, detailLevel);
    R_InitPlanes();
    printf(".");
    R_InitLightTables();
    printf(".");
    R_InitSkyMap();
    printf(".");

    framecount = 0;
}

//
// R_PointInSubsector
//
subsector_t *
R_PointInSubsector(fixed_t x,
                   fixed_t y)
{
    node_t *node;
    int side;
    int nodenum;
    fixed_t dx;
    fixed_t dy;
    fixed_t left;
    fixed_t right;

    // single subsector is a special case
    if (!numnodes)
        return subsectors;

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
                    if ((node->dy ^ dx) & 0x80000000)
                    {
                        // (left is negative)
                        side = 1;
                    }
                    else
                    {
                        side = 0;
                    }
                }
                else
                {
                    left = FixedMul(node->dy >> FRACBITS, dx);
                    right = FixedMul(dy, node->dx >> FRACBITS);

                    if (right < left)
                    {
                        // front side
                        side = 0;
                    }
                    else
                    {
                        // back side
                        side = 1;
                    }
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
void R_SetupFrame(player_t *player)
{
    int i;

    viewplayer = player;
    viewx = player->mo->x;
    viewy = player->mo->y;
    viewangle = player->mo->angle;
    extralight = player->extralight;

    viewz = player->viewz;

    viewsin = finesine[viewangle >> ANGLETOFINESHIFT];
    viewcos = finecosine[viewangle >> ANGLETOFINESHIFT];

    if (player->fixedcolormap)
    {
        fixedcolormap = colormaps + player->fixedcolormap * 256 * sizeof(lighttable_t);

        walllights = scalelightfixed;

        for (i = 0; i < MAXLIGHTSCALE; i++)
            scalelightfixed[i] = fixedcolormap;
    }
    else
        fixedcolormap = 0;

    framecount++;
    validcount++;
    destview = destscreen + Mul80(viewwindowy) + (viewwindowx >> 2);
}

//
// R_RenderView
//
void R_RenderPlayerView(player_t *player)
{
    R_SetupFrame(player);

    // Clear buffers.
    R_ClearClipSegs();
    R_ClearDrawSegs();
    R_ClearPlanes();
    R_ClearSprites();

    // check for new console commands.
    NetUpdate();

    // Set potato mode VGA plane
    if (detailshift == 2)
    {
        outp(SC_INDEX + 1, 15);
    }

    // The head node is the last node output.
    R_RenderBSPNode(firstnode);

    // Check for new console commands.
    NetUpdate();

    R_DrawPlanes();

    // Check for new console commands.
    NetUpdate();

    R_DrawMasked();

    // Check for new console commands.
    NetUpdate();
}
