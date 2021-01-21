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
//	Refresh of things, i.e. objects represented by sprites.
//

#include <string.h>
#include <stdlib.h>

#include "doomdef.h"
#include "doomstat.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

#include "doomstat.h"

#include <conio.h>

#define SC_INDEX 0x3C4

#define MINZ (FRACUNIT * 4)
#define BASEYCENTER 100

#define INITIAL_SPRITES 128

typedef struct
{
    int x1;
    int x2;

    int column;
    int topclip;
    int bottomclip;

} maskdraw_t;

//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
fixed_t pspritescale;
fixed_t pspriteiscale;
fixed_t pspriteiscaleshifted;

lighttable_t **spritelights;

// constant arrays
//  used for psprite clipping and initializing clipping
short negonearray[SCREENWIDTH];
short screenheightarray[SCREENWIDTH];

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t sprites patches
spritedef_t sprites[NUMSPRITES];

spriteframe_t sprtemp[29];
int maxframe;
char *spritename;

//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
void R_InstallSpriteLump(int lump,
                         unsigned frame,
                         unsigned rotation,
                         byte flipped)
{
    int r;

    if ((int)frame > maxframe)
        maxframe = frame;

    if (rotation == 0)
    {
        // the lump should be used for all rotations

        sprtemp[frame].rotate = false;
        for (r = 0; r < 8; r++)
        {
            sprtemp[frame].lump[r] = lump - firstspritelump;
            sprtemp[frame].flip[r] = (byte)flipped;
        }
        return;
    }

    // the lump is only used for one rotation
    sprtemp[frame].rotate = true;

    // make 0 based
    rotation--;
    sprtemp[frame].lump[rotation] = lump - firstspritelump;
    sprtemp[frame].flip[rotation] = (byte)flipped;
}

//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
//  (4 chars exactly) to be used.
// Builds the sprite rotation matrixes to account
//  for horizontally flipped sprites.
// Will report an error if the lumps are inconsistant.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//  letter/number appended.
// The rotation character can be 0 to signify no rotations.
//
void R_InitSpriteDefs(char **namelist)
{
    int i;
    int l;
    int intname;
    int frame;
    int rotation;
    int start;
    int end;
    int patched;

    start = firstspritelump - 1;
    end = lastspritelump + 1;

    // scan all the lump names for each of the names,
    //  noting the highest frame letter.
    // Just compare 4 characters as ints
    for (i = 0; i < NUMSPRITES; i++)
    {
        spritename = namelist[i];
        SetBytes(sprtemp, -1, sizeof(sprtemp));

        maxframe = -1;
        intname = *(int *)namelist[i];

        // scan the lumps,
        //  filling in the frames for whatever is found
        for (l = start + 1; l < end; l++)
        {
            if (*(int *)lumpinfo[l].name == intname)
            {
                frame = lumpinfo[l].name[4] - 'A';
                rotation = lumpinfo[l].name[5] - '0';

                if (modifiedgame)
                    patched = W_GetNumForName(lumpinfo[l].name);
                else
                    patched = l;

                R_InstallSpriteLump(patched, frame, rotation, 0);

                if (lumpinfo[l].name[6])
                {
                    frame = lumpinfo[l].name[6] - 'A';
                    rotation = lumpinfo[l].name[7] - '0';
                    R_InstallSpriteLump(l, frame, rotation, 1);
                }
            }
        }

        // check the frames that were found for completeness
        if (maxframe == -1)
        {
            sprites[i].numframes = 0;
            continue;
        }

        maxframe++;

        // allocate space for the frames present and copy sprtemp to it
        sprites[i].numframes = maxframe;
        sprites[i].spriteframes = Z_MallocUnowned(maxframe * sizeof(spriteframe_t), PU_STATIC);
        CopyBytes(sprtemp, sprites[i].spriteframes, maxframe * sizeof(spriteframe_t));
        //memcpy(sprites[i].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
    }
}

//
// GAME FUNCTIONS
//
vissprite_t *vissprites;
vissprite_t *vissprite_p;
int newvissprite;

vissprite_t **vissprite_ptrs; // killough
size_t num_vissprite, num_vissprite_alloc, num_vissprite_ptrs;

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites(char **namelist)
{
    SetWords(negonearray, -1, SCREENWIDTH);
    R_InitSpriteDefs(namelist);
}

//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites(void)
{
    num_vissprite = 0; // killough
}

//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
short *mfloorclip;
short *mceilingclip;

fixed_t spryscale;
fixed_t sprtopscreen;

