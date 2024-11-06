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

#include <string.h>
#include <stdlib.h>
#include "options.h"
#include "i_system.h"
#include "i_ibm.h"
#include "i_debug.h"
#include "z_zone.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_data.h"
#include "r_draw.h"

#include "std_func.h"

#include <conio.h>
#include "i_debug.h"

#include "sizeopt.h"

#define SC_INDEX 0x3C4
#define ANGLETOSKYSHIFT 22

//
// opening
//

// Here comes the obnoxious "visplane".
#define MAXVISPLANES 128
visplane_t visplanes[MAXVISPLANES];
visplane_t *lastvisplane;
visplane_t *floorplane;
visplane_t *ceilingplane;

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

fixed_t cachedstep[SCREENHEIGHT];

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
    if (y == PIXELCOORD_MAX)
        return;
#if defined(MODE_CGA16) || defined(MODE_CGA512) || defined(MODE_CGA_AFH)
    if (y & 1)
        return;
#endif
    BOUNDS_CHECK(x1, y);
    ds_x1 = x1;
    ds_y = y;

    if (planeheight != cachedheight[y])
    {
        fixed_t step;

        cachedheight[y] = planeheight;
        distance = cacheddistance[y] = FixedMulEDX(planeheight, yslope[y]);

        step = FixedMulHStep(distance, basexscale);
        step |= FixedMulLStep(distance, baseyscale);
        ds_step = step;
        cachedstep[y] = step;
    }
    else
    {
        distance = cacheddistance[y];
        ds_step = cachedstep[y];
    }

    angle = (viewangle + xtoviewangle[x1]) >> ANGLETOFINESHIFT;
    length = FixedMul(distance, distscale[x1]);

    ds_frac = (((viewx + FixedMul(finecosine[angle], length)) << 10) & 0xFFFF0000) | (((viewyneg - FixedMul(finesine[angle], length)) >> 6) & 0xFFFF);

    if (fixedcolormap)
        ds_colormap = fixedcolormap;
    else
    {
        if (distance >= (MAXLIGHTZ << LIGHTZSHIFT))
        {
            ds_colormap = planezlight[MAXLIGHTZ - 1];
        }
        else
        {
            ds_colormap = planezlight[distance >> LIGHTZSHIFT];
        }
    }

    // high or low detail
    spanfunc();
}

void R_MapPlaneFlat(int y, int x1)
{
    fixed_t distance;
    unsigned index;
    if (y == PIXELCOORD_MAX)
        return;
#if defined(MODE_CGA16) || defined(MODE_CGA512) || defined(MODE_CGA_AFH)
    if (y & 1)
        return;
#endif
    BOUNDS_CHECK(x1, y);
    ds_x1 = x1;
    ds_y = y;

    if (planeheight != cachedheight[y])
    {
        cachedheight[y] = planeheight;
        distance = cacheddistance[y] = FixedMulEDX(planeheight, yslope[y]);
    }
    else
    {
        distance = cacheddistance[y];
    }

    if (fixedcolormap)
        ds_colormap = fixedcolormap;
    else
    {
        if (distance >= (MAXLIGHTZ << LIGHTZSHIFT))
        {
            ds_colormap = planezlight[MAXLIGHTZ - 1];
        }
        else
        {
            ds_colormap = planezlight[distance >> LIGHTZSHIFT];
        }
    }

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

    int *floorclipint = (int *)floorclip;
    int *ceilingclipint = (int *)ceilingclip;

    // opening / clipping determination
    for (i = 0; i < viewwidthhalf; i += 4)
    {
        floorclipint[i] = viewheight32;
        ceilingclipint[i] = -1;
        floorclipint[i + 1] = viewheight32;
        ceilingclipint[i + 1] = -1;
        floorclipint[i + 2] = viewheight32;
        ceilingclipint[i + 2] = -1;
        floorclipint[i + 3] = viewheight32;
        ceilingclipint[i + 3] = -1;
    }

    lastvisplane = visplanes;
    lastopening = openings;

    // texture calculation
    SetDWords(cachedheight, 0, sizeof(cachedheight) / 4);

    // left to right mapping
    angle = (viewangle - ANG90) >> ANGLETOFINESHIFT;

    // scale will be unit scale at SCREENWIDTH/2 distance
    optCosine = finecosine[angle];
    optSine = finesine[angle];
    basexscale = ((abs(optCosine) >> 14) >= centerxfrac) ? ((optCosine ^ centerxfrac) >> 31) ^ MAXINT : FixedDiv2(optCosine, centerxfrac);
    baseyscale = -(((abs(optSine) >> 14) >= centerxfrac) ? ((optSine ^ centerxfrac) >> 31) ^ MAXINT : FixedDiv2(optSine, centerxfrac));
}

