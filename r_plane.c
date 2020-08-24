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
#define MAXSLOTS 32
visplane_t *visplanes[MAXSLOTS]; // killough
visplane_t *freetail;                // killough
visplane_t **freehead = &freetail;   // killough
visplane_t *floorplane, *ceilingplane;

// ?
#define MAXOPENINGS SCREENWIDTH * 64
short openings[MAXOPENINGS];
short *lastopening;

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

#define R_VisplaneHash(picnum, lightlevel, height) \
    (((unsigned)(picnum)*2 + (unsigned)(lightlevel) + (unsigned)(height)*4) & (MAXSLOTS - 1))

//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes(void)
{
    // Doh!
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
void R_MapPlane(int y, int x1)
{
    angle_t angle;
    fixed_t distance;
    fixed_t length;
    unsigned index;

    if (planeheight != cachedheight[y])
    {
        cachedheight[y] = planeheight;
        distance = cacheddistance[y] = FixedMul(planeheight, yslope[y]);
        if (!untexturedSurfaces)
        {
            ds_xstep = cachedxstep[y] = FixedMul(distance, basexscale);
            ds_ystep = cachedystep[y] = FixedMul(distance, baseyscale);
        }
    }
    else
    {
        distance = cacheddistance[y];
        if (!untexturedSurfaces)
        {
            ds_xstep = cachedxstep[y];
            ds_ystep = cachedystep[y];
        }
    }

    if (!untexturedSurfaces)
    {
        length = FixedMul(distance, distscale[x1]);
        angle = (viewangle + xtoviewangle[x1]) >> ANGLETOFINESHIFT;
        ds_xfrac = viewx + FixedMul(finecosine[angle], length);
        ds_yfrac = -viewy - FixedMul(finesine[angle], length);
    }

    if (fixedcolormap)
        ds_colormap = fixedcolormap;
    else
    {
        index = distance >> LIGHTZSHIFT;

        if (index >= MAXLIGHTZ)
            index = MAXLIGHTZ - 1;

        ds_colormap = planezlight[index];
    }

    ds_y = y;
    ds_x1 = x1;

    // high or low detail
    spanfunc();
}

//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes(void)
{
    int i;
    angle_t angle;
    fixed_t optCosine, optSine;

    // opening / clipping determination
    for (i = 0; i < viewwidth; i++)
        floorclip[i] = viewheight, ceilingclip[i] = -1;

    for (i = 0; i < MAXSLOTS; i++) // new code -- killough
        for (*freehead = visplanes[i], visplanes[i] = NULL; *freehead;)
            freehead = &(*freehead)->next;

    lastopening = openings;

    // texture calculation
    memset(cachedheight, 0, sizeof(cachedheight));

    // left to right mapping
    angle = (viewangle - ANG90) >> ANGLETOFINESHIFT;
    // scale will be unit scale at SCREENWIDTH/2 distance
    optCosine = finecosine[angle];
    optSine = finesine[angle];

    basexscale = ((abs(optCosine) >> 14) >= centerxfrac) ? ((optCosine ^ centerxfrac) >> 31) ^ MAXINT : FixedDiv2(optCosine, centerxfrac);
    baseyscale = -(((abs(optSine) >> 14) >= centerxfrac) ? ((optSine ^ centerxfrac) >> 31) ^ MAXINT : FixedDiv2(optSine, centerxfrac));
}

//
// R_FindPlane
//
visplane_t *R_FindPlane(fixed_t height, int picnum, int lightlevel)
{
    visplane_t *check;
    unsigned hash; // killough

    if (picnum == skyflatnum)    // killough 10/98
        lightlevel = height = 0; // killough 7/19/98: most skies map together

    // New visplane algorithm uses hash table -- killough
    hash = R_VisplaneHash(picnum, lightlevel, height);

    for (check = visplanes[hash]; check; check = check->next) // killough
        if (height == check->height &&
            picnum == check->picnum &&
            lightlevel == check->lightlevel)
            return check;

    check = freetail;
    if (!check)
        check = calloc(1, sizeof *check);
    else if (!(freetail = freetail->next))
        freehead = &freetail;
    check->next = visplanes[hash];
    visplanes[hash] = check;

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = SCREENWIDTH;
    check->maxx = -1;

    memset(check->top, 0xff, sizeof check->top);

    return check;
}

//
// R_CheckPlane
//
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

    for (x = intrl; x <= intrh && pl->top[x] == 0xff; x++)
        ;

    if (x > intrh)
        pl->minx = unionl, pl->maxx = unionh;
    else
    {
        unsigned hash = R_VisplaneHash(pl->picnum, pl->lightlevel, pl->height);
        
        visplane_t *new_pl = freetail;
        if (!new_pl)
            new_pl = calloc(1, sizeof *new_pl);
        else if (!(freetail = freetail->next))
            freehead = &freetail;
        new_pl->next = visplanes[hash];
        visplanes[hash] = new_pl;

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
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanes(void)
{
    visplane_t *pl;
    int light;
    int x;
    int stop;
    int angle;
    unsigned short i;

    byte t1, b1, t2, b2;
    for (i = 0; i < MAXSLOTS; i++)
        for (pl = visplanes[i]; pl; pl = pl->next)
        {
            if (!pl->modified)
                continue;

            if (pl->minx > pl->maxx)
                continue;

            // sky flat
            if (pl->picnum == skyflatnum)
            {
                dc_iscale = pspriteiscale >> detailshift;

                // Sky is allways drawn full bright,
                //  i.e. colormaps[0] is used.
                // Because of this hack, sky is not affected
                //  by INVUL inverse mapping.
                dc_colormap = colormaps;
                dc_texturemid = skytexturemid;
                for (x = pl->minx; x <= pl->maxx; x++)
                {
                    dc_yl = pl->top[x];
                    dc_yh = pl->bottom[x];

                    if (dc_yl <= dc_yh)
                    {
                        dc_x = x;

                        if (!flatSky)
                        {
                            angle = (viewangle + xtoviewangle[x]) >> ANGLETOSKYSHIFT;
                            dc_source = R_GetColumn(skytexture, angle);
                        }

                        skyfunc();
                    }
                }
                continue;
            }

            if (flatSurfaces)
            {
                //dc_iscale = pspriteiscale >> detailshift;
                dc_colormap = colormaps;

                dc_source = W_CacheLumpNum(firstflat +
                                               flattranslation[pl->picnum],
                                           PU_STATIC);

                for (x = pl->minx; x <= pl->maxx; x++)
                {
                    dc_yl = pl->top[x];
                    dc_yh = pl->bottom[x];

                    if (dc_yl <= dc_yh)
                    {
                        dc_x = x;

                        switch (detailshift)
                        {
                        case 0:
                            R_DrawColumnFlat();
                            break;
                        case 1:
                            R_DrawColumnFlatLow();
                            break;
                        case 2:
                            R_DrawColumnFlatPotato();
                            break;
                        }
                    }
                }

                Z_ChangeTag(dc_source, PU_CACHE);
            }
            else
            {
                // regular flat

                ds_source = W_CacheLumpNum(firstflat +
                                               flattranslation[pl->picnum],
                                           PU_STATIC);
                planeheight = abs(pl->height - viewz);
                light = (pl->lightlevel >> LIGHTSEGSHIFT) + extralight;

                if (light >= LIGHTLEVELS)
                    light = LIGHTLEVELS - 1;

                if (light < 0)
                    light = 0;

                planezlight = zlight[light];
                pl->top[pl->maxx + 1] = 0xff;
                pl->top[pl->minx - 1] = 0xff;

                stop = pl->maxx + 1;

                for (x = pl->minx; x <= stop; x++)
                {
                    t1 = pl->top[x - 1];
                    b1 = pl->bottom[x - 1];
                    t2 = pl->top[x];
                    b2 = pl->bottom[x];

                    ds_x2 = x - 1;

                    while (t1 < t2 && t1 <= b1)
                    {
                        R_MapPlane(t1, spanstart[t1]);
                        t1++;
                    }
                    while (b1 > b2 && b1 >= t1)
                    {
                        R_MapPlane(b1, spanstart[b1]);
                        b1--;
                    }

                    while (t2 < t1 && t2 <= b2)
                    {
                        spanstart[t2] = x;
                        t2++;
                    }
                    while (b2 > b1 && b2 >= t2)
                    {
                        spanstart[b2] = x;
                        b2--;
                    }
                }

                Z_ChangeTag(ds_source, PU_CACHE);
            }
        }
}
