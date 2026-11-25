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
//	Wall renderer which maps two adjacent columns at a time of the
//	detail of the low, so a row of them goes to the view with one
//	aligned 32 bit store instead of two stores of 16 bits: the idea of
//	the renderer of the four columns of r_col4.c, of issue #143, "you
//	render 4 vertical wall lines simultaneously", of the detail which
//	keeps two bytes of the view for one pixel of it.
//
//	The renderer takes the place of the column function of the seg
//	loops of the detail of the low.  A column asked for is kept in a
//	slot, and the two slots are drawn together when they hold two
//	consecutive columns of a 32 bit aligned address: the detail of the
//	low writes a pixel of it with the byte of it in the two bytes of a
//	row of the view, so the two columns of a group are the four bytes of
//	one of its rows, the rows of the group in which the two columns are
//	visible being one 32 bit store each, the rows of the top and the
//	bottom edge being stores of 16 bits, for a row of them costs fewer
//	stores so, and the columns which join no group are drawn one at a
//	time.  The renderer is used only where two columns of the view are
//	the four bytes of a row, which is what R_SetupColumn2() tells.
//
//	The columns kept point into the WAD cache, which the seg loops
//	purge when they take a lump, so every such loop hands the columns
//	kept to the view first, with R_FLUSH_COLUMNS2().
//
//	The rows of a group in which the two columns are visible are of the
//	loops of the file, of the two positions of a row of the registers of
//	the machine and of the four bytes of it of one store of 32 bits, the
//	loop of the group of one light table and of one step walking the two
//	columns with the table of them and with their step, and the loop of
//	the rest of them of the table, of the step and of the source of
//	every column of its own, as of the two loops of the four columns of
//	col4.asm, r_main.c choosing the loops of a CPU of them of its own
//	switch of the CPU, as it chooses the renderers of a mode.
*/

#include "doomdef.h"
#include "options.h"
#include "i_system.h"
#include "r_local.h"
#include "r_col2.h"
#include "v_video.h"
#include "doomstat.h"

#if (defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)) \
    && !defined(MODE_CGA16) && !defined(MODE_CVB) && !defined(MODE_CGA512)

/* the row of a y and the column of an x of the view, two bytes of the view
   being one pixel of it of the detail of the low, so the two columns of a
   group are the four bytes of a row of it */
#if defined(MODE_VBE2_DIRECT)
#define COL2_ROW(y) (destview + MulScreenWidth(y))
#define COL2_COL(x) ((x) << 1)
#else
#define COL2_ROW(y) (ylookup[(y)])
#define COL2_COL(x) (columnofs[(x)])
#endif

/* the rows of the view must be words for a group to be one 32 bit store */
typedef char col2screenwidthaligned[(SCREENWIDTH % 4 == 0) ? 1 : -1];

/*
 * The format of the texture position of a row.
 *
 * The column renderers of the modes of the detail of the low drawn here are
 * assembly, of linearl.asm and of vbe2dl.asm, and they keep the position and
 * the step of a row in seven significant bits and 25 fraction bits, not in the
 * FRACBITS of the C renderers of r_draw.c:
 *
 *     ecx = dc_iscale << 9                        ; the step of a row
 *     edx = (dc_texturemid + (dc_yl - centery) * dc_iscale) << 9
 *     ebx = edx >> 25                             ; the texel, "shr"
 *
 * The texel of a row is of 0 to 127 and a texture wraps vertically at 128
 * texels, "The graphics wrap vertically at 128 pixels" of the assembly source,
 * and the bits above the seven of the position fall away, which the walls
 * close to the view and the tall walls and the masked walls reach.  The texel
 * is taken out of the position with a shift which is not signed, for a signed
 * shift names the bytes before the column of the texture, and the position of a
 * row is walked with the step added once per row, wrapped at 32 bits, as the
 * renderers of the detail of the low of the modes walk it.
 *
 * Of the detail of the low a row of a column of the view is one texel of the
 * texture, as of the detail of one pixel of each row, the pixel of it being of
 * the two bytes of a row of the view; the renderers of the modes of the detail
 * of the low do not step two rows of the view for one texel of them.
 */
