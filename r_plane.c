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
//	Here is a core component: drawing the floors and ceilings,
//	 while maintaining a per column clipping list only.
//	Moreover, the sky areas have to be determined.
//

#include <stdlib.h>

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"

planefunction_t floorfunc;
planefunction_t ceilingfunc;

//
// opening
//

// Here comes the obnoxious "visplane".
#define MAXVISPLANES 128
visplane_t *visplanes[MAXVISPLANES];
visplane_t *floorplane;
visplane_t *ceilingplane;
visplane_t *freetail;              // killough
visplane_t **freehead = &freetail; // killough

// ?
#define MAXOPENINGS SCREENWIDTH * 64
short openings[MAXOPENINGS];
short *lastopening;

#define R_VisplaneHash(picnum, lightlevel, height) \
    (((unsigned)(picnum)*3 + (unsigned)(lightlevel) + (unsigned)(height)*7) & (MAXVISPLANES - 1))

//
// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1
//
short floorclip[SCREENWIDTH];
short ceilingclip[SCREENWIDTH];

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
int spanstart[SCREENHEIGHT];
int spanstop[SCREENHEIGHT];

//
// texture mapping
//
lighttable_t **planezlight;
fixed_t planeheight;

fixed_t yslope[SCREENHEIGHT];
fixed_t distscale[SCREENWIDTH];
fixed_t basexscale;
fixed_t baseyscale;

fixed_t cachedheight[SCREENHEIGHT];
fixed_t cacheddistance[SCREENHEIGHT];
fixed_t cachedxstep[SCREENHEIGHT];
fixed_t cachedystep[SCREENHEIGHT];

//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes(void)
{
    // Doh!
}

// New function, by Lee Killough

visplane_t *R_NewVisplane(unsigned hash)
{
    visplane_t *check = freetail;
    if (!check)
        check = calloc(1, sizeof *check);
    else if (!(freetail = freetail->next))
        freehead = &freetail;
    check->next = visplanes[hash];
    visplanes[hash] = check;
    return check;
}

//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  basexscale
//  baseyscale
//  viewx
//  viewy
//
// BASIC PRIMITIVE
//

void R_MapPlane(int y, int x1, int x2)
{
    angle_t angle;
    fixed_t distance, length;
    unsigned index;

    if (planeheight != cachedheight[y])
    {
        cachedheight[y] = planeheight;
        distance = cacheddistance[y] = FixedMul(planeheight, yslope[y]);
        ds_xstep = cachedxstep[y] = FixedMul(distance, basexscale);
        ds_ystep = cachedystep[y] = FixedMul(distance, baseyscale);
    }
    else
    {
        distance = cacheddistance[y];
        ds_xstep = cachedxstep[y];
        ds_ystep = cachedystep[y];
    }

    length = FixedMul(distance, distscale[x1]);
    angle = (viewangle + xtoviewangle[x1]) >> ANGLETOFINESHIFT;

    ds_xfrac = viewx + FixedMul(finecosine[angle], length);
    ds_yfrac = -viewy - FixedMul(finesine[angle], length);

    if (!(ds_colormap = fixedcolormap))
    {
        index = distance >> LIGHTZSHIFT;
        if (index >= MAXLIGHTZ)
            index = MAXLIGHTZ - 1;
        ds_colormap = planezlight[index];
    }

    ds_y = y;
    ds_x1 = x1;
    ds_x2 = x2;

    R_DrawSpan();
}

//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes(void)
{
    int i;
    angle_t angle;

    // opening / clipping determination
    for (i = 0; i < viewwidth; i++)
        floorclip[i] = viewheight, ceilingclip[i] = -1;

    for (i = 0; i < MAXVISPLANES; i++) // new code -- killough
        for (*freehead = visplanes[i], visplanes[i] = NULL; *freehead;)
            freehead = &(*freehead)->next;

    lastopening = openings;

    // texture calculation
    memset(cachedheight, 0, sizeof(cachedheight));

    // left to right mapping
    angle = (viewangle - ANG90) >> ANGLETOFINESHIFT;
    // scale will be unit scale at SCREENWIDTH/2 distance
    basexscale = FixedDiv(finecosine[angle], centerxfrac);
    baseyscale = -FixedDiv(finesine[angle], centerxfrac);
}

visplane_t *R_FindPlane(fixed_t height, int picnum, int lightlevel)
{
    visplane_t *check;
    unsigned hash; // killough

    // New visplane algorithm uses hash table -- killough
    hash = R_VisplaneHash(picnum, lightlevel, height);

    for (check = visplanes[hash]; check; check = check->next) // killough
        if (height == check->height &&
            picnum == check->picnum &&
            lightlevel == check->lightlevel)
            return check;

    check = R_NewVisplane(hash); // killough

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = viewwidth; // Was SCREENWIDTH -- killough 11/98
    check->maxx = -1;

    memset(check->top, 0xff, sizeof check->top);

    return check;
}

visplane_t *R_CheckPlane(visplane_t *pl, int start, int stop)
{
    int intrl, intrh, unionl, unionh, x;

    if (start < pl->minx)
        intrl = pl->minx, unionl = start;
    else
        unionl = pl->minx, intrl = start;

    if (stop > pl->maxx)
        intrh = pl->maxx, unionh = stop;
    else
        unionh = pl->maxx, intrh = stop;

    for (x = intrl; x <= intrh && pl->top[x] == 0xffff; x++)
        ;

    if (x > intrh)
        pl->minx = unionl, pl->maxx = unionh;
    else
    {
        unsigned hash = R_VisplaneHash(pl->picnum, pl->lightlevel, pl->height);
        visplane_t *new_pl = R_NewVisplane(hash);

        new_pl->height = pl->height;
        new_pl->picnum = pl->picnum;
        new_pl->lightlevel = pl->lightlevel;
        pl = new_pl;
        pl->minx = start;
        pl->maxx = stop;
        memset(pl->top, 0xff, sizeof pl->top);
    }

    return pl;
}

//
// R_MakeSpans
//
void R_MakeSpans(int x,
                 int t1,
                 int b1,
                 int t2,
                 int b2)
{
    for (; t1 < t2 && t1 <= b1; t1++)
        R_MapPlane(t1, spanstart[t1], x - 1);
    for (; b1 > b2 && b1 >= t1; b1--)
        R_MapPlane(b1, spanstart[b1], x - 1);
    while (t2 < t1 && t2 <= b2)
        spanstart[t2++] = x;
    while (b2 > b1 && b2 >= t2)
        spanstart[b2--] = x;
}

void R_DrawPlane(visplane_t *pl)
{
    int x;
    if (pl->minx <= pl->maxx)
    {
        int stop, light;

        ds_source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum],
                                   PU_STATIC);

        planeheight = abs(pl->height - viewz);
        light = (pl->lightlevel >> LIGHTSEGSHIFT) + extralight;

        if (light >= LIGHTLEVELS)
            light = LIGHTLEVELS - 1;

        if (light < 0)
            light = 0;

        stop = pl->maxx + 1;
        planezlight = zlight[light];
        pl->top[pl->minx - 1] = pl->top[stop] = 0xffff;

        for (x = pl->minx; x <= stop; x++)
            R_MakeSpans(x, pl->top[x - 1], pl->bottom[x - 1], pl->top[x], pl->bottom[x]);

        Z_ChangeTag(ds_source, PU_CACHE);
    }
}

//
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanes(void)
{
    visplane_t *pl;
    int i;
    for (i = 0; i < MAXVISPLANES; i++)
        for (pl = visplanes[i]; pl; pl = pl->next)
            R_DrawPlane(pl);
}
