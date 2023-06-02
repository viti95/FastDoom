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
//	The actual span/column drawing functions.
//	Here find the main potential for optimization,
//	 e.g. inline assembly, different algorithms.
//

#include <conio.h>
#include "doomdef.h"
#include "options.h"
#include "i_system.h"
#include "i_ibm.h"
#include "z_zone.h"
#include "w_wad.h"
#include "r_data.h"

#include "r_local.h"

// Needs access to LFB (guess what).
#include "v_video.h"

// State.
#include "doomstat.h"

#include "sizeopt.h"

// status bar height at bottom of screen
#define SBARHEIGHT 32

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//

#if !defined(MODE_T8050) && !defined(MODE_T8025) && !defined(MODE_T4025) && !defined(MODE_T4050) && !defined(MODE_T8043) && !defined(MODE_MDA)
int viewwidth;
int viewheight;
int viewheightshift;
int viewheightopt;
int viewheight32;
int scaledviewwidth;
int viewwidthhalf;
int viewwidthlimit;
int viewwindowx;
int viewwindowy;
#endif

#if defined(MODE_13H) || defined(MODE_VBE2)
int endscreen;
int startscreen;
#endif

#if defined(USE_BACKBUFFER)
int columnofs[SCREENWIDTH];
byte *ylookup[SCREENHEIGHT];
#endif

#if defined(MODE_Y)
byte *ylookup[SCREENHEIGHT];
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_VBE2_DIRECT) || defined(MODE_MDA)
byte **ylookup;
#endif

int automapheight;

#if defined(USE_BACKBUFFER)
byte *background_buffer = 0;
#endif

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//

#define SC_INDEX 0x3C4
#define SC_RESET 0
#define SC_CLOCK 1
#define SC_MAPMASK 2
#define SC_CHARMAP 3
#define SC_MEMMODE 4

#define GC_INDEX 0x3CE
#define GC_SETRESET 0
#define GC_ENABLESETRESET 1
#define GC_COLORCOMPARE 2
#define GC_DATAROTATE 3
#define GC_READMAP 4
#define GC_MODE 5
#define GC_MISCELLANEOUS 6
#define GC_COLORDONTCARE 7
#define GC_BITMASK 8

//
// R_DrawColumn
// Source is the top of the column to scale.
//
lighttable_t *dc_colormap;
int dc_x;
int dc_yl;
int dc_yh;
fixed_t dc_iscale;
fixed_t dc_texturemid;

// first pixel in a column (possibly virtual)
byte *dc_source;

#if defined(MODE_Y)
void R_DrawSkyFlat(void)
{
    register int count;
    register byte *dest;

    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    dest = destview + Mul80(dc_yl) + (dc_x >> 2);

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = 220;
        *(dest + SCREENWIDTH / 4) = 220;
        *(dest + SCREENWIDTH / 2) = 220;
        *(dest + SCREENWIDTH / 4 + SCREENWIDTH / 2) = 220;
        dest += SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        *dest = 220;
        dest += SCREENWIDTH / 4;
        count--;
    };
}
#endif

#if defined(MODE_Y)
void R_DrawSkyFlatLow(void)
{
    register int count;
    register byte *dest;

    outp(SC_INDEX + 1, 3 << ((dc_x & 1) << 1));

    dest = destview + Mul80(dc_yl) + (dc_x >> 1);

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = 220;
        *(dest + SCREENWIDTH / 4) = 220;
        *(dest + SCREENWIDTH / 2) = 220;
        *(dest + SCREENWIDTH / 4 + SCREENWIDTH / 2) = 220;
        dest += SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        *dest = 220;
        dest += SCREENWIDTH / 4;
        count--;
    };
}
#endif

#if defined(MODE_Y)
void R_DrawSkyFlatPotato(void)
{
    register int count;
    register byte *dest;

    dest = destview + Mul80(dc_yl) + dc_x;

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = 220;
        *(dest + SCREENWIDTH / 4) = 220;
        *(dest + SCREENWIDTH / 2) = 220;
        *(dest + SCREENWIDTH / 4 + SCREENWIDTH / 2) = 220;
        dest += SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        *dest = 220;
        dest += SCREENWIDTH / 4;
        count--;
    };
}
#endif

#if defined(MODE_VBE2_DIRECT)
void R_DrawSkyFlatVBE2(void)
{
    int count;
    byte *dest;
    fixed_t frac, fracstep;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

    dest = destview + Mul320(dc_yl) + dc_x;

    do
    {
        *dest = 220;
        dest += SCREENWIDTH;
    } while (count--);
}

void R_DrawSkyFlatLowVBE2(void)
{
    int count;
    unsigned short *dest;

    dest = (unsigned short *)((byte *)destview + Mul320(dc_yl) + (dc_x << 1));
    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = 220 | 220 << 8;
        *(dest + SCREENWIDTH / 2) = 220 | 220 << 8;
        *(dest + 2 * SCREENWIDTH / 2) = 220 | 220 << 8;
        *(dest + 3 * SCREENWIDTH / 2) = 220 | 220 << 8;
        dest += 4 * SCREENWIDTH / 2;
        count -= 4;
    }

    while (count >= 0)
    {
        *(dest) = 220 | 220 << 8;
        dest += SCREENWIDTH / 2;
        count--;
    };
}

void R_DrawSkyFlatPotatoVBE2(void)
{
    int count;
    unsigned int *dest;

    dest = (unsigned int *)((byte *)destview + Mul320(dc_yl) + (dc_x << 2));
    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        *(dest + SCREENWIDTH / 4) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        *(dest + 2 * SCREENWIDTH / 4) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        *(dest + 3 * SCREENWIDTH / 4) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        dest += 4 * SCREENWIDTH / 4;
        count -= 4;
    }

    while (count >= 0)
    {
        *(dest) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        dest += SCREENWIDTH / 4;
        count--;
    };
}

void R_DrawFuzzColumnFlatVBE2(void)
{
    int count;
    byte *dest;
    fixed_t frac, fracstep;

    dest = destview + Mul320(dc_yl) + dc_x;

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = colormaps[6 * 256 + dest[0]];
        *(dest + SCREENWIDTH) = colormaps[6 * 256 + dest[SCREENWIDTH]];
        *(dest + 2 * SCREENWIDTH) = colormaps[6 * 256 + dest[2 * SCREENWIDTH]];
        *(dest + 3 * SCREENWIDTH) = colormaps[6 * 256 + dest[3 * SCREENWIDTH]];
        dest += 4 * SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        *dest = colormaps[6 * 256 + dest[0]];
        dest += SCREENWIDTH;
        count--;
    };
}

void R_DrawFuzzColumnFlatLowVBE2(void)
{
    int count;
    byte *dest;

    dest = destview + Mul320(dc_yl) + (dc_x << 1);

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        lighttable_t color = colormaps[6 * 256 + dest[0]];

        *(dest) = color;
        *(dest + 1) = color;

        color = colormaps[6 * 256 + dest[SCREENWIDTH]];

        *(dest + SCREENWIDTH) = color;
        *(dest + SCREENWIDTH + 1) = color;

        color = colormaps[6 * 256 + dest[2 * SCREENWIDTH]];

        *(dest + 2 * SCREENWIDTH) = color;
        *(dest + 2 * SCREENWIDTH + 1) = color;

        color = colormaps[6 * 256 + dest[3 * SCREENWIDTH]];

        *(dest + 3 * SCREENWIDTH) = color;
        *(dest + 3 * SCREENWIDTH + 1) = color;

        dest += 4 * SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        lighttable_t color = colormaps[6 * 256 + dest[0]];

        *(dest) = color;
        *(dest + 1) = color;

        dest += SCREENWIDTH;
        count--;
    };
}

void R_DrawFuzzColumnFlatPotatoVBE2(void)
{
    int count;
    byte *dest;

    dest = destview + Mul320(dc_yl) + (dc_x << 2);

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        lighttable_t color = colormaps[6 * 256 + dest[0]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        color = colormaps[6 * 256 + dest[SCREENWIDTH]];

        *(dest + SCREENWIDTH) = color;
        *(dest + SCREENWIDTH + 1) = color;
        *(dest + SCREENWIDTH + 2) = color;
        *(dest + SCREENWIDTH + 3) = color;

        color = colormaps[6 * 256 + dest[2 * SCREENWIDTH]];

        *(dest + 2 * SCREENWIDTH) = color;
        *(dest + 2 * SCREENWIDTH + 1) = color;
        *(dest + 2 * SCREENWIDTH + 2) = color;
        *(dest + 2 * SCREENWIDTH + 3) = color;

        color = colormaps[6 * 256 + dest[3 * SCREENWIDTH]];

        *(dest + 3 * SCREENWIDTH) = color;
        *(dest + 3 * SCREENWIDTH + 1) = color;
        *(dest + 3 * SCREENWIDTH + 2) = color;
        *(dest + 3 * SCREENWIDTH + 3) = color;

        dest += 4 * SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        lighttable_t color = colormaps[6 * 256 + dest[0]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        dest += SCREENWIDTH;
        count--;
    };
}

void R_DrawFuzzColumnFlatSaturnVBE2(void)
{
    int count;
    byte *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul320(dc_yl) + dc_x;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
    }

    do
    {
        *dest = 0;
        dest += 2 * SCREENWIDTH;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = 0;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = 0;
        }
    }
}

