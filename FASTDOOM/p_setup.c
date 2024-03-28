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
//	Do all the WAD I/O, get map description,
//	set up initial state and misc. LUTs.
//

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"
#include "z_zone.h"

#include "m_misc.h"

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "doomstat.h"

void P_SpawnMapThing(mapthing_t *mthing);

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int numvertexes;
vertex_t *vertexes;

seg_t *segs;

int numsectors;
sector_t *sectors;

int numsubsectors;
subsector_t *subsectors;

int firstnode;
node_t *nodes;

int numlines;
line_t *lines;

int numsides;
side_t *sides;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int bmapwidth;
int bmapheight;  // size in mapblocks
short *blockmap; // int for larger maps
// offsets in blockmap are from here
short *blockmaplump;
// origin of block map
fixed_t bmaporgx;
fixed_t bmaporgy;
// for thing chains
mobj_t **blocklinks;
// LUT bmapwidth muls
int *bmapwidthmuls;

// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
byte *rejectmatrix;

//
// P_LoadVertexes
//
void P_LoadVertexes(int lump)
{
    byte *data;
    int i;
    mapvertex_t *ml;
    vertex_t *li;

    // Determine number of lumps:
    //  total lump length / vertex record length.
    numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);

    // Allocate zone memory for buffer.
    vertexes = Z_MallocUnowned(numvertexes * sizeof(vertex_t), PU_LEVEL);

    // Load data into cache.
    data = W_CacheLumpNum(lump, PU_STATIC);

    ml = (mapvertex_t *)data;
    li = vertexes;

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    for (i = 0; i < numvertexes; i++, li++, ml++)
    {
        li->x = ml->x << FRACBITS;
        li->y = ml->y << FRACBITS;
    }

    // Free buffer memory.
    Z_Free(data);
}

