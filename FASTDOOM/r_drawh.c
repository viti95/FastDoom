/*
 * r_drawh.c
 *
 * Horizontal column rendering ("horiz" method), ported from
 * r_drawt.c in the ZDoom 1.22 source.
 *
 * Instead of writing wall columns directly to the screen, columns
 * are first expanded into a small temporary buffer which holds four
 * interleaved columns (one byte per column per row). The buffer is
 * then flushed to the frame buffer in wide (word / dword) chunks.
 * The spans of the four columns are aligned (rt_nudgecols) so that
 * contiguous regions are written with the widest possible store.
 *
 * Unlike ZDoom 1.22, the color map is applied while expanding the
 * column into the temp buffer (per-column light level), so the flush
 * functions are plain copies.
 *
 * This is the C89 reference implementation. The ASM optimized version
 * can replace R_DrawColumnHoriz() and rt_Draw1col() / rt_Draw2cols()
 * / rt_Draw4cols() later; the rest of the pipeline (span rings,
 * R_HorizGroupFlush() / R_HorizSegEnd()) stays the same.
 *
 * Requires an 8bpp linear frame buffer (MODE_13H / MODE_VBE2
 * backbuffer, MODE_PGC, MODE_VBE2_DIRECT).
 */

#include "options.h"
#include "doomdef.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "v_video.h"
#include "r_drawh.h"

#if defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)

/*
 * Temporary buffer: 4 columns per row, one row per view row.
 */
static byte rt_temp[SCREENHEIGHT * 4];

/*
 * Span rings per temp column: (top, bottom) pairs.
 */
#define RT_MAXSPANS 256
static unsigned short rt_tspans[4][RT_MAXSPANS][2];
static unsigned short *rt_ctspan[4];	/* write pointers */
static unsigned short *rt_span[4];	/* read pointers  */

/* First screen column of the group currently being filled. */
static int rt_pendstart = 0;

/* Frame buffer row pitch, in bytes. */
static int rt_pitch;

#if !defined(USE_BACKBUFFER)
/* MODE_VBE2_DIRECT: row pointers into the VBE frame buffer. */
static byte *rt_row[SCREENHEIGHT];
#endif

/*
 * Span -> screen copy functions. The color map is already applied
 * when the column was expanded into the temp buffer.
 */
static void (*rt_post1)(int hx, int sx, int yl, int yh);
static void (*rt_post2)(int hx, int sx, int yl, int yh);
static void (*rt_post4)(int sx, int yl, int yh);

/* Frame buffer address of (x, y). */
#if defined(USE_BACKBUFFER)
#define RT_SCREEN(x, y) (ylookup[(y)] + columnofs[(x)])
#else
#define RT_SCREEN(x, y) (rt_row[(y)] + (x))
#endif

//
// Copies one span at hx to the screen at sx.
//
static void rt_Copy1col(int hx, int sx, int yl, int yh)
{
    byte *source = &rt_temp[hx + 4 * yl];
    byte *dest = (byte *)RT_SCREEN(sx, yl);
    int count = yh - yl + 1;
    int pitch = rt_pitch;

    if (count < 0)
        return;

    do
    {
        *dest = *source;
        source += 4;
        dest += pitch;
    } while (--count);
}

//
// Copies two spans at hx and hx+1 to the screen at sx and sx+1.
//
static void rt_Copy2cols(int hx, int sx, int yl, int yh)
{
    unsigned short *source = (unsigned short *)&rt_temp[hx + 4 * yl];
    unsigned short *dest = (unsigned short *)RT_SCREEN(sx, yl);
    int count = yh - yl + 1;
    int pitch = rt_pitch / sizeof(unsigned short);

    if (count < 0)
        return;

    do
    {
        *dest = *source;
        source += 2;
        dest += pitch;
    } while (--count);
}

//
// Copies all four spans to the screen starting at sx.
//
static void rt_Copy4cols(int sx, int yl, int yh)
{
    unsigned int *source = (unsigned int *)&rt_temp[4 * yl];
    unsigned int *dest = (unsigned int *)RT_SCREEN(sx, yl);
    int count = yh - yl + 1;
    int pitch = rt_pitch / sizeof(unsigned int);

    if (count < 0)
        return;

    do
    {
        *dest = *source;
        source++;
        dest += pitch;
    } while (--count);
}

