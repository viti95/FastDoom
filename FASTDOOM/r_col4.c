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
//
//	The rows of a group in which the four columns are visible are of the
//	assembly of col4.asm, which draws a group of one light table and of one
//	step of the registers of the machine and of the registers of MMX, r_main.c
//	choosing between the two of them as it chooses the other renderers of a
//	mode, and of the loop of the registers of the machine the groups of light
//	tables, of steps and of sources of their own.
*/

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

/* The columns kept: of two groups of four of them, for the seg loops ask for
   the columns of the tier of the top of a seg and for the columns of the tier
   of its bottom of every one of their columns one after the other, so the
   columns of the one tier are kept in the one group and the columns of the
   other tier of the same seg in the other one, the segs of one tier keeping
   the one group only.  The columns of a group are of the columns of the view
   next to one another and of rows which leave the band of the group of rows
   none, the band being the rows of the view every one of the columns of the
   group shows, of the loop of the four bytes of a row writing them.  Of the
   light table and of the step the group is of the shape of the loop of the
   four of them at once, or of the shape of the loop of the four of them of
   their own, which COL4_MID tells. */
#define COL4_GROUP 2
#define COL4_GSLOT 4

/* The four columns of the group drawn, col4slots[0] the leftmost one, with
   the x, the first row and the number of the rows of them: the renderer of
   col4.asm reads them, so they are not of this file only.  The columns kept of
   the groups of the file are of the slots of every one of them, the slots of
   the group drawn being copied to these of the renderer, of the renderer
   reading the slots of the one group only while the two of them are kept. */
col4slot_t col4slots[COL4_GSLOT];
static col4slot_t groupslot[COL4_GROUP][COL4_GSLOT];
int col4x;
int col4y;
int col4rows;

/* dc_x of the first column of a group and the number of the columns of it
   kept, col4pending the number of the columns kept of the view, the number
   published in r_col4.h, so the seg loops test it of their own */
static int slotx[COL4_GROUP];
static int pending[COL4_GROUP];
int col4pending;

/* The band of the rows of a group, the rows of the view every one of the
   columns of it shows, of its first row the highest of the tops of them to
   its last row the lowest of their bottoms; and the rows of the view the
   columns of a group touch, of the lowest of their tops to the highest of
   their bottoms, the two of the groups kept at the same time touching none of
   them of the tiers of one seg being one over the other. */
static int bandtop[COL4_GROUP];
static int bandbot[COL4_GROUP];
static int rowtop[COL4_GROUP];
static int rowbot[COL4_GROUP];

/* The renderer of the middle rows of a group of four columns of one light
   table and of one step, of the assembly of col4.asm: of the registers of the
   machine here, and of the registers of MMX for the CPUs of MMX, which
   r_main.c chooses of its own switch of the CPU, as it chooses the renderers
   of the mode. */
void (*col4midfunc)(void) = R_DrawColumns4Mid;

/* a group starting at x is 32 bit aligned when ((x + alignx) & 3) == 0 */
static int alignx;

/* The byte renderers of the modes name a pixel of a light table by writing
   the byte of the source over the low byte of the address of the table,
   "mov al,[eax]", which names a pixel only of a table beginning at an
   address of 256 bytes, as the tables of the game do.  The mask names the
   pixel of a table which is not of 256 bytes as the mode names it. */
#define COL4_MAP(map) ((lighttable_t *)((unsigned long)(map) & ~0xffUL))

/* The column asked for kept in the group (g), the group of none kept being
   opened with it, of the x of the group, of the band of its rows and of the
   rows of the view it touches being of it then; of a group of columns kept the
   rows of it are joined with the rows of the column asked for, the band of the
   group taking the rows the four of the columns show and the rows of the view
   of the group taking all of the rows of them.  The group of one column of
   view (x) opened, the group of columns kept of the column of its right. */
