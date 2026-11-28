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
//	The wall renderers of the columns of the view kept, of the two of
//	them in a file: the renderer of the four columns at a time of the detail
//	of high, of the idea of Ken Silverman from issue #143, "you render 4
//	vertical wall lines simultaneously", and the renderer of the two columns
//	at a time of the detail of the low, of the same idea and of the detail
//	which keeps two bytes of the view for one pixel of it.
//
//	The bookkeeping of the columns kept is the same of the two of them, which
//	is why they are here together: the state of the groups is one, and the shape
//	of the renderer of the frame, the columns of a group, the bytes of a pixel of
//	the view, the slots of the renderer of asm of them and the loops of its middle
//	rows, is told by R_SetupColumn4() and R_SetupColumn2(), which drop the columns
//	kept of the frame before.  The columns kept point into the WAD cache, which
//	the seg loops purge when they take a lump, so every such loop hands the
//	columns kept to the view first, with R_FLUSH_COLUMNS4() and R_FLUSH_COLUMNS2(),
//	which test colp, the number of the columns kept of the renderer of the frame.
//
//	The middle rows of a group are of the file of asm of the renderer of them,
//	col2.asm and col4.asm, which draw the group of one light table and of one step
//	of the registers of the machine, of the registers of MMX and of the
//	instructions of SSE2, r_main.c choosing the loop of a CPU of them, as it
//	chooses the renderers of a mode.
*/

#include "doomdef.h"
#include "options.h"
#include "i_system.h"
#include "r_local.h"
#include "r_col2.h"
#include "r_col4.h"
#include "v_video.h"
#include "doomstat.h"

#if (defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)) \
    && !defined(MODE_CGA16) && !defined(MODE_CVB) && !defined(MODE_CGA512)

/* the row of a y and the column of an x of the view, the columns of the view of
   the detail of (shift) of them being of the bytes of them of the number of it:
   the detail of high of one byte of the view for a pixel of it and the detail of
   the low of the two of them */
#if defined(MODE_VBE2_DIRECT)
#define COL_ROW(y) (destview + MulScreenWidth(y))
#define COL_COL(x, shift) ((x) << (shift))
#else
#define COL_ROW(y) (ylookup[(y)])
#define COL_COL(x, shift) (columnofs[(x)])
#endif

/* the rows of the view must be of the units of four bytes for a group of them to
   be one store of 32 bits of a row of them */
typedef char colscreenswidthaligned[(SCREENWIDTH % 4 == 0) ? 1 : -1];

/*
 * The format of the texture position of a row.
 *
 * The assembly renderers of the modes keep the position and the step of a row in
 * seven significant bits and 25 fraction bits, not in the FRACBITS of the C
 * renderers of r_draw.c:
 *
 *     ecx = dc_iscale << 9                        ; the step of a row
 *     edx = (dc_texturemid + (dc_yl - centery) * dc_iscale) << 9
 *     ebx = edx >> 25                             ; the texel, "shr"
 *
 * The texel of a row is of 0 to 127, "The graphics wrap vertically at 128
 * pixels" of the assembly source, and it is taken out of the position with a
 * shift which is not signed, for a signed shift names the bytes before the column
 * of the texture.  The position is walked with the step added once per row,
 * wrapped at 32 bits, as the assembly renderers walk it.
 */
#define COL_FRACSH 25

/* the position of the first row of a column, of dc_yl */
#define COL_SEEDAT(mid, y, step) \
    (fixed_t)(((mid) + ((y) - centery) * (step)) << 9)

/* the step of a row */
#define COL_STEP(step) \
    (fixed_t)((step) << 9)

/* the texel of a row, of seven bits which are not signed */
#define COL_TEXEL(frac) \
    ((int)(((unsigned int)(frac)) >> COL_FRACSH))

typedef struct
{
    byte *source;
    lighttable_t *colormap;
    fixed_t step;
    int mid;
    int top;
    int bottom;
} colslot_t;