//
// R_HorizInitFrame
// Per-frame initialization. Call once per frame before rendering.
//
void R_HorizInitFrame(void)
{
#if !defined(USE_BACKBUFFER)
    int y;
#endif

    rt_pitch = MulScreenWidth(1);

#if !defined(USE_BACKBUFFER)
    /* destview may change (view window), so rebuild the row table. */
    for (y = 0; y < viewheight; y++)
        rt_row[y] = destview + y * rt_pitch;
#endif

    rt_InitCols();
    rt_pendstart = 0;
}

//
// rt_InitCols
// Resets the span read pointers. Call before starting a new group
// of columns (all pending spans must have been flushed already).
//
void rt_InitCols(void)
{
    int y;

    for (y = 3; y >= 0; y--)
        rt_span[y] = rt_ctspan[y] = &rt_tspans[y][0][0];
}

//
// R_DrawColumnHoriz
// Expands the current column into the temporary buffer and records
// the span. Same dc_* interface as R_DrawColumn().
//
void R_DrawColumnHoriz(void)
{
    int count = dc_yh - dc_yl;
    int x = dc_x & 3;
    byte *dest;
    byte *source = dc_source;
    lighttable_t *map = dc_colormap;
    fixed_t fracstep = dc_iscale;
    fixed_t frac = dc_texturemid + (dc_yl - centery) * fracstep;

    if (count++ < 0)
        return;

    count++;

    /* Record the span for this temp column. The first span recorded
     * into an empty slot starts a new group of four columns. */
    if (rt_ctspan[x] == rt_span[x])
        rt_pendstart = dc_x & ~3;
    rt_ctspan[x][0] = (unsigned short)dc_yl;
    rt_ctspan[x][1] = (unsigned short)dc_yh;
    rt_ctspan[x] += 2;

    dest = &rt_temp[x + 4 * dc_yl];

    do
    {
        *dest = map[source[(frac >> FRACBITS) & 255]];
        frac += fracstep;
        dest += 4;
    } while (--count);
}

//
// R_FillColumnHoriz
// Fills the current column with dc_color into the temporary buffer.
//
void R_FillColumnHoriz(void)
{
    int count = dc_yh - dc_yl;
    int x = dc_x & 3;
    byte color = dc_color;
    byte *dest;

    if (count++ < 0)
        return;

    count++;

    /* Record the span for this temp column. The first span recorded
     * into an empty slot starts a new group of four columns. */
    if (rt_ctspan[x] == rt_span[x])
        rt_pendstart = dc_x & ~3;
    rt_ctspan[x][0] = (unsigned short)dc_yl;
    rt_ctspan[x][1] = (unsigned short)dc_yh;
    rt_ctspan[x] += 2;

    dest = &rt_temp[x + 4 * dc_yl];

    do
    {
        *dest = color;
        dest += 4;
    } while (--count);
}

//
// Draws all spans at hx to the screen at sx.
//
void rt_Draw1col(int hx, int sx)
{
    while (rt_span[hx] < rt_ctspan[hx])
    {
        rt_post1(hx, sx, rt_span[hx][0], rt_span[hx][1]);
        rt_span[hx] += 2;
    }
}

