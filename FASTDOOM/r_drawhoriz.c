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
//	Sprite and wall rendering with the "horizontal column"
//	(two-pass, cache-friendly) method, inspired by
//	R_DrawColumnHoriz / R_DrawMaskedColumnHoriz from ZDoom
//	(r_columnmethod 1).
//
//	Instead of scaling one column and writing it to the framebuffer
//	immediately (walking down the screen with a full pitch stride,
//	which thrashes the cache on linear framebuffers), columns are
//	first scaled ("pre-pass") into a small interleaved temporary
//	buffer, horiztemp, four columns at a time: pixel y of column c
//	of the current group lives at horiztemp[(y << 2) + c], so a
//	whole screen row of the group sits in one dword.
//
//	While staging, the pre-pass also marks the screen row as
//	"active for column c" in horizrowmask. The post-pass then only
//	needs to test one byte per screen row to know which of the four
//	staged columns touch that row, and walks the framebuffer row by
//	row: up to four adjacent screen pixels are written per row,
//	keeping the framebuffer writes sequential.
//
//	Sprites use R_DrawVisSpriteHoriz / R_DrawVisPSpriteHoriz, which
//	replace the whole sprite column loop. Walls replace only the
//	colfunc itself (R_StageColumnHoriz), so the normal seg loop and
//	the masked seg ranges both go through the staging, and
//	R_FlushWallHoriz() writes the pending group at each drawing
//	boundary where a later seg could otherwise overwrite columns.
//	The textured sky uses the very same column interface through
//	skyfunc, so R_DrawSky stages its columns the same way (the
//	full-direct 1:1 sky table and the flat sky keep their classic
//	drawers).
//
//	The staging works with any linear byte-per-pixel destination
//	and a constant row stride, i.e. all the backbuffered modes
//	(ylookup/columnofs) and the VBE2 direct modes (destview). The
//	Low/Potato horizontal pixel duplication is handled by the
//	post-pass, the CGA composite modes' column decimation by the
//	sprite pre-pass. Fuzz (shadow/translucent) sprites keep using
//	the classic mode-specific column drawers.
//
//	Source texture addressing matches the column drawers in
//	linearh/linearl/linearp.asm and vbe2d*.asm: the source byte
//	index is (texturepos >> 16) & 127, i.e. composite columns and
//	patch posts wrap inside a 128 byte window (the asm computes it
//	as (frac << 9) >> 25).
//
//	Performance notes (what the current shape buys):
//
//	  * Every staged span is a contiguous run of screen rows, so
//	    the row mask is built R_MarkRows()-style, one dword
//	    read-modify-write per four rows, instead of the per-pixel
//	    "mask |= bit" it used to do (that was three memory ops per
//	    pixel, i.e. as much traffic as the pixel itself).
//	  * The staging loops are unwound four screen rows at a time:
//	    one pointer add, one loop test and one texture position
//	    add (through the s1..s3/s4 offsets) per four pixels, and
//	    the four staged bytes go to fixed 4-byte strides, which is
//	    cheaper than packing them into a dword for a single
//	    read-modify-write store.
//	  * Columns whose iscale is zero (one texture pixel for the
//	    whole column, common for distant sprites and walls) skip
//	    the per-row lookups entirely and fill the staging buffer
//	    with a replicated dword.
//	  * The colormap is applied while staging, for walls *and* for
//	    sprites, so everything the post-pass moves is already
//	    screen colors. That is what lets the post-pass be a plain
//	    copy (the assembly wide path skips the table lookups for a
//	    NULL colormap, and R_FlushHorizGroupFast() below is a pure
//	    4-pixel-per-row copy loop).
//	  * The post-pass has a C fast path (R_FlushHorizGroupFast)
//	    for the common full four-column group: it walks the
//	    framebuffer with a constant row stride (no ylookup load per
//	    row, no per-row wide-path tests) and writes the four pixels
//	    of a row with one dword store at pixel width 1 (two at
//	    width 2, four at width 4). Rows where only some of the four
//	    columns are active (wall top/bottom slopes, sprite edges,
//	    decimated CGA columns) are written per column from the same
//	    loop. Anything the fast path does not cover (partial
//	    groups) goes to the assembly R_FlushHorizGroupAsm.
//
//	The assembly post-pass (linearhz.asm) stays the fallback and
//	shares the staging state with this file (_horiztemp32,
//	_horizrowmask, _horizymin, _horizymax); the pre-passes stay in
//	C, and R_StageColumnHoriz is shaped like the asm colfuncs (no
//	arguments, dc_* globals) so it can be replaced one-for-one by
//	an asm version.
//

