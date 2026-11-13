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
//	Wall renderer that maps four adjacent columns at a time, so the
//	view is written with aligned 32 bit stores instead of one store
//	per pixel.
//
//	The idea is Ken Silverman's from issue #143: "Do aligned 32-bit
//	writes to video memory.  Easy for ceilings.  Yes it's possible for
//	walls (which are rendered in the vertical direction) too!  The
//	trick is: you render 4 vertical wall lines simultaneously."
//
//	The four lines of a group are texture mapped side by side: for
//	every row of the group one texel is taken from each of the four
//	columns, the four palette indices are joined in one register and
//	the whole row of the group goes to the backbuffer (the buffered
//	modes) or to video memory (the VBE2 direct modes) with a single
//	32 bit store.  Four bytes are written with one store, which is
//	what the 386 and the 486 pay for with four stores, and in video
//	memory four writes are saved for one.
//
//	The renderer replaces the column function called by the seg loops.
//	Every wall column asked for is dropped into a slot and the four
//	slots are drawn together when they hold four consecutive columns
//	whose first one starts at a 32 bit aligned address.  The rows of
//	the group in which all four columns are visible are written with
//	one store per row; the rows of the top and the bottom edge, in
//	which the columns are of different height, are drawn column by
//	column with byte stores, as they leave the bytes of the columns
//	which are not visible there alone and a row of them costs fewer
//	stores that way than with a read of the word, a mask and a write.
//	The columns which can not join a group (the columns in front of
//	and behind an aligned group, the columns after a gap left by an
//	opening which is too high to draw, the columns of a seg which is
//	not drawn to its end, ...) are drawn one at a time, which costs
//	the same as the column renderers used before.
//
//	The segs of Doom are of a few columns and the wall of a seg changes
//	its height from column to column, so the work of a group which is
//	not of one height is kept as small as it can be: the columns are
//	kept in the slots with as few stores as there are, the groups of
//	one height do not touch the edges at all, and the loop of the rows
//	which are drawn with 32 bit stores keeps the least values it can:
//	a seg maps its columns with one texture, one light level and one
//	step, so the loop of a seg has three pointers, four texture
//	positions, one colormap and one step to keep.
*/

#include <limits.h>

#include "doomdef.h"
#include "options.h"
#include "i_system.h"
#include "r_local.h"
#include "r_col4.h"
#include "v_video.h"
#include "doomstat.h"

#if defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)

/* The buffered modes draw into a byte per pixel backbuffer and the VBE2
   direct modes draw into linear video memory, both SCREENWIDTH bytes per
   row, so the same code serves them. */
#if defined(USE_BACKBUFFER)
#define COL4_ROW(y) (ylookup[(y)])
#define COL4_COL(x) (columnofs[(x)])
#else
#define COL4_ROW(y) (destview + MulScreenWidth(y))
#define COL4_COL(x) (x)
#endif

/* A row of four texels is one 32 bit store, so the rows of the view have to
   be words: a mode which does not map four columns at a time is not served
   by this renderer. */
typedef char col4screenwidthaligned[(SCREENWIDTH % 4 == 0) ? 1 : -1];

typedef struct
{
    byte *source;
    lighttable_t *colormap;
    unsigned int step;
    int mid;
    int top;
    int bottom;
} col4slot_t;

/* the four columns of the group, slots[0] is the leftmost one */
static col4slot_t slots[4];

/* dc_x of slots[0] and the number of columns kept in the slots */
static int slotx;
static int nslots;

/* a group starting at x is 32 bit aligned when ((x + alignx) & 3) == 0 */
static int alignx;

