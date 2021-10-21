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

#include "doomstat.h"


#include <stdio.h>

// in AM_map.c
extern byte automapactive;

void STlib_init(void)
{
}

// ?
void STlib_initNum(st_number_t *n,
                   int x,
                   int y,
                   patch_t **pl,
                   int *num,
                   byte *on)
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

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100)
void STlib_drawNumText(st_number_t *n, int x, int y)
{
    int num = *n->num;
    char strnum[4];

    // if non-number, do not draw it
    if (num == 1994 || num < 0)
        return;

    n->oldnum = *n->num;

    sprintf(strnum, "%3i", num);
    V_WriteTextDirect(x, y, strnum);
}
#endif

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void STlib_drawNum(st_number_t *n, byte refresh)
{
    int num = *n->num;
    int w;
    int h;
    int x;

    // [crispy] redraw only if necessary
    if (n->oldnum == num && !refresh)
    {
        return;
    }

#if defined(USE_BACKBUFFER)
    updatestate |= I_STATBAR;
#endif

    w = n->p[0]->width;
    h = n->p[0]->height;
    x = n->x;

    n->oldnum = *n->num;

    // clear the area
    x = n->x - 3 * w;

    if (simpleStatusBar)
    {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_SetRect(ST_BACKGROUND_COLOR, w * 3, h, x, n->y, screen0);
#endif
#if defined(USE_BACKBUFFER)
        V_SetRect(ST_BACKGROUND_COLOR, w * 3, h, x, n->y, backbuffer);
#endif
    }
    else
    {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_CopyRect(x, n->y - ST_Y, screen4, w * 3, h, x, n->y, screen0);
#endif
#if defined(USE_BACKBUFFER)
        V_CopyRect(x, n->y - ST_Y, screen4, w * 3, h, x, n->y, backbuffer);
#endif
    }

    // if non-number, do not draw it
    if (num == 1994)
        return;

    x = n->x;

    // in the special case of 0, you draw 0
    if (!num)
    {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_DrawPatchScreen0(x - w, n->y, n->p[0]);
#endif
#if defined(USE_BACKBUFFER)
        V_DrawPatchDirect(x - w, n->y, n->p[0]);
#endif
        return;
    }

    // draw the new number
    do
    {
        int original = num;

        num = Div10(num);
        x -= w;
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_DrawPatchScreen0(x, n->y, n->p[original - Mul10(num)]);
#endif
#if defined(USE_BACKBUFFER)
        V_DrawPatchDirect(x, n->y, n->p[original - Mul10(num)]);
#endif
    } while (num);
}

void STlib_drawNum_Direct(st_number_t *n)
{
    int num = *n->num;
    int w;
    int h;
    int x;

    w = n->p[0]->width;
    h = n->p[0]->height;
    x = n->x;

    n->oldnum = *n->num;

    // clear the area
    x = n->x - 3 * w;

    // if non-number, do not draw it
    if (num == 1994)
        return;

    x = n->x;

    // in the special case of 0, you draw 0
    if (!num)
    {
        V_DrawPatchDirect(x - w, n->y, n->p[0]);
        return;
    }

    // draw the new number
    do
    {
        int original = num;

        num = Div10(num);
        x -= w;
        V_DrawPatchDirect(x, n->y, n->p[original - Mul10(num)]);
    } while (num);
}
#endif

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void STlib_updateNum(st_number_t *n, byte refresh)
{
    if (*n->on)
        STlib_drawNum(n, refresh);
}

void STlib_updateNum_Direct(st_number_t *n)
{
    STlib_drawNum_Direct(n);
}
#endif

//
void STlib_initPercent(st_percent_t *p,
                       int x,
                       int y,
                       patch_t **pl,
                       int *num,
                       byte *on,
                       patch_t *percent)
{
    STlib_initNum(&p->n, x, y, pl, num, on);
    p->p = percent;
}

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void STlib_updatePercent(st_percent_t *per, int refresh)
{
    if (refresh && *per->n.on)
    {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_DrawPatchScreen0(per->n.x, per->n.y, per->p);
#endif
#if defined(USE_BACKBUFFER)
        V_DrawPatchDirect(per->n.x, per->n.y, per->p);
#endif
    }

    STlib_updateNum(&per->n, refresh);
}

void STlib_updatePercent_Direct(st_percent_t *per)
{
    V_DrawPatchDirect(per->n.x, per->n.y, per->p);
    STlib_updateNum_Direct(&per->n);
}
#endif

void STlib_initMultIcon(st_multicon_t *i,
                        int x,
                        int y,
                        patch_t **il,
                        int *inum,
                        byte *on)
{
    i->x = x;
    i->y = y;
    i->oldinum = -1;
    i->inum = inum;
    i->on = on;
    i->p = il;
}

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void STlib_updateMultIcon_Direct(st_multicon_t *mi)
{
    int w;
    int h;
    int x;
    int y;

    if (*mi->inum != -1){
        V_DrawPatchDirect(mi->x, mi->y, mi->p[*mi->inum]);
    }
}

void STlib_updateMultIcon(st_multicon_t *mi, byte refresh)
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

            if (simpleStatusBar)
            {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
                V_SetRect(ST_BACKGROUND_COLOR, w, h, x, y, screen0);
#endif
#if defined(USE_BACKBUFFER)
                V_SetRect(ST_BACKGROUND_COLOR, w, h, x, y, backbuffer);
#endif
            }
            else
            {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
                V_CopyRect(x, y - ST_Y, screen4, w, h, x, y, screen0);
#endif
#if defined(USE_BACKBUFFER)
                V_CopyRect(x, y - ST_Y, screen4, w, h, x, y, backbuffer);
#endif
            }
        }
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
        V_DrawPatchScreen0(mi->x, mi->y, mi->p[*mi->inum]);
#endif
#if defined(USE_BACKBUFFER)
        V_DrawPatchDirect(mi->x, mi->y, mi->p[*mi->inum]);
#endif
        mi->oldinum = *mi->inum;

#if defined(USE_BACKBUFFER)
        updatestate |= I_STATBAR;
#endif
    }
}
#endif

void STlib_initBinIcon(st_binicon_t *b,
                       int x,
                       int y,
                       patch_t *i,
                       byte *on)
{
    b->x = x;
    b->y = y;
    b->on = on;
    b->p = i;
}

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void STlib_updateBinIcon(st_binicon_t *bi, byte refresh)
{
    if (*bi->on && refresh)
    {
        if (simpleStatusBar)
        {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
            V_SetRect(ST_BACKGROUND_COLOR, 40, 30, bi->x, bi->y, screen0);
#endif
#if defined(USE_BACKBUFFER)
            V_SetRect(ST_BACKGROUND_COLOR, 40, 30, bi->x, bi->y, backbuffer);
#endif
        }
        else
        {
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
            V_DrawPatchScreen0(bi->x, bi->y, bi->p);
#endif
#if defined(USE_BACKBUFFER)
            V_DrawPatchDirect(bi->x, bi->y, bi->p);
#endif
        }

#if defined(USE_BACKBUFFER)
        updatestate |= I_STATBAR;
#endif
    }
}

void STlib_updateBinIcon_Direct(st_binicon_t *bi)
{
    V_DrawPatchDirect(bi->x, bi->y, bi->p);
}
#endif