void R_ClearPlanesFlat(void)
{
    int i;
    int *floorclipint = (int *)floorclip;
    int *ceilingclipint = (int *)ceilingclip;

    // opening / clipping determination
    for (i = 0; i < viewwidthhalf; i += 4)
    {
        floorclipint[i] = viewheight32;
        ceilingclipint[i] = -1;
        floorclipint[i + 1] = viewheight32;
        ceilingclipint[i + 1] = -1;
        floorclipint[i + 2] = viewheight32;
        ceilingclipint[i + 2] = -1;
        floorclipint[i + 3] = viewheight32;
        ceilingclipint[i + 3] = -1;
    }

    lastvisplane = visplanes;
    lastopening = openings;
}

short skyflatnum;
short skytexture;

//
// R_FindPlane
//
visplane_t *R_FindPlane(fixed_t height, fixed_t prevheight, int picnum, int lightlevel)
{
    visplane_t *check;

    if (picnum == skyflatnum)
    {
        height = 0; // all skys map together
        lightlevel = 0;
    }

    for (check = visplanes; check < lastvisplane; check++)
    {
        if (height == check->height && picnum == check->picnum && lightlevel == check->lightlevel)
        {
            break;
        }
    }

    if (check < lastvisplane)
        return check;

    lastvisplane++;
    if (highResTimer) {
      check->height = FixedInterpolate(prevheight, height, interpolation_weight);
    } else {
      check->height = height;
    }
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = SCREENWIDTH;
    check->maxx = -1;

    SetDWords(check->top, 0xffffffff, sizeof(check->top) / 4);

    check->modified = 0;

    return check;
}

//
// R_CheckPlane
//
visplane_t *R_CheckPlane(visplane_t *pl, int start, int stop)
{
    int intrl;
    int intrh;
    int unionl;
    int unionh;
    int x;

    if (start < pl->minx)
    {
        intrl = pl->minx;
        unionl = start;
    }
    else
    {
        unionl = pl->minx;
        intrl = start;
    }

    if (stop > pl->maxx)
    {
        intrh = pl->maxx;
        unionh = stop;
    }
    else
    {
        unionh = pl->maxx;
        intrh = stop;
    }

    for (x = intrl; x <= intrh && pl->top[x] == PIXELCOORD_MAX; x++) // dropoff overflow
        ;

    if (x > intrh)
    {
        pl->minx = unionl;
        pl->maxx = unionh;

        // use the same one
        return pl;
    }

    // make a new visplane
    lastvisplane->height = pl->height;
    lastvisplane->picnum = pl->picnum;
    lastvisplane->lightlevel = pl->lightlevel;

    pl = lastvisplane++;
    pl->minx = start;
    pl->maxx = stop;

    SetDWords(pl->top, 0xffffffff, sizeof(pl->top) / 4);

    pl->modified = 0;

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
    int x, i;
    int stop;
    int angle;

    int lump;
    int ofs;
    int tex;
    int col;

    pixelcoord_t t1, b1, t2, b2;
    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        // regular flat

        ds_source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);
        planeheight = abs(pl->height - viewz);
        light = (pl->lightlevel >> LIGHTSEGSHIFT) + extralight;

        if (light > LIGHTLEVELS - 1)
            planezlight = zlight[LIGHTLEVELS - 1];
        else if (light < 0)
            planezlight = zlight[0];
        else
            planezlight = zlight[light];

        pl->top[pl->maxx + 1] = PIXELCOORD_MAX;
        pl->top[pl->minx - 1] = PIXELCOORD_MAX;

        stop = pl->maxx + 1;

        for (x = pl->minx; x <= stop; x++)
        {
            int count;

            t1 = pl->top[x - 1];
            b1 = pl->bottom[x - 1];
            t2 = pl->top[x];
            b2 = pl->bottom[x];

            ds_x2 = x - 1;

            while (t1 < t2 && t1 <= b1)
            {
                mapPlane(t1, spanstart[t1]);
                t1++;
            }
            while (b1 > b2 && b1 >= t1)
            {
                mapPlane(b1, spanstart[b1]);
                b1--;
            }

            //I_Printf("============\n");

            //I_Printf("t2 before: %b\n", t2);
            //I_Printf("t1: %b\n", t1);
            //I_Printf("b2: %b\n", b2);

            count = min(t1, b2+1) - t2;
            //I_Printf("Count: %b\n", count);

            if (count > 0)
            {
                SetDWords(spanstart+t2, x, count);
                t2 += count;
            }
            


            /*while (t2 < t1 && t2 <= b2)
            {
                spanstart[t2] = x;
                t2++;
            }*/

            //I_Printf("t2 after: %b\n", t2);

            //I_Printf("============\n");

            while (b2 > b1 && b2 >= t2)
            {
                spanstart[b2] = x;
                b2--;
            }
        }
    }
}

