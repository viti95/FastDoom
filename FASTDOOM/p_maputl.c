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
//	Movement/collision utility functions,
//	as used by function in p_map.c.
//	BLOCKMAP Iterator functions,
//	and some PIT_* functions to use for iteration.
//

#include <stdlib.h>
#include "options.h"
#include "m_misc.h"

#include "doomdef.h"
#include "p_local.h"

// State.
#include "r_state.h"

#include "std_func.h"

//
// P_AproxDistance
// Gives an estimation of distance (not exact)
//

fixed_t P_AproxDistance(fixed_t dx, fixed_t dy)
{
    dx = abs(dx);
    dy = abs(dy);

    return dx + dy - ((dx < dy ? dx : dy) >> 1);
}

//
// P_PointOnLineSide
// Returns 0 or 1
//
byte P_PointOnLineSide(fixed_t x, fixed_t y, line_t *line)
{
    fixed_t dx;
    fixed_t dy;
    fixed_t left;
    fixed_t right;

    switch (line->slopetype)
    {
    case ST_VERTICAL:
        return (x <= line->v1->x) ^ (line->dy <= 0);
    case ST_HORIZONTAL:
        return (y <= line->v1->y) ^ (line->dx >= 0);
    default:
        dx = (x - line->v1->x);
        dy = (y - line->v1->y);

        left = FixedMulEDX(line->dys, dx);
        right = FixedMulEDX(dy, line->dxs);

        return right >= left;
    }
}

//
// P_PointOnDivlineSide
// Returns 0 or 1.
//
byte P_PointOnDivlineSide(fixed_t x,
                          fixed_t y,
                          divline_t *line)
{
    fixed_t dx;
    fixed_t dy;
    fixed_t left;
    fixed_t right;

    switch (line->type)
    {
    case 0:
        return (x <= line->x) ? (line->dy > 0) : (line->dy < 0);

    case 1:
        return (y <= line->y) ? (line->dx < 0) : (line->dx > 0);

    default:
        dx = (x - line->x);
        dy = (y - line->y);

        // try to quickly decide by looking at sign bits
        if ((line->dy ^ line->dx ^ dx ^ dy) & 0x80000000)
            return ROLAND1(line->dy ^ dx);

        left = FixedMulEDX(line->dy >> 8, dx >> 8);
        right = FixedMulEDX(dy >> 8, line->dx >> 8);

        return right >= left;
    }
}

//
// P_InterceptVector
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
fixed_t
P_InterceptVector(divline_t *v2,
                  divline_t *v1)
{
    fixed_t frac;
    fixed_t num;
    fixed_t den;

    num = FixedMulEDX((v1->x - v2->x) >> 8, v1->dy) + FixedMulEDX((v2->y - v1->y) >> 8, v1->dx);

    if (num == 0)
    {
        return 0;
    }

    den = FixedMulEDX(v2->dx, v1->dy >> 8) - FixedMulEDX(v2->dy, v1->dx >> 8);

    if (den == 0)
    {
        return 0;
    }

    frac = FixedDiv(num, den);

    return frac;
}

fixed_t
P_InterceptVector2(divline_t *v2,
                   line_t *v1)
{
    fixed_t frac;
    fixed_t num;
    fixed_t den;

    num = FixedMulEDX((v1->v1->x - v2->x) >> 8, v1->dy) + FixedMulEDX((v2->y - v1->v1->y) >> 8, v1->dx);

    if (num == 0)
    {
        return 0;
    }

    den = FixedMulEDX(v2->dx, v1->dy >> 8) - FixedMulEDX(v2->dy, v1->dx >> 8);

    if (den == 0)
    {
        return 0;
    }

    frac = FixedDiv(num, den);

    return frac;
}

//
// P_LineOpening
// Sets opentop and openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//
fixed_t opentop;
fixed_t openbottom;
fixed_t openrange;
fixed_t lowfloor;

void P_LineOpening(line_t *linedef)
{
    sector_t *front;
    sector_t *back;

    if (linedef->sidenum[1] == -1)
    {
        // single sided line
        openrange = 0;
        return;
    }

    front = linedef->frontsector;
    back = linedef->backsector;

    if (front->ceilingheight < back->ceilingheight)
        opentop = front->ceilingheight;
    else
        opentop = back->ceilingheight;

    if (front->floorheight > back->floorheight)
    {
        openbottom = front->floorheight;
        lowfloor = back->floorheight;
    }
    else
    {
        openbottom = back->floorheight;
        lowfloor = front->floorheight;
    }

    openrange = opentop - openbottom;
}