#define COL2_FRACSH 25

/* the position of the first row of a column, of dc_yl */
#define COL2_SEEDAT(mid, y, step) \
    (fixed_t)(((mid) + ((y) - centery) * (step)) << 9)

/* the step of a row */
#define COL2_STEP(step) \
    (fixed_t)((step) << 9)

/* the texel of a row, of seven bits which are not signed */
#define COL2_TEXEL(frac) \
    ((int)(((unsigned int)(frac)) >> COL2_FRACSH))

typedef struct
{
    byte *source;
    lighttable_t *colormap;
    fixed_t step;
    int mid;
    int top;
    int bottom;
} col2slot_t;

/* The columns kept: of two groups of two of them, for the seg loops ask for
   the columns of the tier of the top of a seg and for the columns of the tier
   of its bottom of every one of their columns one after the other, so the
   columns of the one tier are kept in the one group and the columns of the
   other tier of the same seg in the other one, the segs of one tier keeping
   the one group only.  The columns of a group are of the columns of the view
   next to one another and of rows which leave the band of the group of rows
   none, the band being the rows of the view every one of the columns of the
   group shows, of the loop of the four bytes of a row writing them.  Of the
   light table and of the step the group is of the shape of the loop of the two
   of them at once, or of the loop of the two of them of their own, which
   COL2_MID tells. */
#define COL2_GROUP 2
#define COL2_GSLOT 2

/* The two columns of the group drawn, col2slots[0] the leftmost one, with the
   x, the first row and the number of the rows of them: the renderer of a CPU
   of the file of asm of them reads them, as the renderer of the four columns
   of col4.asm reads the slots of its own, so they are not of this file only.
   The columns kept of the groups of the file are of the slots of every one of
   them, the two of them of the renderer being copied of the group drawn, of
   the renderer reading the slots of the one group of the two of them kept.  Of
   the slots of the renderer the four of the fields of them are copied, the
   renderer reading none of the rows of the column of the slot, the rows being
   of the file of the slots of the group of its own. */
col2slot_t col2slots[COL2_GSLOT];
static col2slot_t groupslot[COL2_GROUP][COL2_GSLOT];
int col2x;
int col2y;
int col2rows;

/* dc_x of the first column of a group and the number of the columns of it
   kept, col2pending the number of the columns kept of the view, the number
   published in r_col2.h, so the seg loops test it of their own */
static int slotx[COL2_GROUP];
static int pending[COL2_GROUP];
int col2pending;

/* The band of the rows of a group, the rows of the view every one of the
   columns of it shows, of its first row the highest of the tops of them to its
   last row the lowest of their bottoms; and the rows of the view the columns
   of a group touch, of the lowest of their tops to the highest of their
   bottoms, the two of the groups kept at the same time touching none of them
   of the tiers of one seg being one over the other. */
static int bandtop[COL2_GROUP];
static int bandbot[COL2_GROUP];
static int rowtop[COL2_GROUP];
static int rowbot[COL2_GROUP];

/* The renderer of the middle rows of a group of two columns of one light table
   and of one step, of the loop of the file, of the two positions of a row of
   the registers of the machine, which the loop of a CPU of the file of asm of
   them takes of the switch of the CPU of r_main.c, as the loops of the four
   columns of col4.asm are chosen. */
void (*col2midfunc)(void) = R_DrawColumns2Mid;

/* The renderer of the middle rows of a group of two columns of light tables,
   of steps and of sources of their own, of the loop of the file, of the source,
   of the table and of the step of every column of the group of its own, of the
   four bytes of a row of the view of one store of 32 bits of the two of them,
   which the loop of a CPU of the file of asm of them takes of the switch of
   the CPU of r_main.c, as the loops of the four columns of col4.asm are
   chosen. */
void (*col2midallfunc)(void) = R_DrawColumns2MidAll;

