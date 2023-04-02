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
//	Preparation of data for rendering,
//	generation of lookups, caching, retrieval by name.
//

#include <string.h>
#include <strings.h>
#include <malloc.h>
#include <stdio.h>
#include <limits.h>
#include "options.h"
#include "i_system.h"
#include "z_zone.h"

#include "w_wad.h"

#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"

#include "doomstat.h"
#include "r_sky.h"

#include <alloca.h>

#include "r_data.h"

//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
typedef struct
{
    short originx;
    short originy;
    short patch;
    short stepdir;
    short colormap;
} mappatch_t;

//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//
typedef struct
{
    char name[8];
    boolean masked;
    short width;
    short height;
    void **columndirectory; // OBSOLETE
    short patchcount;
    mappatch_t patches[1];
} maptexture_t;

// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.
typedef struct
{
    // Block origin (allways UL),
    // which has allready accounted
    // for the internal origin of the patch.
    int originx;
    int originy;
    short patch;
} texpatch_t;

// A maptexturedef_t describes a rectangular texture,
//  which is composed of one or more mappatch_t structures
//  that arrange graphic patches.
typedef struct texture_t texture_t;

struct texture_t
{
    // Keep name for switch changing, etc.
    char name[8];
    short width;
    short height;
    // Index in textures list
    short index;
    // Next in hash table chain
    texture_t *next;
    // All the patches[patchcount]
    //  are drawn back to front into the cached texture.
    short patchcount;
    texpatch_t patches[1];
};

int firstflat;
int lastflat;
int numflats;

int firstspritelump;
int lastspritelump;
int numspritelumps;

short numtextures;
texture_t **textures;
texture_t **textures_hashtable;

int *texturewidthmask;
// needed for texture pegging
fixed_t *textureheight;
int *texturecompositesize;
short **texturecolumnlump;
unsigned short **texturecolumnofs;
byte **texturecomposite;

// for global animation
int *flattranslation;
int *texturetranslation;

// needed for pre rendering
fixed_t *spritewidth;
fixed_t *spriteoffset;
fixed_t *spritetopoffset;

lighttable_t datacolormaps[34 * 256 + 255];
lighttable_t *colormaps = datacolormaps;

//
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.
//

//
// R_DrawColumnInCache
// Clip and draw a column
//  from a patch into a cached post.
//
void R_DrawColumnInCache(column_t *patch,
                         byte *cache,
                         int originy,
                         int cacheheight)
{
    int count;
    int position;
    byte *source;
    byte *dest;

    dest = (byte *)cache + 3;

    while (patch->topdelta != 0xff)
    {
        source = (byte *)patch + 3;
        count = patch->length;
        position = originy + patch->topdelta;

        if (position < 0)
        {
            count += position;
            position = 0;
        }

        if (count > cacheheight - position)
            count = cacheheight - position;

        if (count > 0)
        {
            CopyBytes(source, cache + position, count);
            // memcpy(cache + position, source, count);
        }

        patch = (column_t *)((byte *)patch + patch->length + 4);
    }
}

//
// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
void R_GenerateComposite(int texnum)
{
    byte *block;
    texture_t *texture;
    texpatch_t *patch;
    patch_t *realpatch;
    int x;
    int x1;
    int x2;
    int i;
    column_t *patchcol;
    short *collump;
    unsigned short *colofs;

    texture = textures[texnum];

    block = Z_Malloc(texturecompositesize[texnum], PU_STATIC, &texturecomposite[texnum]);

    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    // Composite the columns together.
    patch = texture->patches;

    for (i = 0, patch = texture->patches; i < texture->patchcount; i++, patch++)
    {
        realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);
        x1 = patch->originx;
        x2 = x1 + realpatch->width;

        if (x1 < 0)
            x = 0;
        else
            x = x1;

        if (x2 > texture->width)
            x2 = texture->width;

        for (; x < x2; x++)
        {
            // Column does not have multiple patches?
            if (collump[x] >= 0)
                continue;

            patchcol = (column_t *)((byte *)realpatch + realpatch->columnofs[x - x1]);
            R_DrawColumnInCache(patchcol, block + colofs[x], patch->originy, texture->height);
        }
    }

    // Now that the texture has been built in column cache,
    //  it is purgable from zone memory.
    Z_ChangeTag(block, PU_CACHE);
}

