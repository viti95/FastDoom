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
#include "options.h"
#include "doomdef.h"
#include "doomdata.h"
#include "doomstat.h"

#include "m_misc.h"

#include "v_video.h"
#include "i_debug.h"

#if defined(TEXT_MODE)
#include "i_text.h"
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
byte screen0[SCREENWIDTH * SCREENHEIGHT];
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
byte screen4[SCREENWIDTH * SBARHEIGHT];
#if PIXEL_SCALING!=1 && PIXEL_SCALING!=2 && PIXEL_SCALING!=3 && PIXEL_SCALING!=4
#error "PIXEL_SCALING must be 1, 2, 3 or 4"
#endif

#endif

#if defined(USE_BACKBUFFER)
byte backbuffer[SCREENWIDTH * SCREENHEIGHT];
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
int dirtybox[4];
#endif

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
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
void V_MarkRect(int x, int y, int width, int height)
{
    BOUNDS_CHECK(x, y);
    BOUNDS_CHECK(x + width - 1, y + height - 1);
    M_AddToBox(dirtybox, x, y);
    M_AddToBox(dirtybox, x + width - 1, y + height - 1);
}
#endif

//
// V_CopyRect
//
void V_CopyRect(int srcx, int srcy, byte *srcscrn, int width, int height, int destx, int desty, byte *destscrn)
{
    byte *src;
    byte *dest;
    // The buffers are pixel doubled in hi-res mode
#if PIXEL_SCALING==2
    srcx *= 2;
    srcy *= 2;
    destx *= 2;
    desty *= 2;
    width *= 2;
    height *= 2;
#elif PIXEL_SCALING==3
    srcx *= 3;
    srcy *= 3;
    destx *= 3;
    desty *= 3;
    width *= 3;
    height *= 3;
#elif PIXEL_SCALING==4
    srcx *= 4;
    srcy *= 4;
    destx *= 4;
    desty *= 4;
    width *= 4;
    height *= 4;
#endif
    BOUNDS_CHECK(srcx, srcy);
    BOUNDS_CHECK(srcx + width - 1, srcy + height - 1);
    BOUNDS_CHECK(destx, desty);
    BOUNDS_CHECK(destx + width - 1, desty + height - 1);

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
    V_MarkRect(destx, desty, width, height);
#endif

    src = srcscrn + MulScreenWidth(srcy) + srcx;
    dest = destscrn + MulScreenWidth(desty) + destx;


    if (width & 1)
    {
        for (; height > 0; height--)
        {
            *(dest) = *(src);
            src++;
            dest++;

            CopyWords(src, dest, width / 2);
            src += SCREENWIDTH - 1;
            dest += SCREENWIDTH - 1;
        }
    }
    else
    {
        for (; height > 0; height--)
        {
            CopyWords(src, dest, width / 2);
            src += SCREENWIDTH;
            dest += SCREENWIDTH;
        }
    }
}


//
// V_DrawPatch
// Masks a column based masked pic to the screen.
//
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
#include "i_debug.h"
void V_DrawPatch(int x, int y, byte *scrn, patch_t *patch)
{

    int count;
    int col = 0;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;
    SCALED_BOUNDS_CHECK(x, y);
    y -= patch->topoffset;
    x -= patch->leftoffset;
    SCALED_BOUNDS_CHECK(x, y);

    desttop = scrn + MulScreenWidth(y * PIXEL_SCALING) + x * PIXEL_SCALING;

    w = patch->width;
    for (; col < w; x++, col++, desttop+=PIXEL_SCALING)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            register const byte *source = (byte *)column + 3;
            register byte *dest = desttop + MulScreenWidth(column->topdelta * PIXEL_SCALING);
            register int count = column->length;
            register byte s0, s1;
#if PIXEL_SCALING==1
            if ((count -= 4) >= 0)
                do
                {
                    s0 = source[0];
                    s1 = source[1];
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                    s0 = source[2];
                    s1 = source[3];
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                    source += 4;
                } while ((count -= 4) >= 0);
            if (count += 4)
                do
                {
                    *dest = *source++;
                    dest += SCREENWIDTH;
                } while (--count);
#elif PIXEL_SCALING==2
              if ((count -= 4) >= 0)
                  do {
                      s0 = source[0];
                      s1 = source[1];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest += SCREENWIDTH;
                      s0 = source[2];
                      s1 = source[3];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest += SCREENWIDTH;
                      source += 4;
                  } while ((count -= 4) >= 0);
              if (count += 4)
                  do
                  {
                      byte s0 = *source++;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                  } while (--count);
#elif PIXEL_SCALING==3
              if ((count -= 4) >= 0)
                  do {
                      s0 = source[0];
                      s1 = source[1];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      s0 = source[2];
                      s1 = source[3];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      source += 4;
                  } while ((count -= 4) >= 0);
              if (count += 4)
                  do
                  {
                      byte s0 = *source++;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                  } while (--count);
#elif PIXEL_SCALING==4
              if ((count -= 4) >= 0)
                  do {
                      s0 = source[0];
                      s1 = source[1];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      s0 = source[2];
                      s1 = source[3];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      source += 4;
                  } while ((count -= 4) >= 0);
              if (count += 4)
                  do
                  {
                      byte s0 = *source++;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                  } while (--count);
#endif
            column = (column_t *)(source + 1);
        }
    }
}
#endif