//
// Adjusts two columns so that they both start on the same row.
// Returns false if it succeeded, true if a column ran out.
//
static int rt_nudgecols(int hx, int sx)
{
    if (rt_span[hx][0] < rt_span[hx + 1][0])
    {
spaghetti1:
        /* first column starts before the second; it might also
         * end before it */
        if (rt_span[hx][1] < rt_span[hx + 1][0])
        {
            while (rt_span[hx] < rt_ctspan[hx]
                   && rt_span[hx][1] < rt_span[hx + 1][0])
            {
                rt_post1(hx, sx, rt_span[hx][0], rt_span[hx][1]);
                rt_span[hx] += 2;
            }
            if (rt_span[hx] >= rt_ctspan[hx])
            {
                /* the first column ran out of spans */
                rt_Draw1col(hx + 1, sx + 1);
                return 1;
            }
            if (rt_span[hx][0] > rt_span[hx + 1][0])
                goto spaghetti2;	/* second starts before first now */
            else if (rt_span[hx][0] == rt_span[hx + 1][0])
                return 0;
        }
        rt_post1(hx, sx, rt_span[hx][0], rt_span[hx + 1][0] - 1);
        rt_span[hx][0] = rt_span[hx + 1][0];
    }
    if (rt_span[hx][0] > rt_span[hx + 1][0])
    {
spaghetti2:
        /* second column starts before the first; it might also
         * end before it */
        if (rt_span[hx + 1][1] < rt_span[hx][0])
        {
            while (rt_span[hx + 1] < rt_ctspan[hx + 1]
                   && rt_span[hx + 1][1] < rt_span[hx][0])
            {
                rt_post1(hx + 1, sx + 1,
                         rt_span[hx + 1][0], rt_span[hx + 1][1]);
                rt_span[hx + 1] += 2;
            }
            if (rt_span[hx + 1] >= rt_ctspan[hx + 1])
            {
                /* the second column ran out of spans */
                rt_Draw1col(hx, sx);
                return 1;
            }
            if (rt_span[hx][0] < rt_span[hx + 1][0])
                goto spaghetti1;	/* first starts before second now */
            else if (rt_span[hx][0] == rt_span[hx + 1][0])
                return 0;
        }
        rt_post1(hx + 1, sx + 1,
                 rt_span[hx + 1][0], rt_span[hx][0] - 1);
        rt_span[hx + 1][0] = rt_span[hx][0];
    }
    return 0;
}

//
// Copies all spans at hx and hx+1 to the screen at sx and sx+1.
// hx and sx should be word-aligned.
//
void rt_Draw2cols(int hx, int sx)
{
loop:
    if (rt_span[hx] >= rt_ctspan[hx])
    {
        /* no first column, do the second (if any) */
        rt_Draw1col(hx + 1, sx + 1);
        return;
    }
    if (rt_span[hx + 1] >= rt_ctspan[hx + 1])
    {
        /* no second column, we already know there is a first one */
        rt_Draw1col(hx, sx);
        return;
    }

    /* both columns have spans, align their tops */
    if (rt_nudgecols(hx, sx))
        return;

    /* now draw as much as possible as a series of words */
    if (rt_span[hx][1] < rt_span[hx + 1][1])
    {
        /* first column ends first, so draw down to its bottom */
        rt_post2(hx, sx, rt_span[hx][0], rt_span[hx][1]);
        rt_span[hx + 1][0] = rt_span[hx][1] + 1;
        rt_span[hx] += 2;
    }
    else
    {
        /* second column ends first, or they end at the same spot */
        rt_post2(hx, sx, rt_span[hx + 1][0], rt_span[hx + 1][1]);
        if (rt_span[hx][1] == rt_span[hx + 1][1])
        {
            rt_span[hx] += 2;
            rt_span[hx + 1] += 2;
        }
        else
        {
            rt_span[hx][0] = rt_span[hx + 1][1] + 1;
            rt_span[hx + 1] += 2;
        }
    }

    goto loop;	/* keep going until all columns have no more spans */
}

