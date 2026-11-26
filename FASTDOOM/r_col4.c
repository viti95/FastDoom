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
//	time.  The columns kept are of the groups of them, one group of the
//	columns of a tier of a seg and one group of every one of the posts of
//	a column of a wall masked, the groups kept of the columns asked for
//	before a group drawn being handed over to the view first, so the wall
//	of the seg asked for the last is drawn over the walls of the segs
//	asked for before it.  The renderer is used only where four columns of
//	the view are the four bytes of a row, which is what R_SetupColumn4()
//	tells.
//
//	The columns kept point into the WAD cache, which the seg loops
//	purge when they take a lump, so every such loop hands the columns
//	kept to the view first, with R_FLUSH_COLUMNS4().
//
//	The rows of a group in which the four columns are visible are of the
//	assembly of col4.asm, which draws a group of one light table and of one
//	step of the registers of the machine and of the registers of MMX, r_main.c
//	choosing between the two of them as it chooses the other renderers of a
//	mode, and of the loop of the registers of the machine, of the registers of
//	MMX and of the instructions of SSE2 the groups of light tables, of steps
//	and of sources of their own, r_main.c choosing between the three of them
//	of the same way.
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

/* The columns kept: of the groups of four of them, for the seg loops ask for
   the columns of the tier of the top of a seg and for the columns of the tier
   of its bottom of every one of their columns one after the other, and for the
   posts of a column of a wall masked of the first of them to the last one, so
   the columns of the one tier are kept in the one group and the columns of the
   other tier of the same seg in the other one, every one of the posts of a
   column of the view keeping a group of its own, the segs of one tier and the
   walls of one post keeping the one group only.  The columns of a group are of
   the columns of the view next to one another and of rows which leave the band
   of the group of rows none, the band being the rows of the view every one of
   the columns of the group shows, of the loop of the four bytes of a row
   writing them.  Of the light table and of the step the group is of the shape
   of the loop of the four of them at once, or of the shape of the loop of the
   four of them of their own, which COL4_MID tells.

   The columns kept of the groups of the number of them below are handed over to
   the view before the columns of a group are drawn, of the columns of them
   being of a column of the view of the same x as a column of the group and of
   the rows of them touching, the walls of the seg of the columns asked for the
   last being drawn over the walls of the segs of the columns asked for before
   them, as the seg loops draw them.

   Of the groups the number is of the segs of the view which of the columns of
   their own keep them at the same time: of a seg of the tier of the top and of
   the tier of the bottom one of every one of them, and of a wall masked one of
   every one of the posts of the column of the view, the segs of the frame of
   the nearer the segs of the view of the tiers of them and of the posts of them
   kept at the same time.  Of the columns of a column of a wall masked of the
   posts of them the number of them being of the groups of the renderer above,
   the posts of the columns of the view of none of the groups of their own are
   drawn one at a time with byte stores, the four bytes of a row of the group of
   the four of them being none of them of the columns kept at the same time. */
#define COL4_GROUP 24
#define COL4_GSLOT 4

/* The four columns of the group drawn, col4slots[0] the leftmost one, with
   the x, the first row and the number of the rows of them: the renderer of
   col4.asm reads them, so they are not of this file only.  The columns kept of
   the groups of the file are of the slots of every one of them, the four of
   them of the renderer being copied of the group drawn, of the renderer
   reading the slots of the one group only while the two of them are kept.  Of
   the slots of the renderer the four of the fields of them are copied, the
   renderer of col4.asm reading none of the rows of the column of the slot, the
   rows being of the file of the slots of the group of its own. */
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

/* the order of the requests of the groups kept, the number of a group given to
   it when it is opened, the group of the number of it below being the group of
   the columns asked for first, and gcount the number of the groups opened */
static int gseq[COL4_GROUP];
static int gcount;