//
// THING POSITION SETTING
//

//
// P_UnsetThingPosition
// Unlinks a thing from block map and sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//
void P_UnsetThingPosition(mobj_t *thing)
{
    int blockx;
    int blocky;

    if (!(thing->flags & MF_NOSECTOR))
    {
        // inert things don't need to be in blockmap?
        // unlink from subsector
        if (thing->snext)
            thing->snext->sprev = thing->sprev;

        if (thing->sprev)
            thing->sprev->snext = thing->snext;
        else
            thing->subsector->sector->thinglist = thing->snext;
    }

    if (!(thing->flags & MF_NOBLOCKMAP))
    {
        // inert things don't need to be in blockmap
        // unlink from block map
        if (thing->bnext)
            thing->bnext->bprev = thing->bprev;

        if (thing->bprev)
            thing->bprev->bnext = thing->bnext;
        else
        {
            blockx = (thing->x - bmaporgx) >> MAPBLOCKSHIFT;
            blocky = (thing->y - bmaporgy) >> MAPBLOCKSHIFT;

            if (blockx >= 0 && blockx < bmapwidth && blocky >= 0 && blocky < bmapheight)
            {
                blocklinks[bmapwidthmuls[blocky] + blockx] = thing->bnext;
            }
        }
    }
}

//
// P_SetThingPosition
// Links a thing into both a block and a subsector
// based on it's x y.
// Sets thing->subsector properly
//
void P_SetThingPositionSubsector(mobj_t *thing, subsector_t *ss)
{
    sector_t *sec;
    int blockx;
    int blocky;
    mobj_t **link;

    // link into subsector
    thing->subsector = ss;

    if (!(thing->flags & MF_NOSECTOR))
    {
        // invisible things don't go into the sector links
        sec = ss->sector;

        thing->sprev = NULL;
        thing->snext = sec->thinglist;

        if (sec->thinglist)
            sec->thinglist->sprev = thing;

        sec->thinglist = thing;
    }

    // link into blockmap
    if (!(thing->flags & MF_NOBLOCKMAP))
    {
        // inert things don't need to be in blockmap
        blockx = (thing->x - bmaporgx) >> MAPBLOCKSHIFT;
        blocky = (thing->y - bmaporgy) >> MAPBLOCKSHIFT;

        if (blockx >= 0 && blockx < bmapwidth && blocky >= 0 && blocky < bmapheight)
        {
            link = &blocklinks[bmapwidthmuls[blocky] + blockx];
            thing->bprev = NULL;
            thing->bnext = *link;
            if (*link)
                (*link)->bprev = thing;

            *link = thing;
        }
        else
        {
            // thing is off the map
            thing->bnext = thing->bprev = NULL;
        }
    }
}

void P_SetThingPosition(mobj_t *thing)
{
    P_SetThingPositionSubsector(thing, R_PointInSubsector(thing->x, thing->y));
}

//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//

//
// P_BlockLinesIterator
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
byte P_NotBlockLinesIterator(int x, int y, byte (*func)(line_t *))
{
    int offset;
    short *list;
    line_t *ld;

    if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
        return 0;

    offset = bmapwidthmuls[y] + x;

    offset = *(blockmap + offset);

    for (list = blockmaplump + offset; *list != -1; list++)
    {
        ld = &lines[*list];

        if (ld->validcount == validcount)
            continue; // line has already been checked

        ld->validcount = validcount;

        if (!func(ld))
            return 1;
    }
    return 0; // everything was checked
}

//
// P_BlockThingsIterator
//
byte P_NotBlockThingsIterator(int x, int y, byte (*func)(mobj_t *))
{
    mobj_t *mobj;

    if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
        return 0;

    for (mobj = blocklinks[bmapwidthmuls[y] + x]; mobj; mobj = mobj->bnext)
    {
        if (!func(mobj))
            return 1;
    }
    return 0;
}

byte P_NotBlockThingsIterator2(int x, int y, byte (*func)(mobj_t *))
{
    mobj_t *mobj;

    for (mobj = blocklinks[bmapwidthmuls[y] + x]; mobj; mobj = mobj->bnext)
    {
        if (!func(mobj))
            return 1;
    }
    return 0;
}