void R_DrawFuzzColumnSaturnVBE2(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul320(dc_yl) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];

        dest += 2 * SCREENWIDTH;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        }
    }
}

void R_DrawFuzzColumnTransVBE2(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count <= 0)
        return;

    dest = destview + Mul320(dc_yl) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        *dest = tintmap[(*dest << 8) + dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    } while (count--);
}

void R_DrawFuzzColumnFlatSaturnLowVBE2(void)
{
    int count;
    byte *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul320(dc_yl) + (dc_x << 1);

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
    }

    do
    {
        *((unsigned short *)dest) = 0;
        dest += 2 * SCREENWIDTH;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *((unsigned short *)dest) = 0;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *((unsigned short *)dest) = 0;
        }
    }
}

void R_DrawFuzzColumnSaturnLowVBE2(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul320(dc_yl) + (dc_x << 1);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

        *(dest) = color;
        *(dest + 1) = color;

        dest += 2 * SCREENWIDTH;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

        *(dest) = color;
        *(dest + 1) = color;
    }
    else
    {
        if (!(initialdrawpos))
        {
            lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

            *(dest) = color;
            *(dest + 1) = color;
        }
    }
}

void R_DrawFuzzColumnTransLowVBE2(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count <= 0)
        return;

    dest = destview + Mul320(dc_yl) + (dc_x << 1);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        byte color = tintmap[(*dest << 8) + dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];

        *(dest) = color;
        *(dest + 1) = color;

        dest += SCREENWIDTH;
        frac += fracstep;
    } while (count--);
}

void R_DrawFuzzColumnFlatSaturnPotatoVBE2(void)
{
    int count;
    byte *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul320(dc_yl) + (dc_x << 2);

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
    }

    do
    {
        *((unsigned int *)dest) = 0;
        dest += 2 * SCREENWIDTH;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *((unsigned int *)dest) = 0;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *((unsigned int *)dest) = 0;
        }
    }
}

void R_DrawFuzzColumnSaturnPotatoVBE2(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul320(dc_yl) + (dc_x << 2);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        dest += 2 * SCREENWIDTH;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;
    }
    else
    {
        if (!(initialdrawpos))
        {
            lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

            *(dest) = color;
            *(dest + 1) = color;
            *(dest + 2) = color;
            *(dest + 3) = color;
        }
    }
}

void R_DrawFuzzColumnTransPotatoVBE2(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count <= 0)
        return;

    dest = destview + Mul320(dc_yl) + (dc_x << 2);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        byte color = tintmap[(*dest << 8) + dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        dest += SCREENWIDTH;
        frac += fracstep;
    } while (count--);
}

void R_DrawSpanFlatVBE2(void)
{
    byte *dest;
    int countp;

    lighttable_t color = ds_colormap[ds_source[FLATPIXELCOLOR]];

    dest = destview + Mul320(ds_y) + ds_x1;

    countp = ds_x2 - ds_x1 + 1;

    if (countp & 1)
    {
        *(dest) = color;
        dest++;
        countp--;
    }

    if (countp > 0)
    {
        unsigned short colorcomp = color << 8 | color;
        countp /= 2;
        SetWords(dest, colorcomp, countp);
    }
}

void R_DrawSpanFlatLowVBE2(void)
{
    byte *dest;
    int countp;

    unsigned short color = ds_colormap[ds_source[FLATPIXELCOLOR]];
    color |= color << 8;

    dest = destview + Mul320(ds_y) + (ds_x1 << 1);

    countp = ds_x2 - ds_x1 + 1;

    if (countp & 1)
    {
        *((unsigned short *)dest) = color;
        dest += 2;
        countp--;
    }

    if (countp > 0)
    {
        unsigned int colorcomp = color << 16 | color;
        countp /= 2;
        SetDWords(dest, colorcomp, countp);
    }
}

void R_DrawSpanFlatPotatoVBE2(void)
{
    byte *dest;
    int countp;

    unsigned int color = ds_colormap[ds_source[FLATPIXELCOLOR]];
    color |= color << 8;
    color |= color << 16;

    dest = destview + Mul320(ds_y) + (ds_x1 << 2);

    countp = ds_x2 - ds_x1 + 1;

    SetDWords(dest, color, countp);
}

#endif