#include <stdlib.h>

#include "options.h"

#if defined(SUPPORTS_HORIZ_RENDER)

#include "doomdef.h"
#include "doomstat.h"

#include "i_system.h"
#include "i_debug.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"
#include "r_things.h"
#include "r_drawhoriz.h"

#include "std_func.h"

// Interleaved column staging buffer: pixel y of column c of the
// current group lives at HORIZTEMPB[(y << 2) + c]. Four columns per
// screen row keep the post-pass writes to the framebuffer
// sequential along each row, and one whole screen row of the group
// fits in a single dword: the buffer is declared as unsigned so
// horiztemp32[y] is an aligned 32-bit load of the four staged
// pixels, used by the wide stores in the post-pass.
// The staging buffers and the row extent are shared with the
// assembly post-pass (linearhz.asm), so they are plain globals
// (_horiztemp32, _horizrowmask, _horizymin, _horizymax there).
unsigned horiztemp32[SCREENHEIGHT];
#define HORIZTEMPB ((byte *)horiztemp32)

// Bit c on row y: column c of the current group has a staged,
// visible pixel on screen row y. Built by the pre-pass per staged
// span (contiguous run of rows), consumed and cleared by the
// post-pass. This replaces per-column span lists: the flush needs a
// single byte load per screen row instead of chasing span pointers
// for each column.
// Declared as dwords (same address as the byte array the assembly
// uses) so a whole run of four screen rows can be marked with one
// dword write, which is what keeps the marking off the per-pixel
// path.
unsigned horizrowmask[(SCREENHEIGHT + 3) / 4];
#define HORIZMASKB ((byte *)horizrowmask)

// Vertical extent of the rows that can have mask bits set for the
// current group, so the flush skips the blank rows above and below
// it entirely.
int horizymin;
int horizymax;

// Base x of the wall group currently staged into horiztemp, or -1
// when nothing is pending. Walls stage one colfunc() column at a
// time; the group is flushed when the 4-column group changes or
// when the renderer switches context (per seg / per masked range).
static int wallgroupx = -1;

// Dword constants for replicating a byte over a whole dword, and
// for masking the low n bytes of a dword (n in 1..4). The spans are
// never empty, so n == 0 is not needed (and would shift by 32).
#define BYTEPAT4(v) ((unsigned)(v) * 0x01010101u)
#define LOWBYTES(n) (0xffffffffu >> ((4u - (unsigned)(n)) << 3))

//
// Some composite CGA modes only draw a decimated subset of the
// sprite columns in certain detail levels. Mirror what the classic
// R_DrawVisSprite does, so both renderers look identical.
//
#if defined(MODE_CGA16) || defined(MODE_CVB)
#define HORIZ_SKIP_MASK ((detailshift == DETAIL_HIGH) ? 1 : 0)
#elif defined(MODE_CGA512)
#define HORIZ_SKIP_MASK                                                      \
    ((detailshift == DETAIL_HIGH                                               \
          ? 3                                                                  \
          : (detailshift == DETAIL_LOW ? 1 : 0)))
#else
#define HORIZ_SKIP_MASK 0
#endif

//
// Start a fresh 4-column group. The row masks of the previous
// group are guaranteed already cleared by the flush.
//
static void R_BeginHorizGroup(void)
{
    horizymin = SCREENHEIGHT;
    horizymax = -1;
}

//
// Mark screen rows yl..yh as active for column c of the current
// group. yl..yh is a contiguous run, so the whole span is done with
// dword read-modify-writes (one per four rows) on the aligned dword
// containing yl, with the head and tail bytes masked out so rows
// outside the span are never touched.
//
static void R_MarkRows(int yl, int yh, unsigned bitmask)
{
    unsigned shift = (unsigned)yl & 3;
    unsigned n = (unsigned)(yh - yl) + 1;
    unsigned *p = horizrowmask + ((unsigned)yl >> 2);
    unsigned pat = BYTEPAT4(bitmask);

    if (n + shift <= 4)
    {
        // The span fits in the dword it starts in.
        *p |= (pat & LOWBYTES(n)) << (shift << 3);
        return;
    }

    *p |= pat << (shift << 3); // head: rows yl..end of the dword
    n -= 4 - shift;
    p++;

    while (n >= 4)
    {
        *p |= pat;
        p++;
        n -= 4;
    }

    if (n)
        *p |= pat & LOWBYTES(n); // tail: the remaining 1..3 rows
}