/* the column asked for, kept to be drawn together with its three neighbours */
#define COL4_STORE(slot)                         \
    {                                            \
        (slot).source = dc_source;               \
        (slot).colormap = dc_colormap;           \
        (slot).step = (unsigned int)dc_iscale;   \
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

/*
 * One column, drawn with byte stores, the rows from 'from' to 'to' only:
 * used for the columns which are not a complete, aligned group and for the
 * rows of the top and the bottom edge of a group, where the four columns
 * are not of the same height.  The rows of the group which are drawn with
 * 32 bit stores are left to them.
 */
#define COL4_DRAWONE(col, x, from, to)                                       \
    {                                                                        \
        const byte *source = (col)->source;                                  \
        const lighttable_t *map = (col)->colormap;                           \
        const unsigned int step = (col)->step;                              \
        unsigned int frac;                                                   \
        byte *dest;                                                          \
        int count;                                                           \
        int top = (col)->top;                                                \
        int bottom = (col)->bottom;                                          \
                                                                             \
        if (top < (from))                                                    \
            top = (from);                                                    \
        if (bottom > (to))                                                   \
            bottom = (to);                                                   \
                                                                             \
        if (top <= bottom)                                                   \
        {                                                                    \
            frac = (unsigned int)(col)->mid + (unsigned int)(top - centery) * step; \
            dest = COL4_ROW(top) + COL4_COL(x);                              \
            count = bottom - top + 1;                                        \
                                                                             \
            do                                                               \
            {                                                                \
                *dest = map[source[frac >> 16]];                             \
                frac += step;                                                \
                dest += SCREENWIDTH;                                         \
            } while (--count);                                               \
        }                                                                    \
    }

static void R_DrawColumn4One(col4slot_t *col, int x, int from, int to)
{
    COL4_DRAWONE(col, x, from, to)
}

/*
 * The rows of the top and the bottom edge of a group, where the columns are
 * not of the same height: the rows [top, ylo) of the column and the rows
 * (yhi, bottom] of it, drawn with byte stores, for a row of them costs one
 * store and a row of the group is written with one store only when the four
 * columns are in it.  The texture position of both ranges is taken from the
 * position of the row ylo, so the slots are read once for the two ranges.
 */
#define COL4_EDGES(col, x, ylo, yhi)                                           \
    {                                                                          \
        const byte *source = (col)->source;                                    \
        const lighttable_t *map = (col)->colormap;                             \
        const unsigned int step = (col)->step;                                 \
        unsigned int frac = (unsigned int)(col)->mid                           \
                          + (unsigned int)(ylo - centery) * step;              \
        int top = (col)->top;                                                  \
        int bottom = (col)->bottom;                                            \
        byte *dest;                                                            \
        int count;                                                             \
                                                                               \
        if (top < (ylo))                                                       \
        {                                                                      \
            count = (ylo) - top;                                               \
            frac -= (unsigned int)count * step;                                \
            dest = COL4_ROW(top) + COL4_COL(x);                                \
            do                                                                 \
            {                                                                  \
                *dest = map[source[frac >> 16]];                               \
                frac += step;                                                  \
                dest += SCREENWIDTH;                                           \
            } while (--count);                                                 \
        }                                                                      \
                                                                               \
        if (bottom > (yhi))                                                    \
        {                                                                      \
            int y = (yhi) + 1;                                                 \
            count = bottom - y + 1;                                            \
            frac = (unsigned int)(col)->mid + (unsigned int)(y - centery) * step; \
            dest = COL4_ROW(y) + COL4_COL(x);                                  \
            do                                                                 \
            {                                                                  \
                *dest = map[source[frac >> 16]];                               \
                frac += step;                                                  \
                dest += SCREENWIDTH;                                           \
            } while (--count);                                                 \
        }                                                                      \
    }

/*
 * The rows of the group in which the four columns are visible: one texel
 * of every column is taken, the four are joined and the row is written
 * with one 32 bit store.
 */

/* one row of the group: 4 texels -> 1 aligned 32 bit store */
#define COL4_ROW4()                                                        \
    {                                                                      \
        unsigned int pixel;                                                \
                                                                           \
        pixel = (unsigned int)map[source0[frac0 >> 16]];                   \
        pixel |= (unsigned int)map[source1[frac1 >> 16]] << 8;             \
        pixel |= (unsigned int)map[source2[frac2 >> 16]] << 16;            \
        pixel |= (unsigned int)map[source3[frac3 >> 16]] << 24;            \
                                                                           \
        *(unsigned int *)(void *)dest = pixel;                             \
                                                                           \
        frac0 += step;                                                     \
        frac1 += step;                                                     \
        frac2 += step;                                                     \
        frac3 += step;                                                     \
        dest += SCREENWIDTH;                                               \
    }

/*
 * The rows of a seg: the four columns of a seg are mapped by one texture,
 * one light level and one step, so only the three pointers and the four
 * texture positions are of their own, which is what the loop has to keep.
 */
static void R_DrawColumns4Mid(byte *dest, int y, int rows)
{
    const byte *source0 = slots[0].source;
    const byte *source1 = slots[1].source;
    const byte *source2 = slots[2].source;
    const byte *source3 = slots[3].source;

    const lighttable_t *map = slots[0].colormap;
    const unsigned int step = slots[0].step;

    unsigned int frac0 = slots[0].mid + (unsigned int)(y - centery) * step;
    unsigned int frac1 = slots[1].mid + (unsigned int)(y - centery) * step;
    unsigned int frac2 = slots[2].mid + (unsigned int)(y - centery) * step;
    unsigned int frac3 = slots[3].mid + (unsigned int)(y - centery) * step;

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

/* the rows of a group of four columns of a different light level or step */
#define COL4_ROW4ALL()                                                     \
    {                                                                      \
        unsigned int pixel;                                                \
                                                                           \
        pixel = (unsigned int)map0[source0[frac0 >> 16]];                  \
        pixel |= (unsigned int)map1[source1[frac1 >> 16]] << 8;            \
        pixel |= (unsigned int)map2[source2[frac2 >> 16]] << 16;           \
        pixel |= (unsigned int)map3[source3[frac3 >> 16]] << 24;           \
                                                                           \
        *(unsigned int *)(void *)dest = pixel;                             \
                                                                           \
        frac0 += step0;                                                    \
        frac1 += step1;                                                    \
        frac2 += step2;                                                    \
        frac3 += step3;                                                    \
        dest += SCREENWIDTH;                                               \
    }

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

    const unsigned int step0 = slots[0].step;
    const unsigned int step1 = slots[1].step;
    const unsigned int step2 = slots[2].step;
    const unsigned int step3 = slots[3].step;

    unsigned int frac0 = slots[0].mid + (unsigned int)(y - centery) * step0;
    unsigned int frac1 = slots[1].mid + (unsigned int)(y - centery) * step1;
    unsigned int frac2 = slots[2].mid + (unsigned int)(y - centery) * step2;
    unsigned int frac3 = slots[3].mid + (unsigned int)(y - centery) * step3;

    while (rows--)
        COL4_ROW4ALL();
}

/* every column of the group on its own, as the renderers of a single
   column do: used when the group has no row in which the four are visible */
static void R_DrawColumns4Alone(int x)
{
    int i;

    for (i = 0; i < 4; i++)
        R_DrawColumn4One(&slots[i], x + i, INT_MIN, INT_MAX);
}

/*
 * The four columns of a group, drawn with 32 bit stores where the four of
 * them are visible.
 */
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

    /* the rows in which the four of them are visible: they are the rows
       from the highest of the tops of the columns to the lowest of their
       bottoms, for a column is not drawn above its top nor below its
       bottom.  'equal' is kept on the way, for a group of columns of one
       height has no rows of the edges at all. */
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

    /* The group of one height, the wall between two floors: the four
       columns are visible in every row of the group, so the edges of the
       group are its own rows and there is nothing to draw beside the rows
       of 32 bit stores. */
    if (equal)
    {
        /* the columns of a seg are mapped by one texture, one light level
           and one step: the loop of their rows keeps three pointers, four
           texture positions, one colormap and one step */
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
            R_DrawColumns4MidAll(COL4_ROW(ylo) + COL4_COL(x), ylo,
                                 yhi - ylo + 1);
        }
        return;
    }

    /* the rows of the two edges, one setup for the column of both of them */
    for (i = 0; i < 4; i++)
        COL4_EDGES(&slots[i], x + i, ylo, yhi);

    /* the rows with the four columns, one 32 bit store per row */
    R_DrawColumns4MidAll(COL4_ROW(ylo) + COL4_COL(x), ylo, yhi - ylo + 1);
}