//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
void R_DrawVisSprite(vissprite_t *vis)
{
    column_t *column;
    int texturecolumn;
    fixed_t frac;
    patch_t *patch;
    fixed_t fracstep;
    fixed_t nextfrac;

    patch = W_CacheLumpNum(vis->patch + firstspritelump, PU_CACHE);

    dc_colormap = vis->colormap;

    if (!dc_colormap)
    {
        // NULL colormap = shadow draw
        if (saturnShadows)
            dc_colormap = colormaps;
        colfunc = fuzzcolfunc;
    }

    dc_iscale = abs(vis->xiscale) >> detailshift;
    dc_texturemid = vis->texturemid;
    frac = vis->startfrac;
    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);

    dc_x = vis->x1;
    do
    {
        int topscreen;
        int bottomscreen;
        fixed_t basetexturemid;

        int yl, yh;
        short mfc_x, mcc_x;

        basetexturemid = dc_texturemid;
        mfc_x = mfloorclip[dc_x];
        mcc_x = mceilingclip[dc_x];

        texturecolumn = frac >> FRACBITS;
        column = (column_t *)((byte *)patch + patch->columnofs[texturecolumn]);

        for (; column->topdelta != 0xff;)
        {
            // calculate unclipped screen coordinates
            //  for post
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

            dc_source = (byte *)column + 3;
            dc_texturemid = basetexturemid - (column->topdelta << FRACBITS);

            dc_yh = yh;
            dc_yl = yl;

            colfunc();

            column = (column_t *)((byte *)column + column->length + 4);
        }

        dc_texturemid = basetexturemid;
        dc_x += 1;
        frac += vis->xiscale;
    } while (dc_x <= vis->x2);

    colfunc = basecolfunc;
}

//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
void R_ProjectSprite(mobj_t *thing)
{
    fixed_t tr_x;
    fixed_t tr_y;

    fixed_t gzt; // killough 3/27/98

    fixed_t tx;
    fixed_t tz;

    fixed_t xscale;

    int x1;
    int x2;

    spritedef_t *sprdef;
    spriteframe_t *sprframe;
    int lump;

    unsigned rot;
    byte flip;

    int index;

    int calcopt;

    vissprite_t *vis;

    angle_t ang;
    fixed_t iscale;

    // transform the origin point
    tr_x = thing->x - viewx;
    tr_y = thing->y - viewy;

    if (nearSprites && !(thing->flags & MF_SHOOTABLE) && (abs(tr_x) > 40000000 || abs(tr_y) > 40000000))
        return;

    tz = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);

    // thing is behind view plane?
    if (tz < MINZ)
        return;

    tx = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos);

    // too far off the side?
    if (abs(tx) > (tz << 2))
        return;

    xscale = FixedDiv(projection, tz);

    // decide which patch to use for sprite relative to player
    sprdef = &sprites[thing->sprite];
    sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
        // choose a different rotation based on player view
        ang = R_PointToAngle(thing->x, thing->y);
        rot = (ang - thing->angle + (unsigned)(ANG45 / 2) * 9) >> 29;
        lump = sprframe->lump[rot];
        flip = sprframe->flip[rot];
    }
    else
    {
        // use single rotation for all views
        lump = sprframe->lump[0];
        flip = sprframe->flip[0];
    }

    // calculate edges of the shape
    tx -= spriteoffset[lump];
    x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;

    // off the right side?
    if (x1 > viewwidth)
        return;

    tx += spritewidth[lump];
    x2 = ((centerxfrac + FixedMul(tx, xscale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    // quickly reject sprites with bad x ranges
    if (x1 >= x2)
    {
        return;
    }

    // killough 4/9/98: clip things which are out of view due to height
    // viti95 6/6/20: optimize by removing divisions and using multiplications instead. Also discard first than calculate other things.
    calcopt = viewheight << FRACBITS;

    if (FixedMul(thing->z - viewz, xscale) > calcopt)
        return;

    gzt = thing->z + spritetopoffset[lump];

    if (calcopt - viewheight < FixedMul(viewz - gzt, xscale))
        return;

    if (num_vissprite >= num_vissprite_alloc) // killough
    {
        if (num_vissprite_alloc == 0)
        {
            num_vissprite_alloc = INITIAL_SPRITES; // Initial num sprites -> 128
            vissprites = Z_MallocUnowned(num_vissprite_alloc * sizeof(*vissprites), PU_STATIC);
        }
        else
        {
            vissprite_t *vissprites_old;
            size_t num_vissprite_alloc_old;

            vissprites_old = vissprites;
            num_vissprite_alloc_old = num_vissprite_alloc;

            num_vissprite_alloc = num_vissprite_alloc * 2;
            vissprites = Z_MallocUnowned(num_vissprite_alloc * sizeof(*vissprites), PU_STATIC);
            CopyBytes(vissprites_old, vissprites, num_vissprite_alloc_old * sizeof(*vissprites));
            //memcpy(vissprites, vissprites_old, num_vissprite_alloc_old * sizeof(*vissprites));
            Z_Free(vissprites_old);
        }
    }
    vis = vissprites + num_vissprite++;
    vis->mobjflags = thing->flags;
    vis->scale = xscale << detailshift;
    vis->gx = thing->x;
    vis->gy = thing->y;
    vis->gz = thing->z;
    vis->gzt = gzt; // killough 3/27/98
    vis->texturemid = gzt - viewz;
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 > viewwidthlimit ? viewwidthlimit : x2;
    //iscale = FixedDiv(FRACUNIT, xscale);
    iscale = FixedMul(tz, iprojection) >> 8;

    if (flip)
    {
        vis->startfrac = spritewidth[lump] - 1;
        vis->xiscale = -iscale;
    }
    else
    {
        vis->startfrac = 0;
        vis->xiscale = iscale;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);
    vis->patch = lump;

    // get light level
    if (thing->flags & MF_SHADOW)
    {
        // shadow draw
        vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
        // fixed map
        vis->colormap = fixedcolormap;
    }
    else if (thing->frame & FF_FULLBRIGHT)
    {
        // full bright
        vis->colormap = colormaps;
    }

    else
    {
        // diminished light
        index = xscale >> (LIGHTSCALESHIFT - detailshift);

        if (index >= MAXLIGHTSCALE)
            index = MAXLIGHTSCALE - 1;

        vis->colormap = spritelights[index];
    }
}

//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites(sector_t *sec)
{
    mobj_t *thing;
    int lightnum;

    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == validcount)
        return;

    // Well, now it will be done.
    sec->validcount = validcount;

    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT) + extralight;

    if (lightnum < 0)
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS - 1];
    else
        spritelights = scalelight[lightnum];

    // Handle all things in sector.
    for (thing = sec->thinglist; thing; thing = thing->snext)
        R_ProjectSprite(thing);
}

