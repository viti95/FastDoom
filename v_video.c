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
//	Gamma correction LUT stuff.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//

#include <conio.h>
#include "i_system.h"
#include "r_local.h"

#include "doomdef.h"
#include "doomdata.h"
#include "doomstat.h"

#include "m_misc.h"

#include "v_video.h"

byte screen0[SCREENWIDTH * SCREENHEIGHT];
byte screen4[SCREENWIDTH * 32];

int dirtybox[4];

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

int usegamma;

//
// V_MarkRect
//
void V_MarkRect(int x,
                int y,
                int width,
                int height)
{
    M_AddToBox(dirtybox, x, y);
    M_AddToBox(dirtybox, x + width - 1, y + height - 1);
}

//
// V_CopyRect
//
void V_CopyRect(int srcx, int srcy, byte *srcscrn, int width, int height, int destx, int desty, byte *destscrn)
{
    byte *src;
    byte *dest;

    if (textmode)
        return;

    V_MarkRect(destx, desty, width, height);

    src = srcscrn + Mul320(srcy) + srcx;
    dest = destscrn + Mul320(desty) + destx;

    for (; height > 0; height--)
    {
        CopyBytes(src, dest, width);
        //memcpy(dest, src, width);
        src += SCREENWIDTH;
        dest += SCREENWIDTH;
    }
}

void V_SetRect(byte color, int width, int height, int destx, int desty, byte *destscrn)
{
    byte *dest;

    if (textmode)
        return;

    V_MarkRect(destx, desty, width, height);

    dest = destscrn + Mul320(desty) + destx;

    for (; height > 0; height--)
    {
        SetBytes(dest, color, width);
        dest += SCREENWIDTH;
    }
}

//
// V_DrawPatch
// Masks a column based masked pic to the screen.
//
void V_DrawPatch(int x, int y, byte *scrn, patch_t *patch)
{

    int count;
    int col = 0;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;

    if (textmode)
        return;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    desttop = scrn + Mul320(y) + x;

    w = patch->width;

    for (; col < w; x++, col++, desttop++)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            register const byte *source = (byte *)column + 3;
            register byte *dest = desttop + Mul320(column->topdelta);
            register int count = column->length;

            if ((count -= 4) >= 0)
                do
                {
                    register byte s0, s1;
                    s0 = source[0];
                    s1 = source[1];
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                    s0 = source[2];
                    s1 = source[3];
                    source += 4;
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                } while ((count -= 4) >= 0);
            if (count += 4)
                do
                {
                    *dest = *source++;
                    dest += SCREENWIDTH;
                } while (--count);
            column = (column_t *)(source + 1);
        }
    }
}

void V_DrawPatchScreen0(int x, int y, patch_t *patch)
{

    int count;
    int col = 0;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;

    if (textmode8025)
        return;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    V_MarkRect(x, y, patch->width, patch->height);

    desttop = screen0 + Mul320(y) + x;

    w = patch->width;

    for (; col < w; x++, col++, desttop++)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            register const byte *source = (byte *)column + 3;
            register byte *dest = desttop + Mul320(column->topdelta);
            register int count = column->length;

            if ((count -= 4) >= 0)
                do
                {
                    register byte s0, s1;
                    s0 = source[0];
                    s1 = source[1];
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                    s0 = source[2];
                    s1 = source[3];
                    source += 4;
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                } while ((count -= 4) >= 0);
            if (count += 4)
                do
                {
                    *dest = *source++;
                    dest += SCREENWIDTH;
                } while (--count);
            column = (column_t *)(source + 1);
        }
    }
}

//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//
void V_DrawPatchFlippedScreen0(int x, int y, patch_t *patch)
{

    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;

    if (textmode)
        return;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    V_MarkRect(x, y, patch->width, patch->height);

    col = 0;
    desttop = screen0 + Mul320(y) + x;

    w = patch->width;

    for (; col < w; x++, col++, desttop++)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[w - 1 - col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest = desttop + Mul320(column->topdelta);
            count = column->length;

            while (count--)
            {
                *dest = *source++;
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

void V_Blit(unsigned int dest_page, int source_x, int source_y, int dest_x, int dest_y, unsigned int width, unsigned int height, unsigned int src_page)
{
    int line;
    int x;
    unsigned int source_offset;
    unsigned int dest_offset;

    if (textmode)
        return;

    outpw(SC_INDEX, (0xff00 + 0x02)); //select all planes
    outpw(GC_INDEX, 0x08);            //set OR mode

    source_offset = (((unsigned int)source_y * (unsigned int)SCREENWIDTH + source_x) >> 2) + src_page;
    dest_offset = (((unsigned int)dest_y * (unsigned int)SCREENWIDTH + dest_x) >> 2) + dest_page;

    for (line = 0; line < height; line++) //for each scan line
    {
        for (x = 0; x < width >> 2; x++) //for each scan pixel
        {
            volatile char pixel = pcscreen[source_offset + x]; //read pixel to load the latches
            pcscreen[dest_offset + x] = 0;                     //write four pixels
        }

        source_offset += SCREENWIDTH >> 2;
        dest_offset += SCREENWIDTH >> 2;
    }

    outpw(GC_INDEX + 1, 0x00ff);
}

void V_WriteTextDirect(int x, int y, char *string){
    unsigned short *dest;
    
    dest = textdestscreen + Mul80(y) + x;

    while(*string){        
        *dest++ = 12 << 8 | *string;
        string++;
    }
}

void V_WriteCharDirect(int x, int y, unsigned char c){
    unsigned short *dest;

    dest = textdestscreen + Mul80(y) + x;
    *dest = 12 << 8 | c;
}

//
// V_DrawPatchDirect
// Draws directly to the screen on the pc.
//
void V_DrawPatchDirect(int x,
                       int y,
                       patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;

    if (textmode)
        return;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    desttop = destscreen + Mul80(y) + (x >> 2);

    w = patch->width;
    for (col = 0; col < w; col++)
    {
        outp(SC_INDEX + 1, 1 << (x & 3));
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column

        while (column->topdelta != 0xff)
        {
            register const byte *source = (byte *)column + 3;
            register byte *dest = desttop + Mul80(column->topdelta);
            register int count = column->length;

            if ((count -= 4) >= 0)
                do
                {
                    register byte s0, s1;
                    s0 = source[0];
                    s1 = source[1];
                    dest[0] = s0;
                    dest[SCREENWIDTH / 4] = s1;
                    dest += SCREENWIDTH / 2;
                    s0 = source[2];
                    s1 = source[3];
                    source += 4;
                    dest[0] = s0;
                    dest[SCREENWIDTH / 4] = s1;
                    dest += SCREENWIDTH / 2;
                } while ((count -= 4) >= 0);
            if (count += 4)
                do
                {
                    *dest = *source++;
                    dest += SCREENWIDTH / 4;
                } while (--count);
            column = (column_t *)(source + 1);
        }

        desttop += ((++x) & 3) == 0; // go to next byte, not next plane
    }
}