#if PIXEL_SCALING!=1
void V_DrawPatchNativeRes(int x, int y, byte *scrn, patch_t *patch)
{

    int count;
    int col = 0;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;
    BOUNDS_CHECK(x, y);
    y -= patch->topoffset;
    x -= patch->leftoffset;

    desttop = scrn + MulScreenWidth(y) + x;

    w = patch->width;

    for (; col < w; x++, col++, desttop++)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            register const byte *source = (byte *)column + 3;
            register byte *dest = desttop + MulScreenWidth(column->topdelta);
            register int count = column->length;
            register byte s0, s1;
            if ((count -= 4) >= 0)
                do
                {
                    s0 = source[0];
                    s1 = source[1];
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                    s0 = source[2];
                    s1 = source[3];
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                    source += 4;
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
#endif

#if defined(MODE_Y_HALF)
void V_DrawPatch(int x, int y, byte *scrn, patch_t *patch)
{

    int count;
    int col = 0;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;
    SCALED_BOUNDS_CHECK(x, y);
    y -= (patch->topoffset / 2);
    x -= patch->leftoffset;
    SCALED_BOUNDS_CHECK(x, y);

    desttop = scrn + MulScreenWidth(y * PIXEL_SCALING) + x * PIXEL_SCALING;

    w = patch->width;
    for (; col < w; x++, col++, desttop+=PIXEL_SCALING)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            register const byte *source = (byte *)column + 3;
            register byte *dest = desttop + MulScreenWidth(column->topdelta / 2);
            register int count = column->length;
            register byte s0, s1;

            if ((count -= 4) >= 0)
                do
                {
                    s0 = source[0];
                    dest[0] = s0;
                    dest += SCREENWIDTH;
                    s0 = source[2];
                    dest[0] = s0;
                    dest += SCREENWIDTH;
                    source += 4;
                } while ((count -= 4) >= 0);
            if (count += 4)
                do
                {
                    if (count % 2 == 0){
                        *dest = *source;
                        dest += SCREENWIDTH;
                    }

                    source++;

                } while (--count);

            column = (column_t *)(source + 1);
        }
    }
}
#endif

