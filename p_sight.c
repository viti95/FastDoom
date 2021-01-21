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

#include "doomdef.h"

#include "i_system.h"
#include "p_local.h"

// State.
#include "r_state.h"

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
    divline_t divl;
    vertex_t *v1;
    vertex_t *v2;
    fixed_t frac;
    fixed_t slope;

    fixed_t opt;

    fixed_t numIV;
    fixed_t denIV;

    fixed_t left, right;

    sub = &subsectors[num];

    // check lines
    count = sub->numlines;
    seg = &segs[sub->firstline];

    for (; count; seg++, count--)
    {
        line = seg->linedef;

        // allready checked other side?
        if (line->validcount == validcount)
            continue;

        line->validcount = validcount;

        v1 = line->v1;
        v2 = line->v2;

        s1 = !strace.dx ? v1->x == strace.x ? 2 : v1->x <= strace.x ? strace.dy > 0 : strace.dy < 0 : !strace.dy ? v1->x == strace.y ? 2 : v1->y <= strace.y ? strace.dx < 0 : strace.dx > 0 : (right = ((v1->y - strace.y) >> FRACBITS) * (strace.dx >> FRACBITS)) < (left = ((v1->x - strace.x) >> FRACBITS) * (strace.dy >> FRACBITS)) ? 0 : right == left ? 2 : 1;
        s2 = !strace.dx ? v2->x == strace.x ? 2 : v2->x <= strace.x ? strace.dy > 0 : strace.dy < 0 : !strace.dy ? v2->x == strace.y ? 2 : v2->y <= strace.y ? strace.dx < 0 : strace.dx > 0 : (right = ((v2->y - strace.y) >> FRACBITS) * (strace.dx >> FRACBITS)) < (left = ((v2->x - strace.x) >> FRACBITS) * (strace.dy >> FRACBITS)) ? 0 : right == left ? 2 : 1;

        // line isn't crossed?
        if (s1 == s2)
            continue;

        divl.x = v1->x;
        divl.y = v1->y;
        divl.dx = v2->x - v1->x;
        divl.dy = v2->y - v1->y;
        
        s1 = !divl.dx ? strace.x == divl.x ? 2 : strace.x <= divl.x ? divl.dy > 0 : divl.dy < 0 : !divl.dy ? strace.x == divl.y ? 2 : strace.y <= divl.y ? divl.dx < 0 : divl.dx > 0 : (right = ((strace.y - divl.y) >> FRACBITS) * (divl.dx >> FRACBITS)) < (left = ((strace.x - divl.x) >> FRACBITS) * (divl.dy >> FRACBITS)) ? 0 : right == left ? 2 : 1;
        s2 = !divl.dx ? t2x == divl.x ? 2 : t2x <= divl.x ? divl.dy > 0 : divl.dy < 0 : !divl.dy ? t2x == divl.y ? 2 : t2y <= divl.y ? divl.dx < 0 : divl.dx > 0 : (right = ((t2y - divl.y) >> FRACBITS) * (divl.dx >> FRACBITS)) < (left = ((t2x - divl.x) >> FRACBITS) * (divl.dy >> FRACBITS)) ? 0 : right == left ? 2 : 1;

        // line isn't crossed?
        if (s1 == s2)
            continue;

        // stop because it is not two sided anyway
        // might do this after updating validcount?
        if (!(line->flags & ML_TWOSIDED))
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
        denIV = FixedMul(divl.dy >> 8, strace.dx) - FixedMul(divl.dx >> 8, strace.dy);

        if (denIV == 0)
        {
            frac = 0;
        }
        else
        {
            numIV = FixedMul((divl.x - strace.x) >> 8, divl.dy) + FixedMul((strace.y - divl.y) >> 8, divl.dx);
            frac = FixedDiv(numIV, denIV);
        }

        if (front->floorheight != back->floorheight)
        {
            opt = openbottom - sightzstart;
            //slope = FixedDiv(openbottom - sightzstart, frac);
            slope = ((abs(opt) >> 14) >= abs(frac)) ? ((opt ^ frac) >> 31) ^ MAXINT : FixedDiv2(opt, frac);
            if (bottomslope < slope)
                bottomslope = slope;
        }

        if (front->ceilingheight != back->ceilingheight)
        {
            opt = opentop - sightzstart;
            //slope = FixedDiv(opentop - sightzstart, frac);
            slope = ((abs(opt) >> 14) >= abs(frac)) ? ((opt ^ frac) >> 31) ^ MAXINT : FixedDiv2(opt, frac);
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
    int side;
    fixed_t left, right;
    int calc_side;

    if (bspnum & NF_SUBSECTOR)
    {
        if (bspnum == -1)
            return P_CrossSubsector(0);
        else
            return P_CrossSubsector(bspnum & (~NF_SUBSECTOR));
    }

    bsp = &nodes[bspnum];

    // decide which side the start point is on
    side = !bsp->dx ? strace.x == bsp->x ? 0 : strace.x <= bsp->x ? bsp->dy > 0 : bsp->dy < 0 : !bsp->dy ? strace.x == bsp->y ? 0 : strace.y <= bsp->y ? bsp->dx < 0 : bsp->dx > 0 : (right = ((strace.y - bsp->y) >> FRACBITS) * (bsp->dx >> FRACBITS)) < (left = ((strace.x - bsp->x) >> FRACBITS) * (bsp->dy >> FRACBITS)) ? 0 : right == left ? 0 : 1;

    // cross the starting side
    if (!P_CrossBSPNode(bsp->children[side]))
        return 0;

    calc_side = !bsp->dx ? t2x == bsp->x ? 2 : t2x <= bsp->x ? bsp->dy > 0 : bsp->dy < 0 : !bsp->dy ? t2x == bsp->y ? 2 : t2y <= bsp->y ? bsp->dx < 0 : bsp->dx > 0 : (right = ((t2y - bsp->y) >> FRACBITS) * (bsp->dx >> FRACBITS)) < (left = ((t2x - bsp->x) >> FRACBITS) * (bsp->dy >> FRACBITS)) ? 0 : right == left ? 2 : 1;

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
    pnum = Div96((int)t1->subsector->sector - (int)sectors);
    pnum *= numsectors;
    pnum += Div96((int)t2->subsector->sector - (int)sectors);
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

    // the head node is the last node output
    return P_CrossBSPNode(firstnode);
}