/* a run of rows of one column of the view, of the dest of them the first of them
   and of the frac of them the position of it, walked with the step of them; the
   writer of a pixel of a row of the byte of the view of the detail of high and the
   writer of the two bytes of a row of it of the detail of the low */
typedef void (*colrun_t)(const byte *source, const lighttable_t *map,
                         fixed_t frac, fixed_t fracstep, byte *dest,
                         int count);

/* The columns kept, of twenty four groups and of the columns of a group the two
   of them of the detail of the low and the four of them of the detail of high,
   which are the four bytes of a row of the view of their detail: one group of the
   columns of a tier of a seg, one of the tier of its bottom, and one of every one
   of the posts of a column of a wall masked, the columns of a group being next to
   one another and of rows which leave the band of the group, the rows of the view
   every one of its columns shows, none.  The segs of the frame of the nearer keep
   of the tiers of them and of the posts of them more groups at the same time, and
   the posts of a column of a wall masked of more groups than the renderer has are
   drawn one at a time.

   The columns kept of the groups of the columns asked for before a group, of the
   columns of them of the same x of the view and of the rows of them touching, are
   handed over to the view before the columns of the group are drawn, as the seg
   loops draw them. */
#define COL_GROUP 24
#define COL_GSLOT 4

/* The slots of the renderer of the middle rows of a group of them of the file of
   asm of them, col2.asm and col4.asm, of the group drawn copied to them, the
   leftmost column of the group the first of them: the x of the group, the first of
   its rows and the number of the rows of them being in col2x, col2y and col2rows
   and in col4x, col4y and col4rows, which the file writes before the call of the
   loop of them.  The renderer reads none of the rows of the slots, the rows being
   of the slots of the group of its own. */
colslot_t col2slots[2];
colslot_t col4slots[4];
int col2x;
int col2y;
int col2rows;
int col4x;
int col4y;
int col4rows;

/* the number of the columns kept of the view, of the renderer of the frame one
   of the two of the renderers of the file being, which the macros of the flush of
   the seg loops test */
int colp;

/* The loops of the middle rows of a group of them of the files of asm of them,
   of the group of one light table and of one step and of the group of the columns
   of the sources, of the tables and of the steps of their own, the four bytes of a
   row of the view being written with one store of 32 bits of them: of the
   registers of the machine by default, of the registers of MMX for the CPUs of MMX
   and of the instructions of SSE2 for the Pentium 4, r_main.c writing the pointers
   of them of its own switch of the CPU. */
void (*col2midfunc)(void) = R_DrawColumns2Mid;
void (*col2midallfunc)(void) = R_DrawColumns2MidAll;
void (*col4midfunc)(void) = R_DrawColumns4Mid;
void (*col4midallfunc)(void) = R_DrawColumns4MidAll;

/* The state of the groups of the columns kept, of the two of the renderers at
   once, the renderer of the frame one of them being and the columns kept of the
   frame before being dropped by the setup of it. */

/* dc_x of the first column of a group and the number of the columns of it kept */
static int slotx[COL_GROUP];
static int pending[COL_GROUP];

/* the order of the requests of a group, given to it when it is opened, the group
   of the number of it the lower one keeping the columns asked for first, and gcount
   the number of the groups opened */
static int gseq[COL_GROUP];
static int gcount;

/* the list of the groups which keep columns, of nglist of them, of the group of
   its columns kept taken being left of it */
static int glist[COL_GROUP];
static int nglist;

/* The group of the column asked for while the groups of the columns asked for
   before it are handed over to the view, its clash not being ended by its own turn.

   The groups which keep columns of the bit of them of gmask set, of the groups of
   the bit of them of none set the group of the lowest one of them taken when none
   of them is free, the segs of the view asking for more tiers of them and more
   posts of them than the groups of the renderer being. */
static int busy;
static unsigned int gmask;

/* the band of the rows of a group, the rows of the view every one of its columns
   shows, and the rows of the view its columns touch, of the lowest of their tops to
   the highest of their bottoms, the groups of the two tiers of one seg of the rows
   of them one over the other being */
static int bandtop[COL_GROUP];
static int bandbot[COL_GROUP];
static int rowtop[COL_GROUP];
static int rowbot[COL_GROUP];