//
// R_GenerateLookup
//
void R_GenerateLookup(int texnum)
{
    texture_t *texture;
    byte *patchcount; // patchcount[texture->width]
    texpatch_t *patch;
    patch_t *realpatch;
    int x;
    int x1;
    int x2;
    int i;
    short *collump;
    unsigned short *colofs;

    texture = textures[texnum];

    // Composited texture not created yet.
    texturecomposite[texnum] = 0;

    texturecompositesize[texnum] = 0;
    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    // Now count the number of columns
    //  that are covered by more than one patch.
    // Fill in the lump / offset, so columns
    //  with only a single patch are all done.
    patchcount = (byte *)alloca(texture->width);
    memset(patchcount, 0, texture->width);
    patch = texture->patches;

    for (i = 0, patch = texture->patches;
         i < texture->patchcount;
         i++, patch++)
    {
        realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);
        x1 = patch->originx;
        x2 = x1 + realpatch->width;

        if (x1 < 0)
            x = 0;
        else
            x = x1;

        if (x2 > texture->width)
            x2 = texture->width;
        for (; x < x2; x++)
        {
            patchcount[x]++;
            collump[x] = patch->patch;
            colofs[x] = realpatch->columnofs[x - x1] + 3;
        }
    }

    for (x = 0; x < texture->width; x++)
    {
        if (!patchcount[x])
        {
            return;
        }

        if (patchcount[x] > 1)
        {
            // Use the cached block.
            collump[x] = -1;
            colofs[x] = texturecompositesize[texnum];
            texturecompositesize[texnum] += texture->height;
        }
    }
}

void GenerateTextureHashTable(void)
{
    texture_t **rover;
    short i;
    int key;

    textures_hashtable = Z_MallocUnowned(sizeof(texture_t *) * numtextures, PU_STATIC);

    memset(textures_hashtable, 0, sizeof(texture_t *) * numtextures);

    // Add all textures to hash table

    for (i = 0; i < numtextures; ++i)
    {
        // Store index

        textures[i]->index = i;

        // Vanilla Doom does a linear search of the texures array
        // and stops at the first entry it finds.  If there are two
        // entries with the same name, the first one in the array
        // wins. The new entry must therefore be added at the end
        // of the hash chain, so that earlier entries win.

        key = W_LumpNameHash(textures[i]->name) % numtextures;

        rover = &textures_hashtable[key];

        while (*rover != NULL)
        {
            rover = &(*rover)->next;
        }

        // Hook into hash table

        textures[i]->next = NULL;
        *rover = textures[i];
    }
}

