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
//	This generic C implementation works on any linear byte-per
//	-pixel destination with a constant row stride, i.e. all the
//	backbuffered modes (ylookup/columnofs) and the VBE2 direct
//	modes (destview). It handles the Low/Potato horizontal pixel
//	duplication and the CGA composite modes' column decimation.
//	Fuzz (shadow/translucent) sprites keep using the classic
//	mode-specific column drawers.
//
//	Source texture addressing matches the column drawers in
//	linearh/linearl/linearp.asm and vbe2d*.asm: the source byte
//	index is (texturepos >> 16) & 127, i.e. composite columns and
//	patch posts wrap inside a 128 byte window (the asm computes it
//	as (frac << 9) >> 25).
//
//	An optimized assembly post-pass can replace R_FlushHorizGroup
//	later without changing anything else.
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
static unsigned horiztemp32[SCREENHEIGHT];
#define HORIZTEMPB ((byte *)horiztemp32)

// Bit c set on row y: column c of the current group has a staged,
// visible pixel on screen row y. Built by the pre-pass while it
// scales pixels (one cached byte OR per pixel), consumed and
// cleared by the post-pass. This replaces per-column span lists:
// the flush needs a single byte load per screen row instead of
// chasing span pointers for each column.
static byte horizrowmask[SCREENHEIGHT];

// Vertical extent of the rows that can have mask bits set for the
// current group, so the flush skips the blank rows above and below
// it entirely.
static int horizymin;
static int horizymax;

// Base x of the wall group currently staged into horiztemp, or -1
// when nothing is pending. Walls stage one colfunc() column at a
// time; the group is flushed when the 4-column group changes or
// when the renderer switches context (per seg / per masked range).
static int wallgroupx = -1;

// Walls apply dc_colormap during the pre-pass (lighting can differ
// between the columns of a group), so their post-pass is a plain
// copy. The identity "colormap" lets the flush loop stay shared
// with the sprites, which map during the post-pass instead.
static byte horizidentity[256];
static int horizidentityready;

//
// Screen addressing. Backbuffered modes draw through ylookup[]
// (row pointer, viewwindowy baked in) plus columnofs[] (includes
// viewwindowx and the detailshift pixel doubling). VBE2 direct
// modes draw straight into the framebuffer at destview, which
// already includes the viewwindow offset. Both are linear with a
// constant row stride of MulScreenWidth(1), so the flush walks the
// destination with an incrementing row pointer instead of
// re-evaluating ylookup/MulScreenWidth for every row.
//
#if defined(USE_BACKBUFFER)
#define HORIZ_ROW(y) (ylookup[(y)])
#define HORIZ_COFS(x) (columnofs[(x)])
#else
#define HORIZ_ROW(y) (destview + MulScreenWidth(y))
#define HORIZ_COFS(x) ((x) << detailshift)
#endif

#define HORIZ_ROWSTRIDE MulScreenWidth(1)

//
// Some composite CGA modes only draw a decimated subset of the
// sprite columns in certain detail levels. Mirror what the classic
// R_DrawVisSprite does, so both renderers look identical.
//
#if defined(MODE_CGA16) || defined(MODE_CVB)
#define HORIZ_SKIP_COLUMN(x) ((detailshift == DETAIL_HIGH) && ((x) & 1))
#elif defined(MODE_CGA512)
#define HORIZ_SKIP_COLUMN(x)                                                   \
    ((detailshift == DETAIL_HIGH && ((x) & 3)) ||                              \
     (detailshift == DETAIL_LOW && ((x) & 1)))
#else
#define HORIZ_SKIP_COLUMN(x) (0)
#endif

//
// Start a fresh 4-column group. The row masks of the previous
// group are guaranteed already cleared by R_FlushHorizGroup.
//
static void R_BeginHorizGroup(void)
{
    horizymin = SCREENHEIGHT;
    horizymax = -1;
}

