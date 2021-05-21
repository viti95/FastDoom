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

#include "i_system.h"
#include "i_ibm.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

// Needs access to LFB (guess what).
#include "v_video.h"

// State.
#include "doomstat.h"

#include "vmode.h"

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

int viewwidth;
int viewwidthlimit;
int scaledviewwidth;
int viewheight;
int viewwindowx;
int viewwindowy;

int columnofs[SCREENWIDTH];

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
byte *ylookup[SCREENHEIGHT];
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y || EXE_VIDEOMODE == EXE_VIDEOMODE_80X25 || EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
byte **ylookup;
#endif

int automapheight;

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
void R_DrawColumnText8025(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;

    odd = dc_yl % 2;
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
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

    position = ((ds_xfrac << 10) & 0xffff0000) | ((ds_yfrac >> 6) & 0xffff);
    step = ((ds_xstep << 10) & 0xffff0000) | ((ds_ystep >> 6) & 0xffff);

    odd = ds_y % 2;
    shift = 8 | (odd << 2);

    even = (ds_y + 1) % 2;
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
void R_DrawSkyFlatText8025(void)
{
    int count;
    int countblock;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;

    odd = dc_yl % 2;
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
void R_DrawFuzzColumnSaturnText8025(void)
{
    fixed_t frac;
    fixed_t fracstep;
    int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    int initialdrawpos = 0;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = dc_yl + dc_x;
    odd = dc_yl % 2;
    dest = textdestscreen + Mul80(dc_yl / 2) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos & 1)
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
            if (!(initialdrawpos & 1))
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
            if (!(initialdrawpos & 1))
            {
                vmem = *dest;
                vmem = vmem & 0xF000;
                *dest = vmem | dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 223;
            }
        }
    }
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
void R_DrawFuzzColumnSaturnText8050(void)
{
    int count;
    unsigned short *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos = 0;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = dc_yl + dc_x;

    dest = textdestscreen + Mul80(dc_yl) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos & 1)
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
        if (!(initialdrawpos & 1))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]] << 8 | 219;
        }
    }
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
void R_DrawFuzzColumnFastText8025(void)
{
    register int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    unsigned short local_color;

    odd = dc_yl % 2;
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
void R_DrawFuzzColumnFastText8050(void)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
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

    position = ((ds_xfrac << 10) & 0xffff0000) | ((ds_yfrac >> 6) & 0xffff);
    step = ((ds_xstep << 10) & 0xffff0000) | ((ds_ystep >> 6) & 0xffff);

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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawSpanPotato(void)
{
    unsigned position;
    unsigned step;

    byte *source;
    byte *colormap;
    byte *dest;

    int count;

    position = ((ds_xfrac << 10) & 0xffff0000) | ((ds_yfrac >> 6) & 0xffff);
    step = ((ds_xstep << 10) & 0xffff0000) | ((ds_ystep >> 6) & 0xffff);

    source = ds_source;
    colormap = ds_colormap;

    dest = destview + Mul80(ds_y);
    count = dest + ds_x2;
    dest += ds_x1;

    do
    {
        unsigned xtemp;
        unsigned ytemp;
        unsigned spot;

        ytemp = position >> 4;
        ytemp = ytemp & 4032;
        xtemp = position >> 26;
        spot = xtemp | ytemp;
        *dest++ = colormap[source[spot]];
        position += step;
    } while (dest <= count);
}
#endif

//
// Spectre/Invisibility.
//
#define FUZZTABLE 50

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y || EXE_VIDEOMODE == EXE_VIDEOMODE_80X25 || EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
#define FUZZOFF (SCREENWIDTH / 4)
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawFuzzColumn(void)
{
    register int count;
    register byte *dest;

    if (!dc_yl)
        dc_yl = 1;

    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;

    if (count < 0)
        return;

    outpw(GC_INDEX, GC_READMAP + ((dc_x & 3) << 8));
    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    dest = destview + Mul80(dc_yl) + (dc_x >> 2);

    do
    {
        *dest = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];
        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
        dest += SCREENWIDTH / 4;
    } while (count--);
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawFuzzColumnLow(void)
{
    register int count;
    register byte *dest;

    if (!dc_yl)
        dc_yl = 1;

    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;

    if (count < 0)
        return;

    outpw(GC_INDEX, GC_READMAP + ((dc_x & 1) << 9));
    outp(SC_INDEX + 1, 3 << ((dc_x & 1) << 1));

    dest = destview + Mul80(dc_yl) + (dc_x >> 1);

    do
    {
        *dest = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];
        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
        dest += SCREENWIDTH / 4;
    } while (count--);
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawFuzzColumnPotato(void)
{
    register int count;
    register byte *dest;

    if (!dc_yl)
        dc_yl = 1;

    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;

    if (count < 0)
        return;

    dest = destview + Mul80(dc_yl) + dc_x;

    do
    {
        *dest = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];
        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;
        dest += SCREENWIDTH / 4;
    } while (count--);
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawFuzzColumnFast(void)
{
    register int count;
    register byte *dest;

    outpw(GC_INDEX, GC_READMAP + ((dc_x & 3) << 8));
    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    dest = destview + Mul80(dc_yl) + (dc_x >> 2);

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = colormaps[6 * 256 + dest[0]];
        *(dest + SCREENWIDTH / 4) = colormaps[6 * 256 + dest[SCREENWIDTH / 4]];
        *(dest + SCREENWIDTH / 2) = colormaps[6 * 256 + dest[SCREENWIDTH / 2]];
        *(dest + SCREENWIDTH / 4 + SCREENWIDTH / 2) = colormaps[6 * 256 + dest[SCREENWIDTH / 4 + SCREENWIDTH / 2]];
        dest += SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        *(dest) = colormaps[6 * 256 + dest[0]];
        dest += SCREENWIDTH / 4;
        count--;
    }
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawFuzzColumnFastLow(void)
{
    register int count;
    register byte *dest;

    outpw(GC_INDEX, GC_READMAP + ((dc_x & 1) << 9));
    outp(SC_INDEX + 1, 3 << ((dc_x & 1) << 1));

    dest = destview + Mul80(dc_yl) + (dc_x >> 1);

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = colormaps[6 * 256 + dest[0]];
        *(dest + SCREENWIDTH / 4) = colormaps[6 * 256 + dest[SCREENWIDTH / 4]];
        *(dest + SCREENWIDTH / 2) = colormaps[6 * 256 + dest[SCREENWIDTH / 2]];
        *(dest + SCREENWIDTH / 4 + SCREENWIDTH / 2) = colormaps[6 * 256 + dest[SCREENWIDTH / 4 + SCREENWIDTH / 2]];
        dest += SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        *(dest) = colormaps[6 * 256 + dest[0]];
        dest += SCREENWIDTH / 4;
        count--;
    }
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawFuzzColumnFastPotato(void)
{
    register int count;
    register byte *dest;

    dest = destview + Mul80(dc_yl) + dc_x;

    count = dc_yh - dc_yl;

    while (count >= 3)
    {
        *(dest) = colormaps[6 * 256 + dest[0]];
        *(dest + SCREENWIDTH / 4) = colormaps[6 * 256 + dest[SCREENWIDTH / 4]];
        *(dest + SCREENWIDTH / 2) = colormaps[6 * 256 + dest[SCREENWIDTH / 2]];
        *(dest + SCREENWIDTH / 4 + SCREENWIDTH / 2) = colormaps[6 * 256 + dest[SCREENWIDTH / 4 + SCREENWIDTH / 2]];
        dest += SCREENWIDTH;
        count -= 4;
    }

    while (count >= 0)
    {
        *(dest) = colormaps[6 * 256 + dest[0]];
        dest += SCREENWIDTH / 4;
        count--;
    }
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
void R_DrawFuzzColumnText8025(void)
{
    register int count;
    unsigned short *dest;
    byte odd;
    unsigned short vmem;
    unsigned short local_color;

    odd = dc_yl % 2;
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawFuzzColumnSaturn(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos = 0;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    initialdrawpos = dc_yl + dc_x;

    dest = destview + Mul80(dc_yl) + (dc_x >> 2);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos & 1)
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
        if (!(initialdrawpos & 1))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        }
    }
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawFuzzColumnSaturnLow(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos = 0;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    outp(SC_INDEX + 1, 3 << ((dc_x & 1) << 1));

    initialdrawpos = dc_yl + dc_x;

    dest = destview + Mul80(dc_yl) + (dc_x >> 1);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos & 1)
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
        if (!(initialdrawpos & 1))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        }
    }
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawFuzzColumnSaturnPotato(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos = 0;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = dc_yl + dc_x;

    dest = destview + Mul80(dc_yl) + dc_x;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos & 1)
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
        if (!(initialdrawpos & 1))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        }
    }
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