//
// R_InitTextures
// Initializes the texture list
//  with the textures from the world map.
//
void R_InitTextures(void)
{
    maptexture_t *mtexture;
    texture_t *texture;
    mappatch_t *mpatch;
    texpatch_t *patch;

    int i;
    int j;

    int *maptex;
    int *maptex2;
    int *maptex1;

    char name[9];
    char *names;
    char *name_p;

    short *patchlookup;

    int totalwidth;
    int nummappatches;
    int offset;
    int maxoff;
    int maxoff2;
    short numtextures1;
    short numtextures2;

    int *directory;

    int temp1;
    int temp2;
    int temp3;

    // Load the patch names from pnames.lmp.
    name[8] = 0;
    names = W_CacheLumpName("PNAMES", PU_STATIC);
    nummappatches = *((int *)names);
    name_p = names + 4;
    patchlookup = alloca(nummappatches * sizeof(*patchlookup));

    for (i = 0; i < nummappatches; i++)
    {
        strncpy(name, name_p + i * 8, 8);
        patchlookup[i] = W_GetNumForName(name);
    }
    Z_Free(names);

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    maptex = maptex1 = W_CacheLumpName("TEXTURE1", PU_STATIC);
    numtextures1 = *maptex;
    maxoff = W_LumpLength(W_GetNumForName("TEXTURE1"));
    directory = maptex + 1;

    if (W_GetNumForName("TEXTURE2") != -1)
    {
        maptex2 = W_CacheLumpName("TEXTURE2", PU_STATIC);
        numtextures2 = *maptex2;
        maxoff2 = W_LumpLength(W_GetNumForName("TEXTURE2"));
    }
    else
    {
        maptex2 = NULL;
        numtextures2 = 0;
        maxoff2 = 0;
    }
    numtextures = numtextures1 + numtextures2;

    textures = Z_MallocUnowned(numtextures * 4, PU_STATIC);
    texturecolumnlump = Z_MallocUnowned(numtextures * 4, PU_STATIC);
    texturecolumnofs = Z_MallocUnowned(numtextures * 4, PU_STATIC);
    texturecomposite = Z_MallocUnowned(numtextures * 4, PU_STATIC);
    texturecompositesize = Z_MallocUnowned(numtextures * 4, PU_STATIC);
    texturewidthmask = Z_MallocUnowned(numtextures * 4, PU_STATIC);
    textureheight = Z_MallocUnowned(numtextures * 4, PU_STATIC);

    totalwidth = 0;

    //	Really complex printing shit...
    temp1 = W_GetNumForName("S_START"); // P_???????
    temp2 = W_GetNumForName("S_END") - 1;
    temp3 = ((temp2 - temp1 + 63) / 64) + ((numtextures + 63) / 64);
    printf("[");
    for (i = 0; i < temp3; i++)
        printf(" ");
    printf("        ]");
    for (i = 0; i < temp3; i++)
        printf("\x8");
    printf("\x8\x8\x8\x8\x8\x8\x8\x8\x8");

    for (i = 0; i < numtextures; i++, directory++)
    {
        if (!(i & 63))
            printf(".");

        if (i == numtextures1)
        {
            // Start looking in second texture file.
            maptex = maptex2;
            maxoff = maxoff2;
            directory = maptex + 1;
        }

        offset = *directory;

        mtexture = (maptexture_t *)((byte *)maptex + offset);

        texture = textures[i] =
            Z_MallocUnowned(sizeof(texture_t) + sizeof(texpatch_t) * (mtexture->patchcount - 1), PU_STATIC);

        texture->width = mtexture->width;
        texture->height = mtexture->height;
        texture->patchcount = mtexture->patchcount;

        CopyBytes(mtexture->name, texture->name, sizeof(texture->name));
        // memcpy(texture->name, mtexture->name, sizeof(texture->name));
        mpatch = &mtexture->patches[0];
        patch = &texture->patches[0];

        for (j = 0; j < texture->patchcount; j++, mpatch++, patch++)
        {
            patch->originx = mpatch->originx;
            patch->originy = mpatch->originy;
            patch->patch = patchlookup[mpatch->patch];
        }
        texturecolumnlump[i] = Z_MallocUnowned(texture->width * 2, PU_STATIC);
        texturecolumnofs[i] = Z_MallocUnowned(texture->width * 2, PU_STATIC);

        j = 1;
        while (j * 2 <= texture->width)
            j <<= 1;

        texturewidthmask[i] = j - 1;
        textureheight[i] = texture->height << FRACBITS;

        totalwidth += texture->width;
    }

    Z_Free(maptex1);
    if (maptex2)
        Z_Free(maptex2);

    // Precalculate whatever possible.
    for (i = 0; i < numtextures; i++)
        R_GenerateLookup(i);

    // Create translation table for global animation.
    texturetranslation = Z_MallocUnowned((numtextures + 1) * 4, PU_STATIC);

    for (i = 0; i < numtextures; i++)
        texturetranslation[i] = i;

    GenerateTextureHashTable();
}