//
// Post-pass: map the up-to-four staged columns in horiztemp to the
// screen at xstart..., writing row by row so that adjacent columns
// land next to each other in memory. colormap translates the
// staged texture bytes; NULL means the bytes are already screen
// colors (wall staging maps with the per-column colormap).
//
// One loop per pixel width (1, 2 or 4 bytes). A row whose mask
// has all four columns active (m == fastmask) is written with
// wide 32-bit stores from a single staged-dword load; every other
// mask goes through the generic do/while, which stops as soon as
// the shifted mask runs out of bits. fastmask is 0 when the group
// offsets are not contiguous, and since m is never 0 inside the
// mask test that simply disables the wide path.
//
static void R_FlushHorizGroup(int xstart, int group, byte *colormap)
{
    int c;
    int y;
    int ymax;
    int off[4];
    unsigned m;
    unsigned t;
    unsigned d;
    unsigned fastmask;
    byte *map;
    byte *row;
    byte *tp;

    if (horizymax < horizymin)
        return;

    if (colormap)
    {
        map = colormap;
    }
    else
    {
        if (!horizidentityready)
        {
            int i;
            for (i = 0; i < 256; i++)
                horizidentity[i] = i;
            horizidentityready = 1;
        }
        map = horizidentity;
    }

    for (c = 0; c < group; c++)
        off[c] = HORIZ_COFS(xstart + c);

    // Can whole rows be written with wide 32-bit stores? That
    // needs a full 4-column group whose destination offsets
    // advance by exactly one (duplicated) pixel per column;
    // decimated CGA composite groups have gaps and partial
    // groups never reach four columns. Destination alignment is
    // not guaranteed (viewwindowx is arbitrary), but unaligned
    // dword access is legal on every CPU this renderer targets
    // and still cheaper than several separate stores.
    fastmask = 0;
    if (group == 4)
    {
        unsigned step = 1 << detailshift;

        if (off[1] == off[0] + step && off[2] == off[1] + step &&
            off[3] == off[2] + step)
            fastmask = 15;
    }

    y = horizymin;
    ymax = horizymax;
    row = HORIZ_ROW(y);

    if (!detailshift)
    {
        for (; y <= ymax; y++)
        {
            m = horizrowmask[y];

            if (m)
            {
                if (m == fastmask)
                {
                    // All four columns active: one aligned
                    // 32-bit load of the staged row and one
                    // 32-bit store.
                    t = horiztemp32[y];

                    if (map == horizidentity)
                        // Wall/sky: the staged bytes are
                        // already screen colors, no lookup.
                        *(unsigned *)(row + off[0]) = t;
                    else
                    {
                        d = map[t & 255];
                        d |= map[(t >> 8) & 255] << 8;
                        d |= map[(t >> 16) & 255] << 16;
                        d |= map[t >> 24] << 24;
                        *(unsigned *)(row + off[0]) = d;
                    }
                }
                else
                {
                    tp = HORIZTEMPB + ((unsigned)y << 2);
                    c = 0;
                    do
                    {
                        if (m & 1)
                            row[off[c]] = map[tp[c]];
                        m >>= 1;
                        c++;
                    } while (m);
                }

                horizrowmask[y] = 0;
            }

            row += HORIZ_ROWSTRIDE;
        }
    }
    else if (detailshift == DETAIL_LOW)
    {
        for (; y <= ymax; y++)
        {
            m = horizrowmask[y];

            if (m)
            {
                if (m == fastmask)
                {
                    // All four columns active: two 32-bit
                    // stores, each holding two duplicated
                    // pixels.
                    unsigned d2;

                    t = horiztemp32[y];

                    d = map[t & 255];
                    d |= d << 8;
                    d2 = map[(t >> 8) & 255];
                    d2 |= d2 << 8;
                    d |= d2 << 16;

                    d2 = map[(t >> 16) & 255];
                    d2 |= d2 << 8;
                    t = map[t >> 24];
                    t |= t << 8;
                    d2 |= t << 16;

                    *(unsigned *)(row + off[0]) = d;
                    *(unsigned *)(row + off[0] + 4) = d2;
                }
                else
                {
                    tp = HORIZTEMPB + ((unsigned)y << 2);
                    c = 0;
                    do
                    {
                        if (m & 1)
                        {
                            unsigned short color = map[tp[c]];
                            color |= color << 8;
                            *(unsigned short *)(row + off[c]) = color;
                        }
                        m >>= 1;
                        c++;
                    } while (m);
                }

                horizrowmask[y] = 0;
            }

            row += HORIZ_ROWSTRIDE;
        }
    }
    else
    {
        // Four bytes per pixel: the per-pixel stores are already
        // dwords; the wide path only drops the per-column tests
        // and writes from a single staged-dword load.
        for (; y <= ymax; y++)
        {
            m = horizrowmask[y];

            if (m)
            {
                if (m == fastmask)
                {
                    t = horiztemp32[y];

                    d = map[t & 255];
                    d |= d << 8;
                    d |= d << 16;
                    *(unsigned *)(row + off[0]) = d;

                    d = map[(t >> 8) & 255];
                    d |= d << 8;
                    d |= d << 16;
                    *(unsigned *)(row + off[0] + 4) = d;

                    d = map[(t >> 16) & 255];
                    d |= d << 8;
                    d |= d << 16;
                    *(unsigned *)(row + off[0] + 8) = d;

                    d = map[t >> 24];
                    d |= d << 8;
                    d |= d << 16;
                    *(unsigned *)(row + off[0] + 12) = d;
                }
                else
                {
                    tp = HORIZTEMPB + ((unsigned)y << 2);
                    c = 0;
                    do
                    {
                        if (m & 1)
                        {
                            d = map[tp[c]];
                            d |= d << 8;
                            d |= d << 16;
                            *(unsigned *)(row + off[c]) = d;
                        }
                        m >>= 1;
                        c++;
                    } while (m);
                }

                horizrowmask[y] = 0;
            }

            row += HORIZ_ROWSTRIDE;
        }
    }
}

