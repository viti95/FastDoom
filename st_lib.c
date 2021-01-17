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
//	The status bar widget code.
//
#include "std_func.h"

#include "doomdef.h"

#include "z_zone.h"
#include "v_video.h"

#include "i_system.h"

#include "w_wad.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"

// in AM_map.c
extern boolean automapactive;

void STlib_init(void)
{
}

// ?
void STlib_initNum(st_number_t *n,
                   int x,
                   int y,
                   patch_t **pl,
                   int *num,
                   boolean *on)
{
    n->x = x;
    n->y = y;
    n->oldnum = 0;
    n->num = num;
    n->on = on;
    n->p = pl;
}

//
// A fairly efficient way to draw a number
//  based on differences from the old number.
// Note: worth the trouble?
//
void STlib_drawNum(st_number_t *n,
                   boolean refresh)
{
    int num = *n->num;

    int w = n->p[0]->width;
    int h = n->p[0]->height;
    int x = n->x;

    // [crispy] redraw only if necessary
    if (n->oldnum == num && !refresh)
    {
        return;
    }

    n->oldnum = *n->num;

    // clear the area
    x = n->x - 3 * w;

    V_CopyRect(x, n->y - ST_Y, screen4, w * 3, h, x, n->y, screen0);

    // if non-number, do not draw it
    if (num == 1994)
        return;

    x = n->x;

    // in the special case of 0, you draw 0
    if (!num)
    {
        V_DrawPatch(x - w, n->y, screen0, n->p[0]);
        return;
    }

    // draw the new number
    do
    {
        int original = num;

        num = Div10(num);
        x -= w;
        V_DrawPatch(x, n->y, screen0, n->p[original - Mul10(num)]);
    } while (num);
}

//
void STlib_updateNum(st_number_t *n,
                     boolean refresh)
{
    if (*n->on)
        STlib_drawNum(n, refresh);
}

//
void STlib_initPercent(st_percent_t *p,
                       int x,
                       int y,
                       patch_t **pl,
                       int *num,
                       boolean *on,
                       patch_t *percent)
{
    STlib_initNum(&p->n, x, y, pl, num, on);
    p->p = percent;
}

void STlib_updatePercent(st_percent_t *per,
                         int refresh)
{
    if (refresh && *per->n.on)
        V_DrawPatch(per->n.x, per->n.y, screen0, per->p);

    STlib_updateNum(&per->n, refresh);
}

void STlib_initMultIcon(st_multicon_t *i,
                        int x,
                        int y,
                        patch_t **il,
                        int *inum,
                        boolean *on)
{
    i->x = x;
    i->y = y;
    i->oldinum = -1;
    i->inum = inum;
    i->on = on;
    i->p = il;
}

void STlib_updateMultIcon(st_multicon_t *mi,
                          boolean refresh)
{
    int w;
    int h;
    int x;
    int y;

    if (*mi->on && (mi->oldinum != *mi->inum || refresh) && (*mi->inum != -1))
    {
        if (mi->oldinum != -1)
        {
            x = mi->x - mi->p[mi->oldinum]->leftoffset;
            y = mi->y - mi->p[mi->oldinum]->topoffset;
            w = mi->p[mi->oldinum]->width;
            h = mi->p[mi->oldinum]->height;

            V_CopyRect(x, y - ST_Y, screen4, w, h, x, y, screen0);
        }
        V_DrawPatch(mi->x, mi->y, screen0, mi->p[*mi->inum]);
        mi->oldinum = *mi->inum;
    }
}

void STlib_initBinIcon(st_binicon_t *b,
                       int x,
                       int y,
                       patch_t *i,
                       boolean *on)
{
    b->x = x;
    b->y = y;
    b->on = on;
    b->p = i;
}

void STlib_updateBinIcon(st_binicon_t *bi,
                         boolean refresh)
{
    if (*bi->on && refresh)
        V_DrawPatch(bi->x, bi->y, screen0, bi->p);
}