//
// R_InitFlats
//
void R_InitFlats(void)
{
    int i;

    firstflat = W_GetNumForName("F_START") + 1;
    lastflat = W_GetNumForName("F_END") - 1;
    numflats = lastflat - firstflat + 1;

    // Create translation table for global animation.
    flattranslation = Z_MallocUnowned((numflats + 1) * 4, PU_STATIC);

    for (i = 0; i < numflats; i++)
        flattranslation[i] = i;
}

//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//
void R_InitSpriteLumps(void)
{
    int i;
    patch_t *patch;

    firstspritelump = W_GetNumForName("S_START") + 1;
    lastspritelump = W_GetNumForName("S_END") - 1;

    numspritelumps = lastspritelump - firstspritelump + 1;
    spritewidth = Z_MallocUnowned(numspritelumps * 4, PU_STATIC);
    spriteoffset = Z_MallocUnowned(numspritelumps * 4, PU_STATIC);
    spritetopoffset = Z_MallocUnowned(numspritelumps * 4, PU_STATIC);

    for (i = 0; i < numspritelumps; i++)
    {
        if (!(i & 63))
            printf(".");

        patch = W_CacheLumpNum(firstspritelump + i, PU_CACHE);
        spritewidth[i] = patch->width << FRACBITS;
        spriteoffset[i] = patch->leftoffset << FRACBITS;
        spritetopoffset[i] = patch->topoffset << FRACBITS;
    }
}

//
// R_InitColormaps
//
void R_InitColormaps(void)
{
    int lump;

    // Load in the light tables,
    //  256 byte align tables.
    lump = W_GetNumForName("COLORMAP");
    colormaps = (byte *)(((int)datacolormaps + 255) & ~0xff);
    W_ReadLump(lump, colormaps);
}

// -----------------------------------------------------------------------------
// R_InitTintMap
// [crispy] initialize translucency filter map
// based in parts on the implementation from boom202s/R_DATA.C:676-787
// -----------------------------------------------------------------------------

byte *tintmap;

enum
{
    r,
    g,
    b
} rgb_t;

// [crispy] copied over from i_video.c
int V_GetPaletteIndex(byte *palette, int r, int g, int b)
{
    int best, best_diff, diff;
    int i;

    best = 0;
    best_diff = INT_MAX;

    for (i = 0; i < 256; ++i)
    {
        diff = (r - palette[3 * i + 0]) * (r - palette[3 * i + 0]) + (g - palette[3 * i + 1]) * (g - palette[3 * i + 1]) + (b - palette[3 * i + 2]) * (b - palette[3 * i + 2]);

        if (diff < best_diff)
        {
            best = i;
            best_diff = diff;
        }

        if (diff == 0)
        {
            break;
        }
    }

    return best;
}

#define TRANSPARENCY_LEVEL 25

static void R_InitTintMap(void)
{
    // [JN] Generate TINTMAP dynamically.

    // Compose a default transparent filter map based on PLAYPAL.
    unsigned char *playpal = W_CacheLumpName("PLAYPAL", PU_STATIC);
    tintmap = Z_MallocUnowned(256 * 256, PU_STATIC);

    {
        byte *fg, *bg, blend[3];
        byte *tp75 = tintmap;
        int i, j;

        // [crispy] background color
        for (i = 0; i < 256; i++)
        {
            // [crispy] foreground color
            for (j = 0; j < 256; j++)
            {
                // [crispy] shortcut: identical foreground and background
                if (i == j)
                {
                    *tp75++ = i;
                    continue;
                }

                bg = playpal + 3 * i;
                fg = playpal + 3 * j;

                blend[r] = (TRANSPARENCY_LEVEL * fg[r] + (100 - TRANSPARENCY_LEVEL) * bg[r]) / 100;
                blend[g] = (TRANSPARENCY_LEVEL * fg[g] + (100 - TRANSPARENCY_LEVEL) * bg[g]) / 100;
                blend[b] = (TRANSPARENCY_LEVEL * fg[b] + (100 - TRANSPARENCY_LEVEL) * bg[b]) / 100;
                *tp75++ = V_GetPaletteIndex(playpal, blend[r], blend[g], blend[b]);
            }
        }
    }

    Z_ChangeTag(playpal, PU_CACHE);
}