#if defined(MODE_Y_HALF)
void V_DrawPatchScreen0(int x, int y, patch_t *patch)
{
    int count;
    int col = 0;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w, i;
    SCALED_BOUNDS_CHECK(x, y);
    y -= (patch->topoffset / 2);
    x -= patch->leftoffset;
    SCALED_BOUNDS_CHECK(x, y);
    V_MarkRect(x * PIXEL_SCALING, y * PIXEL_SCALING, patch->width * PIXEL_SCALING, patch->height / 2);

    desttop = screen0 + MulScreenWidth(y * PIXEL_SCALING) + x * PIXEL_SCALING;

    w = patch->width;

    for (; col < w; x++, col++, desttop+=PIXEL_SCALING)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            register const byte *source = (byte *)column + 3;
            register byte *dest = desttop + MulScreenWidth(column->topdelta / 2);
            register int count = column->length;
            register byte s0, s1;

            if ((count -= 4) >= 0)
                do
                {
                    s0 = source[0];
                    dest[0] = s0;
                    dest += SCREENWIDTH;
                    s0 = source[2];
                    dest[0] = s0;
                    dest += SCREENWIDTH;
                    source += 4;
                } while ((count -= 4) >= 0);
            if (count += 4)
                do
                {
                    if (count % 2 == 0){
                        *dest = *source;
                        dest += SCREENWIDTH;
                    }

                    source++;

                } while (--count);

            column = (column_t *)(source + 1);
        }
    }
}
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
void V_DrawPatchScreen0(int x, int y, patch_t *patch)
{
    int count;
    int col = 0;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w, i;
    SCALED_BOUNDS_CHECK(x, y);
    y -= patch->topoffset;
    x -= patch->leftoffset;
    SCALED_BOUNDS_CHECK(x, y);
    V_MarkRect(x * PIXEL_SCALING, y * PIXEL_SCALING, patch->width * PIXEL_SCALING, patch->height * PIXEL_SCALING);

     desttop = screen0 + MulScreenWidth(y * PIXEL_SCALING) + x * PIXEL_SCALING;

    w = patch->width;

    for (; col < w; x++, col++, desttop+=PIXEL_SCALING)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            register const byte *source = (byte *)column + 3;
            register byte *dest = desttop + MulScreenWidth(column->topdelta * PIXEL_SCALING);
            register int count = column->length;
            register byte s0, s1;
#if PIXEL_SCALING==1
            if ((count -= 4) >= 0)
                do
                {
                    s0 = source[0];
                    s1 = source[1];
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                    s0 = source[2];
                    s1 = source[3];
                    dest[0] = s0;
                    dest[SCREENWIDTH] = s1;
                    dest += SCREENWIDTH * 2;
                    source += 4;
                } while ((count -= 4) >= 0);
            if (count += 4)
                do
                {
                    *dest = *source++;
                    dest += SCREENWIDTH;
                } while (--count);
#elif PIXEL_SCALING==2
              if ((count -= 4) >= 0)
                  do {
                      s0 = source[0];
                      s1 = source[1];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest += SCREENWIDTH;
                      s0 = source[2];
                      s1 = source[3];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest += SCREENWIDTH;
                      source += 4;
                  } while ((count -= 4) >= 0);
              if (count += 4)
                  do
                  {
                      s0 = *source;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest += SCREENWIDTH;
                      source++;
                  } while (--count);
#elif PIXEL_SCALING==3
              if ((count -= 4) >= 0)
                  do {
                      s0 = source[0];
                      s1 = source[1];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      s0 = source[2];
                      s1 = source[3];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest += SCREENWIDTH;
                      source += 4;
                  } while ((count -= 4) >= 0);
              if (count += 4)
                  do
                  {
                      byte s0 = *source++;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest += SCREENWIDTH;
                  } while (--count);
#elif PIXEL_SCALING==4
              if ((count -= 4) >= 0)
                  do {
                      s0 = source[0];
                      s1 = source[1];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      s0 = source[2];
                      s1 = source[3];
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      dest[0] = s1;
                      dest[1] = s1;
                      dest[2] = s1;
                      dest[3] = s1;
                      dest += SCREENWIDTH;
                      source += 4;
                  } while ((count -= 4) >= 0);
              if (count += 4)
                  do
                  {
                      byte s0 = *source++;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                      dest[0] = s0;
                      dest[1] = s0;
                      dest[2] = s0;
                      dest[3] = s0;
                      dest += SCREENWIDTH;
                  } while (--count);
#endif
            column = (column_t *)(source + 1);
        }
    }
}
#endif