/* The groups of columns kept, of the list of them glist[] the number of them
   being nglist, and the groups of none of the columns kept, of the list of them
   flist[] the number of them being nf, a group of the last of them of the list
   of them taken when a group is opened and of the list of them given back to it
   when its columns are drawn: of the loops of the x of a column of them and of
   the band of their rows the groups of columns kept of none testing, so the
   loops of them are of the number of the groups of columns kept at the same
   time, which is of the segs of one tier and of the walls of one post of the
   columns of the view of one group only. */
static int glist[COL4_GROUP];
static int nglist;

/* The group of the column asked for of the loops of the clash of none ending:
   the columns kept of the groups of the columns asked for before them handed
   over to the view end first of their own, of the loop of the clash of the group
   of the columns asked for being none of the groups ended, the group of it of
   the list of the groups of columns kept of taken the columns of it of the slot
   of the column asked for written.  Of the group of the column asked for of none
   kept the group of the loop of the clash of the group of it of taken the group
   of the columns of it kept of the list of the groups of none of the columns kept
   given back, the group of the columns asked for of the columns of it written. */
static int busy;
static int flist[COL4_GROUP];
static int nf;

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

/* The renderer of the middle rows of a group of four columns of light tables,
   of steps and of sources of their own, of the assembly of col4.asm: of the
   registers of the machine here, of the registers of MMX for the CPUs of MMX,
   which walk the four positions of the row and take the four texels of them,
   and of the instructions of SSE2 for the Pentium 4, the loads of the bytes
   of the sources and of the pixels of the tables being of the registers of
   the machine of the three of them, every one of them writing the four bytes
   of a row of the view with one store of 32 bits.  r_main.c chooses between
   the three of them of its own switch of the CPU, as it chooses the renderer
   of the group of one light table and of one step. */
void (*col4midallfunc)(void) = R_DrawColumns4MidAll;

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
/* the column asked for kept of the group (g), the rows of the band of the
   group being the rows of the view of the four of the columns of it (of the
   highest of the tops of them to the lowest of their bottoms), and the rows of
   the view of the group of the test of the clash being the union of the rows
   of the columns of the group, kept always: the group of none opening with the
   test of the clash of the group of it being open, of the union of the group
   of it being of the columns kept then */
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
        /* the group opened of none of the columns kept takes the number of  \
        the requests of its columns, the group of the tier of the top of a \
        seg being the group of the columns asked for first of them, and joins \
        the list of the groups of columns kept */                         \
        if (!pending[g])                                                 \
        {                                                                \
            gseq[g] = ++gcount;                                          \
            glist[nglist++] = g;                                         \
        }                                                                \
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
                                                                       \
        /* the column of the view kept of the number of them of the file of    \
        the renderer one more, of the loop of the groups of their own of    \
        none being */                                                       \
        col4pending++;                                                 \
    } while (0)

/* The column asked for of the next column of the group (g): of the column of
   the view next to the columns of the group and of rows which leave the band
   of the group of rows none, the band being the rows of the view every one of
   the columns of the group shows.  Of the light table, of the step and of the
   source the group takes of the shape it is: the loop of the middle rows of
   col4.asm of one table and of one step walks the four of the columns at once,
   and the loop of the four of them of their own walks the rest, of the
   registers of the machine, of the registers of MMX and of the instructions
   of SSE2, all of them writing the four bytes of a row with one store of 32
   bits, as COL4_MID tells. */
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

/* The group (g) drawn and the group (o) kept have a column of the one x of the
   view of them, the columns of it touching the rows of the columns of the other
   one of them, and the group (o) is of the columns asked for before the columns
   of the group (g), its number of gseq[] being the lower one of the two of them:
   the columns kept of the group (o) are handed over to the view first, the seg of
   the columns of it being the farther one of the view of the seg of the columns
   of the group (g), and the wall of the tier of the top of a seg being under the
   wall of the tier of its bottom of it. */
#define COL4_CLASH2(g, o)                                                      \
    (slotx[o] < slotx[g] + pending[g] && slotx[g] < slotx[o] + pending[o]      \
     && rowtop[o] <= rowbot[g] && rowtop[g] <= rowbot[o])

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

/* the four columns of the slots (slot) drawn one at a time, of no row showing
   all of them */