/* the slots of the groups of the columns kept, of COL_GSLOT of them every one of
   them, of the renderer of the two columns the first two of a group taking */
static colslot_t groupslot[COL_GROUP][COL_GSLOT];

/* The shape of the renderer of the frame, of the setup of it written: the
   columns of a group and the bytes of a pixel of the view, of the alignment of the
   groups of them being of the number of them of a group taken; the slots and the
   variables of the renderer of asm of them; the pointers of the loops of its middle
   rows, of r_main.c writing them of its own switch of the CPU; and the writer of a
   run of rows of one column of the view of the detail of it */
static int gslot;
static int alignmask;
static int shift;
static colslot_t *renderSlots;
static int *renderX;
static int *renderY;
static int *renderRows;
static void (**renderMid)(void);
static void (**renderMidAll)(void);
static colrun_t renderRun;

/* the columns of the view of the first row of the groups, of the alignment of the
   groups of them being: a group starting at x is aligned when
   ((x + alignx) & alignmask) == 0 */
static int alignx;

/* the bit of the group (g) of gmask, set of its columns kept opened and cleared
   of its columns drawn */
#define COL_BIT(g) (1u << (g))

/* The byte renderers of the modes name a pixel of a light table by writing the
   byte of the source over the low byte of the address of the table, "mov
   al,[eax]", which names a pixel only of a table beginning at an address of 256
   bytes, as the tables of the game do: of the table of more than 256 bytes of them
   the mask names the pixel as the modes of them name it */
#define COL_MAP(map) ((lighttable_t *)((unsigned long)(map) & ~0xffUL))

/* The column asked for kept in the slot of the group (g) of the number of its
   columns kept, the group of none kept being opened with it, of the x (x) of the
   column of its right: of a group of columns kept the band of its rows takes the
   rows the columns of it show and the rows of the view of the group takes all of
   the rows of them, the rows of the column asked for being joined with them. */
#define COL_STORE(g, map, x)                                          \
    do {                                                              \
        colslot_t *col = groupslot[g] + pending[g];                   \
                                                                      \
        col->source = dc_source;                                      \
        col->colormap = (map);                                        \
        col->step = dc_iscale;                                        \
        col->mid = dc_texturemid;                                     \
        col->top = dc_yl;                                             \
        col->bottom = dc_yh;                                          \
                                                                      \
        if (!(gmask & COL_BIT(g)))                                    \
        {                                                             \
            /* the group opened takes the number of the requests of its       \
            columns and joins the list of the groups of columns kept */       \
            gmask |= COL_BIT(g);                                      \
            gseq[g] = ++gcount;                                       \
            glist[nglist++] = g;                                      \
                                                                      \
            bandtop[g] = rowtop[g] = dc_yl;                           \
            bandbot[g] = rowbot[g] = dc_yh;                           \
            slotx[g] = (x);                                           \
            pending[g] = 1;                                           \
        }                                                             \
        else                                                          \
        {                                                             \
            if (dc_yl > bandtop[g])                                   \
                bandtop[g] = dc_yl;                                   \
            if (dc_yh < bandbot[g])                                   \
                bandbot[g] = dc_yh;                                   \
            if (dc_yl < rowtop[g])                                    \
                rowtop[g] = dc_yl;                                    \
            if (dc_yh > rowbot[g])                                    \
                rowbot[g] = dc_yh;                                    \
                                                                      \
            pending[g]++;                                             \
        }                                                             \
                                                                      \
        /* one more column of the view kept, the loops of the groups of their \
        own of none being */                                          \
        colp++;                                                       \
    } while (0)

/* The column asked for of the column of the view next to the columns of the
   group (g) and of rows which leave the band of the group, the rows of the view
   every one of its columns shows, none, so of the loop of the four bytes of a row
   of them being.  Of the light table, of the step and of the source the group of
   one of the two shapes of the loop of the middle rows of them, which COL_MID
   tells.

   Of the groups of the list of the groups of columns kept testing. */