#define COL4_STORE(g, map, x)                                          \
    do {                                                               \
        col4slot_t *col = groupslot[g] + pending[g];                   \
                                                                       \
        col->source = dc_source;                                       \
        col->colormap = (map);                                         \
        col->step = dc_iscale;                                         \
        col->mid = dc_texturemid;                                      \
        col->top = dc_yl;                                              \
        col->bottom = dc_yh;                                           \
                                                                       \
        if (pending[g])                                                \
        {                                                              \
            if (dc_yl > bandtop[g])                                    \
                bandtop[g] = dc_yl;                                    \
            if (dc_yh < bandbot[g])                                    \
                bandbot[g] = dc_yh;                                    \
            if (dc_yl < rowtop[g])                                     \
                rowtop[g] = dc_yl;                                     \
            if (dc_yh > rowbot[g])                                     \
                rowbot[g] = dc_yh;                                     \
                                                                       \
            pending[g]++;                                              \
        }                                                              \
        else                                                           \
        {                                                              \
            bandtop[g] = rowtop[g] = dc_yl;                            \
            bandbot[g] = rowbot[g] = dc_yh;                            \
            slotx[g] = (x);                                            \
                                                                       \
            pending[g] = 1;                                            \
        }                                                              \
    } while (0)

/* The column asked for of the next column of the group (g): of the column of
   the view next to the columns of the group and of rows which leave the band
   of the group of rows none, the band being the rows of the view every one of
   the columns of the group shows.  Of the light table, of the step and of the
   source the group takes of the shape it is: the loop of the middle rows of
   col4.asm of one table and of one step walks the four of the columns at once,
   and the loop of the four of them of their own walks the rest, both of them
   writing the four bytes of a row, as COL4_MID tells. */
#define COL4_MATCH(g)                                                      \
    (pending[g] && dc_x == slotx[g] + pending[g]                           \
     && dc_yl <= bandbot[g] && dc_yh >= bandtop[g])

/* The columns of the two groups kept are drawn one of them after the other,
   which is of the pixels of the view only of a column of the one of them being
   of the same x as a column of the other and of the rows of them touching, the
   nearer seg of the view being asked for last and drawn last.  Of the two
   tiers of one seg the rows of the one are over the rows of the other, the seg
   loops asking for the columns of the tier of the top of a column before the
   columns of the tier of its bottom, so the groups of the two of them touch
   none of the rows of a column of the view. */
#define COL4_CLASH(g)                                                      \
    (pending[g] && dc_x >= slotx[g] && dc_x < slotx[g] + pending[g]        \
     && dc_yl <= rowbot[g] && dc_yh >= rowtop[g])

/* One column drawn with byte stores, of all of its rows: the columns which
   join no group, the rows of the edges of a group being of COL4_EDGES; of the
   texture position of COL4_FRACSH, read as the mode reads it. */
static void R_DrawColumn4All(col4slot_t *col, int x)
{
    const byte *source = col->source;
    const lighttable_t *map = col->colormap;
    fixed_t fracstep = COL4_STEP(col->step);
    int top = col->top;
    int bottom = col->bottom;
    fixed_t frac;
    byte *dest;
    int count;

    if (top <= bottom)
    {
        frac = COL4_SEEDAT(col->mid, top, col->step);
        dest = COL4_ROW(top) + COL4_COL(x);
        count = bottom - top + 1;

        do
        {
            *dest = map[source[COL4_TEXEL(frac)]];
            frac += fracstep;
            dest += SCREENWIDTH;
        } while (--count);
    }
}

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

/* the four columns kept drawn one at a time, of no row showing all of them */
static void R_DrawColumns4Alone(int x)
{
    int i;

    for (i = 0; i < 4; i++)
        if (col4slots[i].top <= col4slots[i].bottom)
            R_DrawColumn4All(&col4slots[i], x + i);
}

/* The group of one light table and of one step, which is what a seg of one
   texture gives: the loop of the middle rows of col4.asm walks the four of
   them with the light table of the leftmost of the group and with its step,
   the sources of the four of them being read of their own and the positions of
   the four of them being taken over the position of the leftmost one, so the
   columns of the texture of more than one lump, of a wall not parallel to the
   view and of the texture which wraps are of the loop of the four bytes of a
   row as well.

   Told of the four columns of the group, every one of them compared with the
   leftmost of the group, the comparisons of the light table and of the step
   being transitive. */
#define COL4_MID()                                              \
    (col4slots[0].colormap == col4slots[1].colormap             \
     && col4slots[0].colormap == col4slots[2].colormap          \
     && col4slots[0].colormap == col4slots[3].colormap          \
     && col4slots[0].step == col4slots[1].step                  \
     && col4slots[0].step == col4slots[2].step                  \
     && col4slots[0].step == col4slots[3].step)