//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
void V_DrawPatchFlippedScreen0(int x, int y, patch_t *patch)
{

    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;
    BOUNDS_CHECK(x, y);
    y -= patch->topoffset;
    x -= patch->leftoffset;

    V_MarkRect(x * PIXEL_SCALING, y * PIXEL_SCALING, patch->width * PIXEL_SCALING, patch->height / 2);

    col = 0;
    desttop = screen0 + MulScreenWidth(y * PIXEL_SCALING) + x * PIXEL_SCALING;

    w = patch->width;

    for (; col < w; x++, col++, desttop+=PIXEL_SCALING)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[w - 1 - col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest = desttop + MulScreenWidth(column->topdelta * PIXEL_SCALING);
            count = column->length;

#if PIXEL_SCALING==1
            while (count--)
            {
                *dest = *source++;
                dest += SCREENWIDTH;
            }
#elif PIXEL_SCALING==2
            while (count--)
            {
                register byte s = *source++;
                dest[0] = s;
                dest[1] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest += SCREENWIDTH;
            }
#elif PIXEL_SCALING==3
            while (count--)
            {
                register byte s = *source++;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest += SCREENWIDTH;
            }        
#elif PIXEL_SCALING==4
            while (count--)
            {
                register byte s = *source++;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
            }                         
#endif
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}
#endif

#if defined(MODE_Y_HALF)
void V_DrawPatchFlippedScreen0(int x, int y, patch_t *patch)
{

    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;
    BOUNDS_CHECK(x, y);
    y -= (patch->topoffset / 2);
    x -= patch->leftoffset;

    V_MarkRect(x * PIXEL_SCALING, y * PIXEL_SCALING, patch->width * PIXEL_SCALING, patch->height * PIXEL_SCALING);

    col = 0;
    desttop = screen0 + MulScreenWidth(y * PIXEL_SCALING) + x * PIXEL_SCALING;

    w = patch->width;

    for (; col < w; x++, col++, desttop+=PIXEL_SCALING)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[w - 1 - col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest = desttop + MulScreenWidth(column->topdelta / 2);
            count = column->length;

            while (count > 0)
            {
                *dest = *source;
                dest += SCREENWIDTH;

                source += 2;
                count -= 2;
            }

            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}
#endif

#if defined(MODE_T4025) || defined(MODE_T4050)
void V_WriteTextColorDirect(int x, int y, char *string, unsigned short color)
{
    unsigned short *dest;

    dest = textdestscreen + Mul40(y) + x;

    while (*string)
    {
        *dest++ = color | *string;
        string++;
    }
}

void V_WriteTextDirect(int x, int y, char *string)
{
    unsigned short *dest;

    dest = textdestscreen + Mul40(y) + x;

    while (*string)
    {
        *dest++ = 12 << 8 | *string;
        string++;
    }
}

void V_WriteCharColorDirect(int x, int y, unsigned char c, unsigned short color)
{
    unsigned short *dest;

    dest = textdestscreen + Mul40(y) + x;
    *dest = color | c;
}

void V_WriteCharDirect(int x, int y, unsigned char c)
{
    unsigned short *dest;

    dest = textdestscreen + Mul40(y) + x;
    *dest = 12 << 8 | c;
}
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_MDA)
void V_WriteTextColorDirect(int x, int y, char *string, unsigned short color)
{
    unsigned short *dest;

    dest = textdestscreen + Mul80(y) + x;

    while (*string)
    {
        *dest++ = color | *string;
        string++;
    }
}

void V_WriteTextDirect(int x, int y, char *string)
{
    unsigned short *dest;

    dest = textdestscreen + Mul80(y) + x;

    while (*string)
    {
        *dest++ = 12 << 8 | *string;
        string++;
    }
}

void V_WriteCharColorDirect(int x, int y, unsigned char c, unsigned short color)
{
    unsigned short *dest;

    dest = textdestscreen + Mul80(y) + x;
    *dest = color | c;
}

void V_WriteCharDirect(int x, int y, unsigned char c)
{
    unsigned short *dest;

    dest = textdestscreen + Mul80(y) + x;
    *dest = 12 << 8 | c;
}
#endif

//
// V_DrawPatchDirect
// Draws directly to the screen on the pc.
//
#if defined(MODE_VBE2_DIRECT)
void V_DrawPatchDirect(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    col = 0;
    desttop = destscreen + MulScreenWidth(y * PIXEL_SCALING) + x * PIXEL_SCALING;
    w = patch->width;
    for (; col < w; x++, col++, desttop+=PIXEL_SCALING)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);
        // Step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest = desttop + MulScreenWidth(column->topdelta * PIXEL_SCALING);
            count = column->length;
#if PIXEL_SCALING==1
            while (count--)
            {
                *dest = *source++;
                dest += SCREENWIDTH;
            }
#elif PIXEL_SCALING==2
            while (count--)
            {
                register byte s = *source++;
                dest[0] = s;
                dest[1] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest += SCREENWIDTH;
            }
#elif PIXEL_SCALING==3
            while (count--)
            {
                register byte s = *source++;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest += SCREENWIDTH;
            }
#elif PIXEL_SCALING==4
            while (count--)
            {
                register byte s = *source++;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
            }
#endif
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}
#endif