/*
 * The columns kept in the slots which are not a complete group are drawn
 * one at a time.
 */
void R_FlushColumn4(void)
{
    while (nslots)
    {
        /* COL4_SHIFT() moves the columns to the left and their x with them */
        R_DrawColumn4One(&slots[0], slotx, INT_MIN, INT_MAX);
        COL4_SHIFT();
        nslots--;
    }
}

/*
 * R_DrawColumn4
 *
 * The column function of the seg loops.  The column asked for joins the
 * group when it is the next one of the group, else the group kept is
 * drawn and a new group is started with it.
 */
void R_DrawColumn4(void)
{
    int x = dc_x;

    if (nslots && x == slotx + nslots)
    {
        COL4_STORE(slots[nslots]);

        if (++nslots == 4)
        {
            if (((slotx + alignx) & 3) == 0)
            {
                R_DrawColumns4(slotx);
                nslots = 0;
            }
            else
            {
                /* a group has to start at a 32 bit aligned column, so the
                   leftmost column is drawn on its own and the three which
                   are left are kept for the group of the next column */
                R_DrawColumn4One(&slots[0], slotx, INT_MIN, INT_MAX);
                COL4_SHIFT();
                nslots = 3;
            }
        }
    }
    else
    {
        if (nslots == 1)
        {
            /* the end of a seg of a few columns: the column kept goes to
               the view without the loop of the flush */
            R_DrawColumn4One(&slots[0], slotx, INT_MIN, INT_MAX);
            nslots = 0;
        }
        else
            R_FlushColumn4();

        COL4_STORE(slots[0]);
        slotx = x;
        nslots = 1;
    }
}

/*
 * R_SetupColumn4
 *
 * Called when the view of the frame is set up: the columns kept of the
 * frame before are dropped and the alignment of the groups, which
 * depends on the origin of the view, is calculated.
 */
void R_SetupColumn4(void)
{
    nslots = 0;

#if defined(USE_BACKBUFFER)
    alignx = (int)(((unsigned long)ylookup[0] + (unsigned long)columnofs[0]) & 3);
#else
    alignx = (int)((unsigned long)destview & 3);
#endif
}

#endif