/* the middle rows of the four columns of the view of x, of the rows of ylo to
   yhi, handed over to the renderer of the columns kept of col4.asm */
#define COL4_MID_DRAW(x, ylo, yhi)    \
    do {                              \
        col4x = (x);                  \
        col4y = (ylo);                \
        col4rows = (yhi) - (ylo) + 1; \
        col4midfunc();                \
    } while (0)

/* the middle rows of the four columns of the view of x, of the rows of ylo to
   yhi, of a group which is not of the shape of the loop above: of more than
   one light table, of more than one step, or of sources of more than one
   stride, so every one of its columns is walked on its own */
#define COL4_ALL_DRAW(x, ylo, yhi)    \
    do {                              \
        col4x = (x);                  \
        col4y = (ylo);                \
        col4rows = (yhi) - (ylo) + 1; \
        R_DrawColumns4MidAll();       \
    } while (0)

/* the group of four columns, with 32 bit stores where the four are visible */
static void R_DrawColumns4(int x)
{
    int ylo, yhi;
    int equal;
    int i;

    ylo = col4slots[0].top;
    yhi = col4slots[0].bottom;
    equal = 1;

    if (ylo > yhi)
    {
        /* an empty column, so there is no row with the four of them */
        R_DrawColumns4Alone(x);
        return;
    }

    /* the rows of the four columns, from the highest of the tops of them to
       the lowest of their bottoms; 'equal' tells the group of one height,
       which has no rows of the edges at all */
    for (i = 1; i < 4; i++)
    {
        int top = col4slots[i].top;
        int bottom = col4slots[i].bottom;

        if (top > bottom)
        {
            /* an empty column, so there is no row with the four of them */
            R_DrawColumns4Alone(x);
            return;
        }

        if (top != ylo)
        {
            equal = 0;
            if (top > ylo)
                ylo = top;
        }
        if (bottom != yhi)
        {
            equal = 0;
            if (bottom < yhi)
                yhi = bottom;
        }
    }

    if (ylo > yhi)
    {
        /* the walls of the group do not overlap, so no row shows the four
           of them and every column is drawn on its own */
        R_DrawColumns4Alone(x);
        return;
    }

    /* the group of one height, a wall between two floors: every row of it
       shows the four columns, so there is no edge to draw */
    if (equal)
    {
        /* the columns of a seg are of one light table and one step */
        if (COL4_MID())
            COL4_MID_DRAW(x, ylo, yhi);
        else
            COL4_ALL_DRAW(x, ylo, yhi);
        return;
    }

    /* the rows of the two edges, one setup for the column of both of them */
    for (i = 0; i < 4; i++)
        COL4_EDGES(&col4slots[i], x + i, ylo, yhi);

    /* the rows with the four columns, one 32 bit store per row */
    if (COL4_MID())
        COL4_MID_DRAW(x, ylo, yhi);
    else
        COL4_ALL_DRAW(x, ylo, yhi);
}

/* the columns kept of the group (g) drawn one at a time with byte stores, of
   the group of less than four of them, of the left of them to the right */
static void R_FlushGroup4(int g)
{
    col4slot_t *slot = groupslot[g];
    int x = slotx[g];
    int i;

    for (i = 0; i < pending[g]; i++, x++)
        R_DrawColumn4All(&slot[i], x);

    pending[g] = 0;
}

/* the four columns of the group (g) drawn of the rows of their band with the
   renderer of the columns kept of col4.asm, the slots of the group being
   copied to the slots of the renderer first, of the renderer reading the slots
   of the one group of the two of them kept */