//
// P_LoadSegs
//
void P_LoadSegs(int lump)
{
    byte *data;
    int i;
    mapseg_t *ml;
    seg_t *li;
    line_t *ldef;
    int linedef;
    int side;
    int numsegs;

    numsegs = W_LumpLength(lump) / sizeof(mapseg_t);
    segs = Z_MallocUnowned(numsegs * sizeof(seg_t), PU_LEVEL);
    memset(segs, 0, numsegs * sizeof(seg_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    ml = (mapseg_t *)data;
    li = segs;
    for (i = 0; i < numsegs; i++, li++, ml++)
    {
        li->v1 = &vertexes[ml->v1];
        li->v2 = &vertexes[ml->v2];

        li->angle = ml->angle << 16;
        li->offset = ml->offset << 16;
        linedef = ml->linedef;
        ldef = &lines[linedef];
        li->linedef = ldef;
        side = ml->side;
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;
        if (ldef->flags & ML_TWOSIDED)
            li->backsector = sides[ldef->sidenum[side ^ 1]].sector;
        else
            li->backsector = 0;
    }

    Z_Free(data);
}

//
// P_LoadSubsectors
//
void P_LoadSubsectors(int lump)
{
    byte *data;
    int i;
    mapsubsector_t *ms;
    subsector_t *ss;

    numsubsectors = W_LumpLength(lump) / sizeof(mapsubsector_t);
    subsectors = Z_MallocUnowned(numsubsectors * sizeof(subsector_t), PU_LEVEL);
    data = W_CacheLumpNum(lump, PU_STATIC);

    ms = (mapsubsector_t *)data;
    memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
    ss = subsectors;

    for (i = 0; i < numsubsectors; i++, ss++, ms++)
    {
        ss->numlines = ms->numsegs;
        ss->firstline = ms->firstseg;
    }

    Z_Free(data);
}

//
// P_LoadSectors
//
void P_LoadSectors(int lump)
{
    byte *data;
    int i;
    mapsector_t *ms;
    sector_t *ss;

    numsectors = W_LumpLength(lump) / sizeof(mapsector_t);
    sectors = Z_MallocUnowned(numsectors * sizeof(sector_t), PU_LEVEL);
    memset(sectors, 0, numsectors * sizeof(sector_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    ms = (mapsector_t *)data;
    ss = sectors;
    for (i = 0; i < numsectors; i++, ss++, ms++)
    {
        ss->floorheight = ms->floorheight << FRACBITS;
        ss->ceilingheight = ms->ceilingheight << FRACBITS;
        ss->floorpic = R_FlatNumForName(ms->floorpic);
        ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
        ss->lightlevel = ms->lightlevel;
        ss->special = ms->special;
        ss->tag = ms->tag;
        ss->thinglist = NULL;
    }

    Z_Free(data);
}

//
// P_LoadNodes
//
void P_LoadNodes(int lump)
{
    byte *data;
    int i;
    int j;
    int k;
    mapnode_t *mn;
    node_t *no;
    int numnodes;

    numnodes = W_LumpLength(lump) / sizeof(mapnode_t);
    firstnode = numnodes - 1;
    nodes = Z_MallocUnowned(numnodes * sizeof(node_t), PU_LEVEL);
    data = W_CacheLumpNum(lump, PU_STATIC);

    mn = (mapnode_t *)data;
    no = nodes;

    for (i = 0; i < numnodes; i++, no++, mn++)
    {
        no->x = mn->x << FRACBITS;
        no->xs = no->x >> FRACBITS;
        no->y = mn->y << FRACBITS;
        no->ys = no->y >> FRACBITS;
        no->dx = mn->dx << FRACBITS;
        no->dxs = no->dx >> FRACBITS;
        no->dy = mn->dy << FRACBITS;
        no->dys = no->dy >> FRACBITS;

        no->dxlt0 = no->dx < 0;
        no->dxgt0 = no->dx > 0;

        no->dylt0 = no->dy < 0;
        no->dygt0 = no->dy > 0;

        no->dyXORdx = no->dy ^ no->dx;
        
        for (j = 0; j < 2; j++)
        {
            no->children[j] = mn->children[j];
            for (k = 0; k < 4; k++)
                no->bbox[j][k] = mn->bbox[j][k] << FRACBITS;
        }
    }

    Z_Free(data);
}

//
// P_LoadThings
//
void P_LoadThings(int lump)
{
    byte *data;
    int i;
    mapthing_t *mt;
    int numthings;
    byte spawn;

    data = W_CacheLumpNum(lump, PU_STATIC);
    numthings = W_LumpLength(lump) / sizeof(mapthing_t);

    mt = (mapthing_t *)data;
    for (i = 0; i < numthings; i++, mt++)
    {
        spawn = 1;

        // Do not spawn cool, new monsters if !commercial
        if (gamemode != commercial)
        {
            switch (mt->type)
            {
            case 68: // Arachnotron
            case 64: // Archvile
            case 88: // Boss Brain
            case 89: // Boss Shooter
            case 69: // Hell Knight
            case 67: // Mancubus
            case 71: // Pain Elemental
            case 65: // Former Human Commando
            case 66: // Revenant
            case 84: // Wolf SS
                spawn = 0;
                break;
            }
        }
        if (spawn == 0)
            break;

        // Do spawn all other stuff.
        P_SpawnMapThing(mt);
    }

    Z_Free(data);
}

//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs(int lump)
{
    byte *data;
    int i;
    maplinedef_t *mld;
    line_t *ld;
    vertex_t *v1;
    vertex_t *v2;

    numlines = W_LumpLength(lump) / sizeof(maplinedef_t);
    lines = Z_MallocUnowned(numlines * sizeof(line_t), PU_LEVEL);
    memset(lines, 0, numlines * sizeof(line_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    mld = (maplinedef_t *)data;
    ld = lines;
    for (i = 0; i < numlines; i++, mld++, ld++)
    {
        ld->flags = mld->flags;
        ld->special = mld->special;
        ld->tag = mld->tag;
        v1 = ld->v1 = &vertexes[mld->v1];
        v2 = ld->v2 = &vertexes[mld->v2];
        ld->dx = v2->x - v1->x;
        ld->dxs = ld->dx >> FRACBITS;
        ld->dy = v2->y - v1->y;
        ld->dys = ld->dy >> FRACBITS;
        
        if (!ld->dx)
            ld->slopetype = ST_VERTICAL;
        else if (!ld->dy)
            ld->slopetype = ST_HORIZONTAL;
        else
        {
            if (FixedDiv(ld->dy, ld->dx) > 0)
                ld->slopetype = ST_POSITIVE;
            else
                ld->slopetype = ST_NEGATIVE;
        }

        if (v1->x < v2->x)
        {
            ld->bbox[BOXLEFT] = v1->x;
            ld->bbox[BOXRIGHT] = v2->x;
        }
        else
        {
            ld->bbox[BOXLEFT] = v2->x;
            ld->bbox[BOXRIGHT] = v1->x;
        }

        if (v1->y < v2->y)
        {
            ld->bbox[BOXBOTTOM] = v1->y;
            ld->bbox[BOXTOP] = v2->y;
        }
        else
        {
            ld->bbox[BOXBOTTOM] = v2->y;
            ld->bbox[BOXTOP] = v1->y;
        }

        ld->sidenum[0] = mld->sidenum[0];
        ld->sidenum[1] = mld->sidenum[1];

        if (ld->sidenum[0] != -1)
            ld->frontsector = sides[ld->sidenum[0]].sector;
        else
            ld->frontsector = 0;

        if (ld->sidenum[1] != -1)
            ld->backsector = sides[ld->sidenum[1]].sector;
        else
            ld->backsector = 0;
    }

    Z_Free(data);
}

//
// P_LoadSideDefs
//
void P_LoadSideDefs(int lump)
{
    byte *data;
    int i;
    mapsidedef_t *msd;
    side_t *sd;

    numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
    sides = Z_MallocUnowned(numsides * sizeof(side_t), PU_LEVEL);
    memset(sides, 0, numsides * sizeof(side_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    msd = (mapsidedef_t *)data;
    sd = sides;
    for (i = 0; i < numsides; i++, msd++, sd++)
    {
        sd->textureoffset = msd->textureoffset << FRACBITS;
        sd->rowoffset = msd->rowoffset << FRACBITS;
        sd->toptexture = R_TextureNumForName(msd->toptexture);
        sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
        sd->midtexture = R_TextureNumForName(msd->midtexture);
        sd->sector = &sectors[msd->sector];
    }

    Z_Free(data);
}

//
// P_LoadBlockMap
//
void P_LoadBlockMap(int lump)
{
    int count;
    int i;

    blockmaplump = W_CacheLumpNum(lump, PU_LEVEL);
    blockmap = blockmaplump + 4;

    bmaporgx = blockmaplump[0] << FRACBITS;
    bmaporgy = blockmaplump[1] << FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];

    // clear out mobj chains
    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = Z_MallocUnowned(count, PU_LEVEL);
    memset(blocklinks, 0, count);

    // LUT bmapwidth muls
    count = sizeof(int) * bmapheight;
    bmapwidthmuls = Z_MallocUnowned(count, PU_LEVEL);

    for (i = 0; i < bmapheight; i++){
        bmapwidthmuls[i] = i * bmapwidth;
    }
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines(void)
{
    line_t **linebuffer;
    int i;
    int j;
    int total;
    line_t *li;
    sector_t *sector;
    subsector_t *ss;
    seg_t *seg;
    fixed_t bbox[4];
    int block;

    // look up sector number for each subsector
    ss = subsectors;
    for (i = 0; i < numsubsectors; i++, ss++)
    {
        seg = &segs[ss->firstline];
        ss->sector = seg->sidedef->sector;
    }

    // count number of lines in each sector
    li = lines;
    total = numlines;
    for (i = 0; i < numlines; i++, li++)
    {
        li->frontsector->linecount++;

        if (li->backsector && li->backsector != li->frontsector)
        {
            li->backsector->linecount++;
            total++;
        }
    }

    // build line tables for each sector
    linebuffer = Z_MallocUnowned(total * 4, PU_LEVEL);
    sector = sectors;
    for (i = 0; i < numsectors; i++, sector++)
    {
        bbox[BOXTOP] = bbox[BOXRIGHT] = MININT;
        bbox[BOXBOTTOM] = bbox[BOXLEFT] = MAXINT;

        sector->lines = linebuffer;
        li = lines;
        for (j = 0; j < numlines; j++, li++)
        {
            if (li->frontsector == sector || li->backsector == sector)
            {
                *linebuffer++ = li;
                M_AddToBox(bbox, li->v1->x, li->v1->y);
                M_AddToBox(bbox, li->v2->x, li->v2->y);
            }
        }

        // set the degenmobj_t to the middle of the bounding box
        sector->soundorg.x = (bbox[BOXRIGHT] + bbox[BOXLEFT]) / 2;
        sector->soundorg.y = (bbox[BOXTOP] + bbox[BOXBOTTOM]) / 2;

        // adjust bounding box to map blocks
        block = (bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block > bmapheight - 1 ? bmapheight - 1 : block;
        sector->blockbox[BOXTOP] = block;

        block = (bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXBOTTOM] = block;

        block = (bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block > bmapwidth - 1 ? bmapwidth - 1 : block;
        sector->blockbox[BOXRIGHT] = block;

        block = (bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXLEFT] = block;
    }
}

//
// P_SetupLevel
//
void P_SetupLevel(int episode,
                  int map,
                  int playermask,
                  skill_t skill)
{
    int i;
    char lumpname[9];
    int lumpnum;

    totalkills = totalitems = totalsecret = 0;
    wminfo.partime = 180;

    players.killcount = players.secretcount = players.itemcount = 0;

    // Initial height of PointOfView
    // will be set by player think.
    players.viewz = 1;

    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start();
    S_ClearSounds();

    Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

    P_InitThinkers();

    // find map name
    if (gamemode == commercial)
    {
        if (map < 10)
            sprintf(lumpname, "map0%i", map);
        else
            sprintf(lumpname, "map%i", map);
    }
    else
    {
        lumpname[0] = 'E';
        lumpname[1] = '0' + episode;
        lumpname[2] = 'M';
        lumpname[3] = '0' + map;
        lumpname[4] = 0;
    }

    lumpnum = W_GetNumForName(lumpname);

    leveltime = 0;

    // note: most of this ordering is important
    P_LoadBlockMap(lumpnum + ML_BLOCKMAP);
    P_LoadVertexes(lumpnum + ML_VERTEXES);
    P_LoadSectors(lumpnum + ML_SECTORS);
    P_LoadSideDefs(lumpnum + ML_SIDEDEFS);

    P_LoadLineDefs(lumpnum + ML_LINEDEFS);
    P_LoadSubsectors(lumpnum + ML_SSECTORS);
    P_LoadNodes(lumpnum + ML_NODES);
    P_LoadSegs(lumpnum + ML_SEGS);

    rejectmatrix = W_CacheLumpNum(lumpnum + ML_REJECT, PU_LEVEL);
    P_GroupLines();

    P_LoadThings(lumpnum + ML_THINGS);

    // clear special respawning que
    iquehead = iquetail = 0;

    // set up world state
    P_SpawnSpecials();

    // preload graphics
    R_PrecacheLevel();
}

const char *sprnames[NUMSPRITES] = {
    "TROO", "SHTG", "PUNG", "PISG", "PISF", "SHTF", "SHT2", "CHGG", "CHGF", "MISG",
    "MISF", "SAWG", "PLSG", "PLSF", "BFGG", "BFGF", "BLUD", "PUFF", "BAL1", "BAL2",
    "PLSS", "PLSE", "MISL", "BFS1", "BFE1", "BFE2", "TFOG", "IFOG", "PLAY", "POSS",
    "SPOS", "VILE", "FIRE", "FATB", "FBXP", "SKEL", "MANF", "FATT", "CPOS", "SARG",
    "HEAD", "BAL7", "BOSS", "BOS2", "SKUL", "SPID", "BSPI", "APLS", "APBX", "CYBR",
    "PAIN", "SSWV", "KEEN", "BBRN", "BOSF", "ARM1", "ARM2", "BAR1", "BEXP", "FCAN",
    "BON1", "BON2", "BKEY", "RKEY", "YKEY", "BSKU", "RSKU", "YSKU", "STIM", "MEDI",
    "SOUL", "PINV", "PSTR", "PINS", "MEGA", "SUIT", "PMAP", "PVIS", "CLIP", "AMMO",
    "ROCK", "BROK", "CELL", "CELP", "SHEL", "SBOX", "BPAK", "BFUG", "MGUN", "CSAW",
    "LAUN", "PLAS", "SHOT", "SGN2", "COLU", "SMT2", "GOR1", "POL2", "POL5", "POL4",
    "POL3", "POL1", "POL6", "GOR2", "GOR3", "GOR4", "GOR5", "SMIT", "COL1", "COL2",
    "COL3", "COL4", "CAND", "CBRA", "COL6", "TRE1", "TRE2", "ELEC", "CEYE", "FSKU",
    "COL5", "TBLU", "TGRN", "TRED", "SMBT", "SMGT", "SMRT", "HDB1", "HDB2", "HDB3",
    "HDB4", "HDB5", "HDB6", "POB1", "POB2", "BRS1", "TLMP", "TLP2"};

//
// P_Init
//
void P_Init(void)
{
    P_InitSwitchList();
    P_InitPicAnims();
    R_InitSprites(sprnames);
}