//
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanesFlatter(void)
{
    visplane_t *pl;

    byte color;
    int x;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        dc_color = colormaps[source[FLATPIXELCOLOR]];

        x = pl->minx;
        outp(SC_INDEX + 1, 1 << (x & 3));

        do
        {
            if (pl->top[x] > pl->bottom[x])
            {
                x += 4;
                continue;
            }

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();

            x += 4;
        } while (x <= pl->maxx);

        // Plane 1
        x = pl->minx + 1;

        if (x > pl->maxx)
            continue;

        outp(SC_INDEX + 1, 1 << (x & 3));

        do
        {
            if (pl->top[x] > pl->bottom[x])
            {
                x += 4;
                continue;
            }

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();

            x += 4;
        } while (x <= pl->maxx);

        // Plane 2
        x = pl->minx + 2;

        if (x > pl->maxx)
            continue;

        outp(SC_INDEX + 1, 1 << (x & 3));

        do
        {
            if (pl->top[x] > pl->bottom[x])
            {
                x += 4;
                continue;
            }

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();

            x += 4;
        } while (x <= pl->maxx);

        // Plane 3
        x = pl->minx + 3;

        if (x > pl->maxx)
            continue;

        outp(SC_INDEX + 1, 1 << (x & 3));

        do
        {
            if (pl->top[x] > pl->bottom[x])
            {
                x += 4;
                continue;
            }

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();

            x += 4;
        } while (x <= pl->maxx);
    }
}

void R_DrawPlanesFlatterLow(void)
{
    visplane_t *pl;

    byte color;
    int x;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        dc_color = colormaps[source[FLATPIXELCOLOR]];
        // Plane 0
        x = pl->minx;
        outp(SC_INDEX + 1, 3 << ((x & 1) << 1));

        do
        {
            if (pl->top[x] > pl->bottom[x])
            {
                x += 2;
                continue;
            }

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();

            x += 2;
        } while (x <= pl->maxx);

        // Plane 1
        x = pl->minx + 1;

        if (x > pl->maxx)
            continue;

        outp(SC_INDEX + 1, 3 << ((x & 1) << 1));

        do
        {
            if (pl->top[x] > pl->bottom[x])
            {
                x += 2;
                continue;
            }

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();

            x += 2;
        } while (x <= pl->maxx);
    }
}

void R_DrawPlanesFlatterPotato(void)
{
    visplane_t *pl;

    byte color;
    int x;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        dc_color = colormaps[source[FLATPIXELCOLOR]];

        for (x = pl->minx; x <= pl->maxx; x++)
        {
            if (pl->top[x] > pl->bottom[x])
                continue;

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();
        }
    }
}

#if defined(MODE_T8050) || defined(MODE_T8043)
void R_DrawPlanesFlatterText8050(void)
{
    visplane_t *pl;

    int count;
    unsigned short *dest;
    unsigned short color;
    int x;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);
        color = colormaps[source[FLATPIXELCOLOR]] << 8 | 219;

        for (x = pl->minx; x <= pl->maxx; x++)
        {
            if (pl->top[x] > pl->bottom[x])
                continue;

            count = pl->bottom[x] - pl->top[x];
            dest = textdestscreen + Mul80(pl->top[x]) + x;

            while (count >= 3)
            {
                *(dest) = color;
                *(dest + SCREENWIDTH / 4) = color;
                *(dest + SCREENWIDTH / 2) = color;
                *(dest + SCREENWIDTH / 4 + SCREENWIDTH / 2) = color;
                dest += SCREENWIDTH;
                count -= 4;
            }

            while (count >= 0)
            {
                *dest = color;
                dest += SCREENWIDTH / 4;
                count--;
            };
        }
    }
}
#endif