/* a group starting at x is 32 bit aligned when ((x + alignx) & 1) == 0, the
   columns of the view of the detail of the low being two bytes of the view of
   their own, so the alignment of the groups of them is of two columns */
static int alignx;

/* The byte renderers of the modes name a pixel of a light table by writing
   the byte of the source over the low byte of the address of the table, "mov
   al,[eax]", which names a pixel only of a table beginning at an address of 256
   bytes, as the tables of the game do.  The mask names the pixel of a table
   which is not of 256 bytes as the mode names it, the renderers of SSE2 of the
   detail of the low of the modes naming it of the "and" of them of the same
   value. */
#define COL2_MAP(map) ((lighttable_t *)((unsigned long)(map) & ~0xffUL))

/* the four bytes of the two pixels of a row of the view of the two bytes of
   every one of them, the byte of the table of the light of a column of them in
   the two bytes of it, as the renderers of the detail of the low of the modes
   write them with their byte in the two bytes of the register of the store of
   16 bits of them */
#define COL2_PAIR(px0, px1) \
    ((unsigned int)(px0) | ((unsigned int)(px0) << 8) \
     | ((unsigned int)(px1) << 16) | ((unsigned int)(px1) << 24))

/* The column asked for kept in the group (g), the group of none kept being
   opened with it, of the x of the group, of the band of its rows and of the
   rows of the view it touches being of it then; of a group of columns kept the
   rows of it are joined with the rows of the column asked for, the band of the
   group taking the rows the two of the columns show and the rows of the view of
   the group taking all of the rows of them.  The group of one column of view
   (x) opened, the group of columns kept of the column of its right. */
