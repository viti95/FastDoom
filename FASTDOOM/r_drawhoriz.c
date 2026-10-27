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
// immediately (walking down the screen with a full pitch stride,
// which thrashes the cache on linear framebuffers), columns are
// first scaled ("pre-pass") into a small interleaved temporary
// buffer, horiztemp, four columns at a time. Each visible span is
// remembered, then the whole group is flushed to the framebuffer
// at once ("post-pass"), writing row by row: up to four adjacent
// screen pixels are written on each screen row, keeping the
// framebuffer writes sequential.
//
//	Sprites use R_DrawVisSpriteHoriz / R_DrawVisPSpriteHoriz, which
// replace the whole sprite column loop. Walls replace only the
// colfunc itself (R_StageColumnHoriz), so the normal seg loop and
// the masked seg ranges both go through the staging, and
// R_FlushWallHoriz() writes the pending group at each drawing
// boundary where a later seg could otherwise overwrite columns.
//
//	This generic C implementation works on any linear byte-per
// -pixel destination with a SCREENWIDTH row stride, i.e. all the
//	backbuffered modes (ylookup/columnofs) and the VBE2 direct
//	modes (destview). It handles the Low/Potato horizontal pixel
//	duplication and the CGA composite modes' column decimation.
//	Fuzz (shadow/translucent) sprites keep using the classic
//	mode-specific column drawers.
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

// Maximum post spans recorded per column of a group. Regular Doom
// sprite posts are long, so a handful of spans is the norm; if a
// pathological patch exceeds this, the extra posts are dropped.
#define MAXHSPANS 128

// Interleaved column staging buffer: pixel y of column c of the
// current group lives at horiztemp[(y << 2) + c]. Four columns per
// screen row keep the post-pass writes to the framebuffer
// sequential along each row.
static byte horiztemp[SCREENHEIGHT * 4];

// Per-column (yl, yh) span lists, in ascending, non-overlapping
// order (the posts / wall tiers are walked top to bottom).
static int horizspans[4][MAXHSPANS * 2];
static int horiznspans[4];

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
// already includes the viewwindow offset.
//
#if defined(USE_BACKBUFFER)
#define HORIZ_ROW(y) (ylookup[(y)])
#define HORIZ_COFS(x) (columnofs[(x)])
#else
#define HORIZ_ROW(y) (destview + MulScreenWidth(y))
#define HORIZ_COFS(x) ((x) << detailshift)
#endif

//
// Some composite CGA modes only draw a decimated subset of the
// sprite columns in certain detail levels. Mirror what the classic
// R_DrawVisSprite does, so both renderers look identical.
//
// Source texture addressing matches the column drawers in
// linearh/linearl/linearp.asm: the source byte index is
// (texturepos >> 16) & 127, i.e. composite columns and patch
// posts wrap inside a 128 byte window.
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

static void R_ClearHorizSpans(void)
{
    horiznspans[0] = 0;
    horiznspans[1] = 0;
    horiznspans[2] = 0;
    horiznspans[3] = 0;
}

//
// Pre-pass: scale one texture column of the sprite into horiztemp
// (slot c of the current group), recording the visible post spans.
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
    int spanyl;
    short mfc_x;
    short mcc_x;
    fixed_t basetexturemid;
    fixed_t texfrac;
    fixed_t fracstep;
    byte *source;
    byte *dest;
    int nspans;

    nspans = 0;
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

        if (nspans < MAXHSPANS)
        {
            spanyl = yl;
            source = (byte *)column + 3;
            fracstep = dc_iscale;
            texfrac = dc_texturemid - (column->topdelta << FRACBITS) +
                      (yl - centery) * fracstep;

            dest = &horiztemp[(yl << 2) + c];

            while (yl <= yh)
            {
                *dest = source[((unsigned)texfrac >> 16) & 127];
                texfrac += fracstep;
                dest += 4;
                yl++;
            }

            horizspans[c][(nspans << 1)] = spanyl;
            horizspans[c][(nspans << 1) + 1] = yh;
            nspans++;
        }

        column = (column_t *)((byte *)column + column->length + 4);
    }

    dc_texturemid = basetexturemid;
    horiznspans[c] = nspans;
}