//
// Stage one contiguous run of texture pixels, screen rows yl..yh of
// column c of the current group.
//
// source/frac/step/colormap are the dc_source / texture position at
// row yl / dc_iscale / dc_colormap of the classic column drawers,
// with the same 128 byte source window wrap. The bytes are stored
// already translated through colormap, so the post-pass only has to
// move them.
//
static void R_StageSpan(const byte *source, fixed_t frac, fixed_t step,
                        const byte *colormap, int yl, int yh, int c)
{
    unsigned count = (unsigned)(yh - yl) + 1;
    byte *dest = HORIZTEMPB + ((unsigned)yl << 2) + c;

    if (!step)
    {
        // One texture pixel for the whole span: every row of column c
        // gets the same color. Stored as plain byte writes: the
        // staging lanes are not cleared between groups (the flush
        // only clears the row mask), so the value must not be merged
        // into a staged dword with |=.
        byte col = colormap[source[((unsigned)frac >> 16) & 127]];
        int head = (int)count & 3;

        while (head--)
        {
            *dest = col;
            dest += 4;
        }

        count &= ~3u;
        while (count)
        {
            dest[0] = dest[4] = dest[8] = dest[12] = col;
            dest += 16;
            count -= 4;
        }
    }
    else
    {
        fixed_t s1 = step;
        fixed_t s2 = step + step;
        fixed_t s3 = s2 + step;
        fixed_t s4 = s3 + step;
        int head = (int)count & 3;

        // Rows up to a multiple of four, then unwound four at a
        // time: one pointer add, one position add and one loop test
        // per four pixels.
        while (head--)
        {
            *dest = colormap[source[((unsigned)frac >> 16) & 127]];
            frac += step;
            dest += 4;
        }

        count &= ~3u;
        while (count)
        {
            dest[0] = colormap[source[((unsigned)frac >> 16) & 127]];
            dest[4] = colormap[source[((unsigned)frac + (unsigned)s1) >> 16
                                      & 127]];
            dest[8] = colormap[source[((unsigned)frac + (unsigned)s2) >> 16
                                      & 127]];
            dest[12] = colormap[source[((unsigned)frac + (unsigned)s3) >> 16
                                       & 127]];
            frac += s4;
            dest += 16;
            count -= 4;
        }
    }

    R_MarkRows(yl, yh, 1u << c);

    if (yl < horizymin)
        horizymin = yl;
    if (yh > horizymax)
        horizymax = yh;
}