#if defined(MODE_T4050)
void R_DrawPlanesFlatterText4050(void)
{
    visplane_t *pl;

    int count;
    int countblock;
    unsigned short *dest;
    unsigned short vmem;
    unsigned short color;
    unsigned short colorblock;
    int x;
    byte odd;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        color = colormaps[source[FLATPIXELCOLOR]];
        colorblock = color << 8 | 219;

        for (x = pl->minx; x <= pl->maxx; x++)
        {
            if (pl->top[x] > pl->bottom[x])
                continue;

            odd = pl->top[x] & 1;
            dest = textdestscreen + Mul40(pl->top[x] / 2) + x;
            count = pl->bottom[x] - pl->top[x];

            if (count >= 1 && odd || count == 0)
            {
                vmem = *dest;

                if (odd)
                {
                    vmem = vmem & 0x0F00;
                    *dest = vmem | color << 12 | 223;

                    odd = 0;
                    dest += 40;
                }
                else
                {
                    vmem = vmem & 0xF000;
                    *dest = vmem | color << 8 | 223;
                    continue;
                }

                count--;
            }

            countblock = (count + 1) / 2;
            count -= countblock * 2;

            while (countblock)
            {
                *dest = colorblock;
                dest += 40;

                countblock--;
            }

            if (count >= 0 && !odd)
            {
                vmem = *dest;
                vmem = vmem & 0xF000;
                *dest = vmem | color << 8 | 223;
            }
        }
    }
}
#endif

#if defined(MODE_T4025)
void R_DrawPlanesFlatterText4025(void)
{
    visplane_t *pl;

    int count;
    unsigned short *dest;
    unsigned short color;
    int x;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);
        color = colormaps[source[FLATPIXELCOLOR]] << 8 | 219;

        for (x = pl->minx; x <= pl->maxx; x++)
        {
            if (pl->top[x] > pl->bottom[x])
                continue;

            count = pl->bottom[x] - pl->top[x];
            dest = textdestscreen + Mul40(pl->top[x]) + x;

            while (count >= 3)
            {
                *(dest) = color;
                *(dest + SCREENWIDTH / 8) = color;
                *(dest + SCREENWIDTH / 4) = color;
                *(dest + SCREENWIDTH / 8 + SCREENWIDTH / 4) = color;
                dest += SCREENWIDTH / 2;
                count -= 4;
            }

            while (count >= 0)
            {
                *dest = color;
                dest += SCREENWIDTH / 8;
                count--;
            };
        }
    }
}
#endif

#if defined(MODE_MDA)
void R_DrawPlanesFlatterTextMDA(void)
{
    visplane_t *pl;

    int count;
    int countblock;
    unsigned short *dest;
    unsigned short color;
    unsigned short colorblock;
    int x;
    byte odd;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        color = colormaps[source[FLATPIXELCOLOR]];
        colorblock = 0x0F << 8 | color;

        for (x = pl->minx; x <= pl->maxx; x++)
        {
            if (pl->top[x] > pl->bottom[x])
                continue;

            odd = pl->top[x] & 1;
            dest = textdestscreen + Mul80(pl->top[x] / 2) + x;
            count = pl->bottom[x] - pl->top[x];

            if (count >= 1 && odd || count == 0)
            {
                if (odd)
                {
                    *dest = 0x0F << 8 | 0xDB;

                    odd = 0;
                    dest += 80;
                }
                else
                {
                    *dest = 0x0F << 8 | 0xDB;
                    continue;
                }

                count--;
            }

            countblock = (count + 1) / 2;
            count -= countblock * 2;

            while (countblock)
            {
                *dest = colorblock;
                dest += 80;

                countblock--;
            }

            if (count >= 0 && !odd)
            {
                *dest = 0x0F << 8 | 0xDB;
            }
        }
    }
}
#endif

