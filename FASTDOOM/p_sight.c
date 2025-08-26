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
//	LineOfSight/Visibility checks, uses REJECT Lookup Table.
//

#include <stdlib.h>
#include "options.h"
#include "doomdef.h"

#include "i_debug.h"
#include "i_system.h"
#include "p_local.h"

// State.
#include "r_state.h"

#include "std_func.h"

#include "i_debug.h"

//
// P_CheckSight
//
fixed_t sightzstart; // eye z of looker
fixed_t topslope;
fixed_t bottomslope; // slopes to top and bottom of target

divline_t strace; // from t1 to t2
fixed_t t2x;
fixed_t t2y;

//
// P_CrossSubsector
// Returns true
//  if strace crosses the given subsector successfully.
//
byte P_CrossSubsector(int num)
{
    seg_t *seg;
    line_t *line;
    int s1;
    int s2;
    int count;
    subsector_t *sub;
    sector_t *front;
    sector_t *back;
    fixed_t opentop;
    fixed_t openbottom;
    vertex_t *v1;
    vertex_t *v2;
    fixed_t frac, absfrac;
    fixed_t slope;

    fixed_t opt;
    fixed_t left, right;

    sub = &subsectors[num];

    // check lines
    count = sub->numlines;
    seg = &segs[sub->firstline];

    for (; count; seg++, count--)
    {
        fixed_t numIV;
        fixed_t denIV = 0;

        line = seg->linedef;

        // allready checked other side?
        if (line->validcount == validcount)
            continue;

        line->validcount = validcount;

        v1 = line->v1;
        v2 = line->v2;

        switch (strace.type)
        {
        case 0:
            s1 = v1->x == strace.x ? 2 : v1->x <= strace.x ? strace.dy > 0
                                                           : strace.dy < 0;
            s2 = v2->x == strace.x ? 2 : v2->x <= strace.x ? strace.dy > 0
                                                           : strace.dy < 0;
            break;
        case 1:
            s1 = v1->x == strace.y ? 2 : v1->y <= strace.y ? strace.dx < 0
                                                           : strace.dx > 0;
            s2 = v2->x == strace.y ? 2 : v2->y <= strace.y ? strace.dx < 0
                                                           : strace.dx > 0;
            break;
        default:
            s1 = (right = ((v1->y - strace.y) >> FRACBITS) * (strace.dx >> FRACBITS)) < (left = ((v1->x - strace.x) >> FRACBITS) * (strace.dy >> FRACBITS)) ? 0
                 : right == left                                                                                                                            ? 2
                                                                                                                                                            : 1;
            s2 = (right = ((v2->y - strace.y) >> FRACBITS) * (strace.dx >> FRACBITS)) < (left = ((v2->x - strace.x) >> FRACBITS) * (strace.dy >> FRACBITS)) ? 0
                 : right == left                                                                                                                            ? 2
                                                                                                                                                            : 1;
            break;
        }

        // line isn't crossed?
        if (s1 == s2)
            continue;

        switch (line->slopetype)
        {
        case ST_VERTICAL:
            s1 = strace.x == v1->x ? 2 : strace.x <= v1->x ? line->dy > 0
                                                           : line->dy < 0;
            s2 = t2x == v1->x ? 2 : t2x <= v1->x ? line->dy > 0
                                                 : line->dy < 0;
            break;
        case ST_HORIZONTAL:
            s1 = strace.x == v1->y ? 2 : strace.y <= v1->y ? line->dx < 0
                                                           : line->dx > 0;
            s2 = t2x == v1->y ? 2 : t2y <= v1->y ? line->dx < 0
                                                 : line->dx > 0;
            break;
        default:
            s1 = (right = ((strace.y - v1->y) >> FRACBITS) * (line->dxs)) < (left = ((strace.x - v1->x) >> FRACBITS) * (line->dys)) ? 0
                 : right == left                                                                                                    ? 2
                                                                                                                                    : 1;
            s2 = (right = ((t2y - v1->y) >> FRACBITS) * (line->dxs)) < (left = ((t2x - v1->x) >> FRACBITS) * (line->dys)) ? 0
                 : right == left                                                                                          ? 2
                                                                                                                          : 1;
            break;
        }

        // line isn't crossed?
        if (s1 == s2)
            continue;

        // stop because it is not two sided anyway
        // might do this after updating validcount?
        if (!(line->twoSided))
            return 0;

        // crosses a two sided line
        front = seg->frontsector;
        back = seg->backsector;

        // no wall to block sight with?
        if (front->floorheight == back->floorheight && front->ceilingheight == back->ceilingheight)
            continue;

        // possible occluder
        // because of ceiling height differences
        if (front->ceilingheight < back->ceilingheight)
            opentop = front->ceilingheight;
        else
            opentop = back->ceilingheight;

        // because of ceiling height differences
        if (front->floorheight > back->floorheight)
            openbottom = front->floorheight;
        else
            openbottom = back->floorheight;

        // quick test for totally closed doors
        if (openbottom >= opentop)
            return 0; // stop

        // P_InterceptVector2
        switch(line->optimization) {
            case OPT_BOTH:
                numIV = FixedMulEDX((v1->x - strace.x) >> 8, line->dy) + FixedMulEDX((strace.y - v1->y) >> 8, line->dx);
            break;
            case OPT_ONLY_DX:
                numIV = FixedMulEDX((strace.y - v1->y) >> 8, line->dx);
            break;
            case OPT_ONLY_DY:
                numIV = FixedMulEDX((v1->x - strace.x) >> 8, line->dy);
            break;
            default:
                numIV = 0;
            break;
        }
        
        if (numIV == 0)
        {
            frac = 0;
            absfrac = 0;
        }
        else
        {
            if (line->dy != 0) {
                denIV += FixedMulEDX(line->dy >> 8, strace.dx);
            }

            if (line->dx != 0) {
                denIV -= FixedMulEDX(line->dx >> 8, strace.dy);
            }
            
            if (denIV == 0)
            {
                frac = 0;
                absfrac = 0;
            }
            else
            {
                frac = FixedDiv(numIV, denIV);
                absfrac = abs(frac);
            }
        }

        if (front->floorheight != back->floorheight)
        {
            opt = openbottom - sightzstart;
            // slope = FixedDiv(openbottom - sightzstart, frac);
            slope = ((abs(opt) >> 14) >= absfrac) ? ((opt ^ frac) >> 31) ^ MAXINT : FixedDiv2(opt, frac);
            if (bottomslope < slope)
                bottomslope = slope;
        }

        if (front->ceilingheight != back->ceilingheight)
        {
            opt = opentop - sightzstart;
            // slope = FixedDiv(opentop - sightzstart, frac);
            slope = ((abs(opt) >> 14) >= absfrac) ? ((opt ^ frac) >> 31) ^ MAXINT : FixedDiv2(opt, frac);
            if (topslope > slope)
                topslope = slope;
        }

        if (topslope <= bottomslope)
            return 0; // stop
    }
    // passed the subsector ok
    return 1;
}