#if defined(MODE_Y_HALF)
void V_DrawPatchDirect(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;
    BOUNDS_CHECK(x, y);
    y -= (patch->topoffset / 2);
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
            register byte *dest = desttop + Mul80(column->topdelta / 2);
            register int count = column->length;

            if ((count -= 4) >= 0)
                do
                {
                    register byte s0, s1;
                    s0 = source[0];
                    dest[0] = s0;
                    dest += SCREENWIDTH / 4;
                    s0 = source[2];
                    source += 4;
                    dest[0] = s0;
                    dest += SCREENWIDTH / 4;
                } while ((count -= 4) >= 0);
            if (count += 4)
                do
                {
                    if (count % 2 == 0)
                    {
                        *dest = *source;
                        dest += SCREENWIDTH / 4;
                    }

                    source++;

                } while (--count);
            column = (column_t *)(source + 1);
        }

        desttop += ((++x) & 3) == 0; // go to next byte, not next plane
    }
}
#endif

#if defined(MODE_X) || defined(MODE_Y)
void V_DrawPatchDirect(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;
    BOUNDS_CHECK(x, y);
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
#endif

#if defined(USE_BACKBUFFER)
void V_DrawPatchDirect(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    col = 0;
    desttop = backbuffer + MulScreenWidth(y * PIXEL_SCALING) + x * PIXEL_SCALING;
    w = patch->width;
    for (; col < w; x++, col++, desttop+=PIXEL_SCALING)
    {
#if defined(MODE_CGA16) || defined(MODE_CVBS)
    if ((int)desttop & 1)
        continue;
#endif

#if defined(MODE_CGA512)
        switch (detailshift)
        {
        case DETAIL_HIGH:
            if ((int)desttop & 3)
                continue;
            break;
        case DETAIL_LOW:
            if ((int)desttop & 1)
                continue;
            break;
        }
#endif

        column = (column_t *)((byte *)patch + patch->columnofs[col]);
        // Step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest = desttop + MulScreenWidth(column->topdelta * PIXEL_SCALING);
            count = column->length;
#if PIXEL_SCALING==1
            while (count--)
            {
                *dest = *source++;
                dest += SCREENWIDTH;
            }
#elif PIXEL_SCALING==2
            while (count--)
            {
                register byte s = *source++;
                dest[0] = s;
                dest[1] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest += SCREENWIDTH;
            }
#elif PIXEL_SCALING==3
            while (count--)
            {
                register byte s = *source++;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest += SCREENWIDTH;
            }
#elif PIXEL_SCALING==4
            while (count--)
            {
                register byte s = *source++;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
                dest[0] = s;
                dest[1] = s;
                dest[2] = s;
                dest[3] = s;
                dest += SCREENWIDTH;
            }
#endif
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}
#endif

#if defined(MODE_T8043)
void V_DrawPatchDirectText8043(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    unsigned short *desttop;
    unsigned short *dest;
    byte *source;
    int w;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    x /= 4;             // 320 --> 80
    y = (y * 149) / 32; // 200 --> 43

    desttop = textdestscreen + Mul80(y) + x;

    w = patch->width;
    for (col = 0; col < w; col += 4)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest = desttop + Mul80(column->topdelta / 5);
            count = column->length / 4;

            while (count--)
            {
                *dest = ptrlut16colors[*source] << 8 | 219;
                source += 4;
                dest += 80;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }

        desttop += 1;
    }
}
#endif

#if defined(MODE_T8050)
void V_DrawPatchDirectText8050(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    unsigned short *desttop;
    unsigned short *dest;
    byte *source;
    int w;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    x /= 4; // 320 --> 80
    y /= 4; // 200 --> 50

    desttop = textdestscreen + Mul80(y) + x;

    w = patch->width;
    for (col = 0; col < w; col += 4)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest = desttop + Mul80(column->topdelta / 4);
            count = column->length / 4;

            while (count--)
            {
                *dest = ptrlut16colors[*source] << 8 | 219;
                source += 4;
                dest += 80;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }

        desttop += 1;
    }
}
#endif