//
// R_DrawPSprite
//
void R_DrawPSprite(pspdef_t *psp)
{
    fixed_t tx;
    int x1;
    int x2;
    spritedef_t *sprdef;
    spriteframe_t *sprframe;
    int lump;
    byte flip;
    vissprite_t *vis;
    vissprite_t avis;

    // decide which patch to use
    sprdef = &sprites[psp->state->sprite];
    sprframe = &sprdef->spriteframes[psp->state->frame & FF_FRAMEMASK];

    lump = sprframe->lump[0];
    flip = sprframe->flip[0];

    // calculate edges of the shape
    tx = psp->sx - 160 * FRACUNIT;

    tx -= spriteoffset[lump];
    x1 = (centerxfrac + FixedMul(tx, pspritescale)) >> FRACBITS;

    tx += spritewidth[lump];
    x2 = ((centerxfrac + FixedMul(tx, pspritescale)) >> FRACBITS) - 1;

    // store information in a vissprite
    vis = &avis;
    vis->mobjflags = 0;
    vis->texturemid = (BASEYCENTER << FRACBITS) + FRACUNIT / 2 - (psp->sy - spritetopoffset[lump]);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 > viewwidthlimit ? viewwidthlimit : x2;
    vis->scale = pspritescale << detailshift;

    if (flip)
    {
        vis->xiscale = -pspriteiscale;
        vis->startfrac = spritewidth[lump] - 1;
    }
    else
    {
        vis->xiscale = pspriteiscale;
        vis->startfrac = 0;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);

    vis->patch = lump;

    if (viewplayer->powers[pw_invisibility] > 4 * 32 || viewplayer->powers[pw_invisibility] & 8)
    {
        // shadow draw
        vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
        // fixed color
        vis->colormap = fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
        // full bright
        vis->colormap = colormaps;
    }
    else
    {
        // local light
        vis->colormap = spritelights[MAXLIGHTSCALE - 1];
    }

    R_DrawVisSprite(vis);
}

//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites(void)
{
    int i;
    int lightnum;
    pspdef_t *psp;

    // get light level
    lightnum = (viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT) + extralight;

	if (lightnum < 0)
        spritelights = scalelight[0];
    else if (lightnum > LIGHTLEVELS - 1)
        spritelights = scalelight[LIGHTLEVELS - 1];
    else
        spritelights = scalelight[lightnum];

    // clip to screen bounds
    mfloorclip = screenheightarray;
    mceilingclip = negonearray;

    // add all active psprites
    psp = viewplayer->psprites;

    if (psp->state)
        R_DrawPSprite(psp);

    if ((psp + 1)->state)
        R_DrawPSprite((psp + 1));
}

//
// R_SortVisSprites
//
vissprite_t vsprsortedhead;