//
// INTERCEPT ROUTINES
//
intercept_t intercepts[MAXINTERCEPTS];
intercept_t *intercept_p;

divline_t trace;

//
// PIT_AddLineIntercepts.
// Looks for lines in the given block
// that intercept the given trace
// to add to the intercepts list.
//
// A line is crossed if its endpoints
// are on opposite sides of the trace.
// Returns true if earlyout and a solid line hit.
//
byte PIT_AddLineIntercepts(line_t *ld)
{
    byte s1;
    byte s2;
    fixed_t frac;

    // avoid precision problems with two routines
    if (trace.dx > FRACUNIT * 16 || trace.dy > FRACUNIT * 16 || trace.dx < -FRACUNIT * 16 || trace.dy < -FRACUNIT * 16)
    {
        s1 = P_PointOnDivlineSide(ld->v1->x, ld->v1->y, &trace);
        s2 = P_PointOnDivlineSide(ld->v2->x, ld->v2->y, &trace);
    }
    else
    {
        s1 = P_PointOnLineSide(trace.x, trace.y, ld);
        s2 = P_PointOnLineSide(trace.x + trace.dx, trace.y + trace.dy, ld);
    }

    if (s1 == s2)
        return 1; // line isn't crossed

    // hit the line
    // P_MakeDivline
    frac = P_InterceptVector2(&trace, ld);

    if (frac < 0)
        return 1; // behind source

    intercept_p->frac = frac;
    intercept_p->isaline = 1;
    intercept_p->d.line = ld;
    intercept_p++;

    return 1; // continue
}

//
// PIT_AddThingIntercepts
//
byte PIT_AddThingIntercepts(mobj_t *thing)
{
    fixed_t x1;
    fixed_t y1;
    fixed_t x2;
    fixed_t y2;

    byte s1;
    byte s2;

    byte tracepositive;

    divline_t dl;

    fixed_t frac;

    if (!((thing->flags & MF_SHOOTABLE)))
        return 1; // not shootable

    tracepositive = (trace.dx ^ trace.dy) > 0;

    // check a corner to corner crossection for hit
    if (tracepositive)
    {
        y1 = thing->y + thing->radius;
        y2 = thing->y - thing->radius;
    }
    else
    {
        y1 = thing->y - thing->radius;
        y2 = thing->y + thing->radius;
    }

    x1 = thing->x - thing->radius;
    x2 = thing->x + thing->radius;
    s1 = P_PointOnDivlineSide(x1, y1, &trace);
    s2 = P_PointOnDivlineSide(x2, y2, &trace);

    if (s1 == s2)
        return 1; // line isn't crossed

    dl.x = x1;
    dl.y = y1;
    dl.dx = x2 - x1;
    dl.dy = y2 - y1;

    frac = P_InterceptVector(&trace, &dl);

    if (frac < 0)
        return 1; // behind source

    intercept_p->frac = frac;
    intercept_p->isaline = 0;
    intercept_p->d.thing = thing;
    intercept_p++;

    return 1; // keep going
}

//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all lines.
//
void P_TraverseIntercepts(traverser_t func)
{
    int count;
    count = intercept_p - intercepts;

    while (count--)
    {
        fixed_t dist = MAXINT;
        intercept_t *scan;
        intercept_t *in;

        for (scan = intercepts; scan < intercept_p; scan++)
        {
            if (scan->frac < dist)
            {
                dist = scan->frac;
                in = scan;
            }
        }

        if (dist > FRACUNIT || !func(in))
            return; // checked everything in range, don't bother going farther

        in->frac = MAXINT;
    }

    return; // everything was traversed
}