//
// Copies all spans in all four columns to the screen starting at sx.
// sx should be dword-aligned.
//
void rt_Draw4cols(int sx)
{
loop:
    if (rt_span[0] >= rt_ctspan[0])
    {
        /* no first column, do the second (if any) */
        rt_Draw1col(1, sx + 1);
        rt_Draw2cols(2, sx + 2);
        return;
    }
    if (rt_span[1] >= rt_ctspan[1])
    {
        /* no second column, we already know there is a first one */
        rt_Draw1col(0, sx);
        rt_Draw2cols(2, sx + 2);
        return;
    }
    if (rt_span[2] >= rt_ctspan[2])
    {
        /* no third column, do the fourth (if any) */
        rt_Draw2cols(0, sx);
        rt_Draw1col(3, sx + 3);
        return;
    }
    if (rt_span[3] >= rt_ctspan[3])
    {
        /* no fourth column, but there is a third */
        rt_Draw2cols(0, sx);
        rt_Draw1col(2, sx + 2);
        return;
    }

    /* if we get here, then we know all four columns have something,
     * make sure they all align at the top */
    if (rt_nudgecols(0, sx))
    {
        rt_Draw2cols(2, sx + 2);
        return;
    }
    if (rt_nudgecols(2, sx + 2))
    {
        rt_Draw2cols(0, sx);
        return;
    }

    /* first column is now aligned with second at top, and third is
     * aligned with fourth at top. now make sure both halves align
     * at the top and also have some shared space to their bottoms. */
    {
        unsigned int bot1 =
            rt_span[0][1] < rt_span[1][1] ? rt_span[0][1] : rt_span[1][1];
        unsigned int bot2 =
            rt_span[2][1] < rt_span[3][1] ? rt_span[2][1] : rt_span[3][1];

        if (rt_span[0][0] < rt_span[2][0])
        {
            /* first half starts before second half */
            if (bot1 >= rt_span[2][0])
            {
                /* first half ends after second begins */
                rt_post2(0, sx, rt_span[0][0], rt_span[2][0] - 1);
                rt_span[0][0] = rt_span[1][0] = rt_span[2][0];
            }
            else
            {
                /* first half ends before second begins */
                rt_post2(0, sx, rt_span[0][0], bot1);
                if (rt_span[0][1] == bot1)
                    rt_span[0] += 2;
                else
                    rt_span[0][0] = (unsigned short)(bot1 + 1);
                if (rt_span[1][1] == bot1)
                    rt_span[1] += 2;
                else
                    rt_span[1][0] = (unsigned short)(bot1 + 1);
                goto loop;	/* start over */
            }
        }
        else if (rt_span[0][0] > rt_span[2][0])
        {
            /* second half starts before the first */
            if (bot2 >= rt_span[0][0])
            {
                /* second half ends after first begins */
                rt_post2(2, sx + 2, rt_span[2][0], rt_span[0][0] - 1);
                rt_span[2][0] = rt_span[3][0] = rt_span[0][0];
            }
            else
            {
                /* second half ends before first begins */
                rt_post2(2, sx + 2, rt_span[2][0], bot2);
                if (rt_span[2][1] == bot2)
                    rt_span[2] += 2;
                else
                    rt_span[2][0] = (unsigned short)(bot2 + 1);
                if (rt_span[3][1] == bot2)
                    rt_span[3] += 2;
                else
                    rt_span[3][0] = (unsigned short)(bot2 + 1);
                goto loop;	/* start over */
            }
        }

        /* all four columns are now aligned at the top; draw all of
         * them until one ends. */
        bot1 = bot1 < bot2 ? bot1 : bot2;

        rt_post4(sx, rt_span[0][0], bot1);

        {
            int x;

            for (x = 3; x >= 0; x--)
                if (rt_span[x][1] == bot1)
                    rt_span[x] += 2;
                else
                    rt_span[x][0] = (unsigned short)(bot1 + 1);
        }
    }

    goto loop;	/* keep going until all columns have no more spans */
}

//
// R_HorizGroupFlush
// Flushes a complete group of four columns starting at x0 (x0 must
// be a multiple of 4). Call it as soon as the fourth column of the
// group has been expanded into the temp buffer, i.e. inside the
// per-column rendering loop when (x & 3) == 3.
//
void R_HorizGroupFlush(int x0)
{
    rt_Draw4cols(x0);
}

//
// R_HorizSegEnd
// Flushes the pending tail of the current group (the columns after
// the last complete group, up to xend). Call it at the end of a
// rendering range (segment loop, masked seg range).
//
void R_HorizSegEnd(int xend)
{
    int x;

    /* slots before the first drawn column of the group are empty,
     * so starting at rt_pendstart is always safe */
    for (x = rt_pendstart; x <= xend; x++)
        rt_Draw1col(x & 3, x);
}

//
// One-time initialization of the post functions.
//
void R_HorizInit(void)
{
    rt_post1 = rt_Copy1col;
    rt_post2 = rt_Copy2cols;
    rt_post4 = rt_Copy4cols;
}

#else /* !USE_BACKBUFFER && !MODE_VBE2_DIRECT */

//
// Stubs for video modes without an 8bpp linear frame buffer.
//
void R_DrawColumnHoriz(void)
{
}

void R_FillColumnHoriz(void)
{
}

void rt_InitCols(void)
{
}

void rt_Draw1col(int hx, int sx)
{
}

void rt_Draw2cols(int hx, int sx)
{
}

void rt_Draw4cols(int sx)
{
}

void R_HorizGroupFlush(int x0)
{
}

void R_HorizSegEnd(int xend)
{
}

void R_HorizInitFrame(void)
{
}

void R_HorizInit(void)
{
}

#endif /* USE_BACKBUFFER || MODE_VBE2_DIRECT */