//
// P_CrossBSPNode
// Returns true
//  if strace crosses the given node successfully.
//
byte P_CrossBSPNode(int bspnum)
{
    node_t *bsp;
    byte side;
    fixed_t left, right;
    byte calc_side;

    if (bspnum & NF_SUBSECTOR)
    {
        if (bspnum == -1)
            return P_CrossSubsector(0);
        else
            return P_CrossSubsector(bspnum & (~NF_SUBSECTOR));
    }

    bsp = &nodes[bspnum];

    // decide which side the start point is on
    switch (bsp->type)
    {
    case 0:
        side = strace.x == bsp->x ? 0 : strace.x <= bsp->x ? bsp->dy > 0
                                                           : bsp->dy < 0;
        break;
    case 1:
        side = strace.x == bsp->y ? 0 : strace.y <= bsp->y ? bsp->dx < 0
                                                           : bsp->dx > 0;
        break;
    default:
        side = (((strace.y - bsp->y) >> FRACBITS) * (bsp->dxs)) > (((strace.x - bsp->x) >> FRACBITS) * (bsp->dys));
        break;
    }

    // cross the starting side
    if (!P_CrossBSPNode(bsp->children[side]))
        return 0;

    switch (bsp->type)
    {
    case 0:
        calc_side = t2x == bsp->x ? 2 : t2x <= bsp->x ? bsp->dy > 0
                                                      : bsp->dy < 0;
        break;
    case 1:
        calc_side = t2x == bsp->y ? 2 : t2y <= bsp->y ? bsp->dx < 0
                                                      : bsp->dx > 0;
        break;
    default:
        calc_side = (right = ((t2y - bsp->y) >> FRACBITS) * (bsp->dxs)) < (left = ((t2x - bsp->x) >> FRACBITS) * (bsp->dys)) ? 0
                    : right == left                                                                                          ? 2
                                                                                                                             : 1;
        break;
    }

    // the partition plane is crossed here
    if (side == calc_side)
    {
        // the line doesn't touch the other side
        return 1;
    }

    // cross the ending side
    return P_CrossBSPNode(bsp->children[side ^ 1]);
}

//
// P_CheckSight
// Returns true
//  if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
byte P_CheckSight(mobj_t *t1, mobj_t *t2)
{
    int s1;
    int s2;
    int pnum;
    int bytenum;
    int bitnum;

    // First check for trivial rejection.

    // Determine subsector entries in REJECT table.
    // NOTE if the size of sector_t changes, this must be changed.
    ASSERT(sizeof(sector_t) == 128);
    pnum = Div128((int)t1->subsector->sector - (int)sectors);
    pnum *= numsectors;
    pnum += Div128((int)t2->subsector->sector - (int)sectors);
    bytenum = pnum >> 3;
    bitnum = 1 << (pnum & 7);

    // Check in REJECT table.
    if (rejectmatrix[bytenum] & bitnum)
    {
        // can't possibly be connected
        return 0;
    }

    // An unobstructed LOS is possible.
    // Now look from eyes of t1 to any part of t2.

    validcount++;

    sightzstart = t1->z + t1->height - (t1->height >> 2);
    topslope = (t2->z + t2->height) - sightzstart;
    bottomslope = (t2->z) - sightzstart;

    strace.x = t1->x;
    t2x = t2->x;
    strace.dx = t2x - t1->x;
    strace.y = t1->y;
    t2y = t2->y;
    strace.dy = t2y - t1->y;

    if (!strace.dx)
        strace.type = 0;
    else if (!strace.dx)
        strace.type = 1;
    else
        strace.type = 2;

    // the head node is the last node output
    return P_CrossBSPNode(firstnode);
}