#if defined(MODE_T4050)
void R_DrawColumnText4050(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul40(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (count >= 1 && odd || count == 0)
    {
        vmem = *dest;

        if (odd)
        {
            vmem = vmem & 0x0F00;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12 | 223;

            odd = 0;
            dest += 40;
            frac += fracstep;
        }
        else
        {
            vmem = vmem & 0xF000;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;
            return;
        }

        count--;
    }

    countblock = (count + 1) / 2;
    count -= countblock * 2;

    while (countblock)
    {
        unsigned short firstcolor, secondcolor;

        firstcolor = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8;
        frac += fracstep;
        secondcolor = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12;

        *dest = firstcolor | secondcolor | 223;
        dest += 40;

        frac += fracstep;
        countblock--;
    }

    if (count >= 0 && !odd)
    {
        vmem = *dest;
        vmem = vmem & 0xF000;
        *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;
    }
}
void R_DrawSpanText4050(void)
{
    int spot;
    int countp;
    unsigned position;
    unsigned step;
    byte odd;
    byte shift;
    unsigned short *dest;
    unsigned short vmem;
    byte even;
    unsigned short vmem_filter;

    dest = textdestscreen + Mul40(ds_y / 2);
    countp = dest + ds_x2;
    dest += ds_x1;

    position = ds_frac;
    step = ds_step;

    odd = ds_y & 1;
    shift = 8 | (odd << 2);

    even = (ds_y + 1) & 1;
    vmem_filter = 0xF00 << (even * 4);

    do
    {
        unsigned xtemp;
        unsigned ytemp;
        unsigned spot;

        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;

        vmem = *dest;
        vmem = vmem & vmem_filter;
        *dest++ = vmem | ds_colormap[ds_source[spot]] << shift | 223;

        position += step;
    } while (dest <= countp);
}
void R_DrawSkyFlatText4050(void)
{
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul40(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    if (count >= 1 && odd || count == 0)
    {
        vmem = *dest;

        if (odd)
        {
            vmem = vmem & 0x0F00;
            *dest = vmem | 6 << 12 | 223;

            odd = 0;
            dest += 40;
        }
        else
        {
            vmem = vmem & 0xF000;
            *dest = vmem | 6 << 8 | 223;
            return;
        }

        count--;
    }

    countblock = (count + 1) / 2;
    count -= countblock * 2;

    while (countblock)
    {
        *dest = 6 << 8 | 219;
        dest += 40;
        countblock--;
    }

    if (count >= 0 && !odd)
    {
        vmem = *dest;
        vmem = vmem & 0xF000;
        *dest = vmem | 6 << 8 | 223;
    }
}
void R_DrawFuzzColumnFlatText4050(void)
{
    register int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    unsigned short local_color;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul40(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    do
    {
        vmem = *dest;
        vmem = vmem & 0xFF00;

        if (odd)
        {
            local_color = vmem & 0xF000;

            if (local_color >= 0x8000)
            {
                vmem -= 0x8000;
                *dest = vmem | 223;
            }

            odd = 0;
            dest += 40;
        }
        else
        {
            local_color = vmem & 0x0F00;

            if (local_color >= 0x800)
            {
                vmem -= 0x800;
                *dest = vmem | 223;
            }

            odd = 1;
        }
    } while (count--);
}

void R_DrawFuzzColumnFlatSaturnText4050(void)
{
    int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;
    odd = dc_yl & 1;
    dest = textdestscreen + Mul40(dc_yl / 2) + dc_x;

    if (initialdrawpos)
    {
        if (odd)
        {
            dest += 40;
            odd = 0;
        }
        else
        {
            odd = 1;
        }
    }

    if (odd)
    {
        do
        {
            vmem = *dest;

            vmem = vmem & 0x0F00;
            *dest = vmem | 223;

            dest += 40;
        } while (count--);

        if ((dc_yh - dc_yl) & 1)
        {
            vmem = *dest;
            vmem = vmem & 0x0F00;
            *dest = vmem | 223;
        }
        else
        {
            if (!(initialdrawpos))
            {
                vmem = *dest;
                vmem = vmem & 0x0F00;
                *dest = vmem | 223;
            }
        }
    }
    else
    {
        do
        {
            vmem = *dest;

            vmem = vmem & 0xF000;
            *dest = vmem | 223;

            dest += 40;
        } while (count--);

        if ((dc_yh - dc_yl) & 1)
        {
            vmem = *dest;
            vmem = vmem & 0xF000;
            *dest = vmem | 223;
        }
        else
        {
            if (!(initialdrawpos))
            {
                vmem = *dest;
                vmem = vmem & 0xF000;
                *dest = vmem | 223;
            }
        }
    }
}

void R_DrawFuzzColumnSaturnText4050(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;
    odd = dc_yl & 1;
    dest = textdestscreen + Mul40(dc_yl / 2) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        if (odd)
        {
            dest += 40;
            odd = 0;
        }
        else
        {
            odd = 1;
        }
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    if (odd)
    {
        do
        {
            vmem = *dest;

            vmem = vmem & 0x0F00;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12 | 223;

            dest += 40;

            frac += fracstep;
        } while (count--);

        if ((dc_yh - dc_yl) & 1)
        {
            vmem = *dest;
            vmem = vmem & 0x0F00;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12 | 223;
        }
        else
        {
            if (!(initialdrawpos))
            {
                vmem = *dest;
                vmem = vmem & 0x0F00;
                *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12 | 223;
            }
        }
    }
    else
    {
        do
        {
            vmem = *dest;

            vmem = vmem & 0xF000;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;

            dest += 40;

            frac += fracstep;
        } while (count--);

        if ((dc_yh - dc_yl) & 1)
        {
            vmem = *dest;
            vmem = vmem & 0xF000;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;
        }
        else
        {
            if (!(initialdrawpos))
            {
                vmem = *dest;
                vmem = vmem & 0xF000;
                *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;
            }
        }
    }
}

void R_DrawFuzzColumnTransText4050(void)
{
    R_DrawFuzzColumnText4050();
}

#endif

#if defined(MODE_T4025)
void R_DrawColumnText4025(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    unsigned short *dest;

    dest = textdestscreen + Mul40(dc_yl) + dc_x;
    count = dest + Mul40(dc_yh - dc_yl);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 219;
        dest += 40;
        frac += fracstep;
    } while (dest <= count);
}
void R_DrawSpanText4025(void)
{
    int countp;
    unsigned position;
    unsigned step;
    unsigned short *dest;
    byte *source;
    byte *colormap;

    dest = textdestscreen + Mul40(ds_y);
    countp = dest + ds_x2;
    dest += ds_x1;

    position = ds_frac;
    step = ds_step;

    source = ds_source;
    colormap = ds_colormap;

    do
    {
        unsigned xtemp;
        unsigned ytemp;
        unsigned spot;

        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;
        *dest++ = colormap[source[spot]] << 8 | 219;
        position += step;
    } while (dest <= countp);
}
void R_DrawSkyFlatText4025(void)
{
    int count;
    unsigned short *dest;

    dest = textdestscreen + Mul40(dc_yl) + dc_x;
    count = dest + Mul40(dc_yh - dc_yl);

    do
    {
        *dest = 6 << 8 | 219;
        dest += 40;
    } while (dest <= count);
}
void R_DrawFuzzColumnFlatText4025(void)
{
    int count;
    unsigned short *dest;
    unsigned short vmem;

    dest = textdestscreen + Mul40(dc_yl) + dc_x;
    count = dest + Mul40(dc_yh - dc_yl);

    do
    {
        vmem = *dest & 0x0F00;

        if (vmem >= 0x800)
        {
            vmem -= 0x800;
            *dest = vmem | 219;
        }

        dest += 40;
    } while (dest <= count);
}

void R_DrawFuzzColumnFlatSaturnText4025(void)
{
    int count;
    unsigned short *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = textdestscreen + Mul40(dc_yl) + dc_x;

    if (initialdrawpos)
    {
        dest += 40;
    }

    do
    {
        *dest = 219;

        dest += 80;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = 219;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = 219;
        }
    }
}

void R_DrawFuzzColumnSaturnText4025(void)
{
    int count;
    unsigned short *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = textdestscreen + Mul40(dc_yl) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += 40;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 219;

        dest += 80;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 219;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 219;
        }
    }
}

void R_DrawFuzzColumnTransText4025(void)
{
    R_DrawFuzzColumnText4025();
}
#endif

#if defined(MODE_MDA)
void R_DrawLineColumnTextMDA(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

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
            return;
        }

        count--;
    }

    countblock = (count + 1) / 2;
    count -= countblock * 2;

    while (countblock)
    {
        unsigned short firstcolor, secondcolor;

        *dest = 0x0F << 8 | 0xDB;
        dest += 80;

        countblock--;
    }

    if (count >= 0 && !odd)
    {
        *dest = 0x0F << 8 | 0xDB;
    }
}

void R_DrawSpriteTextMDA(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    if (count >= 1 && odd || count == 0)
    {
        if (odd)
        {
            *dest = 0x0F << 8 | 0xB0;

            odd = 0;
            dest += 80;
        }
        else
        {
            *dest = 0x0F << 8 | 0xB0;
            return;
        }

        count--;
    }

    countblock = (count + 1) / 2;
    count -= countblock * 2;

    while (countblock)
    {
        *dest = 0x0F << 8 | 0xB0;
        dest += 80;

        countblock--;
    }

    if (count >= 0 && !odd)
    {
        *dest = 0x0F << 8 | 0xB0;
    }
}

void R_DrawEmptyColumnTextMDA(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

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
            return;
        }

        count--;
    }

    countblock = (count + 1) / 2;
    count -= countblock * 2;

    while (countblock)
    {
        unsigned short firstcolor, secondcolor;

        *dest = 0x0F << 8 | 0x00;
        dest += 80;

        countblock--;
    }

    if (count >= 0 && !odd)
    {
        *dest = 0x0F << 8 | 0xDB;
    }
}
#endif

#if defined(MODE_T8025)
void R_DrawColumnText8025(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (count >= 1 && odd || count == 0)
    {
        vmem = *dest;

        if (odd)
        {
            vmem = vmem & 0x0F00;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12 | 223;

            odd = 0;
            dest += 80;
            frac += fracstep;
        }
        else
        {
            vmem = vmem & 0xF000;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;
            return;
        }

        count--;
    }

    countblock = (count + 1) / 2;
    count -= countblock * 2;

    while (countblock)
    {
        unsigned short firstcolor, secondcolor;

        firstcolor = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8;
        frac += fracstep;
        secondcolor = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12;

        *dest = firstcolor | secondcolor | 223;
        dest += 80;

        frac += fracstep;
        countblock--;
    }

    if (count >= 0 && !odd)
    {
        vmem = *dest;
        vmem = vmem & 0xF000;
        *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;
    }
}
#endif

#if defined(MODE_T8025)
void R_DrawSpanText8025(void)
{
    int spot;
    int countp;
    unsigned position;
    unsigned step;
    byte odd;
    byte shift;
    unsigned short *dest;
    unsigned short vmem;
    byte even;
    unsigned short vmem_filter;

    dest = textdestscreen + Mul80(ds_y / 2);
    countp = dest + ds_x2;
    dest += ds_x1;

    position = ds_frac;
    step = ds_step;

    odd = ds_y & 1;
    shift = 8 | (odd << 2);

    even = (ds_y + 1) & 1;
    vmem_filter = 0xF00 << (even * 4);

    do
    {
        unsigned xtemp;
        unsigned ytemp;
        unsigned spot;

        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;

        vmem = *dest;
        vmem = vmem & vmem_filter;
        *dest++ = vmem | ds_colormap[ds_source[spot]] << shift | 223;

        position += step;
    } while (dest <= countp);
}
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)
void R_DrawColumnText8050(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    unsigned short *dest;

    dest = textdestscreen + Mul80(dc_yl) + dc_x;
    count = dest + Mul80(dc_yh - dc_yl);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 219;
        dest += 80;
        frac += fracstep;
    } while (dest <= count);
}
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)
void R_DrawSkyFlatText8050(void)
{
    int count;
    unsigned short *dest;

    dest = textdestscreen + Mul80(dc_yl) + dc_x;
    count = dest + Mul80(dc_yh - dc_yl);

    do
    {
        *dest = 6 << 8 | 219;
        dest += 80;
    } while (dest <= count);
}
#endif