// ---------------------------------------------------------------------------
// Post-pass
// ---------------------------------------------------------------------------
//
// C fast path for the case that dominates wall, sky and sprite
// drawing: a complete four-column group whose staged bytes are
// already screen colors (the colormap is applied while staging).
//
// The row loop walks the framebuffer with a constant stride, so
// there is no ylookup load, no hz_fast/hz_raw test and no colormap
// lookup per row: one mask load plus one dword store per row at
// pixel width 1, when all four columns are active on that row
// (mask == 15, the usual case inside a wall). The rows where only
// some of the columns are active (wall top/bottom slopes, sprite
// edges, decimated CGA columns) are written column by column from
// the same loop.
//
// Returns 0 when the group is not covered by the fast path (fewer
// than four columns, i.e. the group at the view edge), in which
// case nothing is written and the caller uses the assembly
// post-pass. As the assembly requires, every consumed mask byte is
// cleared.
//
static int R_FlushHorizGroupFast(int xstart, int group)
{
    int y = horizymin;
    int yend = horizymax;
    byte *p;
    byte *mask = HORIZMASKB;
    unsigned *temp = horiztemp32;

    if (y > yend)
        return 1; // nothing staged

    if (group != 4)
        return 0;

#if defined(USE_BACKBUFFER)
    p = ylookup[y] + columnofs[xstart];
#elif defined(MODE_VBE2_DIRECT)
    p = (byte *)destview + MulScreenWidth(y) + (xstart << detailshift);
#endif

    if (!detailshift)
    {
        for (; y <= yend; y++, p += SCREENWIDTH)
        {
            unsigned v = temp[y];
            unsigned m = mask[y];
            mask[y] = 0;

            if (m == 15)
                *(unsigned *)p = v;
            else if (m)
            {
                byte *t = HORIZTEMPB + ((unsigned)y << 2);
                if (m & 1)
                    p[0] = t[0];
                if (m & 2)
                    p[1] = t[1];
                if (m & 4)
                    p[2] = t[2];
                if (m & 8)
                    p[3] = t[3];
            }
        }
    }
    else if (detailshift == 1)
    {
        // Two framebuffer bytes per staged pixel.
        for (; y <= yend; y++, p += SCREENWIDTH)
        {
            unsigned v = temp[y];
            unsigned m = mask[y];
            mask[y] = 0;

            if (m == 15)
            {
                unsigned a = (v & 0xffu) * 0x0101u;
                unsigned b = ((v >> 8) & 0xffu) * 0x0101u;
                *(unsigned *)p = a | (b << 16);
                a = ((v >> 16) & 0xffu) * 0x0101u;
                b = (v >> 24) * 0x0101u;
                *(unsigned *)(p + 4) = a | (b << 16);
            }
            else if (m)
            {
                byte *t = HORIZTEMPB + ((unsigned)y << 2);
                if (m & 1)
                {
                    p[0] = t[0];
                    p[1] = t[0];
                }
                if (m & 2)
                {
                    p[2] = t[1];
                    p[3] = t[1];
                }
                if (m & 4)
                {
                    p[4] = t[2];
                    p[5] = t[2];
                }
                if (m & 8)
                {
                    p[6] = t[3];
                    p[7] = t[3];
                }
            }
        }
    }
    else
    {
        // Four framebuffer bytes per staged pixel.
        for (; y <= yend; y++, p += SCREENWIDTH)
        {
            unsigned v = temp[y];
            unsigned m = mask[y];
            mask[y] = 0;

            if (m == 15)
            {
                *(unsigned *)p = BYTEPAT4(v & 0xffu);
                *(unsigned *)(p + 4) = BYTEPAT4((v >> 8) & 0xffu);
                *(unsigned *)(p + 8) = BYTEPAT4((v >> 16) & 0xffu);
                *(unsigned *)(p + 12) = BYTEPAT4(v >> 24);
            }
            else if (m)
            {
                byte *t = HORIZTEMPB + ((unsigned)y << 2);
                if (m & 1)
                {
                    p[0] = p[1] = p[2] = p[3] = t[0];
                }
                if (m & 2)
                {
                    p[4] = p[5] = p[6] = p[7] = t[1];
                }
                if (m & 4)
                {
                    p[8] = p[9] = p[10] = p[11] = t[2];
                }
                if (m & 8)
                {
                    p[12] = p[13] = p[14] = p[15] = t[3];
                }
            }
        }
    }

    return 1;
}

//
// Write out the staged group: C fast path when it applies, assembly
// post-pass otherwise.
//
static void R_FlushHorizGroup(int xstart, int group)
{
    if (!R_FlushHorizGroupFast(xstart, group))
        R_FlushHorizGroupAsm(xstart, group, NULL);
}

//
// Pre-pass for the sprite drawers. The post coordinates and stepping
// come from the spr*/dc_* globals set up by the caller, like the asm
// colfuncs. Clipping matches the classic R_DrawVisSprite exactly.
//
//
// The stepping comes from the dc_iscale/dc_texturemid/spryscale/
// sprtopscreen/dc_colormap globals the caller sets up, exactly like
// the asm colfuncs, so only the column identity has to be passed.
//
static void R_StageSpriteColumn(patch_t *patch, byte *patchbase, int x, int c,
                                fixed_t frac)
{
    fixed_t iscale = dc_iscale;
    const byte *colormap = dc_colormap;
    column_t *column;
    int topscreen;
    int yl;
    int yh;
    short mfc_x;
    short mcc_x;

    mfc_x = mfloorclip[x];
    mcc_x = mceilingclip[x];

    column = (column_t *)(patchbase + patch->columnofs[frac >> FRACBITS]);

    while (column->topdelta != 0xff)
    {
        // calculate unclipped screen coordinates for the post
        topscreen = sprtopscreen + spryscale * column->topdelta;
        yh = (topscreen + spryscale * column->length - 1) >> FRACBITS;

        if (yh >= mfc_x)
            yh = mfc_x - 1;

        if (yh >= viewheight)
        {
            column = (column_t *)((byte *)column + column->length + 4);
            continue;
        }

        yl = (topscreen + FRACUNIT - 1) >> FRACBITS;

        if (yl <= mcc_x)
            yl = mcc_x + 1;

        if (yl > yh)
        {
            column = (column_t *)((byte *)column + column->length + 4);
            continue;
        }

        BOUNDS_CHECK(yh, yl);

        R_StageSpan((byte *)column + 3,
                    dc_texturemid - ((fixed_t)column->topdelta << FRACBITS) +
                        (yl - centery) * iscale,
                    iscale, colormap, yl, yh, c);

        column = (column_t *)((byte *)column + column->length + 4);
    }
}

