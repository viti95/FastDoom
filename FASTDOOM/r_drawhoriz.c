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
//	(ylookup/columnofs) and the VBE2 direct modes (destview); the
//	asm post-pass derives the screen addresses from them. The
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
//	The post-pass is the assembly R_FlushHorizGroupAsm
//	(linearhz.asm). The pre-passes stay in C; R_StageColumnHoriz is
//	already shaped like the asm colfuncs (no arguments, dc_*
//	globals) and can be replaced one-for-one by an asm version.
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

// Bit c set on row y: column c of the current group has a staged,
// visible pixel on screen row y. Built by the pre-pass while it
// scales pixels (one cached byte OR per pixel), consumed and
// cleared by the post-pass. This replaces per-column span lists:
// the flush needs a single byte load per screen row instead of
// chasing span pointers for each column.
byte horizrowmask[SCREENHEIGHT];

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
// group are guaranteed already cleared by the flush.
//
static void R_BeginHorizGroup(void)
{
    horizymin = SCREENHEIGHT;
    horizymax = -1;
}

//
// Pre-pass: scale one texture column of the sprite into horiztemp
// (slot c of the current group), marking the touched screen rows.
// Clipping matches the classic R_DrawVisSprite exactly. The post
// coordinates and stepping come from the dc_*/spr* globals set up
// by the caller, like the asm colfuncs.
//
static void R_DrawColumnHoriz(patch_t *patch, int x, int c, fixed_t frac)
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
                R_DrawColumnHoriz(patch, x + c, c, frac);

            frac += xiscale;
        }

        R_FlushHorizGroupAsm(x, group, dc_colormap);

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
        R_FlushHorizGroupAsm(wallgroupx, group, NULL);

    wallgroupx = -1;
}

#endif