#if defined(MODE_MDA)
void R_DrawSkyTextMDA(void)
{
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    if (count >= 1 && odd || count == 0)
    {
        if (odd)
        {
            *dest = 6 << 8 | 219;

            odd = 0;
            dest += 80;
        }
        else
        {
            *dest = 6 << 8 | 219;
            return;
        }

        count--;
    }

    countblock = (count + 1) / 2;
    count -= countblock * 2;

    while (countblock)
    {
        *dest = 6 << 8 | 219;
        dest += 80;
        countblock--;
    }

    if (count >= 0 && !odd)
    {
        *dest = 6 << 8 | 219;
    }
}
#endif

#if defined(MODE_T8025)
void R_DrawSkyFlatText8025(void)
{
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    if (count >= 1 && odd || count == 0)
    {
        vmem = *dest;

        if (odd)
        {
            vmem = vmem & 0x0F00;
            *dest = vmem | 6 << 12 | 223;

            odd = 0;
            dest += 80;
        }
        else
        {
            vmem = vmem & 0xF000;
            *dest = vmem | 6 << 8 | 223;
            return;
        }

        count--;
    }

    countblock = (count + 1) / 2;
    count -= countblock * 2;

    while (countblock)
    {
        *dest = 6 << 8 | 219;
        dest += 80;
        countblock--;
    }

    if (count >= 0 && !odd)
    {
        vmem = *dest;
        vmem = vmem & 0xF000;
        *dest = vmem | 6 << 8 | 223;
    }
}
#endif

#if defined(MODE_T8025)

void R_DrawFuzzColumnFlatSaturnText8025(void)
{
    int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;
    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;

    if (initialdrawpos)
    {
        if (odd)
        {
            dest += 80;
            odd = 0;
        }
        else
        {
            odd = 1;
        }
    }

    if (odd)
    {
        do
        {
            vmem = *dest;

            vmem = vmem & 0x0F00;
            *dest = vmem | 223;

            dest += 80;
        } while (count--);

        if ((dc_yh - dc_yl) & 1)
        {
            vmem = *dest;
            vmem = vmem & 0x0F00;
            *dest = vmem | 223;
        }
        else
        {
            if (!(initialdrawpos))
            {
                vmem = *dest;
                vmem = vmem & 0x0F00;
                *dest = vmem | 223;
            }
        }
    }
    else
    {
        do
        {
            vmem = *dest;

            vmem = vmem & 0xF000;
            *dest = vmem | 223;

            dest += 80;
        } while (count--);

        if ((dc_yh - dc_yl) & 1)
        {
            vmem = *dest;
            vmem = vmem & 0xF000;
            *dest = vmem | 223;
        }
        else
        {
            if (!(initialdrawpos))
            {
                vmem = *dest;
                vmem = vmem & 0xF000;
                *dest = vmem | 223;
            }
        }
    }
}

void R_DrawFuzzColumnSaturnText8025(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;
    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        if (odd)
        {
            dest += 80;
            odd = 0;
        }
        else
        {
            odd = 1;
        }
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    if (odd)
    {
        do
        {
            vmem = *dest;

            vmem = vmem & 0x0F00;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12 | 223;

            dest += 80;

            frac += fracstep;
        } while (count--);

        if ((dc_yh - dc_yl) & 1)
        {
            vmem = *dest;
            vmem = vmem & 0x0F00;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12 | 223;
        }
        else
        {
            if (!(initialdrawpos))
            {
                vmem = *dest;
                vmem = vmem & 0x0F00;
                *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 12 | 223;
            }
        }
    }
    else
    {
        do
        {
            vmem = *dest;

            vmem = vmem & 0xF000;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;

            dest += 80;

            frac += fracstep;
        } while (count--);

        if ((dc_yh - dc_yl) & 1)
        {
            vmem = *dest;
            vmem = vmem & 0xF000;
            *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;
        }
        else
        {
            if (!(initialdrawpos))
            {
                vmem = *dest;
                vmem = vmem & 0xF000;
                *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;
            }
        }
    }
}

void R_DrawFuzzColumnTransText8025(void)
{
    R_DrawFuzzColumnText8025();
}
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)

void R_DrawFuzzColumnFlatSaturnText8050(void)
{
    int count;
    unsigned short *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = textdestscreen + Mul80(dc_yl) + dc_x;

    if (initialdrawpos)
    {
        dest += 80;
    }

    do
    {
        *dest = 219;
        dest += 160;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = 219;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = 219;
        }
    }
}

void R_DrawFuzzColumnSaturnText8050(void)
{
    int count;
    unsigned short *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = textdestscreen + Mul80(dc_yl) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += 80;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 219;

        dest += 160;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 219;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 219;
        }
    }
}

void R_DrawFuzzColumnTransText8050(void)
{
    R_DrawFuzzColumnText8050();
}
#endif

#if defined(MODE_T8025)
void R_DrawFuzzColumnFlatText8025(void)
{
    register int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    unsigned short local_color;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    do
    {
        vmem = *dest;
        vmem = vmem & 0xFF00;

        if (odd)
        {
            local_color = vmem & 0xF000;

            if (local_color >= 0x8000)
            {
                vmem -= 0x8000;
                *dest = vmem | 223;
            }

            odd = 0;
            dest += 80;
        }
        else
        {
            local_color = vmem & 0x0F00;

            if (local_color >= 0x800)
            {
                vmem -= 0x800;
                *dest = vmem | 223;
            }

            odd = 1;
        }
    } while (count--);
}
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)
void R_DrawFuzzColumnFlatText8050(void)
{
    int count;
    unsigned short *dest;
    unsigned short vmem;

    dest = textdestscreen + Mul80(dc_yl) + dc_x;
    count = dest + Mul80(dc_yh - dc_yl);

    do
    {
        vmem = *dest & 0x0F00;

        if (vmem >= 0x800)
        {
            vmem -= 0x800;
            *dest = vmem | 219;
        }

        dest += 80;
    } while (dest <= count);
}
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)
void R_DrawSpanText8050(void)
{
    int countp;
    unsigned position;
    unsigned step;
    unsigned short *dest;
    byte *source;
    byte *colormap;

    dest = textdestscreen + Mul80(ds_y);
    countp = dest + ds_x2;
    dest += ds_x1;

    position = ds_frac;
    step = ds_step;

    source = ds_source;
    colormap = ds_colormap;

    do
    {
        unsigned xtemp;
        unsigned ytemp;
        unsigned spot;

        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;
        *dest++ = colormap[source[spot]] << 8 | 219;
        position += step;
    } while (dest <= countp);
}
#endif

//
// Spectre/Invisibility.
//
#define FUZZTABLE 50

#if defined(MODE_Y) || defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA)
#define FUZZOFF (SCREENWIDTH / 4)
#endif

#if defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
#define FUZZOFF (SCREENWIDTH)
#endif

int fuzzoffset[FUZZTABLE] =
    {
        FUZZOFF, -FUZZOFF, FUZZOFF, -FUZZOFF, FUZZOFF, FUZZOFF, -FUZZOFF,
        FUZZOFF, FUZZOFF, -FUZZOFF, FUZZOFF, FUZZOFF, FUZZOFF, -FUZZOFF,
        FUZZOFF, FUZZOFF, FUZZOFF, -FUZZOFF, -FUZZOFF, -FUZZOFF, -FUZZOFF,
        FUZZOFF, -FUZZOFF, -FUZZOFF, FUZZOFF, FUZZOFF, FUZZOFF, FUZZOFF, -FUZZOFF,
        FUZZOFF, -FUZZOFF, FUZZOFF, FUZZOFF, -FUZZOFF, -FUZZOFF, FUZZOFF,
        FUZZOFF, -FUZZOFF, -FUZZOFF, -FUZZOFF, -FUZZOFF, FUZZOFF, FUZZOFF,
        FUZZOFF, FUZZOFF, -FUZZOFF, FUZZOFF, FUZZOFF, -FUZZOFF, FUZZOFF};

int fuzzpos = 0;

#if defined(MODE_VBE2_DIRECT)
void R_DrawFuzzColumnVBE2(void)
{
    int count;
    byte *dest;

    if (!dc_yl)
        dc_yl = 1;
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

    dest = destview + Mul320(dc_yl) + dc_x;

    do
    {
        *dest = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];
        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
        dest += SCREENWIDTH;
    } while (count--);
}

void R_DrawFuzzColumnLowVBE2(void)
{
    int count;
    byte *dest;

    if (!dc_yl)
        dc_yl = 1;
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

    dest = destview + Mul320(dc_yl) + (dc_x << 1);

    do
    {
        lighttable_t color;

        color = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];

        *(dest) = color;
        *(dest + 1) = color;

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
        dest += SCREENWIDTH;
    } while (count--);
}

