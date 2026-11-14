/*
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
//	Wall renderer which maps four adjacent columns at a time, so a row
//	of them goes to the view with one aligned 32 bit store instead of
//	four byte stores: the idea of Ken Silverman from issue #143, "you
//	render 4 vertical wall lines simultaneously".
//
//	The renderer takes the place of the column function of the seg
//	loops.  A column asked for is kept in a slot, and the four slots
//	are drawn together when they hold four consecutive columns of a 32
//	bit aligned address: the rows of the group in which the four
//	columns are visible are one 32 bit store each, the rows of the top
//	and the bottom edge are byte stores, for a row of them costs fewer
//	stores so, and the columns which join no group are drawn one at a
//	time.  The renderer is used only where four columns of the view are
//	the four bytes of a row, which is what R_SetupColumn4() tells.
//
//	The columns kept point into the WAD cache, which the seg loops
//	purge when they take a lump, so every such loop hands the columns
//	kept to the view first, with R_FLUSH_COLUMNS4().
*/

#include <limits.h>

#include "doomdef.h"
#include "options.h"
#include "i_system.h"
#include "r_local.h"
#include "r_col4.h"
#include "v_video.h"
#include "doomstat.h"

#if (defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)) \
    && !defined(MODE_CGA16) && !defined(MODE_CVB) && !defined(MODE_CGA512)

/* the row of a y and the column of an x of the view, one byte per pixel */
#if defined(MODE_VBE2_DIRECT)
#define COL4_ROW(y) (destview + MulScreenWidth(y))
#define COL4_COL(x) (x)
#else
#define COL4_ROW(y) (ylookup[(y)])
#define COL4_COL(x) (columnofs[(x)])
#endif

/* the rows of the view must be words for a group to be one 32 bit store */
typedef char col4screenwidthaligned[(SCREENWIDTH % 4 == 0) ? 1 : -1];

/*
 * The format of the texture position of a row.
 *
 * The column renderers of the modes drawn here are assembly, of linearh.asm
 * and of vbe2dh.asm, and they keep the position and the step of a row in
 * seven significant bits and 25 fraction bits, not in the FRACBITS of the C
 * renderers of r_draw.c:
 *
 *     ecx = dc_iscale << 9                        ; the step of a row
 *     edx = (dc_texturemid + (dc_yl - centery) * dc_iscale) << 9
 *     ebx = edx >> 25                             ; the texel, "shr"
 *
 * The texel of a row is of 0 to 127 and a texture wraps vertically at 128
 * texels, "The graphics wrap vertically at 128 pixels" of the assembly
 * source, and the bits above the seven of the position fall away, which the
 * walls close to the view and the tall walls and the masked walls reach.
 * The texel is taken out of the position with a shift which is not signed,
 * for a signed shift names the bytes before the column of the texture, and
 * the position of a row is walked with the step added once per row, wrapped
 * at 32 bits, as the assembly renderers walk it.
 */
#define COL4_FRACSH 25

/* the position of the first row of a column, of dc_yl */
#define COL4_SEEDAT(mid, y, step) \
    (fixed_t)(((mid) + ((y) - centery) * (step)) << 9)

/* the step of a row */
#define COL4_STEP(step) \
    (fixed_t)((step) << 9)

/* the position of the row (y) of a column kept, walked to from its top */
#define COL4_SEED(col, y) \
    (fixed_t)(COL4_SEEDAT((col)->mid, (col)->top, (col)->step)                \
              + ((y) - (col)->top) * COL4_STEP((col)->step))

/* the texel of a row, of seven bits which are not signed */
#define COL4_TEXEL(frac) \
    ((int)(((unsigned int)(frac)) >> COL4_FRACSH))

typedef struct
{
    byte *source;
    lighttable_t *colormap;
    fixed_t step;
    int mid;
    int top;
    int bottom;
} col4slot_t;

/* the four columns of the group, slots[0] is the leftmost one */
static col4slot_t slots[4];

/* dc_x of slots[0] and the number of columns kept, the number published in
   r_col4.h, so the seg loops test it of their own */
static int slotx;
int col4pending;

/* a group starting at x is 32 bit aligned when ((x + alignx) & 3) == 0 */
static int alignx;

/* The byte renderers of the modes name a pixel of a light table by writing
   the byte of the source over the low byte of the address of the table,
   "mov al,[eax]", which names a pixel only of a table beginning at an
   address of 256 bytes, as the tables of the game do.  The mask names the
   pixel of a table which is not of 256 bytes as the mode names it. */