#if defined(MODE_T8025)
void R_DrawPlanesFlatterText8025(void)
{
    visplane_t *pl;

    int count;
    int countblock;
    unsigned short *dest;
    unsigned short vmem;
    unsigned short color;
    unsigned short colorblock;
    int x;
    byte odd;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        color = colormaps[source[FLATPIXELCOLOR]];
        colorblock = color << 8 | 219;

        for (x = pl->minx; x <= pl->maxx; x++)
        {
            if (pl->top[x] > pl->bottom[x])
                continue;

            odd = pl->top[x] & 1;
            dest = textdestscreen + Mul80(pl->top[x] / 2) + x;
            count = pl->bottom[x] - pl->top[x];

            if (count >= 1 && odd || count == 0)
            {
                vmem = *dest;

                if (odd)
                {
                    vmem = vmem & 0x0F00;
                    *dest = vmem | color << 12 | 223;

                    odd = 0;
                    dest += 80;
                }
                else
                {
                    vmem = vmem & 0xF000;
                    *dest = vmem | color << 8 | 223;
                    continue;
                }

                count--;
            }

            countblock = (count + 1) / 2;
            count -= countblock * 2;

            while (countblock)
            {
                *dest = colorblock;
                dest += 80;

                countblock--;
            }

            if (count >= 0 && !odd)
            {
                vmem = *dest;
                vmem = vmem & 0xF000;
                *dest = vmem | color << 8 | 223;
            }
        }
    }
}
#endif

#if defined(USE_BACKBUFFER)
void R_DrawPlanesFlatterBackbuffer(void)
{
    visplane_t *pl;

    byte color;
    int x;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        dc_color = colormaps[source[FLATPIXELCOLOR]];

        for (x = pl->minx; x <= pl->maxx; x++)
        {
#if defined(MODE_CGA16) || defined(MODE_CVB)
            if (x & 1)
                continue;
#endif

#if defined(MODE_CGA512)
            if (x & 3)
                continue;
#endif

            if (pl->top[x] > pl->bottom[x])
                continue;

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();
        }
    }
}

void R_DrawPlanesFlatterLowBackbuffer(void)
{
    visplane_t *pl;

    byte color;
    int x;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        dc_color = colormaps[source[FLATPIXELCOLOR]];

        for (x = pl->minx; x <= pl->maxx; x++)
        {
#if defined(MODE_CGA512)
            if (x & 1)
                continue;
#endif

            if (pl->top[x] > pl->bottom[x])
                continue;

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();
        }
    }
}

void R_DrawPlanesFlatterPotatoBackbuffer(void)
{
    visplane_t *pl;

    byte color;
    int x;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        dc_color = colormaps[source[FLATPIXELCOLOR]];

        for (x = pl->minx; x <= pl->maxx; x++)
        {
            if (pl->top[x] > pl->bottom[x])
                continue;

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();
        }
    }
}

#endif

#if defined(MODE_VBE2_DIRECT)
void R_DrawPlanesFlatterVBE2(void)
{
    visplane_t *pl;

    byte color;
    int x;
    byte *source;

    for (pl = visplanes; pl < lastvisplane; pl++)
    {
        if (!pl->modified || pl->minx > pl->maxx)
            continue;

        // sky flat
        if (pl->picnum == skyflatnum)
        {
            drawSky(pl);
            continue;
        }

        source = W_CacheLumpNum(firstflat + flattranslation[pl->picnum], PU_CACHE);

        dc_color = colormaps[source[FLATPIXELCOLOR]];

        for (x = pl->minx; x <= pl->maxx; x++)
        {
            if (pl->top[x] > pl->bottom[x])
                continue;

            dc_yh = pl->bottom[x];
            dc_yl = pl->top[x];
            dc_x = x;

            spanfunc();
        }
    }
}
#endif