void R_DrawFuzzColumnPotatoVBE2(void)
{
    int count;
    byte *dest;

    if (!dc_yl)
        dc_yl = 1;
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

    dest = destview + Mul320(dc_yl) + (dc_x << 2);

    do
    {
        lighttable_t color;

        color = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
        dest += SCREENWIDTH;
    } while (count--);
}

#endif

#if defined(MODE_T4050)
void R_DrawFuzzColumnText4050(void)
{
    register int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    unsigned short local_color;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul40(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    do
    {
        vmem = *dest;
        vmem = vmem & 0xFF00;

        if (odd)
        {
            local_color = vmem & 0xF000;

            if (fuzzoffset[fuzzpos] > 0)
            {
                if (local_color >= 0x8000)
                {
                    vmem -= 0x8000;
                    *dest = vmem | 223;
                }
            }
            else
            {
                if (local_color < 0x8000)
                {
                    vmem += 0x8000;
                    *dest = vmem | 223;
                }
            }

            odd = 0;
            dest += 40;
        }
        else
        {
            local_color = vmem & 0x0F00;

            if (fuzzoffset[fuzzpos] > 0)
            {
                if (local_color >= 0x800)
                {
                    vmem -= 0x800;
                    *dest = vmem | 223;
                }
            }
            else
            {
                if (local_color < 0x800)
                {
                    vmem += 0x800;
                    *dest = vmem | 223;
                }
            }

            odd = 1;
        }

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;

    } while (count--);
}
#endif

#if defined(MODE_T4025)
void R_DrawFuzzColumnText4025(void)
{
    int count;
    unsigned short *dest;
    unsigned short vmem;

    dest = textdestscreen + Mul40(dc_yl) + dc_x;
    count = dest + Mul40(dc_yh - dc_yl);

    do
    {
        vmem = *dest & 0x0F00;

        if (fuzzoffset[fuzzpos] > 0)
        {
            if (vmem >= 0x800)
                vmem -= 0x800;
        }
        else
        {
            if (vmem < 0x800)
                vmem += 0x800;
        }

        *dest = vmem | 219;

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;

        dest += 40;
    } while (dest <= count);
}
#endif

#if defined(MODE_T8025)
void R_DrawFuzzColumnText8025(void)
{
    register int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    unsigned short local_color;

    odd = dc_yl & 1;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;
    count = dc_yh - dc_yl;

    do
    {
        vmem = *dest;
        vmem = vmem & 0xFF00;

        if (odd)
        {
            local_color = vmem & 0xF000;

            if (fuzzoffset[fuzzpos] > 0)
            {
                if (local_color >= 0x8000)
                {
                    vmem -= 0x8000;
                    *dest = vmem | 223;
                }
            }
            else
            {
                if (local_color < 0x8000)
                {
                    vmem += 0x8000;
                    *dest = vmem | 223;
                }
            }

            odd = 0;
            dest += 80;
        }
        else
        {
            local_color = vmem & 0x0F00;

            if (fuzzoffset[fuzzpos] > 0)
            {
                if (local_color >= 0x800)
                {
                    vmem -= 0x800;
                    *dest = vmem | 223;
                }
            }
            else
            {
                if (local_color < 0x800)
                {
                    vmem += 0x800;
                    *dest = vmem | 223;
                }
            }

            odd = 1;
        }

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;

    } while (count--);
}
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)
void R_DrawFuzzColumnText8050(void)
{
    int count;
    unsigned short *dest;
    unsigned short vmem;

    dest = textdestscreen + Mul80(dc_yl) + dc_x;
    count = dest + Mul80(dc_yh - dc_yl);

    do
    {
        vmem = *dest & 0x0F00;

        if (fuzzoffset[fuzzpos] > 0)
        {
            if (vmem >= 0x800)
                vmem -= 0x800;
        }
        else
        {
            if (vmem < 0x800)
                vmem += 0x800;
        }

        *dest = vmem | 219;

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;

        dest += 80;
    } while (dest <= count);
}
#endif

#if defined(MODE_Y)

void R_DrawFuzzColumnFlatSaturn(void)
{
    int count;
    byte *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul80(dc_yl) + (dc_x >> 2);

    if (initialdrawpos)
    {
        dest += SCREENWIDTH / 4;
    }

    do
    {
        *dest = 0;
        dest += SCREENWIDTH / 2;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = 0;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = 0;
        }
    }
}

void R_DrawFuzzColumnSaturn(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul80(dc_yl) + (dc_x >> 2);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH / 4;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        dest += SCREENWIDTH / 2;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        }
    }
}

void R_DrawFuzzColumnTrans(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count <= 0)
        return;

    outpw(GC_INDEX, GC_READMAP + ((dc_x & 3) << 8));
    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    dest = destview + Mul80(dc_yl) + (dc_x >> 2);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        *dest = tintmap[(*dest << 8) + dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];
        dest += SCREENWIDTH / 4;
        frac += fracstep;
    } while (count--);
}

void R_DrawFuzzColumnFlatSaturnLow(void)
{
    int count;
    byte *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    outp(SC_INDEX + 1, 3 << ((dc_x & 1) << 1));

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul80(dc_yl) + (dc_x >> 1);

    if (initialdrawpos)
    {
        dest += SCREENWIDTH / 4;
    }

    do
    {
        *dest = 0;
        dest += SCREENWIDTH / 2;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = 0;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = 0;
        }
    }
}

void R_DrawFuzzColumnSaturnLow(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    outp(SC_INDEX + 1, 3 << ((dc_x & 1) << 1));

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul80(dc_yl) + (dc_x >> 1);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH / 4;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];

        dest += SCREENWIDTH / 2;
        frac += fracstep;

    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        }
    }
}

void R_DrawFuzzColumnTransLow(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count <= 0)
        return;

    outp(SC_INDEX + 1, 3 << ((dc_x & 1) << 1));

    dest = destview + Mul80(dc_yl) + (dc_x >> 1);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        *dest = tintmap[(*dest << 8) + dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];
        dest += SCREENWIDTH / 4;
        frac += fracstep;
    } while (count--);
}

void R_DrawFuzzColumnFlatSaturnPotato(void)
{
    int count;
    byte *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul80(dc_yl) + dc_x;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH / 4;
    }

    do
    {
        *dest = 0;
        dest += SCREENWIDTH / 2;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = 0;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = 0;
        }
    }
}

void R_DrawFuzzColumnSaturnPotato(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = destview + Mul80(dc_yl) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH / 4;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];

        dest += SCREENWIDTH / 2;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        }
    }
}

void R_DrawFuzzColumnTransPotato(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count <= 0)
        return;

    dest = destview + Mul80(dc_yl) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        *dest = tintmap[(*dest << 8) + dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];
        dest += SCREENWIDTH / 4;
        frac += fracstep;
    } while (count--);
}
#endif

//
// R_DrawSpan
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
int ds_y;
int ds_x1;
int ds_x2;

lighttable_t *ds_colormap;

fixed_t ds_frac;
fixed_t ds_step;

// start of a 64*64 tile image
byte *ds_source;