#define COL4_MAP(map) ((lighttable_t *)((unsigned long)(map) & ~0xffUL))

/* the column asked for, kept to be drawn together with its three neighbours */
#define COL4_STORE(slot)                         \
    {                                            \
        (slot).source = dc_source;               \
        (slot).colormap = COL4_MAP(dc_colormap); \
        (slot).step = dc_iscale;                 \
        (slot).mid = dc_texturemid;              \
        (slot).top = dc_yl;                      \
        (slot).bottom = dc_yh;                   \
    }

/* the group is drawn from left to right, so the columns move up */
#define COL4_SHIFT()                             \
    {                                            \
        slots[0] = slots[1];                     \
        slots[1] = slots[2];                     \
        slots[2] = slots[3];                     \
        slotx++;                                 \
    }

/* One column drawn with byte stores, of the rows from 'from' to 'to' only:
   the columns which join no group and the rows of the edges of a group, of
   the texture position of COL4_FRACSH, read as the mode reads it. */
#define COL4_DEFONE(name, row)                                               \
static void name(col4slot_t *col, int x, int from, int to)                   \
{                                                                            \
    const byte *source = col->source;                                        \
    const lighttable_t *map = col->colormap;                                 \
    fixed_t fracstep = COL4_STEP(col->step);                                 \
    fixed_t frac;                                                            \
    byte *dest;                                                              \
    int count;                                                               \
    int top = col->top;                                                      \
    int bottom = col->bottom;                                                \
                                                                             \
    if (top < from)                                                          \
        top = from;                                                          \
    if (bottom > to)                                                         \
        bottom = to;                                                         \
                                                                             \
    if (top <= bottom)                                                       \
    {                                                                        \
        frac = COL4_SEED(col, top);                                          \
        dest = (row) + COL4_COL(x);                                          \
        count = bottom - top + 1;                                            \
                                                                             \
        do                                                                   \
        {                                                                    \
            *dest = map[source[COL4_TEXEL(frac)]];                           \
            frac += fracstep;                                                \
            dest += SCREENWIDTH;                                             \
        } while (--count);                                                   \
    }                                                                        \
}

COL4_DEFONE(R_DrawColumn4One, COL4_ROW(top))

/* The rows [top, ylo) and (yhi, bottom] of a column of a group which is not
   of one height, drawn with byte stores, one setup of the slot for both of
   the ranges, the position walked to from the first row of the column. */
#define COL4_EDGES(col, x, ylo, yhi)                                         \
    {                                                                        \
        const byte *source = (col)->source;                                  \
        const lighttable_t *map = (col)->colormap;                           \
        fixed_t fracstep = COL4_STEP((col)->step);                           \
        fixed_t seed = COL4_SEEDAT((col)->mid, (col)->top, (col)->step);     \
        fixed_t frac;                                                        \
        int top = (col)->top;                                                \
        int bottom = (col)->bottom;                                          \
        byte *dest;                                                          \
        int count;                                                           \
                                                                             \
        if (top < (ylo))                                                     \
        {                                                                    \
            count = (ylo) - top;                                             \
            frac = seed;                                                     \
            dest = COL4_ROW(top) + COL4_COL(x);                              \
            do                                                               \
            {                                                                \
                *dest = map[source[COL4_TEXEL(frac)]];                       \
                frac += fracstep;                                            \
                dest += SCREENWIDTH;                                         \
            } while (--count);                                               \
        }                                                                    \
                                                                             \
        if (bottom > (yhi))                                                  \
        {                                                                    \
            int y = (yhi) + 1;                                               \
            count = bottom - y + 1;                                          \
            frac = seed + (y - top) * fracstep;                              \
            dest = COL4_ROW(y) + COL4_COL(x);                                \
            do                                                               \
            {                                                                \
                *dest = map[source[COL4_TEXEL(frac)]];                       \
                frac += fracstep;                                            \
                dest += SCREENWIDTH;                                         \
            } while (--count);                                               \
        }                                                                    \
    }

/* one row of the group, of the four columns visible: the four texels joined
   into one 32 bit store */