//
// Shared core for the thing and player sprite horizontal drawers.
// fuzzfallback handles shadow/translucent sprites (NULL colormap),
// which keep using the classic mode-specific fuzz columns.
//
static void R_DrawVisSpriteHorizCore(vissprite_t *vis,
                                     void (*fuzzfallback)(vissprite_t *))
{
    patch_t *patch;
    byte *patchbase;
    const byte *colormap;
    fixed_t frac;
    fixed_t xiscale;
    int x;
    int x2;
    int base;
    int group;
    int c;
    int skipmask;

    colormap = vis->colormap;

    if (!colormap)
    {
        // NULL colormap = shadow/fuzz draw
        fuzzfallback(vis);
        return;
    }

    // Sprite staging shares horiztemp/horizrowmask with the wall
    // staging, so make sure a pending wall/sky group is on screen
    // before reusing them. No-op when nothing is pending.
    R_FlushWallHoriz();

    patch = W_CacheLumpNum(vis->patch + firstspritelump, PU_CACHE);
    patchbase = (byte *)patch;

    dc_colormap = (byte *)colormap;
    dc_iscale = abs(vis->xiscale) >> detailshift;
    dc_texturemid = vis->texturemid;
    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);

    frac = vis->startfrac;
    xiscale = vis->xiscale;
    x = vis->x1;
    x2 = vis->x2;

    // Composite CGA modes decimate the sprite columns; hoisted out
    // of the column loop (0 in every other mode, so the test
    // disappears).
    skipmask = HORIZ_SKIP_MASK;

    // Same do-while shape as the classic drawers: even a sprite with
    // x1 > x2 draws its first column.
    do
    {
        group = x2 - x + 1;
        if (group > 4)
            group = 4;

        base = x;
        R_BeginHorizGroup();

        for (c = 0; c < group; c++)
        {
            if (!(skipmask & x))
                R_StageSpriteColumn(patch, patchbase, x, c, frac);

            frac += xiscale;
            x++;
        }

        R_FlushHorizGroup(base, group);
    } while (x <= x2);
}

void R_DrawVisSpriteHoriz(vissprite_t *vis)
{
    R_DrawVisSpriteHorizCore(vis, R_DrawVisSprite);
}

void R_DrawVisPSpriteHoriz(vissprite_t *vis)
{
    R_DrawVisSpriteHorizCore(vis, R_DrawVisPSprite);
}

//
// ---------------------------------------------------------------------------
// Horizontal wall columns
// ---------------------------------------------------------------------------
//
// A drop-in replacement for the wall colfunc: instead of writing the
// scaled column straight to the framebuffer, it stages the column
// into the current 4-column group of horiztemp. The group is flushed
// to the screen row-major, which turns four adjacent wall columns
// into sequential framebuffer writes.
//
// Both the solid tiers of R_RenderSegLoop and the posts of
// R_RenderMaskedSegRange use the same dc_* interface and the same
// 128-byte-window source addressing as the classic column drawers,
// so a single staging function covers all wall drawing (and the
// textured sky, which goes through skyfunc).
//
// Ordering: columns within a seg never overlap, and a later seg may
// overwrite columns of an earlier one, so the pending group must not
// survive a seg boundary. The render loop calls R_FlushWallHoriz()
// after every renderSegLoop / renderMaskedSegRange invocation to
// guarantee that.

void R_StageColumnHoriz(void)
{
    int x = dc_x;
    int base = x & ~3;
    int yl = dc_yl;
    int yh = dc_yh;

    if (wallgroupx != base)
    {
        // The previous group (or a finished sprite group) must not
        // leak into the new group.
        R_FlushWallHoriz();
        R_BeginHorizGroup();
        wallgroupx = base;
    }

    if (yl <= yh)
    {
        // Same texture stepping as the asm column drawers: the
        // source index wraps inside a 128 byte window.
        R_StageSpan(dc_source, dc_texturemid + (yl - centery) * dc_iscale,
                    dc_iscale, dc_colormap, yl, yh, x & 3);
    }
}

//
// Write out any pending wall columns. Cheap no-op when nothing is
// staged, so it is safe to call at every wall drawing boundary.
//
void R_FlushWallHoriz(void)
{
    int group;

    if (wallgroupx < 0)
        return;

    // Don't touch columns past the view edge (near/full screen
    // view: a group can extend beyond the last drawn column).
    group = viewwidth - wallgroupx;
    if (group > 4)
        group = 4;

    if (group > 0)
        R_FlushHorizGroup(wallgroupx, group);

    wallgroupx = -1;
}

#endif