#define COL_MATCH(g)                                                       \
    (dc_x == slotx[g] + pending[g]                                         \
     && dc_yl <= bandbot[g] && dc_yh >= bandtop[g])

/* The pixels of the column asked for of a column of the group (g) of the same x
   of the view of one of them being and of the rows of them touching, so the two of
   the groups are to be drawn one of them after the other, the seg of the columns
   asked for the last, the nearer one of the view, being drawn last.  Of the two
   tiers of one seg the rows of the one are over the rows of the other, the seg
   loops asking for the tier of the top of a column before the tier of its bottom,
   so the groups of them of the rows of a column of the view touch none.

   Of the groups of the list of the groups of columns kept testing. */
#define COL_CLASH(g)                                                       \
    (dc_x >= slotx[g] && dc_x < slotx[g] + pending[g]                      \
     && dc_yl <= rowbot[g] && dc_yh >= rowtop[g])

/* The groups (g) and (o) keeping a column of the one x of the view of them of
   the rows of them touching, the group (o) of the number of the requests of its
   columns the lower one of the two of them, its columns kept being handed over to
   the view first: the group of the columns asked for first being of the farther
   seg of the view, and the wall of the tier of the top of a seg being under the
   wall of the tier of its bottom of it */
#define COL_CLASH2(g, o)                                                       \
    (slotx[o] < slotx[g] + pending[g] && slotx[g] < slotx[o] + pending[o]      \
     && rowtop[o] <= rowbot[g] && rowtop[g] <= rowbot[o])

/* The group of the columns of it of the slots of the renderer of them of one
   light table and of one step, which is what a seg of one texture gives: the loop
   of the middle rows of them walks the columns of them with the table of the
   leftmost of the group and with its step, the sources of them being read of their
   own, so the columns of the texture of more than one lump, of a wall not parallel
   to the view and of the texture which wraps are of the loop of the four bytes of
   a row as well.  Told of the column (i) of the group with the leftmost of it, the
   comparisons of the table and of the step being transitive, the group of the
   columns of the tables, of the steps and of the sources of their own being of the
   loop of the columns of them of their own */
#define COL_MID(i)                                                             \
    (renderSlots[0].colormap == renderSlots[i].colormap                        \
     && renderSlots[0].step == renderSlots[i].step)

/* the middle rows of the columns of the group of the view of x, of the rows of
   ylo to yhi, of the (func) of them of the file of asm of them, which reads the
   slots of the renderer and the x, the y and the rows of the group written here */
#define COL_DRAW(x, ylo, yhi, func) \
    do {                            \
        *renderX = (x);             \
        *renderY = (ylo);           \
        *renderRows = (yhi) - (ylo) + 1; \
        (func)();                   \
    } while (0)

/* A run of rows of one column of the view of a byte of them of a pixel of it,
   of the detail of high. */
static void R_DrawRunByte(const byte *source, const lighttable_t *map,
                          fixed_t frac, fixed_t fracstep, byte *dest,
                          int count)
{
    do
    {
        *dest = map[source[COL_TEXEL(frac)]];
        frac += fracstep;
        dest += SCREENWIDTH;
    } while (--count);
}

/* A run of rows of one column of the view of the two bytes of them of a pixel of
   it, of the detail of the low: the byte of the light table of a row writing the
   two bytes of it, as the renderers of the modes of the detail of the low write the
   two pixels of a row. */
static void R_DrawRunWord(const byte *source, const lighttable_t *map,
                          fixed_t frac, fixed_t fracstep, byte *dest, int count)
{
    do
    {
        unsigned int px = map[source[COL_TEXEL(frac)]];

        *(unsigned short *)dest = (unsigned short)(px | (px << 8));
        frac += fracstep;
        dest += SCREENWIDTH;
    } while (--count);
}

/* One column drawn one pixel of the view of a row at a time, of all of its rows,
   of the writer of a run of them of the detail of the renderer of the frame. */