fixed_t ds_xfrac;
fixed_t ds_yfrac;
fixed_t ds_xstep;
fixed_t ds_ystep;

// start of a 64*64 tile image
byte *ds_source;

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
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

        if (countp % 2)
        {
            SetBytes(dest, color, countp);
        }
        else
        {
            unsigned short colorcomp = color << 8 | color;
            SetWords(dest, colorcomp, countp / 2);
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
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

        if (countp % 2)
        {
            SetBytes(dest, color, countp);
        }
        else
        {
            unsigned short colorcomp = color << 8 | color;
            SetWords(dest, colorcomp, countp / 2);
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_DrawSpanFlatPotato(void)
{
    int countp;
    byte *dest;

    lighttable_t color = ds_colormap[ds_source[FLATPIXELCOLOR]];

    dest = destview + Mul80(ds_y) + ds_x1;

    countp = ds_x2 - ds_x1 + 1;

    if (countp % 2)
    {
        SetBytes(dest, color, countp);
    }
    else
    {
        unsigned short colorcomp = color << 8 | color;
        SetWords(dest, colorcomp, countp / 2);
    }
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
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

    odd = ds_y % 2;
    shift = 8 | (odd << 2);
    color = ds_colormap[ds_source[FLATPIXELCOLOR]];
    color = color << shift | 223;

    even = (ds_y + 1) % 2;
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25 || EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
    viewwindowx = 0;
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y || EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
    viewwindowx = (SCREENWIDTH - width) >> 1;
#endif

    // Column offset. For windows.
    for (i = 0; i < width; i++)
        columnofs[i] = viewwindowx + i;

// Samw with base row offset.
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25 || EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
    viewwindowy = 0;
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y || EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
    if (width == SCREENWIDTH)
        viewwindowy = 0;
    else
        viewwindowy = (SCREENHEIGHT - SBARHEIGHT - height) >> 1;
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
    for (i = 0; i < height; i++)
        ylookup[i] = backbuffer + Mul320(i + viewwindowy);
#endif
}

//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y || EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
    if (scaledviewwidth == 320)
        return;
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
    screen1 = (byte *)Z_MallocUnowned(SCREENWIDTH * SCREENHEIGHT, PU_STATIC);
    dest = screen1;
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
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
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
        V_DrawPatch(viewwindowx + x, viewwindowy - 8, screen1, patch);
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
        V_DrawPatch(viewwindowx + x, viewwindowy - 8, background_buffer, patch);
#endif
    }

    patch = W_CacheLumpName("BRDR_B", PU_CACHE);

    for (x = 0; x < scaledviewwidth; x += 8)
    {
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
        V_DrawPatch(viewwindowx + x, viewwindowy + viewheight, screen1, patch);
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
        V_DrawPatch(viewwindowx + x, viewwindowy + viewheight, background_buffer, patch);
#endif
    }
    patch = W_CacheLumpName("BRDR_L", PU_CACHE);

    for (y = 0; y < viewheight; y += 8)
    {
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
        V_DrawPatch(viewwindowx - 8, viewwindowy + y, screen1, patch);
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
        V_DrawPatch(viewwindowx - 8, viewwindowy + y, background_buffer, patch);
#endif
    }
    patch = W_CacheLumpName("BRDR_R", PU_CACHE);

    for (y = 0; y < viewheight; y += 8)
    {
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
        V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + y, screen1, patch);
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
        V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + y, background_buffer, patch);