#define COL4_ROW4()                                                          \
    {                                                                        \
        unsigned int pixel;                                                  \
                                                                             \
        pixel = (unsigned int)map[source0[COL4_TEXEL(frac0)]];               \
        pixel |= (unsigned int)map[source1[COL4_TEXEL(frac1)]] << 8;         \
        pixel |= (unsigned int)map[source2[COL4_TEXEL(frac2)]] << 16;        \
        pixel |= (unsigned int)map[source3[COL4_TEXEL(frac3)]] << 24;        \
                                                                             \
        *(unsigned int *)(void *)dest = pixel;                               \
                                                                             \
        frac0 += fracstep;                                                   \
        frac1 += fracstep;                                                   \
        frac2 += fracstep;                                                   \
        frac3 += fracstep;                                                   \
        dest += SCREENWIDTH;                                                 \
    }

/* The rows of the group of one light level and one step, which is what a seg
   of one texture gives: the loop keeps three pointers and four positions. */
static void R_DrawColumns4Mid(byte *dest, int y, int rows)
{
    const byte *source0 = slots[0].source;
    const byte *source1 = slots[1].source;
    const byte *source2 = slots[2].source;
    const byte *source3 = slots[3].source;

    const lighttable_t *map = slots[0].colormap;

    fixed_t fracstep = COL4_STEP(slots[0].step);

    fixed_t frac0 = COL4_SEED(&slots[0], y);
    fixed_t frac1 = COL4_SEED(&slots[1], y);
    fixed_t frac2 = COL4_SEED(&slots[2], y);
    fixed_t frac3 = COL4_SEED(&slots[3], y);

    /* two rows per pass, they do not depend on each other */
    while (rows > 1)
    {
        COL4_ROW4();
        COL4_ROW4();
        rows -= 2;
    }

    if (rows)
        COL4_ROW4();
}

/* the rows of a group whose columns are not of one light level and one step */
static void R_DrawColumns4MidAll(byte *dest, int y, int rows)
{
    const byte *source0 = slots[0].source;
    const byte *source1 = slots[1].source;
    const byte *source2 = slots[2].source;
    const byte *source3 = slots[3].source;

    const lighttable_t *map0 = slots[0].colormap;
    const lighttable_t *map1 = slots[1].colormap;
    const lighttable_t *map2 = slots[2].colormap;
    const lighttable_t *map3 = slots[3].colormap;

    fixed_t step0 = COL4_STEP(slots[0].step);
    fixed_t step1 = COL4_STEP(slots[1].step);
    fixed_t step2 = COL4_STEP(slots[2].step);
    fixed_t step3 = COL4_STEP(slots[3].step);

    fixed_t frac0 = COL4_SEED(&slots[0], y);
    fixed_t frac1 = COL4_SEED(&slots[1], y);
    fixed_t frac2 = COL4_SEED(&slots[2], y);
    fixed_t frac3 = COL4_SEED(&slots[3], y);

    while (rows-- > 0)
    {
        unsigned int pixel;

        pixel = (unsigned int)map0[source0[COL4_TEXEL(frac0)]];
        pixel |= (unsigned int)map1[source1[COL4_TEXEL(frac1)]] << 8;
        pixel |= (unsigned int)map2[source2[COL4_TEXEL(frac2)]] << 16;
        pixel |= (unsigned int)map3[source3[COL4_TEXEL(frac3)]] << 24;

        *(unsigned int *)(void *)dest = pixel;

        frac0 += step0;
        frac1 += step1;
        frac2 += step2;
        frac3 += step3;
        dest += SCREENWIDTH;
    }
}

/* the four columns kept drawn one at a time, of no row showing all of them */
static void R_DrawColumns4Alone(int x)
{
    int i;

    for (i = 0; i < 4; i++)
        R_DrawColumn4One(&slots[i], x + i, INT_MIN, INT_MAX);
}