//
// Post-pass: map the up-to-four staged columns in horiztemp to the
// screen at xstart..., writing row by row so that adjacent columns
// land next to each other in memory. colormap translates the
// staged texture bytes; NULL means the bytes are already screen
// colors (wall staging maps with the per-column colormap).
//
static void R_FlushHorizGroup(int xstart, int group, byte *colormap)
{
    int c;
    int y;
    int ymin;
    int ymax;
    int ncols;
    int pos[4];
    int off[4];
    byte *map;

    if (!colormap)
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
    else
        map = colormap;

    ymin = SCREENHEIGHT;
    ymax = -1;
    ncols = 0;

    for (c = 0; c < group; c++)
    {
        pos[c] = 0;
        off[c] = HORIZ_COFS(xstart + c);

        if (horiznspans[c])
        {
            ncols++;
            if (horizspans[c][0] < ymin)
                ymin = horizspans[c][0];
            if (horizspans[c][(horiznspans[c] << 1) - 1] > ymax)
                ymax = horizspans[c][(horiznspans[c] << 1) - 1];
        }
    }

    if (!ncols)
        return;

    if (!detailshift)
    {
        for (y = ymin; y <= ymax; y++)
        {
            byte *row = HORIZ_ROW(y);

            for (c = 0; c < group; c++)
            {
                int *spans = horizspans[c];
                int i = pos[c];

                if (i < horiznspans[c] && y > spans[(i << 1) + 1])
                {
                    i++;
                    pos[c] = i;
                }

                if (i < horiznspans[c] && y >= spans[i << 1])
                    row[off[c]] = map[horiztemp[(y << 2) + c]];
            }
        }
    }
    else if (detailshift == DETAIL_LOW)
    {
        for (y = ymin; y <= ymax; y++)
        {
            byte *row = HORIZ_ROW(y);

            for (c = 0; c < group; c++)
            {
                int *spans = horizspans[c];
                int i = pos[c];

                if (i < horiznspans[c] && y > spans[(i << 1) + 1])
                {
                    i++;
                    pos[c] = i;
                }

                if (i < horiznspans[c] && y >= spans[i << 1])
                {
                    unsigned short color = map[horiztemp[(y << 2) + c]];
                    color |= color << 8;
                    *(unsigned short *)(row + off[c]) = color;
                }
            }
        }
    }
    else
    {
        for (y = ymin; y <= ymax; y++)
        {
            byte *row = HORIZ_ROW(y);

            for (c = 0; c < group; c++)
            {
                int *spans = horizspans[c];
                int i = pos[c];

                if (i < horiznspans[c] && y > spans[(i << 1) + 1])
                {
                    i++;
                    pos[c] = i;
                }

                if (i < horiznspans[c] && y >= spans[i << 1])
                {
                    unsigned int color = map[horiztemp[(y << 2) + c]];
                    color |= color << 8;
                    color |= color << 16;
                    *(unsigned int *)(row + off[c]) = color;
                }
            }
        }
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
    fixed_t frac;
    int x;
    int group;
    int c;

    if (!vis->colormap)
    {
        // NULL colormap = shadow/fuzz draw
        fuzzfallback(vis);
        return;
    }

    patch = W_CacheLumpNum(vis->patch + firstspritelump, PU_CACHE);

    dc_colormap = vis->colormap;
    dc_iscale = abs(vis->xiscale) >> detailshift;
    dc_texturemid = vis->texturemid;
    frac = vis->startfrac;
    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);

    x = vis->x1;

    // Same do-while shape as the classic drawers: even a sprite with
    // x1 > x2 draws its first column.
    do
    {
        group = vis->x2 - x + 1;
        if (group > 4)
            group = 4;

        R_ClearHorizSpans();

        for (c = 0; c < group; c++)
        {
            if (!HORIZ_SKIP_COLUMN(x + c))
                R_DrawColumnHoriz(vis, patch, x + c, c, frac);

            frac += vis->xiscale;
        }

        R_FlushHorizGroup(x, group, dc_colormap);

        x += group;
    } while (x <= vis->x2);
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
    int n;
    byte *dest;
    byte *source;
    byte *colormap;
    fixed_t frac;
    fixed_t fracstep;

    x = dc_x;
    base = x & ~3;

    if (wallgroupx != base)
    {
        // The previous group (or stale sprite spans) must not leak
        // into the new group.
        R_FlushWallHoriz();
        R_ClearHorizSpans();
        wallgroupx = base;
    }

    yl = dc_yl;
    yh = dc_yh;

    if (yl > yh)
        return;

    c = x & 3;
    n = horiznspans[c];

    if (n >= MAXHSPANS)
        return;

    horizspans[c][(n << 1)] = yl;
    horizspans[c][(n << 1) + 1] = yh;
    horiznspans[c] = n + 1;

    // [RH] Same texture stepping as the asm column drawers: the
    // source index wraps inside a 128 byte window (see
    // R_DrawColumnBackbuffer: (frac << 9) >> 25).
    colormap = dc_colormap;
    source = dc_source;
    fracstep = dc_iscale;
    frac = dc_texturemid + (yl - centery) * fracstep;
    dest = &horiztemp[(yl << 2) + c];

    while (yl <= yh)
    {
        *dest = colormap[source[((unsigned)frac >> 16) & 127]];
        frac += fracstep;
        dest += 4;
        yl++;
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
        R_FlushHorizGroup(wallgroupx, group, NULL);

    wallgroupx = -1;
    R_ClearHorizSpans();
}

#endif