//#define bcopyp(d, s, n) memcpy(d, s, (n) * sizeof(void *))
#define bcopyp(d, s, n) CopyDWords(s, d, (n))

static void msort(vissprite_t **s, vissprite_t **t, int n)
{
    if (n >= 16)
    {
        int n1 = n / 2, n2 = n - n1;
        vissprite_t **s1 = s, **s2 = s + n1, **d = t;

        msort(s1, t, n1);
        msort(s2, t, n2);

        while ((*s1)->scale > (*s2)->scale ? (*d++ = *s1++, --n1) : (*d++ = *s2++, --n2))
            ;

        if (n2)
            bcopyp(d, s2, n2);
        else
            bcopyp(d, s1, n1);

        bcopyp(s, t, n);
    }
    else
    {
        int i;
        for (i = 1; i < n; i++)
        {
            vissprite_t *temp = s[i];
            if (s[i - 1]->scale < temp->scale)
            {
                int j = i;
                while ((s[j] = s[j - 1])->scale < temp->scale && --j)
                    ;
                s[j] = temp;
            }
        }
    }
}

void R_SortVisSprites(void)
{
    if (num_vissprite)
    {
        int i = num_vissprite;

        // If we need to allocate more pointers for the vissprites,
        // allocate as many as were allocated for sprites -- killough
        // killough 9/22/98: allocate twice as many

        if (num_vissprite_ptrs < num_vissprite * 2)
        {
            if (num_vissprite_ptrs > 0)
                Z_Free(vissprite_ptrs);
            num_vissprite_ptrs = num_vissprite_alloc * 2;
            vissprite_ptrs = Z_MallocUnowned(num_vissprite_ptrs * sizeof *vissprite_ptrs, PU_STATIC);
        }

        while (--i >= 0)
            vissprite_ptrs[i] = vissprites + i;

        // killough 9/22/98: replace qsort with merge sort, since the keys
        // are roughly in order to begin with, due to BSP rendering.

        msort(vissprite_ptrs, vissprite_ptrs + num_vissprite, num_vissprite);
    }
}

//
// R_DrawSprite
//
void R_DrawSprite(vissprite_t *spr)
{
    drawseg_t *ds;
    short clipbot[SCREENWIDTH];
    short cliptop[SCREENWIDTH];
    int x;
    int r1;
    int r2;
    fixed_t scale;
    fixed_t lowscale;
    int silhouette;

    if (spr->x1 > spr->x2)
        return;

    for (x = spr->x1; x <= spr->x2; x++)
    {
        clipbot[x] = viewheight;
        cliptop[x] = -1;
    }

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (ds = ds_p - 1; ds >= drawsegs; ds--)
    {
        // determine if the drawseg obscures the sprite
        if (ds->x1 > spr->x2 || ds->x2 < spr->x1 || (!ds->silhouette && !ds->maskedtexturecol))
        {
            // does not cover sprite
            continue;
        }

        r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
        r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

        if (ds->scale1 > ds->scale2)
        {
            lowscale = ds->scale2;
            scale = ds->scale1;
        }
        else
        {
            lowscale = ds->scale1;
            scale = ds->scale2;
        }

        if (scale < spr->scale || (lowscale < spr->scale && !R_PointOnSegSide(spr->gx, spr->gy, ds->curline)))
        {
            // masked mid texture?
            if (ds->maskedtexturecol)
                R_RenderMaskedSegRange(ds, r1, r2);
            // seg is behind sprite
            continue;
        }

        // clip this piece of the sprite
        if (ds->silhouette & SIL_BOTTOM && spr->gz < ds->bsilheight) //bottom sil
            for (x = r1; x <= r2; x++)
            {
                if (clipbot[x] == viewheight)
                    clipbot[x] = ds->sprbottomclip[x];
            }

        if (ds->silhouette & SIL_TOP && spr->gzt > ds->tsilheight) // top sil
            for (x = r1; x <= r2; x++)
                if (cliptop[x] == -1)
                    cliptop[x] = ds->sprtopclip[x];
    }

    // all clipping has been performed, so draw the sprite

    mfloorclip = clipbot;
    mceilingclip = cliptop;

    R_DrawVisSprite(spr);
}

//
// R_DrawMasked
//
void R_DrawMasked(void)
{
    vissprite_t *spr;
    drawseg_t *ds;
    int i;

    R_SortVisSprites();

    for (i = num_vissprite; --i >= 0;)
        R_DrawSprite(vissprite_ptrs[i]); // killough

    // render any remaining masked mid textures
    for (ds = ds_p - 1; ds >= drawsegs; ds--)
        if (ds->maskedtexturecol)
            R_RenderMaskedSegRange(ds, ds->x1, ds->x2);

    R_DrawPlayerSprites();
}