static void R_DrawColumnAll(const colslot_t *col, int x)
{
    /* of the column of the rows of it none, the seg loops asking for them */
    renderRun(col->source, col->colormap,
              COL_SEEDAT(col->mid, col->top, col->step),
              COL_STEP(col->step),
              COL_ROW(col->top) + COL_COL(x, shift),
              col->bottom - col->top + 1);
}

/* The rows [top, ylo) and (yhi, bottom] of a column of a group which is not of
   one height, drawn one pixel of the view of a row at a time, the position of the
   row walked to from the first row of the column. */
static void R_DrawColumnEdges(const colslot_t *col, int x, int ylo, int yhi)
{
    fixed_t fracstep = COL_STEP(col->step);
    fixed_t seed = COL_SEEDAT(col->mid, col->top, col->step);
    int top = col->top;
    int bottom = col->bottom;

    if (top < ylo)
        renderRun(col->source, col->colormap, seed, fracstep,
                  COL_ROW(top) + COL_COL(x, shift), ylo - top);

    if (bottom > yhi)
    {
        int y = yhi + 1;

        renderRun(col->source, col->colormap, seed + (y - top) * fracstep,
                  fracstep, COL_ROW(y) + COL_COL(x, shift), bottom - y + 1);
    }
}

/* the group of the columns of it of the slots (slot) of the view of x, of the
   rows of the band of them of ylo to yhi: of the rows of the band the rows of the
   edges of the columns of them of one column of the view at a time written, and the
   rows of them of all of its columns showing of the renderer of the columns kept of
   one store of 32 bits of a row */
static void R_DrawColumns(int x, colslot_t *slot, int ylo, int yhi)
{
    int equal = 1;
    int i;

    /* ylo, the highest of the tops of the columns of the group, and yhi, the
       lowest of their bottoms, are of the tests of the group telling, so none of
       the columns of it leaves the band of the group.

       'equal' tells the group of one height, a wall between two floors, which has
       no rows of the edges at all */
    for (i = 0; i < gslot; i++)
        if (slot[i].top != ylo || slot[i].bottom != yhi)
            equal = 0;

    if (!equal)
    {
        /* the rows of the two edges, of the rows of the band of the column of
           them none taking */
        for (i = 0; i < gslot; i++)
            R_DrawColumnEdges(slot + i, x + i, ylo, yhi);
    }

    /* the rows with all of the columns of the group, of one store of 32 bits of a
       row, the group of one light table and of one step being of the loop of the
       columns of them at once and the group of the tables, of the steps and of the
       sources of their own of the loop of the columns of them of their own */
    for (i = 1; i < gslot; i++)
        if (!COL_MID(i))
            break;

    if (i == gslot)
        COL_DRAW(x, ylo, yhi, *renderMid);
    else
        COL_DRAW(x, ylo, yhi, *renderMidAll);
}

/* the group (g) of none of the columns kept left, of the list of the groups of
   columns kept of it taken, the last one of the list of them to its place moved, and
   of the bit of it of gmask cleared */
static void R_FreeGroup(int g)
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

    gmask &= ~COL_BIT(g);
    colp -= pending[g];
    pending[g] = 0;
}

/* the columns kept of the group (g) drawn one at a time, of the left of them to
   the right, and the group left of none of them */
static void R_FlushGroup(int g)
{
    colslot_t *slot = groupslot[g];
    int x = slotx[g];
    int i;

    for (i = 0; i < pending[g]; i++, x++)
        R_DrawColumnAll(&slot[i], x);

    R_FreeGroup(g);
}

/* the columns kept of a group drawn and the group left of none of them */
static void R_EndGroup(int g);

/* the columns of the group (g) drawn of the rows of their band with the renderer
   of the columns kept of the file of asm of them, of a group starting at an aligned
   column of the view */