/* the group of four columns, with 32 bit stores where the four are visible */
static void R_DrawColumns4(int x)
{
    int ylo, yhi;
    int equal;
    int i;

    if (slots[0].top > slots[0].bottom)
    {
        /* an empty column, so there is no row with the four of them */
        R_DrawColumns4Alone(x);
        return;
    }

    ylo = slots[0].top;
    yhi = slots[0].bottom;
    equal = 1;

    /* the rows of the four columns, from the highest of the tops of them to
       the lowest of their bottoms; 'equal' tells the group of one height,
       which has no rows of the edges at all */
    for (i = 1; i < 4; i++)
    {
        if (slots[i].top > slots[i].bottom)
        {
            /* an empty column, so there is no row with the four of them */
            R_DrawColumns4Alone(x);
            return;
        }

        if (slots[i].top != ylo)
        {
            equal = 0;
            if (slots[i].top > ylo)
                ylo = slots[i].top;
        }
        if (slots[i].bottom != yhi)
        {
            equal = 0;
            if (slots[i].bottom < yhi)
                yhi = slots[i].bottom;
        }
    }

    if (ylo > yhi)
    {
        /* the walls of the group do not overlap, so no row shows the four
           of them and every column is drawn on its own */
        R_DrawColumns4Alone(x);
        return;
    }

    /* the group of one height, a wall between two floors: every one of its
       rows shows the four columns, so there is no edge to draw */
    if (equal)
    {
        /* the columns of a seg are of one light level and one step */
        if (slots[0].colormap == slots[1].colormap &&
            slots[0].colormap == slots[2].colormap &&
            slots[0].colormap == slots[3].colormap &&
            slots[0].step == slots[1].step &&
            slots[0].step == slots[2].step &&
            slots[0].step == slots[3].step)
        {
            R_DrawColumns4Mid(COL4_ROW(ylo) + COL4_COL(x), ylo,
                              yhi - ylo + 1);
        }
        else
        {
            R_DrawColumns4MidAll(COL4_ROW(ylo) + COL4_COL(x), ylo,
                                 yhi - ylo + 1);
        }
        return;
    }

    /* the rows of the two edges, one setup for the column of both of them */
    for (i = 0; i < 4; i++)
        COL4_EDGES(&slots[i], x + i, ylo, yhi);

    /* the rows with the four columns, one 32 bit store per row */
    if (slots[0].colormap == slots[1].colormap &&
        slots[0].colormap == slots[2].colormap &&
        slots[0].colormap == slots[3].colormap &&
        slots[0].step == slots[1].step &&
        slots[0].step == slots[2].step &&
        slots[0].step == slots[3].step)
    {
        R_DrawColumns4Mid(COL4_ROW(ylo) + COL4_COL(x), ylo, yhi - ylo + 1);
    }
    else
    {
        R_DrawColumns4MidAll(COL4_ROW(ylo) + COL4_COL(x), ylo, yhi - ylo + 1);
    }
}

/* the columns kept which are not a complete group, drawn one at a time */
void R_FlushColumn4(void)
{
    while (col4pending)
    {
        /* COL4_SHIFT() moves the columns to the left and their x with them */
        R_DrawColumn4One(&slots[0], slotx, INT_MIN, INT_MAX);
        COL4_SHIFT();
        col4pending--;
    }
}

/* the column function of the seg loops: the column asked for joins the group
   kept when it is the next one of it, else the group is drawn first */
void R_DrawColumn4(void)
{
    int x = dc_x;

    if (col4pending && x == slotx + col4pending)
    {
        COL4_STORE(slots[col4pending]);

        if (++col4pending == 4)
        {
            if (((slotx + alignx) & 3) == 0)
            {
                R_DrawColumns4(slotx);
                col4pending = 0;
            }
            else
            {
                /* the group has to start at an aligned column, so the
                   leftmost column goes on its own and the three of its right
                   are kept for the group of the next column */
                R_DrawColumn4One(&slots[0], slotx, INT_MIN, INT_MAX);
                COL4_SHIFT();
                col4pending = 3;
            }
        }
    }
    else
    {
        if (col4pending == 1)
        {
            /* the end of a short seg, the one column kept without the loop */
            R_DrawColumn4One(&slots[0], slotx, INT_MIN, INT_MAX);
            col4pending = 0;
        }
        else
            R_FlushColumn4();

        COL4_STORE(slots[0]);
        slotx = x;
        col4pending = 1;
    }
}

/* Called when the renderers of the frame are set up: the columns kept of the
   frame before are dropped and the alignment of the groups of the view is
   taken.  The renderer is returned, and the function asked for of a scaled
   view, whose renderers write a pixel of two and of four bytes and skip the
   rows a pixel of their size does not reach.  A view which does not start at
   a word is drawn with the renderer too, for the columns in front of the
   first group of its rows are drawn one at a time. */
void (*R_SetupColumn4(void (*fallback)(void)))(void)
{
    col4pending = 0;

    if (detailshift != DETAIL_HIGH)
        return fallback;

    /* the alignment of a group of the columns of the rows of the view */
#if defined(MODE_VBE2_DIRECT)
    alignx = (int)((unsigned long)destview & 3);
#else
    alignx = (int)(((unsigned long)ylookup[0]
                    + (unsigned long)columnofs[0]) & 3);
#endif

    return R_DrawColumn4;
}

#endif