#endif
    }

    // Draw beveled edge.
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
    V_DrawPatch(viewwindowx - 8, viewwindowy - 8, screen1, W_CacheLumpName("BRDR_TL", PU_CACHE));
    V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy - 8, screen1, W_CacheLumpName("BRDR_TR", PU_CACHE));
    V_DrawPatch(viewwindowx - 8, viewwindowy + viewheight, screen1, W_CacheLumpName("BRDR_BL", PU_CACHE));
    V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + viewheight, screen1, W_CacheLumpName("BRDR_BR", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
    V_DrawPatch(viewwindowx - 8, viewwindowy - 8, background_buffer, W_CacheLumpName("BRDR_TL", PU_CACHE));
    V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy - 8, background_buffer, W_CacheLumpName("BRDR_TR", PU_CACHE));
    V_DrawPatch(viewwindowx - 8, viewwindowy + viewheight, background_buffer, W_CacheLumpName("BRDR_BL", PU_CACHE));
    V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + viewheight, background_buffer, W_CacheLumpName("BRDR_BR", PU_CACHE));
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
    for (i = 0; i < 4; i++)
    {
        outp(SC_INDEX, SC_MAPMASK);
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
    Z_Free(screen1);
#endif
}
#endif

//
// Copy a screen buffer.
//
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void R_VideoErase(unsigned ofs, int count)
{
    byte *dest;
    byte *source;
    int countp;

    outp(SC_INDEX, SC_MAPMASK);
    outp(SC_INDEX + 1, 15);
    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1) | 1);
    dest = destscreen + (ofs >> 2);
    source = (byte *)0xac000 + (ofs >> 2);
    countp = count / 4;
    CopyBytes(source, dest, countp);

    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1) & ~1);
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
void R_VideoErase(unsigned ofs, int count)
{
    // LFB copy.
    // This might not be a good idea if memcpy
    //  is not optiomal, e.g. byte by byte on
    //  a 32bit CPU, as GNU GCC/Linux libc did
    //  at one point.

    if (background_buffer)
    {
        CopyBytes(background_buffer + ofs, backbuffer + ofs, count);
    }
}
#endif

