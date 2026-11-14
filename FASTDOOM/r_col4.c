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
//
//	The renderer is drawn with only where four columns of the view are
//	the four bytes of a row, which is what R_SetupColumn4() tells: the
//	modes which keep the view one byte per pixel and which call the
//	column function for every column of the view, with the view of the
//	1:1 detail level.  A view of the DETAIL_LOW and the DETAIL_POTATO
//	level is scaled, its renderers write two and four bytes of a pixel
//	and they skip the rows of the view which the pixel of their size
//	does not reach, so those levels keep the renderers of r_draw.c.
//
//	The columns kept point into the WAD cache: the seg loops take the
//	column of a texture into the cache for every column of a texture
//	whose patches are in more than one lump, and taking a lump into
//	the cache purges other lumps.  Every loop which does that hands
//	the columns kept to the view first, with R_FLUSH_COLUMNS4().
*/

#include <limits.h>
#if defined(WALL4_VERIFY)
#include <stdio.h>
#include <string.h>
#endif

#include "doomdef.h"
#include "options.h"
#include "i_system.h"
#include "r_local.h"
#include "r_col4.h"
#include "v_video.h"
#include "doomstat.h"

#if (defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)) \
    && !defined(MODE_CGA16) && !defined(MODE_CVB) && !defined(MODE_CGA512)

/* The buffered modes draw into a backbuffer of one byte per pixel and the
   direct modes draw into linear video memory, both SCREENWIDTH bytes of a
   row, and the column of a row of the 1:1 view is at its byte. */
#if defined(MODE_VBE2_DIRECT)
#define COL4_ROW(y) (destview + MulScreenWidth(y))
#define COL4_COL(x) (x)
/* the shadow of the view, of the verify build, at the same offsets */
#define COL4_SHROW(y) (col4shadow + MulScreenWidth(y))
#else
#define COL4_ROW(y) (ylookup[(y)])
#define COL4_COL(x) (columnofs[(x)])
#define COL4_SHROW(y) (col4shadow + (ylookup[(y)] - destscreen))
#endif

/* A row of the view is four bytes of a 32 bit store only of its rows are
   words, which is what the mode of the frame is checked for. */
typedef char col4screenwidthaligned[(SCREENWIDTH % 4 == 0) ? 1 : -1];

/*
 * The format of the texture position of a row.
 *
 * The column renderers of the modes drawn with the renderer are assembly,
 * R_DrawColumnBackbuffer of linearh.asm and R_DrawColumnVBE2 of vbe2dh.asm,
 * and they do not shift the position of a row by FRACBITS as the C renderers
 * of r_draw.c do.  They keep the position of the first row of a column and
 * the step of a row as one 32 bit value of 25 fraction bits and of seven
 * significant bits, "7 significant bits, 25 frac" of their comments:
 *
 *     eax = (dc_yl - centery) * dc_iscale
 *     ecx = dc_iscale << 9            ; the step of a row
 *     edx = (dc_texturemid + eax) << 9 ; the position of the first row
 *     ebx = edx >> 25                 ; the texel of the row, with a shift
 *                                     ; which is not signed
 *
 * The texel of a column is therefore named by the seven bits which are left
 * above the 25 fraction bits, and the bits above them fall away.  The name
 * is taken out of the position with a shift to the right which is not
 * signed, "shr ebx,25" of the source, and so the texel of a row is one of
 * 0 to 127, of the 128 rows of a column of a texture: "The graphics wrap
 * vertically at 128 pixels" of the comment of the assembly file.  The
 * position of a wall below the middle of the view and of a wall of a tall
 * texture goes under the numbers which the seven bits name, and the byte
 * renderers of the mode draw such a wall from the texel which the seven
 * bits name, which is the position of the row modulo the rows of a column;
 * the four columns renderer has to take the texel the same way, for the
 * texels of a signed shift of the position are the bytes before the column
 * of a texture, and the wall of a column is drawn from the wrong texels.
 *
 * The step of a wall is the reciprocal 0xffffffff / rw_scale of the scale of
 * the seg,
 * a large number, and the position of a wall which is close to the view and
 * of a wall of a tall texture goes over the seven significant bits, so the
 * bits which fall away are not seldom: the byte renderers draw such a wall
 * from the texel which the shifted value names and they walk it with the
 * step of the shifted value, and the four columns renderer has to name the
 * same texels, for the wall of a column has to be drawn as it is drawn
 * without the renderer.
 *
 * The position of a row below the first row of a column is reached the way
 * the byte renderers reach it, with the step added once per row, which is
 * the step multiplied by the number of the rows and wrapped at 32 bits.
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

#if defined(WALL4_VERIFY)
/* the shadow of the view of the verify build, of the size of the view of the
   mode with a border of the columns and the rows which the view can reach */