#if defined(MODE_Y)
void R_DrawSpanFlat(void)
{
    register byte *dest;
    int dsp_x1;
    int dsp_x2;
    register int countp;

    int first_plane, last_plane;
    int medium_planes;
    int total_pixels;

    lighttable_t color;
    int origin_y;

    total_pixels = ds_x2 - ds_x1;

    color = ds_colormap[ds_source[FLATPIXELCOLOR]];
    origin_y = (int)destview + Mul80(ds_y);

    first_plane = ds_x1 & 3;
    last_plane = (ds_x2 + 1) & 3;
    medium_planes = (total_pixels - first_plane - last_plane) / 4;

    if (medium_planes > 0)
    {
        int dsm_x1;
        int dsm_x2;

        outp(SC_INDEX + 1, 15);

        // Quad pixel mode
        dsm_x1 = ds_x1 / 4;
        dsm_x1 += first_plane != 0;

        dsm_x2 = ds_x2 / 4;
        dsm_x2 -= last_plane != 0;

        dest = (byte *)origin_y + dsm_x1;
        countp = dsm_x2 - dsm_x1 + 1;

        if (countp & 1)
        {
            *(dest) = color;
            dest++;
            countp--;
        }

        if (countp > 0)
        {
            unsigned short colorcomp = color << 8 | color;
            countp /= 2;
            SetWords(dest, colorcomp, countp);
        }
    }

    // Single pixel mode

    dsp_x1 = (ds_x1) / 4;
    dsp_x1 += dsp_x1 * 4 < ds_x1;

    dsp_x2 = (ds_x2) / 4;

    if (dsp_x2 > dsp_x1)
    {
        outp(SC_INDEX + 1, 1 << 0);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
        dest = (byte *)origin_y + dsp_x2;
        *dest = color;
    }
    else if (dsp_x2 == dsp_x1)
    {
        outp(SC_INDEX + 1, 1 << 0);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
    }

    dsp_x1 = (ds_x1 - 1) / 4;
    dsp_x1 += dsp_x1 * 4 < ds_x1 - 1;

    dsp_x2 = (ds_x2 - 1) / 4;

    if (dsp_x2 > dsp_x1)
    {
        outp(SC_INDEX + 1, 1 << 1);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
        dest = (byte *)origin_y + dsp_x2;
        *dest = color;
    }
    else if (dsp_x2 == dsp_x1)
    {
        outp(SC_INDEX + 1, 1 << 1);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
    }

    dsp_x1 = (ds_x1 - 2) / 4;
    dsp_x1 += dsp_x1 * 4 < ds_x1 - 2;

    dsp_x2 = (ds_x2 - 2) / 4;

    if (dsp_x2 > dsp_x1)
    {
        outp(SC_INDEX + 1, 1 << 2);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
        dest = (byte *)origin_y + dsp_x2;
        *dest = color;
    }
    else if (dsp_x2 == dsp_x1)
    {
        outp(SC_INDEX + 1, 1 << 2);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
    }

    dsp_x1 = (ds_x1 - 3) / 4;
    dsp_x1 += dsp_x1 * 4 < ds_x1 - 3;

    dsp_x2 = (ds_x2 - 3) / 4;

    if (dsp_x2 > dsp_x1)
    {
        outp(SC_INDEX + 1, 1 << 3);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
        dest = (byte *)origin_y + dsp_x2;
        *dest = color;
    }
    else if (dsp_x2 == dsp_x1)
    {
        outp(SC_INDEX + 1, 1 << 3);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
    }
}
#endif

#if defined(MODE_Y)
void R_DrawSpanFlatLow(void)
{
    register byte *dest;
    int dsp_x1;
    int dsp_x2;
    register int countp;

    int first_plane, last_plane;
    int medium_planes;
    int total_pixels;

    lighttable_t color;
    int origin_y;

    total_pixels = ds_x2 - ds_x1;

    color = ds_colormap[ds_source[FLATPIXELCOLOR]];
    origin_y = (int)destview + Mul80(ds_y);

    first_plane = ds_x1 & 1;
    last_plane = (ds_x2 + 1) & 1;
    medium_planes = (total_pixels - first_plane - last_plane) / 2;

    if (medium_planes > 0)
    {
        int dsm_x1;
        int dsm_x2;

        outp(SC_INDEX + 1, 15);

        // Quad pixel mode
        dsm_x1 = ds_x1 / 2;
        dsm_x1 += first_plane != 0;

        dsm_x2 = ds_x2 / 2;
        dsm_x2 -= last_plane != 0;

        dest = (byte *)origin_y + dsm_x1;
        countp = dsm_x2 - dsm_x1 + 1;

        if (countp & 1)
        {
            *(dest) = color;
            dest++;
            countp--;
        }

        if (countp > 0)
        {
            unsigned short colorcomp = color << 8 | color;
            countp /= 2;
            SetWords(dest, colorcomp, countp);
        }
    }

    dsp_x1 = (ds_x1) / 2;
    dsp_x1 += dsp_x1 * 2 < ds_x1;

    dsp_x2 = (ds_x2) / 2;

    if (dsp_x2 > dsp_x1)
    {
        outp(SC_INDEX + 1, 3 << 0);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
        dest = (byte *)origin_y + dsp_x2;
        *dest = color;
    }
    else if (dsp_x2 == dsp_x1)
    {
        outp(SC_INDEX + 1, 3 << 0);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
    }

    dsp_x1 = (ds_x1 - 1) / 2;
    dsp_x1 += dsp_x1 * 2 < ds_x1 - 1;

    dsp_x2 = (ds_x2 - 1) / 2;

    if (dsp_x2 > dsp_x1)
    {
        outp(SC_INDEX + 1, 3 << 2);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
        dest = (byte *)origin_y + dsp_x2;
        *dest = color;
    }
    else if (dsp_x2 == dsp_x1)
    {
        outp(SC_INDEX + 1, 3 << 2);
        dest = (byte *)origin_y + dsp_x1;
        *dest = color;
    }
}
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)
void R_DrawSpanFlatText8050(void)
{
    int countp;
    unsigned short *dest;

    unsigned short color = ds_colormap[ds_source[FLATPIXELCOLOR]] << 8 | 219;

    dest = textdestscreen + Mul80(ds_y) + ds_x1;

    countp = ds_x2 - ds_x1 + 1;

    SetWords((byte *)dest, color, countp);
}
#endif

#if defined(MODE_T4050)
void R_DrawSpanFlatText4050(void)
{
    int countp;
    byte odd;
    byte even;
    byte shift;
    unsigned short *dest;
    unsigned short vmem;
    unsigned short vmem_filter;
    unsigned short color;

    dest = textdestscreen + Mul40(ds_y / 2);
    countp = dest + ds_x2;
    dest += ds_x1;

    odd = ds_y & 1;
    shift = 8 | (odd << 2);
    color = ds_colormap[ds_source[FLATPIXELCOLOR]];
    color = color << shift | 223;

    even = (ds_y + 1) & 1;
    vmem_filter = 0xF00 << (even * 4);

    do
    {
        vmem = *dest;
        vmem = vmem & vmem_filter;
        *dest++ = vmem | color;
    } while (dest <= countp);
}
#endif

#if defined(MODE_T4025)
void R_DrawSpanFlatText4025(void)
{
    int countp;
    unsigned short *dest;

    unsigned short color = ds_colormap[ds_source[FLATPIXELCOLOR]] << 8 | 219;

    dest = textdestscreen + Mul40(ds_y) + ds_x1;

    countp = ds_x2 - ds_x1 + 1;

    SetWords((byte *)dest, color, countp);
}
#endif

#if defined(MODE_MDA)
void R_DrawSpanTextMDA(void)
{
}
#endif

#if defined(MODE_T8025)
void R_DrawSpanFlatText8025(void)
{
    int countp;
    byte odd;
    byte even;
    byte shift;
    unsigned short *dest;
    unsigned short vmem;
    unsigned short vmem_filter;
    unsigned short color;

    dest = textdestscreen + Mul80(ds_y / 2);
    countp = dest + ds_x2;
    dest += ds_x1;

    odd = ds_y & 1;
    shift = 8 | (odd << 2);
    color = ds_colormap[ds_source[FLATPIXELCOLOR]];
    color = color << shift | 223;

    even = (ds_y + 1) & 1;
    vmem_filter = 0xF00 << (even * 4);

    do
    {
        vmem = *dest;
        vmem = vmem & vmem_filter;
        *dest++ = vmem | color;
    } while (dest <= countp);
}
#endif

//
// R_InitBuffer
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void R_InitBuffer(int width, int height)
{
    int i;

    // Handle resize,
    //  e.g. smaller view windows
    //  with border and/or status bar.

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    viewwindowx = (SCREENWIDTH - width) >> 1;

#if defined(MODE_13H) || defined(MODE_VBE2)
    startscreen = Mul320(viewwindowy) + viewwindowx;
#endif
#endif

    // Column offset. For windows.
#if defined(USE_BACKBUFFER)
    for (i = 0; i < width; i++)
        columnofs[i] = viewwindowx + (i << detailshift);
#endif

// Same with base row offset.
#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    if (width == SCREENWIDTH)
    {
        viewwindowy = 0;

#if defined(MODE_13H) || defined(MODE_VBE2)
        startscreen = viewwindowx;
        endscreen = Mul320(viewheight);
#endif
    }
    else
    {
        viewwindowy = (SCREENHEIGHT - SBARHEIGHT - height) >> 1;

#if defined(MODE_13H) || defined(MODE_VBE2)
        startscreen = Mul320(viewwindowy) + viewwindowx;
        endscreen = Mul320(viewwindowy + viewheight);
#endif
    }
#endif

#if defined(USE_BACKBUFFER)
    for (i = 0; i < height; i++)
        ylookup[i] = backbuffer + Mul320(i + viewwindowy);
#endif
#if defined(MODE_Y)
    for (i = 0; i < height; i++)
        ylookup[i] = Mul80(i);
#endif
}

//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void R_FillBackScreen(void)
{
    byte *src;
    byte *dest;
    int x;
    int y;
    patch_t *patch;
    int i, count;
    byte *screen1;

    // DOOM border patch.
    char name1[] = "FLOOR7_2";

    // DOOM II border patch.
    char name2[] = "GRNROCK";

    char *name;

#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
    if (scaledviewwidth == 320)
        return;
#endif

#if defined(USE_BACKBUFFER)
    if (scaledviewwidth == SCREENWIDTH)
    {
        if (background_buffer)
        {
            Z_Free(background_buffer);
            background_buffer = 0;
        }

        return;
    }

    if (!background_buffer)
    {
        background_buffer = Z_MallocUnowned(SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), PU_STATIC);
    }