static void R_DrawGroup(int g)
{
    colslot_t *slot = groupslot[g];
    int x = slotx[g];
    int i;

    if (((x + alignx) & alignmask) != 0)
    {
        /* the group has to start at an aligned column, so the leftmost column
           goes on its own and the columns of its right are kept for the group of
           the next column, the band of the group and the rows of the view it
           touching being of the columns kept then */
        R_DrawColumnAll(&slot[0], x);

        for (i = 0; i < gslot - 1; i++)
            slot[i] = slot[i + 1];

        slotx[g] = x + 1;
        pending[g] = gslot - 1;
        colp--;

        /* the band of the group and the rows of the view of it recomputed of the
           columns kept, of the writes to the state of the group of the one of them
           at the end */
        {
            int bt = slot[0].top, bb = slot[0].bottom;
            int rt = bt, rb = bb;

            for (i = 1; i < gslot - 1; i++)
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

    /* the slots of the group to the slots of the renderer of them, the renderer
       reading none of the rows of them */
    for (i = 0; i < gslot; i++)
    {
        renderSlots[i].source = slot[i].source;
        renderSlots[i].colormap = slot[i].colormap;
        renderSlots[i].step = slot[i].step;
        renderSlots[i].mid = slot[i].mid;
    }

    R_DrawColumns(x, slot, bandtop[g], bandbot[g]);
    R_FreeGroup(g);
}

/* The columns kept of the group (g) drawn and the group left of none of them, of
   the group of the columns of it of a group the four bytes of a row of them written
   at once and of a group of less of them the columns drawn one at a time. */
static void R_EndGroup(int g)
{
    int i;

    /* The groups kept of the columns asked for before the columns of the group
       ended, of a column of the one x of the view of the two of them and of the
       rows of them touching, are handed over to the view first, the segs of the
       view being asked for of the farther one to the nearer one, the wall of the
       tier of the top of a seg being under the wall of the tier of its bottom of it
       and the posts of a column of a wall masked of the rows of them of none to the
       other being under the posts of the columns of the rows above them.

       Of the group ended taken of the list of the groups of columns kept, the last
       one of the list of them to its place moved, so of the group of the columns
       asked for of the number of it of gseq[] below the group ended the loop of the
       list of them testing again. */
    if (nglist > 1)
    {
        i = 0;

        while (i < nglist)
        {
            int o = glist[i];

            if (o != g && o != busy && gseq[o] < gseq[g]
                && COL_CLASH2(g, o))
            {
                R_EndGroup(o);
                i = 0;
            }
            else
                i++;
        }
    }

    if (pending[g] == gslot)
        R_DrawGroup(g);
    else
        R_FlushGroup(g);
}

/* The columns kept drawn and the groups of them left of none of them, of the
   group of the columns asked for first to the group of the columns asked for the
   last, of the number of them of gseq[] the lower one being the group of the
   columns asked for first: the segs of the view being asked for of the farther one
   to the nearer one, so the wall of the seg asked for the last, the nearer one of
   the view, is drawn over the walls of the segs asked for before it.

   The macros of the flush of the two of the headers of them, R_FLUSH_COLUMNS2() and
   R_FLUSH_COLUMNS4(), call this of the renderer of the frame one of them being. */
void R_FlushColumn(void)
{
    int i, best;

    /* of the list of the groups of columns kept the group of the columns asked
       for first of them drawn every one of them */
    while (nglist)
    {
        best = glist[0];

        for (i = 1; i < nglist; i++)
            if (gseq[glist[i]] < gseq[best])
                best = glist[i];

        R_EndGroup(best);
    }

    colp = 0;
}

/* the column function of the seg loops of the renderer of the frame: the column
   asked for joins the group kept of its tier when it is of the next column of the
   view of it and of rows which leave the band of the group none, and of a column
   which none of the groups can take the columns kept of them are drawn first */
static void R_DrawColumnKeep(void)
{
    lighttable_t *map;
    int g, i;

    /* the column of no rows, the seg loops asking for the tiers of a seg which
       are not seen */
    if (dc_yl > dc_yh)
        return;

    map = COL_MAP(dc_colormap);

    /* The group of the column asked for, of the groups of columns kept the one of
       the columns of it kept of the column of the view next to them of their left
       and of the rows of them leaving the band of the group of rows none, of the
       list of the groups of columns kept of them testing.

       Of the groups of them more than one of the column asked for matching, the
       group of the columns asked for first of taken, the seg of the columns of it
       being the farther one of the view, so of the order of the walls of the segs of
       none breaking being.  The columns of the tier of the top of a seg join the
       group of them of the tier of them, the group of the tier of the top of them
       the columns of the tier of its bottom of them not joining, for the band of the
       rows of the group of them of the two of the tiers being of the narrow.

       Of the pixels of the column asked for a group of the columns asked for after
       the columns of it covering, the column asked for of the group of it of none
       joining, the pixels of the column asked for of a nearer seg of the view taken
       being of none to the view.  The group of the number of the requests of its
       columns the last one of the pixels of the column asked for covering of taken,
       of the groups of the number of the requests of their columns of it greater none
       of the pixels of the column asked for covering, the segs of the view asking for
       the tier of the top of a column before the tier of its bottom of it.
    */
    g = -1;

    if (nglist == 1)
    {
        /* the case of the segs of one tier and of the walls of one post, the
           columns of the view of one group only being kept and the pixels of the
           column asked for of none of the groups kept covered */
        g = glist[0];

        if (!COL_MATCH(g))
            g = -1;
    }
    else if (nglist > 1)
    {
        /* the number of the requests of the columns of the group kept of the last
           one of the pixels of the column asked for covering, of the groups of the
           number of the requests of their columns of it greater none covering */
        int last = 0;

        for (i = 0; i < nglist; i++)
        {
            int o = glist[i];

            if (gseq[o] > last && COL_CLASH(o))
                last = gseq[o];
        }

        for (i = 0; i < nglist; i++)
        {
            int o = glist[i];

            if (gseq[o] > last && COL_MATCH(o)
                && (g < 0 || gseq[o] < gseq[g]))
                g = o;
        }
    }

    busy = g;

    if (g >= 0)
    {
        /* The columns of the view of the x of the column asked for of more groups
           kept, one group of the columns of a tier of a seg and of a post of a wall
           masked, the segs of the columns of the groups kept being the farther ones
           of the view of the seg of the column asked for: their columns are handed
           over to the view first, the wall of the column asked for being drawn over
           the walls of them.  Of the segs of one tier and of the walls of one post,
           the columns of the view of one group only being kept, none of them of the
           x of the column asked for being. */
        if (nglist > 1)
        {
            /* the groups kept of the x of the column asked for ended of the list
               of them, the list of them of the place of the group of it the last one
               of them moved being, of the first of the list of them testing again */
            i = 0;

            while (i < nglist)
            {
                int o = glist[i];

                if (o != g && COL_CLASH(o))
                {
                    R_EndGroup(o);
                    i = 0;
                }
                else
                    i++;
            }
        }
    }
    else
    {
        /* the group of none of the columns kept: the seg loops ask for the tier
           of the top of a seg before the tier of its bottom and for the posts of a
           column of the view of the first of them to the last one, so every one of
           their tiers and every one of their posts keeps a group of its own */
        g = 0;

        while (g < COL_GROUP && (gmask & COL_BIT(g)))
            g++;

        if (g >= COL_GROUP)
        {
            /* none of the groups of none of the columns kept, the segs of the view
               asking for more tiers of them and more posts of them than the groups of
               the renderer: of the groups kept the one of the columns asked for first,
               its number of gseq[] the lower one of them, is handed over to the view,
               its columns of them drawn one at a time, and of the group of it the
               column asked for opens it again, the columns of the tiers of them and of
               the posts of them keeping the groups of their own. */
            int old = glist[0], best;

            for (i = 1; i < nglist; i++)
            {
                best = glist[i];

                if (gseq[best] < gseq[old])
                    old = best;
            }

            /* the group handed over to the view and the column asked for of it */
            R_EndGroup(old);
            g = old;
            busy = g;
        }
    }

    COL_STORE(g, map, dc_x);

    if (pending[g] == gslot)
    {
        /* the group of the column asked for full: of the groups kept of the
           columns asked for before it, the group ended of the x of them and of their
           rows touching */
        R_EndGroup(g);
    }

    busy = -1;
}

/* The shape of one of the two renderers of the file: the detail of it, the
   columns of a group of them and the bytes of a pixel of the view of the detail of
   it, the slots of the renderer of asm of them and the variables of them of the x,
   of the first row and of the number of the rows of the group drawn, the pointers
   of the loops of its middle rows and the writer of a run of rows of one column of
   it.  The column function of the seg loops of the two of them is R_DrawColumnKeep,
   of the shape of the renderer of the frame it reading. */
typedef struct
{
    int detail;
    int columns;
    int bits;
    colslot_t *slots;
    int *x;
    int *y;
    int *rows;
    void (**midfunc)(void);
    void (**midallfunc)(void);
    colrun_t run;
} colcfg_t;

/* The setup of the renderer of the frame: the columns kept of the frame before
   are dropped and the shape of the renderer of it is taken.

   The column function of the renderer of the frame is returned, and the function
   asked for of the other detail levels and of the views of the modes which do not
   keep the four bytes of a row for the columns of a group, the renderers of them
   writing a pixel of one, of two and of four bytes. */
static void (*R_SetupColumn(void (*fallback)(void), const colcfg_t *cfg))(void)
{
    unsigned long base;
    int i;

    /* the columns kept of the frame before dropped, every one of the groups of
       none of its columns kept left */
    for (i = 0; i < COL_GROUP; i++)
        pending[i] = 0;

    nglist = 0;
    gmask = 0;
    busy = -1;
    gcount = 0;
    colp = 0;

    /* the shape of the renderer of the frame, the state of the file being one */
    gslot = cfg->columns;
    alignmask = cfg->columns - 1;
    shift = cfg->bits;
    renderSlots = cfg->slots;
    renderX = cfg->x;
    renderY = cfg->y;
    renderRows = cfg->rows;
    renderMid = cfg->midfunc;
    renderMidAll = cfg->midallfunc;
    renderRun = cfg->run;

    if (detailshift != cfg->detail)
        return fallback;

    /* the address of the first column of the first row of the view, the rows of
       the view being of the units of four bytes of them of the modes of the guard
       above */
#if defined(MODE_VBE2_DIRECT)
    base = (unsigned long)destview;
#else
    base = (unsigned long)ylookup[0] + (unsigned long)columnofs[0];
#endif

    /* of the address of the first column of the view of less of the units of four
       bytes of them the groups of them of none aligned being, the function asked for
       returned */
    if ((base & ((1UL << cfg->bits) - 1UL)) != 0)
        return fallback;

    alignx = (int)((base >> cfg->bits) & (unsigned long)alignmask);

    return R_DrawColumnKeep;
}

/* the renderer of the two columns of the detail of the low, the two of them
   being the four bytes of a row of the view of it */
static const colcfg_t col2cfg =
{
    DETAIL_LOW, 2, 1,
    col2slots, &col2x, &col2y, &col2rows,
    &col2midfunc, &col2midallfunc,
    R_DrawRunWord
};

/* the renderer of the four columns of the detail of high, the four of them being
   the four bytes of a row of the view of it */
static const colcfg_t col4cfg =
{
    DETAIL_HIGH, 4, 0,
    col4slots, &col4x, &col4y, &col4rows,
    &col4midfunc, &col4midallfunc,
    R_DrawRunByte
};

/* Called when the renderers of the frame are set up: the renderer of the two
   columns of the detail of the low is returned, and the function asked for of the
   other detail levels and of the views which do not keep the four bytes of a row for
   the two columns of a group. */
void (*R_SetupColumn2(void (*fallback)(void)))(void)
{
    return R_SetupColumn(fallback, &col2cfg);
}

/* Called when the renderers of the frame are set up: the renderer of the four
   columns of the detail of high is returned, and the function asked for of the other
   detail levels and of the views which do not keep the four bytes of a row for the
   four columns of a group.  A view which does not start at a word is drawn with the
   renderer, the columns in front of the first group of its rows being drawn one at a
   time. */
void (*R_SetupColumn4(void (*fallback)(void)))(void)
{
    return R_SetupColumn(fallback, &col4cfg);
}

#endif