static byte col4shadow[(SCREENWIDTH + 8) * SCREENHEIGHT + 8];
static int col4shadowed;
static int col4reported;
#endif


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

/* dc_x of slots[0] and the number of columns kept in the slots */
static int slotx;
static int nslots;

/* a group starting at x is 32 bit aligned when ((x + alignx) & 3) == 0 */
static int alignx;

/*
 * The address of a pixel of a light table.
 *
 * The byte renderers of the modes of the renderer reach a pixel of a light
 * table by overwriting the low byte of the address of the table with the
 * byte of the source, "mov al,[eax]" of the assembly source with eax of the
 * address of the table of dc_colormap; the address names the pixel of the
 * table only when the table begins at a address of 256 bytes, which the
 * tables of the game do, and the low byte of the address of the table falls
 * away from the name of the pixel.  The renderer names the pixel of a table
 * the same way with the mask of the low byte, so that a table which is not
 * of 256 bytes is used as the byte renderers of the mode use it.
 */
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

/*
 * One column, drawn with byte stores, the rows from 'from' to 'to' only:
 * used for the columns which are not a complete, aligned group and for the
 * rows of the top and the bottom edge of a group, where the four columns
 * are not of the same height.  The rows of the group which are drawn with
 * 32 bit stores are left to them.
 *
 * The texture position of a row is taken the way the assembly renderers of
 * the modes take it, from the center of the view and in the format of
 * COL4_FRACSH, for the position of a column of a masked wall, of a wall of a
 * sky and of a wall which is close to the view goes below zero and the
 * renderers index the source below it, and the same bytes have to be read
 * there.
 */
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

#if defined(WALL4_VERIFY)
COL4_DEFONE(R_DrawColumn4OneShadow, COL4_SHROW(top))
#endif

/*
 * The rows of the top and the bottom edge of a group, where the columns are
 * not of the same height: the rows [top, ylo) of the column and the rows
 * (yhi, bottom] of it, drawn with byte stores, for a row of them costs one
 * store and a row of the group is written with one store only when the four
 * columns are in it.  The texture position of a row is walked to from the
 * first row of the column, as the byte renderers walk it, so the slots are
 * read once for the two ranges.
 */
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

/*
 * The rows of the group in which the four columns are visible: one texel
 * of every column is taken, the four are joined and the row is written
 * with one 32 bit store.
 */

/* one row of the group: 4 texels -> 1 aligned 32 bit store */
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

    /* the four columns are of one step, so one step is kept */
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


#if defined(WALL4_VERIFY)

/*
 * A build of a machine on which the four columns renderer is to be compared
 * with the byte stores: every group of columns which goes to the view is
 * drawn a second time, column by column, into a shadow of the view, and the
 * bytes of the columns kept are compared.  A difference is printed, with the
 * column, its rows and the position of its texture, so the wall type which
 * does not go with the renderer is found on the machine of the player.
 */