static void R_DrawColumns4Alone(int x, col4slot_t *slot)
{
    int i;

    for (i = 0; i < 4; i++)
        if (slot[i].top <= slot[i].bottom)
            R_DrawColumn4All(&slot[i], x + i);
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
   one light table or of more than one step, so every one of its columns is
   walked on its own, of the loop of the registers of the machine, of the
   registers of MMX or of the instructions of SSE2, r_main.c writing the
   pointer of the loop of them as it writes the pointer of the loop above */
#define COL4_ALL_DRAW(x, ylo, yhi)    \
    do {                              \
        col4x = (x);                  \
        col4y = (ylo);                \
        col4rows = (yhi) - (ylo) + 1; \
        col4midallfunc();             \
    } while (0)

/* the group of four columns of the slots (slot) of the view of x, with 32 bit
   stores where the four are visible, the rows of them of the slots of the
   group and the four of the fields of the renderer of the slots of the
   renderer, the renderer of col4.asm of the fields of these reading */
static void R_DrawColumns4(int x, col4slot_t *slot)
{
    int ylo, yhi;
    int equal;
    int i;

    ylo = slot[0].top;
    yhi = slot[0].bottom;
    equal = 1;

    if (ylo > yhi)
    {
        /* an empty column, so there is no row with the four of them */
        R_DrawColumns4Alone(x, slot);
        return;
    }

    /* the rows of the four columns, from the highest of the tops of them to
       the lowest of their bottoms; 'equal' tells the group of one height,
       which has no rows of the edges at all */
    for (i = 1; i < 4; i++)
    {
        int top = slot[i].top;
        int bottom = slot[i].bottom;

        if (top > bottom)
        {
            /* an empty column, so there is no row with the four of them */
            R_DrawColumns4Alone(x, slot);
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
        R_DrawColumns4Alone(x, slot);
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
        COL4_EDGES(&slot[i], x + i, ylo, yhi);

    /* the rows with the four columns, one 32 bit store per row */
    if (COL4_MID())
        COL4_MID_DRAW(x, ylo, yhi);
    else
        COL4_ALL_DRAW(x, ylo, yhi);
}

/* A group of none of the columns kept, of the list of the groups of them the
   group of columns kept of it taken, the group of the last of them of the list
   of them to the place of it moved, and of the list of the groups of none of the
   columns kept given back to it, the group of its own of the last of them */

static void R_FreeGroup4(int g)
{
    int i;

    for (i = 0; i < nglist; i++)
    {
        if (glist[i] == g)
        {
            nglist--;
            glist[i] = glist[nglist];
            break;
        }
    }

    flist[nf++] = g;
    col4pending -= pending[g];
    pending[g] = 0;
}

/* The column asked for of the group (g) of the columns of it joining, the group
   of them of the four bytes of a row of the columns of it written at once, of
   the columns of the group (g) asked for first and of the column asked for of
   the group of them kept.

   The column asked for of the columns asked for last of, so of the walls of the
   segs of the columns of the groups kept of the columns asked for after the
   columns of the group (g) (of their gseq below the gseq of the column asked
   for) being over the walls of the segs of the columns of them.  The group (g)
   of the group of them of the columns asked for first of being, the group of
   them before the group of them kept handed over to the view, so of the column
   asked for of the x of it and of its rows of a group of them kept clashing,
   the walls of the seg of it of the walls of the segs of the columns of the
   group of them kept above being of the group of them drawn first, the walls of
   the seg of the column asked for of the walls of the segs of the columns of the
   group of them kept under being: none of the order of the walls of the segs of
   them breaking being, the column asked for of the group of it joining none.

   The column asked for of the groups kept of the columns asked for after the
   columns of the group (g) of none clashing, the group of it joining of the
   walls of the seg of it of the walls of the segs of the columns of the groups
   kept of none below being.  The group of the columns of the view of one group
   kept of the columns asked for after them none, the case of the segs of one
   tier and of the walls of one post, the loop of them of none testing. */
static int R_NewerClash4(int g)
{
    int i;

    if (nglist > 1)
        for (i = 0; i < nglist; i++)
        {
            int o = glist[i];

            if (o != g && gseq[o] > gseq[g] && COL4_CLASH(o))
                return 1;
        }

    return 0;
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

    R_FreeGroup4(g);
}

/* the columns kept of a group drawn and the group left of none of them, of the
   file below, the group of the columns asked for first being drawn first of the
   groups kept at the same time */
static void R_EndGroup4(int g);

/* the four columns of the group (g) drawn of the rows of their band with the
   renderer of the columns kept of col4.asm, the slots of the group being
   copied to the slots of the renderer first, of the renderer reading the slots
   of the one group of them kept */
static void R_DrawGroup4(int g)
{
    col4slot_t *slot = groupslot[g];
    int x = slotx[g];
    int i;

    /* the groups kept of the columns asked for before the columns of the group
       drawn, of a column of the one x of the view of them and of the rows of
       them touching, are handed over to the view by R_EndGroup4() of the file
       below, the walls of the segs of the columns of the groups kept being under
       the walls of the seg of the columns of the group drawn */
    if (((x + alignx) & 3) != 0)
    {
        /* the group has to start at an aligned column, so the leftmost column
           goes on its own and the three of its right are kept for the group of
           the next column, of the band of the group and of the rows of the
           view of it being of the columns kept then */
        R_DrawColumn4All(&slot[0], x);

        slot[0] = slot[1];
        slot[1] = slot[2];
        slot[2] = slot[3];

        slotx[g] = x + 1;
        pending[g] = 3;
        col4pending--;

        /* the band of the group and the rows of the view of it of the three of
           the columns kept, of the registers for the comparisons of them and of
           the writes to the state of the group of the one of them at the end */
        {
            int bt = slot[0].top, bb = slot[0].bottom;
            int rt = bt, rb = bb;

            for (i = 1; i < 3; i++)
            {
                if (slot[i].top > bt)
                    bt = slot[i].top;
                if (slot[i].bottom < bb)
                    bb = slot[i].bottom;
                if (slot[i].top < rt)
                    rt = slot[i].top;
                if (slot[i].bottom > rb)
                    rb = slot[i].bottom;
            }

            bandtop[g] = bt;
            bandbot[g] = bb;
            rowtop[g] = rt;
            rowbot[g] = rb;
        }

        return;
    }

    /* the four of the fields of the slots of the group to the slots of the
       renderer of col4.asm, the renderer reading none of the rows of them */
    for (i = 0; i < 4; i++)
    {
        col4slots[i].source = slot[i].source;
        col4slots[i].colormap = slot[i].colormap;
        col4slots[i].step = slot[i].step;
        col4slots[i].mid = slot[i].mid;
    }

    R_DrawColumns4(x, slot);
    R_FreeGroup4(g);
}

/* The columns kept of the group (g) drawn and the group left of none of them:
   of the group of four of the columns the four bytes of a row are written at
   once, and of a group of less of them the columns are drawn one at a time, of
   the renderer of col4.asm writing the four bytes of a row of four of them. */
static void R_EndGroup4(int g)
{
    int i;

    /* The groups kept of the columns asked for before the columns of the group
       ended, of a column of the one x of the view of the two of them and of the
       rows of them touching, are handed over to the view first, the walls of the
       segs of the columns of them being under the walls of the seg of the
       columns of the group ended: the segs of the view being asked for of the
       farther one to the nearer one, the walls of the tier of the top of a seg
       being under the walls of the tier of its bottom of it, and the posts of a
       column of a wall masked of the rows of them of none to the other being
       under the posts of the columns of the view of the rows above them.

       The columns kept of a group are drawn of the rows of the view of the group
       of them, the rows of its band and the rows of the edges of them of them,
       so of the group ended of the columns asked for first the groups kept of
       the columns of it of the x of them and of their rows touching end first of
       their own, of the walls of the segs of the columns of them being under the
       walls of the seg of the columns of the group ended.  The test of the clash
       is of the group ended and of the group kept of their own, the walls of the
       segs of the columns of the groups kept of the two of them of the same
       order of the columns asked for being of the order of the group of them of
       the x of them and of their rows of the test.

       Of the group ended of the list of the groups of columns kept taken the
       list of them of the place of the group of it the group of the last of them
       moved, the loop of them of the group of the last of them of them of none
       testing, the group of the columns asked for of the loop of the group of it
       of none ending of the number of it of gseq[] of the group ended below
       being. */
    if (nglist > 1)
    {
        /* the groups kept of the x of the group of them and of their rows
           touching, the last of them of the list of the groups of it taken, the
           last one of the list of them to the place of the one ended moved being,
           of the first of the list of them testing again */
        i = 0;

        while (i < nglist)
        {
            int o = glist[i];

            if (o != g && o != busy && gseq[o] < gseq[g]
                && COL4_CLASH2(g, o))
            {
                R_EndGroup4(o);
                i = 0;
            }
            else
                i++;
        }
    }

    if (pending[g] == COL4_GSLOT)
        R_DrawGroup4(g);
    else
        R_FlushGroup4(g);
}

/* The columns kept drawn, the groups of the view being left of none of them:
   the columns of a group kept are of the columns of the view from its slotx to
   the right, so every one of them is drawn of its slot, of the slots of the
   groups being none of them moved.

   The groups are handed over to the view of the group of the columns asked for
   first to the group of the columns asked for the last, of the number of them of
   gseq[] the lower one being the group of the columns asked for first, the segs
   of the view being asked for of the farther one to the nearer one and the
   columns of the tier of the top of a seg being asked for before the columns of
   the tier of its bottom of it, of the posts of a column of a wall masked being
   asked for of the first of them to the last one, the seg of the columns asked
   for the last being the nearer one of the view, the wall of it being drawn over
   the walls of the segs of the columns asked for before them. */
void R_FlushColumn4(void)
{
    int i, best;

    /* of the list of the groups of columns kept the group of the columns asked
       for first of them taken every one of the groups of them drawn, of the
       group drawn of the list of them taken the list of them of none left */
    while (nglist)
    {
        best = glist[0];

        for (i = 1; i < nglist; i++)
            if (gseq[glist[i]] < gseq[best])
                best = glist[i];

        R_EndGroup4(best);
    }

    col4pending = 0;
}

/* the column function of the seg loops: the column asked for joins the group
   kept of its tier when it is of the next column of the view of it, of a group
   of the columns of a tier of a seg and of a group of the posts of a column of a
   wall masked, and of a column of a tier or of a post which none of the groups
   can take the group of the columns asked for first is handed over to the view,
   the seg of its columns being the farther one of the view, the group of the
   columns asked for opening of the groups of none of them kept */
void R_DrawColumn4(void)
{
    lighttable_t *map;
    int g, i;

    /* the column of no rows is of none to the view, the seg loops asking for
       the columns of the tiers of a seg which are not seen */
    if (dc_yl > dc_yh)
        return;

    map = COL4_MAP(dc_colormap);

    /* the group of the column asked for, of the groups of columns kept the one
       of the columns of it kept of the column of the view next to them of their
       left and of the rows of them leaving the band of the group of rows none,
       of the list of the groups of columns kept of them testing.

       Of the groups of them more than one of the column asked for matching, the
       group of the columns asked for first of taken: the seg of the columns of
       it the farther one of the view of the seg of the columns of the other ones
       being, the walls of the seg of the column asked for over the walls of the
       segs of the columns of it of the group of it drawn first, so of the order
       of the walls of the segs of none breaking being.  The columns of the tier
       of the top of a seg of the group of them of the tier of them kept of the
       column asked for of the tier of its bottom joining being of the band of
       the group of the rows of the two of the tiers of the narrow, the walls of
       the seg of the columns of the band of them of the group of them written
       at once, the walls of the seg of the columns of the rows of the edges of
       them of one column of the view at a time written, so of the group of the
       tier of the top of them the columns of the tier of its bottom of them not
       joining */
    g = -1;
    for (i = 0; i < nglist; i++)
    {
        int o = glist[i];

        if (COL4_MATCH(o) && !R_NewerClash4(o)
            && (g < 0 || gseq[o] < gseq[g]))
            g = o;
    }

    busy = g;

    if (g >= 0)
    {
        /* the columns kept of the groups of the x of the column asked for and of
           its rows: every one of them is of a column asked for before the column
           asked for, the seg of them being the farther one of the view of the
           seg of the column asked for, and the wall of the tier of the top of a
           seg being under the wall of the tier of its bottom of it, so the
           columns of them are handed over to the view first, the wall of the
           column asked for being drawn over the walls of them.

           The columns of a column of the view of the x of the column asked for
           are kept of the groups of the columns of more tiers of them and of more
           posts of them than the one of them, so of the segs of one tier and of
           the walls of one post, the columns of the view of one group only being
           kept, none of them of the x of the column asked for being, the loop of
           them of the groups of the x of them of none testing */
        if (nglist > 1)
        {
            /* the groups kept of the x of the column asked for, the last of
               them of the list of the groups of it taken, the last one of the
               list of them to the place of the one ended moved being, of the
               first of the list of them testing again */
            i = 0;

            while (i < nglist)
            {
                int o = glist[i];

                if (o != g && COL4_CLASH(o))
                {
                    R_EndGroup4(o);
                    i = 0;
                }
                else
                    i++;
            }
        }
    }
    else
    {
        /* the group of none of the columns kept, of the groups of their own the
           columns of the tiers of a seg and the posts of a wall masked keeping:
           the seg loops ask for the tier of the top of a seg before the tier of
           its bottom and for the posts of a column of the view of the first of
           them to the last one, so the columns of every one of their tiers and of
           every one of their posts keep a group of their own, the four columns of
           the group of a tier of them written at once of the four bytes of a row,
           the group of the tier of the top of a seg being the group of the
           columns asked for first of the two of them, the wall of it being under
           the wall of the tier of its bottom of it, and the group of a post of a
           column of the view of the columns asked for before the group of the
           post of its right, of the rows of them of none to the other */
        g = nf ? flist[--nf] : -1;

        if (g < 0)
        {
            /* none of the groups of none of the columns kept, the segs of the
               view asking for the columns of more tiers of them and of more posts
               of them than the groups of the renderer: of the groups kept the one
               of the columns asked for first, its number of gseq[] the lower one
               of them, is handed over to the view, the seg of its columns being
               the farther one of the view of the segs of the columns of the other
               groups, and of the group of it the column asked for opens it again,
               the columns of the tiers of them and of the posts of them kept in
               the groups of their own being kept of their own, of the columns of
               them of one tier and of one post the four of them of a row of the
               view writing.

               The columns kept of the group handed over to the view are of less
               of four of them, of the columns of them drawn one at a time, which
               is the shape of the columns of a tier of a seg and of a post of a
               wall masked the last of them of the groups of the renderer. */
            int old = glist[0], best;

            for (i = 1; i < nglist; i++)
            {
                best = glist[i];

                if (gseq[best] < gseq[old])
                    old = best;
            }

            /* the columns of the group handed over to the view and the group of
               the list of the groups of columns kept taken, of the list of the
               groups of none of the columns kept the last of them being it */
            R_EndGroup4(old);
            g = flist[--nf];
            busy = g;
        }
    }

    COL4_STORE(g, map, dc_x);

    if (pending[g] == COL4_GSLOT)
    {
        /* the group of the column asked for full, its four bytes of a row
           written at once and its columns kept of none left: of the groups kept
           of the columns asked for before the columns of it, the group ended of
           the x of them and of their rows touching, the walls of the segs of the
           columns of them of the walls of the seg of the columns of the group
           drawn below being */
        R_EndGroup4(g);
    }

    busy = -1;
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
    int i;

    for (i = 0; i < COL4_GROUP; i++)
    {
        pending[i] = 0;
        flist[i] = i;
    }

    nglist = 0;
    nf = COL4_GROUP;
    busy = -1;
    gcount = 0;
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