#endif

    if (gamemode == commercial)
        name = name2;
    else
        name = name1;

    src = W_CacheLumpName(name, PU_CACHE);

#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
    screen1 = (byte *)Z_MallocUnowned(SCREENWIDTH * SCREENHEIGHT, PU_STATIC);
    dest = screen1;
#endif
#if defined(USE_BACKBUFFER)
    dest = background_buffer;
#endif

    for (y = 0; y < SCREENHEIGHT - SBARHEIGHT; y++)
    {
        for (x = 0; x < SCREENWIDTH / 64; x++)
        {
            CopyDWords(src + ((y & 63) << 6), dest, 16);
            dest += 64;
        }
    }

    patch = W_CacheLumpName("BRDR_T", PU_CACHE);

    for (x = 0; x < scaledviewwidth; x += 8)
    {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_DrawPatch(viewwindowx + x, viewwindowy - 8, screen1, patch);
#endif
#if defined(USE_BACKBUFFER)
        V_DrawPatch(viewwindowx + x, viewwindowy - 8, background_buffer, patch);
#endif
    }

    patch = W_CacheLumpName("BRDR_B", PU_CACHE);

    for (x = 0; x < scaledviewwidth; x += 8)
    {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_DrawPatch(viewwindowx + x, viewwindowy + viewheight, screen1, patch);
#endif
#if defined(USE_BACKBUFFER)
        V_DrawPatch(viewwindowx + x, viewwindowy + viewheight, background_buffer, patch);
#endif
    }
    patch = W_CacheLumpName("BRDR_L", PU_CACHE);

    for (y = 0; y < viewheight; y += 8)
    {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_DrawPatch(viewwindowx - 8, viewwindowy + y, screen1, patch);
#endif
#if defined(USE_BACKBUFFER)
        V_DrawPatch(viewwindowx - 8, viewwindowy + y, background_buffer, patch);
#endif
    }
    patch = W_CacheLumpName("BRDR_R", PU_CACHE);

    for (y = 0; y < viewheight; y += 8)
    {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + y, screen1, patch);
#endif
#if defined(USE_BACKBUFFER)
        V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + y, background_buffer, patch);
#endif
    }

    // Draw beveled edge.
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
    V_DrawPatch(viewwindowx - 8, viewwindowy - 8, screen1, W_CacheLumpName("BRDR_TL", PU_CACHE));
    V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy - 8, screen1, W_CacheLumpName("BRDR_TR", PU_CACHE));
    V_DrawPatch(viewwindowx - 8, viewwindowy + viewheight, screen1, W_CacheLumpName("BRDR_BL", PU_CACHE));
    V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + viewheight, screen1, W_CacheLumpName("BRDR_BR", PU_CACHE));
#endif
#if defined(USE_BACKBUFFER)
    V_DrawPatch(viewwindowx - 8, viewwindowy - 8, background_buffer, W_CacheLumpName("BRDR_TL", PU_CACHE));
    V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy - 8, background_buffer, W_CacheLumpName("BRDR_TR", PU_CACHE));
    V_DrawPatch(viewwindowx - 8, viewwindowy + viewheight, background_buffer, W_CacheLumpName("BRDR_BL", PU_CACHE));
    V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + viewheight, background_buffer, W_CacheLumpName("BRDR_BR", PU_CACHE));
#endif

#if defined(MODE_VBE2_DIRECT)
    dest = pcscreen + 3 * 320 * 200;
    CopyDWords(screen1, dest, (SCREENHEIGHT - SBARHEIGHT) * SCREENWIDTH / 4);
#endif

#if defined(MODE_Y)
    for (i = 0; i < 4; i++)
    {
        outp(SC_INDEX + 1, 1 << i);

        dest = (byte *)0xac000;
        src = screen1 + i;
        do
        {
            *dest++ = *src;
            src += 4;
        } while (dest != (byte *)(0xac000 + (SCREENHEIGHT - SBARHEIGHT) * SCREENWIDTH / 4));
    }
#endif

#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
    Z_Free(screen1);
#endif
}
#endif

//
// Copy a screen buffer.
//
#if defined(MODE_VBE2_DIRECT)
void R_VideoErase(unsigned ofs, int count)
{
    byte *dest;
    byte *source;

    dest = destscreen + ofs;
    source = pcscreen + 320 * 200 * 3 + ofs; // Page 3

    if (count & 1)
    {
        *(dest) = *(source);
        dest++;
        source++;
        count--;
    }

    if (count > 0)
    {
        CopyWords(source, dest, count / 2);
    }
}
#endif

#if defined(MODE_Y)
void R_VideoErase(unsigned ofs, int count)
{
    byte *dest;
    byte *source;
    int countp;

    outp(SC_INDEX + 1, 15);
    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1) | 1);
    dest = destscreen + (ofs >> 2);
    source = (byte *)0xac000 + (ofs >> 2);
    countp = count / 4;
    CopyBytes(source, dest, countp);

    outp(GC_INDEX + 1, inp(GC_INDEX + 1) & ~1);
}
#endif

#if defined(USE_BACKBUFFER)
void R_VideoErase(unsigned ofs, int count)
{
    // LFB copy.
    // This might not be a good idea if memcpy
    //  is not optiomal, e.g. byte by byte on
    //  a 32bit CPU, as GNU GCC/Linux libc did
    //  at one point.

    if (background_buffer)
    {
        if (count & 1)
        {
            *(background_buffer + ofs) = *(backbuffer + ofs);
            ofs++;
            count--;
        }

        if (count > 0)
        {
            CopyWords(background_buffer + ofs, backbuffer + ofs, count / 2);
        }
        
    }
}
#endif

//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void R_DrawViewBorder(void)
{
    int top;
    int side;
    int ofs;
    int i;

    if (scaledviewwidth == SCREENWIDTH)
        return;

    top = ((SCREENHEIGHT - SBARHEIGHT) - viewheight) / 2;
    side = (SCREENWIDTH - scaledviewwidth) / 2;

    // copy top and one line of left side
    R_VideoErase(0, Mul320(top) + side);

    // copy one line of right side and bottom
    ofs = Mul320(viewheight + top) - side;
    R_VideoErase(ofs, Mul320(top) + side);

    // copy sides using wraparound
    ofs = Mul320(top) + SCREENWIDTH - side;
    side <<= 1;

    for (i = 1; i < viewheight; i++)
    {
        R_VideoErase(ofs, side);
        ofs += SCREENWIDTH;
    }
}
#endif

#if defined(USE_BACKBUFFER)
void R_DrawSkyFlatBackbuffer(void)
{
    register int count;
    register byte *dest;

    dest = ylookup[dc_yl] + columnofs[dc_x];
    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = 220;
        *(dest + SCREENWIDTH) = 220;
        *(dest + 2 * SCREENWIDTH) = 220;
        *(dest + 3 * SCREENWIDTH) = 220;
        dest += 4 * SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        *dest = 220;
        dest += SCREENWIDTH;
        count--;
    };
}

void R_DrawSkyFlatLowBackbuffer(void)
{
    register int count;
    register unsigned short *dest;

    dest = (unsigned short *)((byte *)ylookup[dc_yl] + columnofs[dc_x]);
    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = 220 | 220 << 8;
        *(dest + SCREENWIDTH / 2) = 220 | 220 << 8;
        *(dest + 2 * SCREENWIDTH / 2) = 220 | 220 << 8;
        *(dest + 3 * SCREENWIDTH / 2) = 220 | 220 << 8;
        dest += 4 * SCREENWIDTH / 2;
        count -= 4;
    }

    while (count >= 0)
    {
        *(dest) = 220 | 220 << 8;
        dest += SCREENWIDTH / 2;
        count--;
    };
}

void R_DrawSkyFlatPotatoBackbuffer(void)
{
    register int count;
    register unsigned int *dest;

    dest = (unsigned int *)((byte *)ylookup[dc_yl] + columnofs[dc_x]);
    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        *(dest + SCREENWIDTH / 4) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        *(dest + 2 * SCREENWIDTH / 4) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        *(dest + 3 * SCREENWIDTH / 4) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        dest += 4 * SCREENWIDTH / 4;
        count -= 4;
    }

    while (count >= 0)
    {
        *(dest) = 220 | 220 << 8 | 220 << 16 | 220 << 24;
        dest += SCREENWIDTH / 4;
        count--;
    };
}