//
// R_InitData
// Locates all the lumps
//  that will be used by all views
// Must be called after W_Init.
//
void R_InitData(void)
{
    R_InitTextures();
    printf(".");
    R_InitFlats();
    printf(".");
    R_InitSpriteLumps();
    printf(".");
    R_InitColormaps();
    printf(".");
    R_InitTintMap();
}

//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
short R_FlatNumForName(char *name)
{
    short i;

    i = W_GetNumForName(name);
    return i - firstflat;
}

//
// R_TextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
short R_TextureNumForName(char *name)
{
    texture_t *texture;
    int key;

    // "NoTexture" marker.
    if (name[0] == '-')
        return 0;

    key = W_LumpNameHash(name) % numtextures;

    texture = textures_hashtable[key];

    while (texture != NULL)
    {
        if (!strncasecmp(texture->name, name, 8))
            return texture->index;

        texture = texture->next;
    }

    return -1;
}

//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
int flatmemory;

void R_PrecacheLevel(void)
{
    char *flatpresent;
    char *texturepresent;
    char spritepresent[NUMSPRITES];

    int i;
    int j;
    int k;
    int lump;

    texture_t *texture;
    thinker_t *th;
    spriteframe_t *sf;

    if (demoplayback && !timingdemo)
        return;

    // Precache flats.
    flatpresent = alloca(numflats);
    memset(flatpresent, 0, numflats);

    for (i = 0; i < numsectors; i++)
    {
        flatpresent[sectors[i].floorpic] = 1;
        flatpresent[sectors[i].ceilingpic] = 1;
    }

    flatmemory = 0;

    for (i = 0; i < numflats; i++)
    {
        if (flatpresent[i])
        {
            lump = firstflat + i;
            flatmemory += lumpinfo[lump].size;
            W_CacheLumpNum(lump, PU_CACHE);
        }
    }

    // Precache textures.
    texturepresent = alloca(numtextures);
    memset(texturepresent, 0, numtextures);

    for (i = 0; i < numsides; i++)
    {
        texturepresent[sides[i].toptexture] = 1;
        texturepresent[sides[i].midtexture] = 1;
        texturepresent[sides[i].bottomtexture] = 1;
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //  indicate a sky floor/ceiling as a flat,
    //  while the sky texture is stored like
    //  a wall texture, with an episode dependend
    //  name.
    texturepresent[skytexture] = 1;

    for (i = 0; i < numtextures; i++)
    {
        if (!texturepresent[i])
            continue;

        // [crispy] precache composite textures
        R_GenerateComposite(i);

        texture = textures[i];

        for (j = 0; j < texture->patchcount; j++)
        {
            lump = texture->patches[j].patch;
            W_CacheLumpNum(lump, PU_CACHE);
        }
    }

    // Precache sprites.
    SetDWords(spritepresent, 0, NUMSPRITES / 4);

    for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
        if (th->function.acp1 == (actionf_p1)P_MobjThinker || th->function.acp1 == (actionf_p1)P_MobjBrainlessThinker || th->function.acp1 == (actionf_p1)P_MobjTicklessThinker)
            spritepresent[((mobj_t *)th)->sprite] = 1;
    }

    for (i = 0; i < NUMSPRITES; i++)
    {
        if (!spritepresent[i])
            continue;

        for (j = 0; j < sprites[i].numframes; j++)
        {
            sf = &sprites[i].spriteframes[j];
            for (k = 0; k < 8; k++)
            {
                lump = firstspritelump + sf->lump[k];
                W_CacheLumpNum(lump, PU_CACHE);
            }
        }
    }
}