static void R_DrawGroup4(int g)
{
    col4slot_t *slot = groupslot[g];
    int x = slotx[g];

    if (((x + alignx) & 3) != 0)
    {
        /* the group has to start at an aligned column, so the leftmost column
           goes on its own and the three of its right are kept for the group of
           the next column, of the band of the group and of the rows of the
           view of it being of the columns kept then */
        int i;

        R_DrawColumn4All(&slot[0], x);

        slot[0] = slot[1];
        slot[1] = slot[2];
        slot[2] = slot[3];

        slotx[g] = x + 1;
        pending[g] = 3;

        bandtop[g] = rowtop[g] = slot[0].top;
        bandbot[g] = rowbot[g] = slot[0].bottom;

        for (i = 1; i < 3; i++)
        {
            if (slot[i].top > bandtop[g])
                bandtop[g] = slot[i].top;
            if (slot[i].bottom < bandbot[g])
                bandbot[g] = slot[i].bottom;
            if (slot[i].top < rowtop[g])
                rowtop[g] = slot[i].top;
            if (slot[i].bottom > rowbot[g])
                rowbot[g] = slot[i].bottom;
        }

        return;
    }

    /* the slots of the group to the slots of the renderer of col4.asm */
    col4slots[0] = slot[0];
    col4slots[1] = slot[1];
    col4slots[2] = slot[2];
    col4slots[3] = slot[3];

    R_DrawColumns4(x);
    pending[g] = 0;
}

/* The columns kept of the group (g) drawn and the group left of none of them:
   of the group of four of the columns the four bytes of a row are written at
   once, and of a group of less of them the columns are drawn one at a time, of
   the renderer of col4.asm writing the four bytes of a row of four of them. */
static void R_EndGroup4(int g)
{
    if (pending[g] == COL4_GSLOT)
        R_DrawGroup4(g);
    else
        R_FlushGroup4(g);
}

/* The columns kept drawn, the group of the view being left of none of them:
   the columns of a group kept are of the columns of the view from its slotx to
   the right, so every one of them is drawn of its slot, of the slots of the
   groups being none of them moved. */
void R_FlushColumn4(void)
{
    /* the seg loops asking for the columns of the tier of the top of a column
       of a seg before the columns of the tier of its bottom, so the group of
       the tier of the top is drawn first, the order of the two of them being
       of the pixels of a column of their own */
    R_EndGroup4(0);
    R_EndGroup4(1);

    col4pending = 0;
}

/* the column function of the seg loops: the column asked for joins the group
   kept of its tier when it is of the next column of the view of it, of the
   groups kept of the tiers of one seg being two of them, and of a column of a
   tier which none of them can take the columns kept are drawn first, the seg
   of the columns kept being the farther one of the view */
void R_DrawColumn4(void)
{
    lighttable_t *map = COL4_MAP(dc_colormap);
    int g;

    /* the column of no rows is of none to the view, the seg loops asking for
       the columns of the tiers of a seg which are not seen */
    if (dc_yl > dc_yh)
        return;

    if (COL4_MATCH(0))
        g = 0;
    else if (COL4_MATCH(1))
        g = 1;
    else
        g = -1;

    if (g >= 0)
    {
        /* the columns kept of the other group of the x of the column asked for
           and of its rows: the columns kept are drawn first, the seg of them
           being the farther one of the view of the seg of the column asked for
           being drawn over it */
        if (COL4_CLASH(1 - g))
            R_EndGroup4(1 - g);
    }
    else if (!pending[0])
    {
        /* the group of the tier of the column asked for opened, the seg loops
           asking for the tier of the top of a seg first, so of the group of
           the tier of the top kept none the column asked for opens it again */
        g = 0;
    }
    else if (!pending[1] && !COL4_CLASH(0))
    {
        /* the column asked for of the other tier of the seg of the columns
           kept, the group of the tier of it opened of its own */
        g = 1;
    }
    else
    {
        /* of none of the groups able to take the column: the columns kept are
           drawn first, the segs of them being the farther ones of the view,
           and the group of the columns asked for is opened first of the seg
           loops asking for the tier of the top of a seg first, the group of
           the tier of its bottom being drawn after the one of the tier of the
           top of the rows of the tier of the top being over the rows of the
           tier of its bottom of one column of the view.  Of a group of four of
           the columns the four bytes of a row are written at once, and of a
           group of less of them the columns are drawn one at a time. */
        R_EndGroup4(0);
        R_EndGroup4(1);

        g = 0;
    }

    COL4_STORE(g, map, dc_x);

    if (pending[g] == COL4_GSLOT)
        R_DrawGroup4(g);

    col4pending = pending[0] + pending[1];
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
    pending[0] = 0;
    pending[1] = 0;
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