void R_DrawSky(visplane_t *pl)
{
    int angle;

    int lump;
    int ofs;
    int tex;
    int col;

    int x;

    dc_iscale = pspriteiscaleshifted_sky;

    dc_colormap = fixedcolormap ? fixedcolormap : colormaps;
    dc_texturemid = 100 * FRACUNIT;

    tex = skytexture;

    for (x = pl->minx; x <= pl->maxx; x++)
    {
#if defined(MODE_CGA16) || defined(MODE_CVB)
        if (detailshift == DETAIL_HIGH)
            if (x & 1)
                continue;
#endif

#if defined(MODE_CGA512)
        switch (detailshift)
        {
        case DETAIL_HIGH:
            if (x & 3)
                continue;
            break;
        case DETAIL_LOW:
            if (x & 1)
                continue;
            break;
        }
#endif

        dc_yl = pl->top[x];
        dc_yh = pl->bottom[x];

        if (dc_yl > dc_yh)
            continue;

        dc_x = x;

        angle = (viewangle + xtoviewangle[x]) >> ANGLETOSKYSHIFT;

        col = angle;
        col &= texturewidthmask[tex];
        lump = texturecolumnlump[tex][col];
        ofs = texturecolumnofs[tex][col];

        dc_source = (byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs;

        skyfunc();
    }
}

void R_DrawSkyFlatPlanar(visplane_t *pl)
{
    byte color;
    int x;
    byte *source;

    dc_color = 220;

    x = pl->minx;

    if (x > pl->maxx)
        return;

    outp(SC_INDEX + 1, 1 << (x & 3));

    do
    {
        if (pl->top[x] > pl->bottom[x])
        {
            x += 4;
            continue;
        }

        dc_yh = pl->bottom[x];
        dc_yl = pl->top[x];
        dc_x = x;

        skyfunc();

        x += 4;
    } while (x <= pl->maxx);

    // Plane 1
    x = pl->minx + 1;

    if (x > pl->maxx)
        return;

    outp(SC_INDEX + 1, 1 << (x & 3));

    do
    {
        if (pl->top[x] > pl->bottom[x])
        {
            x += 4;
            continue;
        }

        dc_yh = pl->bottom[x];
        dc_yl = pl->top[x];
        dc_x = x;

        skyfunc();

        x += 4;
    } while (x <= pl->maxx);

    // Plane 2
    x = pl->minx + 2;

    if (x > pl->maxx)
        return;

    outp(SC_INDEX + 1, 1 << (x & 3));

    do
    {
        if (pl->top[x] > pl->bottom[x])
        {
            x += 4;
            continue;
        }

        dc_yh = pl->bottom[x];
        dc_yl = pl->top[x];
        dc_x = x;

        skyfunc();

        x += 4;
    } while (x <= pl->maxx);

    // Plane 3
    x = pl->minx + 3;

    if (x > pl->maxx)
        return;

    outp(SC_INDEX + 1, 1 << (x & 3));

    do
    {
        if (pl->top[x] > pl->bottom[x])
        {
            x += 4;
            continue;
        }

        dc_yh = pl->bottom[x];
        dc_yl = pl->top[x];
        dc_x = x;

        skyfunc();

        x += 4;
    } while (x <= pl->maxx);

}

void R_DrawSkyFlatPlanarLow(visplane_t *pl)
{
    byte color;
    int x;
    byte *source;

    dc_color = 220;

    // Plane 0
    x = pl->minx;
    outp(SC_INDEX + 1, 3 << ((x & 1) << 1));

    do
    {
        if (pl->top[x] > pl->bottom[x])
        {
            x += 2;
            continue;
        }

        dc_yh = pl->bottom[x];
        dc_yl = pl->top[x];
        dc_x = x;

        skyfunc();

        x += 2;
    } while (x <= pl->maxx);

    // Plane 1
    x = pl->minx + 1;

    if (x > pl->maxx)
        return;

    outp(SC_INDEX + 1, 3 << ((x & 1) << 1));

    do
    {
        if (pl->top[x] > pl->bottom[x])
        {
            x += 2;
            continue;
        }

        dc_yh = pl->bottom[x];
        dc_yl = pl->top[x];
        dc_x = x;

        skyfunc();

        x += 2;
    } while (x <= pl->maxx);

}


void R_DrawSkyFlat(visplane_t *pl)
{
    int x;

    dc_color = 220;

    for (x = pl->minx; x <= pl->maxx; x++)
    {
#if defined(MODE_CGA16) || defined(MODE_CVB)
        if (detailshift == DETAIL_HIGH)
            if (x & 1)
                continue;
#endif

#if defined(MODE_CGA512)
        switch (detailshift)
        {
        case DETAIL_HIGH:
            if (x & 3)
                continue;
            break;
        case DETAIL_LOW:
            if (x & 1)
                continue;
            break;
        }
#endif

        dc_yl = pl->top[x];
        dc_yh = pl->bottom[x];

        if (dc_yl > dc_yh)
            continue;
        dc_x = x;

        skyfunc();
    }
}