/* the view is not all drawn to the shadow, so the shadow is filled once */
static void R_ShadowView4(void)
{
    memset(col4shadow, 0xcd, sizeof(col4shadow));
    col4shadowed = 1;
}

static void R_VerifyColumn4(col4slot_t *col, int x)
{
    int y;

    R_DrawColumn4OneShadow(col, x, INT_MIN, INT_MAX);

    /* the rows the column was drawn to are the rows of the compare, the
       bytes beside them of the group are the bytes of other columns */
    for (y = col->top; y <= col->bottom; y++)
    {
        byte *a = COL4_ROW(y) + COL4_COL(x);
        byte *b = COL4_SHROW(y) + COL4_COL(x);

        if (*a != *b && col4reported < 40)
        {
            col4reported++;
            printf("\nWALL4 x=%d y=%d view %02x byte %02x yl=%d yh=%d"
                   " iscale=%x mid=%x\n",
                   x, y, *a, *b, col->top, col->bottom,
                   (unsigned)col->step, (unsigned)col->mid);
            fflush(stdout);
        }
    }
}

static void R_VerifyColumns4(int x)
{
    int i;

    for (i = 0; i < 4; i++)
        R_VerifyColumn4(&slots[i], x + i);
}

/* the four columns kept are compared, the shadow is drawn with them */
#define COL4_VERIFY(x) R_VerifyColumns4(x)

#else
#define COL4_VERIFY(x)
#endif

/*
 * The four columns kept, drawn one at a time, for there is no row of the
 * group in which the four of them are visible.
 */
static void R_DrawColumns4Alone(int x)
{
    int i;

    for (i = 0; i < 4; i++)
        R_DrawColumn4One(&slots[i], x + i, INT_MIN, INT_MAX);

    COL4_VERIFY(x);
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
            R_DrawColumns4Mid(COL4_ROW(ylo) + COL4_COL(x), ylo,
                              yhi - ylo + 1);
        }
        else
        {
            R_DrawColumns4MidAll(COL4_ROW(ylo) + COL4_COL(x), ylo,
                                 yhi - ylo + 1);
        }
        COL4_VERIFY(x);
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

    COL4_VERIFY(x);
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
#if defined(WALL4_VERIFY)
        R_VerifyColumn4(&slots[0], slotx);
#endif
        COL4_SHIFT();
        nslots--;
    }
}

/*
 * R_DrawColumn4
 *
 * The column function of the seg loops.  The column asked for joins the
 * group when it is the next one of the group, else the group kept is drawn
 * and a new group is started with it.
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
 * Called when the renderers of the frame are set up: the columns kept of
 * the frame before are dropped and the alignment of the groups, which
 * depends on the origin of the view, is calculated.
 *
 * The function returns the column function of the frame: the renderer, and
 * the function asked for when four columns of the view are not the four
 * bytes of a row of the buffer of the mode, which is the case of a view of
 * the DETAIL_LOW and the DETAIL_POTATO level, whose renderers write a pixel
 * of two and of four bytes and skip the rows of the view a pixel of their
 * size does not reach.  A view which does not start at a word is drawn with
 * too, for the columns in front of the first group of its rows are drawn one
 * at a time.
 */
void (*R_SetupColumn4(void (*fallback)(void)))(void)
{
    nslots = 0;

#if defined(WALL4_VERIFY)
    if (!col4shadowed)
        R_ShadowView4();
#endif

    if (detailshift != DETAIL_HIGH)
        return fallback;

    /* a group starts at a 32 bit aligned column of the view; the columns in
       front of the first group of a row of the view are drawn one at a time,
       so the origin of the view is not a reason to leave the renderer out */
#if defined(MODE_VBE2_DIRECT)
    alignx = (int)((unsigned long)destview & 3);
#else
    alignx = (int)(((unsigned long)ylookup[0]
                    + (unsigned long)columnofs[0]) & 3);
#endif

    return R_DrawColumn4;
}

#endif