//
// Pre-pass: scale one texture column of the sprite into horiztemp
// (slot c of the current group), marking the touched screen rows.
// Clipping matches the classic R_DrawVisSprite exactly.
//
static void R_DrawColumnHoriz(vissprite_t *vis, patch_t *patch, int x,
                              int c, fixed_t frac)
{
    column_t *column;
    int texturecolumn;
    int topscreen;
    int bottomscreen;
    int yl;
    int yh;
    int count;
    short mfc_x;
    short mcc_x;
    fixed_t basetexturemid;
    fixed_t texfrac;
    fixed_t fracstep;
    byte *source;
    byte *dest;
    byte *rmask;
    unsigned bitmask;

    bitmask = 1 << c;
    fracstep = dc_iscale;
    basetexturemid = dc_texturemid;
    mfc_x = mfloorclip[x];
    mcc_x = mceilingclip[x];

    texturecolumn = frac >> FRACBITS;
    column = (column_t *)((byte *)patch + patch->columnofs[texturecolumn]);

    while (column->topdelta != 0xff)
    {
        // calculate unclipped screen coordinates for the post
        topscreen = sprtopscreen + spryscale * column->topdelta;
        bottomscreen = topscreen + spryscale * column->length;

        yh = (bottomscreen - 1) >> FRACBITS;

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

        count = yh - yl + 1;
        source = (byte *)column + 3;
        texfrac = dc_texturemid - (column->topdelta << FRACBITS) +
                  (yl - centery) * fracstep;

        dest = HORIZTEMPB + ((unsigned)yl << 2) + c;
        rmask = horizrowmask + yl;

        while (count--)
        {
            *dest = source[((unsigned)texfrac >> 16) & 127];
            *rmask |= bitmask;
            texfrac += fracstep;
            dest += 4;
            rmask++;
        }

        if (yl < horizymin)
            horizymin = yl;
        if (yh > horizymax)
            horizymax = yh;

        column = (column_t *)((byte *)column + column->length + 4);
    }

    dc_texturemid = basetexturemid;
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
    fixed_t frac;
    fixed_t xiscale;
    int x;
    int x2;
    int group;
    int c;

    if (!vis->colormap)
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

    dc_colormap = vis->colormap;
    dc_iscale = abs(vis->xiscale) >> detailshift;
    dc_texturemid = vis->texturemid;
    frac = vis->startfrac;
    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);

    xiscale = vis->xiscale;
    x2 = vis->x2;
    x = vis->x1;

    // Same do-while shape as the classic drawers: even a sprite with
    // x1 > x2 draws its first column.
    do
    {
        group = x2 - x + 1;
        if (group > 4)
            group = 4;

        R_BeginHorizGroup();

        for (c = 0; c < group; c++)
        {
            if (!HORIZ_SKIP_COLUMN(x + c))
                R_DrawColumnHoriz(vis, patch, x + c, c, frac);

            frac += xiscale;
        }

        R_FlushHorizGroup(x, group, dc_colormap);

        x += group;
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
// A drop-in replacement for the wall colfunc: instead of writing
// the scaled column straight to the framebuffer, it stages the
// column into the current 4-column group of horiztemp (already
// translated through dc_colormap, since each column of a group can
// have a different light level). The group is flushed to the
// screen row-major, which turns four adjacent wall columns into
// sequential framebuffer writes.
//
// Both the solid tiers of R_RenderSegLoop and the posts of
// R_RenderMaskedSegRange use the same dc_* interface and the same
// 128-byte-window source addressing as the classic column drawers,
// so a single staging function covers all wall drawing.
//
// Ordering: columns within a seg never overlap, and a later seg
// may overwrite columns of an earlier one, so the pending group
// must not survive a seg boundary. The render loop calls
// R_FlushWallHoriz() after every renderSegLoop /
// renderMaskedSegRange invocation to guarantee that.

void R_StageColumnHoriz(void)
{
    int x;
    int base;
    int c;
    int yl;
    int yh;
    int count;
    unsigned bitmask;
    byte *dest;
    byte *source;
    byte *colormap;
    byte *rmask;
    fixed_t frac;
    fixed_t fracstep;

    x = dc_x;
    base = x & ~3;

    if (wallgroupx != base)
    {
        // The previous group (or a finished sprite group) must not
        // leak into the new group.
        R_FlushWallHoriz();
        R_BeginHorizGroup();
        wallgroupx = base;
    }

    yl = dc_yl;
    yh = dc_yh;

    if (yl > yh)
        return;

    count = yh - yl + 1;
    c = x & 3;
    bitmask = 1 << c;

    // Same texture stepping as the asm column drawers: the source
    // index wraps inside a 128 byte window.
    colormap = dc_colormap;
    source = dc_source;
    fracstep = dc_iscale;
    frac = dc_texturemid + (yl - centery) * fracstep;

    dest = HORIZTEMPB + ((unsigned)yl << 2) + c;
    rmask = horizrowmask + yl;

    while (count--)
    {
        *dest = colormap[source[((unsigned)frac >> 16) & 127]];
        *rmask |= bitmask;
        frac += fracstep;
        dest += 4;
        rmask++;
    }

    if (yl < horizymin)
        horizymin = yl;
    if (yh > horizymax)
        horizymax = yh;
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
        R_FlushHorizGroup(wallgroupx, group, NULL);

    wallgroupx = -1;
}

#endif