#if defined(MODE_T4050)
void V_DrawPatchDirectText4050(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    unsigned short *desttop;
    unsigned short *dest;
    byte *source;
    int w;
    byte odd;
    unsigned short vmem;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    x /= 8; // 320 --> 40
    y /= 4; // 200 --> 50

    desttop = textdestscreen + Mul40(y / 2) + x;

    w = patch->width;
    for (col = 0; col < w; col += 8)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            odd = (column->topdelta / 4 + y) & 1;
            dest = desttop + Mul40(column->topdelta / 8);
            count = column->length / 4;

            while (count--)
            {
                vmem = *dest;

                if (odd)
                {
                    vmem = vmem & 0x0F00;
                    *dest = vmem | ptrlut16colors[*source] << 12 | 223;

                    odd = 0;
                    dest += 40;
                }
                else
                {
                    vmem = vmem & 0xF000;
                    *dest = vmem | ptrlut16colors[*source] << 8 | 223;

                    odd = 1;
                }

                source += 4;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }

        desttop += 1;
    }
}
#endif

#if defined(MODE_T4025)
void V_DrawPatchDirectText4025(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    unsigned short *desttop;
    unsigned short *dest;
    byte *source;
    int w;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    x /= 8; // 320 --> 40
    y /= 8; // 200 --> 25

    desttop = textdestscreen + Mul40(y) + x;

    w = patch->width;
    for (col = 0; col < w; col += 8)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            dest = desttop + Mul40(column->topdelta / 8);
            count = column->length / 4;

            while (count--)
            {
                *dest = ptrlut16colors[*source] << 8 | 219;
                source += 8;
                dest += 40;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }

        desttop += 1;
    }
}
#endif

#if defined(MODE_T8025)
void V_DrawPatchDirectText8025(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    unsigned short *desttop;
    unsigned short *dest;
    byte *source;
    int w;
    byte odd;
    unsigned short vmem;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    x /= 4; // 320 --> 80
    y /= 4; // 200 --> 50

    desttop = textdestscreen + Mul80(y / 2) + x;

    w = patch->width;
    for (col = 0; col < w; col += 4)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            odd = (column->topdelta / 4 + y) & 1;
            dest = desttop + Mul80(column->topdelta / 8);
            count = column->length / 4;

            while (count--)
            {
                vmem = *dest;

                if (odd)
                {
                    vmem = vmem & 0x0F00;
                    *dest = vmem | ptrlut16colors[*source] << 12 | 223;

                    odd = 0;
                    dest += 80;
                }
                else
                {
                    vmem = vmem & 0xF000;
                    *dest = vmem | ptrlut16colors[*source] << 8 | 223;

                    odd = 1;
                }

                source += 4;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }

        desttop += 1;
    }
}
#endif

#if defined(MODE_MDA)
void V_DrawPatchDirectTextMDA(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    unsigned short *desttop;
    unsigned short *dest;
    byte *source;
    int w;
    byte odd;
    unsigned short vmem;

    y -= patch->topoffset;
    x -= patch->leftoffset;

    x /= 4; // 320 --> 80
    y /= 4; // 200 --> 50

    desttop = textdestscreen + Mul80(y / 2) + x;

    w = patch->width;
    for (col = 0; col < w; col += 4)
    {
        column = (column_t *)((byte *)patch + patch->columnofs[col]);

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte *)column + 3;
            odd = (column->topdelta / 4 + y) & 1;
            dest = desttop + Mul80(column->topdelta / 8);
            count = column->length / 4;

            while (count--)
            {
                *dest = 0x07 << 8 | *source;

                if (odd)
                {

                    odd = 0;
                    dest += 80;
                }
                else
                {

                    odd = 1;
                }

                source += 4;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }

        desttop += 1;
    }
}
#endif