#define COL2_STORE(g, map, x)                                          \
    do {                                                               \
        col2slot_t *col = groupslot[g] + pending[g];                   \
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
   the view next to the columns of the group and of rows which leave the band of
   the group of rows none, the band being the rows of the view every one of the
   columns of the group shows.  Of the light table, of the step and of the
   source the group takes of the shape it is: the loop of the middle rows of the
   file walks the two of them with the table of them and with their step, and
   the loop of the two of them of their own walks the rest, of the source, of
   the table and of the step of every one of them of its own, both of them
   writing the four bytes of a row with one store of 32 bits, as COL2_MID
   tells. */
#define COL2_MATCH(g)                                                      \
    (pending[g] && dc_x == slotx[g] + pending[g]                           \
     && dc_yl <= bandbot[g] && dc_yh >= bandtop[g])

/* The columns of the two groups kept are drawn one of them after the other,
   which is of the pixels of the view only of a column of the one of them being
   of the same x as a column of the other and of the rows of them touching, the
   nearer seg of the view being asked for last and drawn last.  Of the two tiers
   of one seg the rows of the one are over the rows of the other, the seg loops
   asking for the columns of the tier of the top of a column before the columns
   of the tier of its bottom, so the groups of the two of them touch none of the
   rows of a column of the view. */
#define COL2_CLASH(g)                                                      \
    (pending[g] && dc_x >= slotx[g] && dc_x < slotx[g] + pending[g]        \
     && dc_yl <= rowbot[g] && dc_yh >= rowtop[g])

/* One column drawn with stores of 16 bits, of all of its rows, the two pixels
   of a row of them of the byte of the table of the light of it: the columns
   which join no group, the rows of the edges of a group being of COL2_EDGES; of
   the texture position of COL2_FRACSH, read as the mode of the detail of the
   low reads it. */
static void R_DrawColumn2All(col2slot_t *col, int x)
{
    const byte *source = col->source;
    const lighttable_t *map = col->colormap;
    fixed_t fracstep = COL2_STEP(col->step);
    int top = col->top;
    int bottom = col->bottom;
    unsigned short *dest;
    fixed_t frac;
    int count;

    if (top <= bottom)
    {
        frac = COL2_SEEDAT(col->mid, top, col->step);
        dest = (unsigned short *)(COL2_ROW(top) + COL2_COL(x));
        count = bottom - top + 1;

        do
        {
            unsigned int px = map[source[COL2_TEXEL(frac)]];

            *dest = (unsigned short)(px | (px << 8));
            frac += fracstep;
            dest = (unsigned short *)((byte *)dest + SCREENWIDTH);
        } while (--count);
    }
}

/* The rows [top, ylo) and (yhi, bottom] of a column of a group which is not of
   one height, drawn with stores of 16 bits, one setup of the slot for both of
   the ranges, the position walked to from the first row of the column. */
#define COL2_EDGES(col, x, ylo, yhi)                                         \
    {                                                                        \
        const byte *source = (col)->source;                                  \
        const lighttable_t *map = (col)->colormap;                           \
        fixed_t fracstep = COL2_STEP((col)->step);                           \
        fixed_t seed = COL2_SEEDAT((col)->mid, (col)->top, (col)->step);     \
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
            dest = COL2_ROW(top) + COL2_COL(x);                              \
            do                                                               \
            {                                                                \
                unsigned int px = map[source[COL2_TEXEL(frac)]];             \
                                                                             \
                *(unsigned short *)dest = (unsigned short)(px | (px << 8));  \
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
            dest = COL2_ROW(y) + COL2_COL(x);                                \
            do                                                               \
            {                                                                \
                unsigned int px = map[source[COL2_TEXEL(frac)]];             \
                                                                             \
                *(unsigned short *)dest = (unsigned short)(px | (px << 8));  \
                frac += fracstep;                                            \
                dest += SCREENWIDTH;                                         \
            } while (--count);                                               \
        }                                                                    \
    }

/* the two columns of the slots (slot) drawn one at a time, of no row showing
   both of them */
static void R_DrawColumns2Alone(int x, col2slot_t *slot)
{
    int i;

    for (i = 0; i < COL2_GSLOT; i++)
        if (slot[i].top <= slot[i].bottom)
            R_DrawColumn2All(&slot[i], x + i);
}

/* The group of one light table and of one step, which is what a seg of one
   texture gives: the loop of the middle rows of the file walks the two of them
   with the light table of the leftmost of the group and with its step, the
   sources of the two of them being read of their own and the positions of the
   two of them being taken of the mid of every one of them, so the columns of
   the texture of more than one lump, of a wall not parallel to the view and of
   the texture which wraps are of the loop of the two bytes of a row as well.

   Told of the two columns of the group, the comparisons of the light table and
   of the step being of the two of them. */
#define COL2_MID()                                              \
    (col2slots[0].colormap == col2slots[1].colormap             \
     && col2slots[0].step == col2slots[1].step)

/* the middle rows of the two columns of the view of x, of the rows of ylo to
   yhi, handed over to the renderer of the columns kept */
#define COL2_MID_DRAW(x, ylo, yhi)     \
    do {                               \
        col2x = (x);                   \
        col2y = (ylo);                 \
        col2rows = (yhi) - (ylo) + 1;  \
        col2midfunc();                 \
    } while (0)

/* the middle rows of the two columns of the view of x, of the rows of ylo to
   yhi, of a group which is not of the shape of the loop above: of more than one
   light table or of more than one step, so every one of its columns is walked
   on its own, of the loop of the file or of the loop of a CPU of the file of
   asm of them, r_main.c writing the pointer of the loop of them as it writes
   the pointer of the loop above */
#define COL2_ALL_DRAW(x, ylo, yhi)    \
    do {                              \
        col2x = (x);                  \
        col2y = (ylo);                \
        col2rows = (yhi) - (ylo) + 1; \
        col2midallfunc();             \
    } while (0)

/* The middle rows of the group of two columns of one light table and of one
   step, of the four bytes of a row of the view of one store of 32 bits, the
   byte of the table of the light of every one of the columns of the group in
   the two bytes of it of the row, as the renderers of the detail of the low of
   the modes write the two pixels of a row of their column.

   The positions of the two columns of a row are walked of the mid of every one
   of them, for the texture position of a column of a seg is not the position of
   its neighbour of the walls not parallel to the view, and of one step of them
   of the group of the test above. */
void R_DrawColumns2Mid(void)
{
    const byte *source0 = col2slots[0].source;
    const byte *source1 = col2slots[1].source;
    const lighttable_t *map = COL2_MAP(col2slots[0].colormap);
    fixed_t fracstep = COL2_STEP(col2slots[0].step);
    fixed_t frac0 = COL2_SEEDAT(col2slots[0].mid, col2y, col2slots[0].step);
    fixed_t frac1 = COL2_SEEDAT(col2slots[1].mid, col2y, col2slots[1].step);
    byte *dest = COL2_ROW(col2y) + COL2_COL(col2x);
    int count = col2rows;

    do
    {
        *(unsigned int *)dest = COL2_PAIR(map[source0[COL2_TEXEL(frac0)]],
                                          map[source1[COL2_TEXEL(frac1)]]);

        frac0 += fracstep;
        frac1 += fracstep;
        dest += SCREENWIDTH;
    } while (--count);
}

/* The middle rows of the group of two columns of light tables, of steps and of
   sources of their own, of the source, of the table and of the step of every
   one of them of its own, of the four bytes of a row of the view of one store
   of 32 bits of them as well: the groups of the segs of more than one texture,
   of the two last columns of a seg with the two first of the next one of the
   same tier, and of the columns of the tiers of one seg of the tables of the
   light of their own. */
void R_DrawColumns2MidAll(void)
{
    const byte *source0 = col2slots[0].source;
    const byte *source1 = col2slots[1].source;
    const lighttable_t *map0 = COL2_MAP(col2slots[0].colormap);
    const lighttable_t *map1 = COL2_MAP(col2slots[1].colormap);
    fixed_t fracstep0 = COL2_STEP(col2slots[0].step);
    fixed_t fracstep1 = COL2_STEP(col2slots[1].step);
    fixed_t frac0 = COL2_SEEDAT(col2slots[0].mid, col2y, col2slots[0].step);
    fixed_t frac1 = COL2_SEEDAT(col2slots[1].mid, col2y, col2slots[1].step);
    byte *dest = COL2_ROW(col2y) + COL2_COL(col2x);
    int count = col2rows;

    do
    {
        *(unsigned int *)dest = COL2_PAIR(map0[source0[COL2_TEXEL(frac0)]],
                                          map1[source1[COL2_TEXEL(frac1)]]);

        frac0 += fracstep0;
        frac1 += fracstep1;
        dest += SCREENWIDTH;
    } while (--count);
}

/* the group of two columns of the slots (slot) of the view of x, with 32 bit
   stores where the two are visible, the rows of them of the slots of the group
   and the two of the fields of the renderer of the slots of the renderer, the
   renderer of the columns kept of the fields of these reading */
static void R_DrawColumns2(int x, col2slot_t *slot)
{
    int ylo, yhi;
    int top, bottom;
    int equal;

    ylo = slot[0].top;
    yhi = slot[0].bottom;
    equal = 1;

    if (ylo > yhi)
    {
        /* an empty column, so there is no row with the two of them */
        R_DrawColumns2Alone(x, slot);
        return;
    }

    top = slot[1].top;
    bottom = slot[1].bottom;

    if (top > bottom)
    {
        /* an empty column, so there is no row with the two of them */
        R_DrawColumns2Alone(x, slot);
        return;
    }

    /* the rows of the two columns, from the highest of the tops of them to the
       lowest of their bottoms; 'equal' tells the group of one height, which has
       no rows of the edges at all */
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

    if (ylo > yhi)
    {
        /* the walls of the group do not overlap, so no row shows the two of
           them and every column is drawn on its own */
        R_DrawColumns2Alone(x, slot);
        return;
    }

    /* the group of one height, a wall between two floors: every row of it
       shows the two columns, so there is no edge to draw */
    if (equal)
    {
        /* the columns of a seg are of one light table and one step */
        if (COL2_MID())
            COL2_MID_DRAW(x, ylo, yhi);
        else
            COL2_ALL_DRAW(x, ylo, yhi);
        return;
    }

    /* the rows of the two edges, one setup for the column of both of them */
    COL2_EDGES(&slot[0], x, ylo, yhi);
    COL2_EDGES(&slot[1], x + 1, ylo, yhi);

    /* the rows with the two columns, one 32 bit store per row */
    if (COL2_MID())
        COL2_MID_DRAW(x, ylo, yhi);
    else
        COL2_ALL_DRAW(x, ylo, yhi);
}

/* the columns kept of the group (g) drawn one at a time with stores of 16
   bits, of the group of less than two of them, of the left of them to the
   right */
static void R_FlushGroup2(int g)
{
    col2slot_t *slot = groupslot[g];
    int x = slotx[g];
    int i;

    for (i = 0; i < pending[g]; i++, x++)
        R_DrawColumn2All(&slot[i], x);

    pending[g] = 0;
}

/* the two columns of the group (g) drawn of the rows of their band with the
   renderer of the columns kept, the slots of the group being copied to the
   slots of the renderer first, of the renderer reading the slots of the one
   group of the two of them kept */
static void R_DrawGroup2(int g)
{
    col2slot_t *slot = groupslot[g];
    int x = slotx[g];

    if (((x + alignx) & 1) != 0)
    {
        /* the group has to start at an aligned column, so the leftmost column
           goes on its own and the one of its right is kept for the group of the
           next column, of the band of the group and of the rows of the view of
           it being of the column kept then, the group of it of the column of
           the view of an address aligned */
        R_DrawColumn2All(&slot[0], x);

        slot[0] = slot[1];

        slotx[g] = x + 1;
        pending[g] = 1;

        bandtop[g] = rowtop[g] = slot[0].top;
        bandbot[g] = rowbot[g] = slot[0].bottom;

        return;
    }

    /* the four of the fields of the slots of the group to the slots of the
       renderer, the renderer reading none of the rows of them */
    col2slots[0].source = slot[0].source;
    col2slots[0].colormap = slot[0].colormap;
    col2slots[0].step = slot[0].step;
    col2slots[0].mid = slot[0].mid;

    col2slots[1].source = slot[1].source;
    col2slots[1].colormap = slot[1].colormap;
    col2slots[1].step = slot[1].step;
    col2slots[1].mid = slot[1].mid;

    R_DrawColumns2(x, slot);
    pending[g] = 0;
}

/* The columns kept of the group (g) drawn and the group left of none of them:
   of the group of two of the columns the four bytes of a row are written at
   once, and of a group of less of them the columns are drawn one at a time, of
   the renderer of the file writing the four bytes of a row of the two of
   them. */
static void R_EndGroup2(int g)
{
    if (pending[g] == COL2_GSLOT)
        R_DrawGroup2(g);
    else
        R_FlushGroup2(g);
}

/* The columns kept drawn, the group of the view being left of none of them: the
   columns of a group kept are of the columns of the view from its slotx to the
   right, so every one of them is drawn of its slot, of the slots of the groups
   being none of them moved. */
void R_FlushColumn2(void)
{
    /* the seg loops asking for the columns of the tier of the top of a column
       of a seg before the columns of the tier of its bottom, so the group of
       the tier of the top is drawn first, the order of the two of them being of
       the pixels of a column of their own */
    R_EndGroup2(0);
    R_EndGroup2(1);

    col2pending = 0;
}

/* the column function of the seg loops: the column asked for joins the group
   kept of its tier when it is of the next column of the view of it, of the
   groups kept of the tiers of one seg being two of them, and of a column of a
   tier which none of them can take the columns kept are drawn first, the seg of
   the columns kept being the farther one of the view */
void R_DrawColumn2(void)
{
    lighttable_t *map;
    int g;

    /* the column of no rows is of none to the view, the seg loops asking for
       the columns of the tiers of a seg which are not seen */
    if (dc_yl > dc_yh)
        return;

    map = COL2_MAP(dc_colormap);

    if (COL2_MATCH(0))
        g = 0;
    else if (COL2_MATCH(1))
        g = 1;
    else
        g = -1;

    if (g >= 0)
    {
        /* the columns kept of the other group of the x of the column asked for
           and of its rows: the columns kept are drawn first, the seg of them
           being the farther one of the view of the seg of the column asked for
           being drawn over it */
        if (COL2_CLASH(1 - g))
            R_EndGroup2(1 - g);
    }
    else if (!pending[0])
    {
        /* the group of the tier of the column asked for opened, the seg loops
           asking for the tier of the top of a seg first, so of the group of the
           tier of the top kept none the column asked for opens it again.

           The columns kept of the group of the other tier are of a seg asked
           for before the seg of the column asked for, the farther one of the
           view, and the group of the column asked for is drawn before the group
           of the columns kept of it, the columns of a group being drawn when
           the two of them are kept.  So the columns kept are handed to the view
           first, of the pixels of a column of their own being of the nearer seg
           of the two of them, the seg of the column asked for being asked for
           last of the BSP of the frame of them. */
        if (pending[1])
            R_EndGroup2(1);

        g = 0;
    }
    else if (!pending[1] && !COL2_CLASH(0))
    {
        /* the column asked for of the other tier of the seg of the columns
           kept, the group of the tier of it opened of its own */
        g = 1;
    }
    else
    {
        /* of none of the groups able to take the column: the columns kept are
           drawn first, the segs of them being the farther ones of the view, and
           the group of the columns asked for is opened first of the seg loops
           asking for the tier of the top of a seg first, the group of the tier
           of its bottom being drawn after the one of the tier of the top of the
           rows of the tier of the top being over the rows of the tier of its
           bottom of one column of the view.  Of a group of two of the columns
           the four bytes of a row are written at once, and of a group of less
           of them the columns are drawn one at a time. */
        R_EndGroup2(0);
        R_EndGroup2(1);

        g = 0;
    }

    COL2_STORE(g, map, dc_x);

    if (pending[g] == COL2_GSLOT)
    {
        /* the group of the column asked for full, its four bytes of a row
           written at once and its columns kept of none left */
        R_DrawGroup2(g);
    }

    col2pending = pending[0] + pending[1];
}

/* Called when the renderers of the frame are set up: the columns kept of the
   frame before are dropped and the alignment of the groups of the view is
   taken.  The renderer is returned, and the function asked for of the other
   detail levels, whose renderers write a pixel of one and of four bytes, and of
   the views of the modes which do not keep the view one byte per pixel.

   The two columns of a group are the four bytes of a row of the view of the
   detail of the low, so the address of the group of the first of them is of the
   units of four bytes of them: of the address of the first column of the view
   of the row of the groups of none of the columns kept the two bytes of a row
   of them being taken, and of the address of it of one byte of them the two
   columns of a group being of the two of the bytes of a row and of the byte of
   a row of their own, the function asked for is returned. */
void (*R_SetupColumn2(void (*fallback)(void)))(void)
{
    unsigned long base;

    pending[0] = 0;
    pending[1] = 0;
    col2pending = 0;

    if (detailshift != DETAIL_LOW)
        return fallback;

    /* the address of the first column of the first row of the view, the rows of
       the view being of the units of four bytes of them of the modes of the
       guard above, SCREENWIDTH of them being so of them */
#if defined(MODE_VBE2_DIRECT)
    base = (unsigned long)destview;
#else
    base = (unsigned long)ylookup[0] + (unsigned long)columnofs[0];
#endif

    /* the columns of the view of the detail of the low being two bytes of them,
       the address of the group of the column of x is of the units of four bytes
       of them when of the half of the address of the first column of the view
       the column of x is of a value of them even, the alignment of the groups of
       the columns of the view being of it */
    if (base & 1)
        return fallback;

    alignx = (int)((base >> 1) & 1);

    return R_DrawColumn2;
}

#endif