//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all lines.
//
void P_PathTraverseLI(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
    fixed_t xt1;
    fixed_t yt1;
    fixed_t xt2;
    fixed_t yt2;

    fixed_t xstep;
    fixed_t ystep;

    fixed_t partial;

    fixed_t xintercept;
    fixed_t yintercept;

    fixed_t opt1, opt2;

    int mapx;
    int mapy;

    int mapxstep;
    int mapystep;

    int count;

    validcount++;
    intercept_p = intercepts;

    if (((x1 - bmaporgx) & (MAPBLOCKSIZE - 1)) == 0)
        x1 += FRACUNIT; // don't side exactly on a line

    if (((y1 - bmaporgy) & (MAPBLOCKSIZE - 1)) == 0)
        y1 += FRACUNIT; // don't side exactly on a line

    trace.x = x1;
    trace.y = y1;
    trace.dx = x2 - x1;
    trace.dy = y2 - y1;

    if (!trace.dx)
        trace.type = 0;
    else if (!trace.dy)
        trace.type = 1;
    else
        trace.type = 2;

    x1 -= bmaporgx;
    y1 -= bmaporgy;
    xt1 = x1 >> MAPBLOCKSHIFT;
    yt1 = y1 >> MAPBLOCKSHIFT;

    x2 -= bmaporgx;
    y2 -= bmaporgy;
    xt2 = x2 >> MAPBLOCKSHIFT;
    yt2 = y2 >> MAPBLOCKSHIFT;

    if (xt2 > xt1)
    {
        mapxstep = 1;
        partial = FRACUNIT - ((x1 >> MAPBTOFRAC) & (FRACUNIT - 1));
        opt1 = y2 - y1;
        opt2 = abs(x2 - x1);
        // ystep = FixedDiv(y2 - y1, abs(x2 - x1));
        ystep = ((abs(opt1) >> 14) >= opt2) ? ((opt1 ^ opt2) >> 31) ^ MAXINT : FixedDiv2(opt1, opt2);
    }
    else if (xt2 < xt1)
    {
        mapxstep = -1;
        partial = (x1 >> MAPBTOFRAC) & (FRACUNIT - 1);
        opt1 = y2 - y1;
        opt2 = abs(x2 - x1);
        // ystep = FixedDiv(y2 - y1, abs(x2 - x1));
        ystep = ((abs(opt1) >> 14) >= opt2) ? ((opt1 ^ opt2) >> 31) ^ MAXINT : FixedDiv2(opt1, opt2);
    }
    else
    {
        mapxstep = 0;
        partial = FRACUNIT;
        ystep = 256 * FRACUNIT;
    }

    yintercept = (y1 >> MAPBTOFRAC) + FixedMul(partial, ystep);

    if (yt2 > yt1)
    {
        mapystep = 1;
        partial = FRACUNIT - ((y1 >> MAPBTOFRAC) & (FRACUNIT - 1));
        opt1 = x2 - x1;
        opt2 = abs(y2 - y1);
        // xstep = FixedDiv(x2 - x1, abs(y2 - y1));
        xstep = ((abs(opt1) >> 14) >= opt2) ? ((opt1 ^ opt2) >> 31) ^ MAXINT : FixedDiv2(opt1, opt2);
    }
    else if (yt2 < yt1)
    {
        mapystep = -1;
        partial = (y1 >> MAPBTOFRAC) & (FRACUNIT - 1);
        opt1 = x2 - x1;
        opt2 = abs(y2 - y1);
        // xstep = FixedDiv(x2 - x1, abs(y2 - y1));
        xstep = ((abs(opt1) >> 14) >= opt2) ? ((opt1 ^ opt2) >> 31) ^ MAXINT : FixedDiv2(opt1, opt2);
    }
    else
    {
        mapystep = 0;
        partial = FRACUNIT;
        xstep = 256 * FRACUNIT;
    }
    xintercept = (x1 >> MAPBTOFRAC) + FixedMul(partial, xstep);

    // Step through map blocks.
    // Count is present to prevent a round off error
    // from skipping the break.
    mapx = xt1;
    mapy = yt1;

    for (count = 0; count < 64; count++)
    {
        if (P_NotBlockLinesIterator(mapx, mapy, PIT_AddLineIntercepts))
        {
            return; // early out
        }

        if (mapx == xt2 && mapy == yt2)
        {
            break;
        }

        if ((yintercept >> FRACBITS) == mapy)
        {
            yintercept += ystep;
            mapx += mapxstep;
        }
        else if ((xintercept >> FRACBITS) == mapx)
        {
            xintercept += xstep;
            mapy += mapystep;
        }
    }
    return;
}