//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y || EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
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

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
void R_DrawSkyFlat_13h(void)
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

void R_DrawFuzzColumn_13h(void)
{
    int count;
    byte *dest;
    fixed_t frac, fracstep;

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

void R_DrawFuzzColumnFast_13h(void)
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

void R_DrawSpanFlat_13h(void)
{
    byte *dest;
    int countp;

    lighttable_t color = ds_colormap[ds_source[FLATPIXELCOLOR]];

    dest = ylookup[ds_y] + columnofs[ds_x1];

    countp = ds_x2 - ds_x1 + 1;

    if (countp % 2)
    {
        SetBytes(dest, color, countp);
    }
    else
    {
        unsigned short colorcomp = color << 8 | color;
        SetWords(dest, colorcomp, countp / 2);
    }
}

void R_DrawFuzzColumnSaturn_13h(void)
{
    int count;
    byte *dest;
    fixed_t frac;
    fixed_t fracstep;
    int initialdrawpos = 0;

    count = (dc_yh - dc_yl) / 2 - 1;

    if (count < 0)
        return;

    initialdrawpos = dc_yl + dc_x;

    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (initialdrawpos & 1)
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
        if (!(initialdrawpos & 1))
        {
            *dest = dc_colormap[dc_source[(frac >> FRACBITS)]];
        }
    }
}
#endif