void R_DrawFuzzColumnBackbuffer(void)
{
    int count;
    byte *dest;

    if (!dc_yl)
        dc_yl = 1;
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    do
    {
        *dest = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];
        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
        dest += SCREENWIDTH;
    } while (count--);
}

void R_DrawFuzzColumnLowBackbuffer(void)
{
    int count;
    byte *dest;

    if (!dc_yl)
        dc_yl = 1;
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    do
    {
        lighttable_t color;

        color = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];

        *(dest) = color;
        *(dest + 1) = color;

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
        dest += SCREENWIDTH;
    } while (count--);
}

void R_DrawFuzzColumnPotatoBackbuffer(void)
{
    int count;
    byte *dest;

    if (!dc_yl)
        dc_yl = 1;
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    do
    {
        lighttable_t color;

        color = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
        dest += SCREENWIDTH;
    } while (count--);
}

void R_DrawFuzzColumnFlatBackbuffer(void)
{
    int count;
    byte *dest;
    fixed_t frac, fracstep;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = colormaps[6 * 256 + dest[0]];
        *(dest + SCREENWIDTH) = colormaps[6 * 256 + dest[SCREENWIDTH]];
        *(dest + 2 * SCREENWIDTH) = colormaps[6 * 256 + dest[2 * SCREENWIDTH]];
        *(dest + 3 * SCREENWIDTH) = colormaps[6 * 256 + dest[3 * SCREENWIDTH]];
        dest += 4 * SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        *dest = colormaps[6 * 256 + dest[0]];
        dest += SCREENWIDTH;
        count--;
    };
}

void R_DrawFuzzColumnFlatLowBackbuffer(void)
{
    int count;
    byte *dest;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        lighttable_t color = colormaps[6 * 256 + dest[0]];

        *(dest) = color;
        *(dest + 1) = color;

        color = colormaps[6 * 256 + dest[SCREENWIDTH]];

        *(dest + SCREENWIDTH) = color;
        *(dest + SCREENWIDTH + 1) = color;

        color = colormaps[6 * 256 + dest[2 * SCREENWIDTH]];

        *(dest + 2 * SCREENWIDTH) = color;
        *(dest + 2 * SCREENWIDTH + 1) = color;

        color = colormaps[6 * 256 + dest[3 * SCREENWIDTH]];

        *(dest + 3 * SCREENWIDTH) = color;
        *(dest + 3 * SCREENWIDTH + 1) = color;

        dest += 4 * SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        lighttable_t color = colormaps[6 * 256 + dest[0]];

        *(dest) = color;
        *(dest + 1) = color;

        dest += SCREENWIDTH;
        count--;
    };
}

void R_DrawFuzzColumnFlatPotatoBackbuffer(void)
{
    int count;
    byte *dest;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        lighttable_t color = colormaps[6 * 256 + dest[0]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        color = colormaps[6 * 256 + dest[SCREENWIDTH]];

        *(dest + SCREENWIDTH) = color;
        *(dest + SCREENWIDTH + 1) = color;
        *(dest + SCREENWIDTH + 2) = color;
        *(dest + SCREENWIDTH + 3) = color;

        color = colormaps[6 * 256 + dest[2 * SCREENWIDTH]];

        *(dest + 2 * SCREENWIDTH) = color;
        *(dest + 2 * SCREENWIDTH + 1) = color;
        *(dest + 2 * SCREENWIDTH + 2) = color;
        *(dest + 2 * SCREENWIDTH + 3) = color;

        color = colormaps[6 * 256 + dest[3 * SCREENWIDTH]];

        *(dest + 3 * SCREENWIDTH) = color;
        *(dest + 3 * SCREENWIDTH + 1) = color;
        *(dest + 3 * SCREENWIDTH + 2) = color;
        *(dest + 3 * SCREENWIDTH + 3) = color;

        dest += 4 * SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        lighttable_t color = colormaps[6 * 256 + dest[0]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        dest += SCREENWIDTH;
        count--;
    };
}

void R_DrawSpanFlatBackbuffer(void)
{
    byte *dest;
    int countp;

    lighttable_t color = ds_colormap[ds_source[FLATPIXELCOLOR]];

    dest = ylookup[ds_y] + columnofs[ds_x1];

    countp = ds_x2 - ds_x1 + 1;

    if (countp & 1)
    {
        *(dest) = color;
        dest++;
        countp--;
    }

    if (countp > 0)
    {
        unsigned short colorcomp = color << 8 | color;
        countp /= 2;
        SetWords(dest, colorcomp, countp);
    }
}

void R_DrawSpanFlatLowBackbuffer(void)
{
    byte *dest;
    int countp;

    unsigned short color = ds_colormap[ds_source[FLATPIXELCOLOR]];
    color |= color << 8;

    dest = ylookup[ds_y] + columnofs[ds_x1];

    countp = ds_x2 - ds_x1 + 1;

    if (countp & 1)
    {
        *((unsigned short *)dest) = color;
        dest += 2;
        countp--;
    }

    if (countp > 0)
    {
        unsigned int colorcomp = color << 16 | color;
        countp /= 2;
        SetDWords(dest, colorcomp, countp);
    }
}

void R_DrawSpanFlatPotatoBackbuffer(void)
{
    byte *dest;
    int countp;

    unsigned int color = ds_colormap[ds_source[FLATPIXELCOLOR]];
    color |= color << 8;
    color |= color << 16;

    dest = ylookup[ds_y] + columnofs[ds_x1];

    countp = ds_x2 - ds_x1 + 1;

    SetDWords(dest, color, countp);
}

void R_DrawFuzzColumnFlatSaturnBackbuffer(void)
{
    int count;
    byte *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
    }

    do
    {
        *dest = 0;
        dest += 2 * SCREENWIDTH;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = 0;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = 0;
        }
    }
}

void R_DrawFuzzColumnSaturnBackbuffer(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];

        dest += 2 * SCREENWIDTH;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
    }
    else
    {
        if (!(initialdrawpos))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        }
    }
}

void R_DrawFuzzColumnTransBackbuffer(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count <= 0)
        return;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        *dest = tintmap[(*dest << 8) + dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    } while (count--);
}

void R_DrawFuzzColumnFlatSaturnLowBackbuffer(void)
{
    int count;
    byte *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
    }

    do
    {
        *((unsigned short *)dest) = 0;
        dest += 2 * SCREENWIDTH;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *((unsigned short *)dest) = 0;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *((unsigned short *)dest) = 0;
        }
    }
}

void R_DrawFuzzColumnSaturnLowBackbuffer(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

        *(dest) = color;
        *(dest + 1) = color;

        dest += 2 * SCREENWIDTH;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

        *(dest) = color;
        *(dest + 1) = color;
    }
    else
    {
        if (!(initialdrawpos))
        {
            lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

            *(dest) = color;
            *(dest + 1) = color;
        }
    }
}

void R_DrawFuzzColumnTransLowBackbuffer(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count <= 0)
        return;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        byte color = tintmap[(*dest << 8) + dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];

        *(dest) = color;
        *(dest + 1) = color;

        dest += SCREENWIDTH;
        frac += fracstep;
    } while (count--);
}

void R_DrawFuzzColumnFlatSaturnPotatoBackbuffer(void)
{
    int count;
    byte *dest;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
    }

    do
    {
        *((unsigned int *)dest) = 0;
        dest += 2 * SCREENWIDTH;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        *((unsigned int *)dest) = 0;
    }
    else
    {
        if (!(initialdrawpos))
        {
            *((unsigned int *)dest) = 0;
        }
    }
}

void R_DrawFuzzColumnSaturnPotatoBackbuffer(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = (dc_yl + dc_x) & 1;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos)
    {
        dest += SCREENWIDTH;
        frac += fracstep;
    }

    fracstep = 2 * fracstep;

    do
    {
        lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        dest += 2 * SCREENWIDTH;
        frac += fracstep;
    } while (count--);

    if ((dc_yh - dc_yl) & 1)
    {
        lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;
    }
    else
    {
        if (!(initialdrawpos))
        {
            lighttable_t color = dc_colormap[dc_source[(frac >> FRACBITS)]];

            *(dest) = color;
            *(dest + 1) = color;
            *(dest + 2) = color;
            *(dest + 3) = color;
        }
    }
}

void R_DrawFuzzColumnTransPotatoBackbuffer(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;

    count = dc_yh - dc_yl;

    if (count <= 0)
        return;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        byte color = tintmap[(*dest << 8) + dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];

        *(dest) = color;
        *(dest + 1) = color;
        *(dest + 2) = color;
        *(dest + 3) = color;

        dest += SCREENWIDTH;
        frac += fracstep;
    } while (count--);
}

#endif