void P_PathTraverseLITH(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
    fixed_t xt1;
    fixed_t yt1;
    fixed_t xt2;
    fixed_t yt2;

    fixed_t xstep;
    fixed_t ystep;

    fixed_t partial;

    fixed_t xintercept;
    fixed_t yintercept;

    fixed_t opt1, opt2;

    int mapx;
    int mapy;

    int mapxstep;
    int mapystep;

    int count;

    validcount++;
    intercept_p = intercepts;

    if (((x1 - bmaporgx) & (MAPBLOCKSIZE - 1)) == 0)
        x1 += FRACUNIT; // don't side exactly on a line

    if (((y1 - bmaporgy) & (MAPBLOCKSIZE - 1)) == 0)
        y1 += FRACUNIT; // don't side exactly on a line

    trace.x = x1;
    trace.y = y1;
    trace.dx = x2 - x1;
    trace.dy = y2 - y1;

    if (!trace.dx)
        trace.type = 0;
    else if (!trace.dy)
        trace.type = 1;
    else
        trace.type = 2;

    x1 -= bmaporgx;
    y1 -= bmaporgy;
    xt1 = x1 >> MAPBLOCKSHIFT;
    yt1 = y1 >> MAPBLOCKSHIFT;

    x2 -= bmaporgx;
    y2 -= bmaporgy;
    xt2 = x2 >> MAPBLOCKSHIFT;
    yt2 = y2 >> MAPBLOCKSHIFT;

    if (xt2 > xt1)
    {
        mapxstep = 1;
        partial = FRACUNIT - ((x1 >> MAPBTOFRAC) & (FRACUNIT - 1));
        opt1 = y2 - y1;
        opt2 = abs(x2 - x1);
        // ystep = FixedDiv(y2 - y1, abs(x2 - x1));
        ystep = ((abs(opt1) >> 14) >= opt2) ? ((opt1 ^ opt2) >> 31) ^ MAXINT : FixedDiv2(opt1, opt2);
    }
    else if (xt2 < xt1)
    {
        mapxstep = -1;
        partial = (x1 >> MAPBTOFRAC) & (FRACUNIT - 1);
        opt1 = y2 - y1;
        opt2 = abs(x2 - x1);
        // ystep = FixedDiv(y2 - y1, abs(x2 - x1));
        ystep = ((abs(opt1) >> 14) >= opt2) ? ((opt1 ^ opt2) >> 31) ^ MAXINT : FixedDiv2(opt1, opt2);
    }
    else
    {
        mapxstep = 0;
        partial = FRACUNIT;
        ystep = 256 * FRACUNIT;
    }

    yintercept = (y1 >> MAPBTOFRAC) + FixedMul(partial, ystep);

    if (yt2 > yt1)
    {
        mapystep = 1;
        partial = FRACUNIT - ((y1 >> MAPBTOFRAC) & (FRACUNIT - 1));
        opt1 = x2 - x1;
        opt2 = abs(y2 - y1);
        // xstep = FixedDiv(x2 - x1, abs(y2 - y1));
        xstep = ((abs(opt1) >> 14) >= opt2) ? ((opt1 ^ opt2) >> 31) ^ MAXINT : FixedDiv2(opt1, opt2);
    }
    else if (yt2 < yt1)
    {
        mapystep = -1;
        partial = (y1 >> MAPBTOFRAC) & (FRACUNIT - 1);
        opt1 = x2 - x1;
        opt2 = abs(y2 - y1);
        // xstep = FixedDiv(x2 - x1, abs(y2 - y1));
        xstep = ((abs(opt1) >> 14) >= opt2) ? ((opt1 ^ opt2) >> 31) ^ MAXINT : FixedDiv2(opt1, opt2);
    }
    else
    {
        mapystep = 0;
        partial = FRACUNIT;
        xstep = 256 * FRACUNIT;
    }
    xintercept = (x1 >> MAPBTOFRAC) + FixedMul(partial, xstep);

    // Step through map blocks.
    // Count is present to prevent a round off error
    // from skipping the break.
    mapx = xt1;
    mapy = yt1;

    for (count = 0; count < 64; count++)
    {
        if (P_NotBlockLinesIterator(mapx, mapy, PIT_AddLineIntercepts) || P_NotBlockThingsIterator(mapx, mapy, PIT_AddThingIntercepts))
        {
            return; // early out
        }

        if (mapx == xt2 && mapy == yt2)
        {
            break;
        }

        if ((yintercept >> FRACBITS) == mapy)
        {
            yintercept += ystep;
            mapx += mapxstep;
        }
        else if ((xintercept >> FRACBITS) == mapx)
        {
            xintercept += xstep;
            mapy += mapystep;
        }
    }
    return;
}
