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
//	Mission begin melt/wipe screen special effect.
//

#include "doomstat.h"

#include "i_random.h"

#include "z_zone.h"
#include "i_system.h"
#include "v_video.h"
#include "m_misc.h"

#include "doomdef.h"

#include "f_wipe.h"

//
//                       SCREEN WIPE PACKAGE
//

// when zero, stop the wipe
static boolean go = 0;

static byte *wipe_scr_start;
static byte *wipe_scr_end;
static byte *wipe_scr;

void wipe_shittyColMajorXform(short *array)
{
    int x;
    int y;
    short *dest;

    dest = (short *)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, 0);

    for (y = 0; y < SCREENHEIGHT; y++)
        for (x = 0; x < SCREENWIDTH / 2; x++)
            dest[Mul200(x) + y] = array[Mul160(y) + x];

    CopyDWords(dest, array, (SCREENWIDTH * SCREENHEIGHT) / 4);
    //memcpy(array, dest, SCREENWIDTH * SCREENHEIGHT);

    Z_Free(dest);
}

static int *y;

int wipe_initMelt()
{
    int i;

    // copy start screen to main screen
    CopyDWords(wipe_scr_start, wipe_scr, (SCREENWIDTH * SCREENHEIGHT) / 4);
    //memcpy(wipe_scr, wipe_scr_start, SCREENWIDTH * SCREENHEIGHT);

    // makes this wipe faster (in theory)
    // to have stuff in column-major format
    wipe_shittyColMajorXform((short *)wipe_scr_start);
    wipe_shittyColMajorXform((short *)wipe_scr_end);

    // setup initial column positions
    // (y<0 => not ready to scroll yet)
    y = (int *)Z_Malloc(SCREENWIDTH * sizeof(int), PU_STATIC, 0);
    y[0] = -(M_Random & 15);
    for (i = 1; i < SCREENWIDTH; i++)
    {
        y[i] = y[i - 1] + M_Random_Mod3_Minus1;
        if (y[i] > 0)
            y[i] = 0;
        else if (y[i] == -16)
            y[i] = -15;
    }

    return 0;
}

int wipe_doMelt(int ticks)
{
    int i;
    int j;
    int dy;
    int idx;

    short *s;
    short *d;
    boolean done = true;

    if (noMelt)
        return true;

    while (ticks--)
    {
        for (i = 0; i < SCREENWIDTH / 2; i++)
        {
            if (y[i] < 0)
            {
                y[i]++;
                done = false;
            }
            else if (y[i] < SCREENHEIGHT)
            {
                dy = (y[i] < 16) ? y[i] + 1 : 8;
                if (y[i] + dy >= SCREENHEIGHT)
                    dy = SCREENHEIGHT - y[i];
                s = &((short *)wipe_scr_end)[Mul200(i) + y[i]];
                d = &((short *)wipe_scr)[Mul160(y[i]) + i];
                idx = 0;
                for (j = dy; j; j--)
                {
                    d[idx] = *(s++);
                    idx += SCREENWIDTH / 2;
                }
                y[i] += dy;
                s = &((short *)wipe_scr_start)[Mul200(i)];
                d = &((short *)wipe_scr)[Mul160(y[i]) + i];
                idx = 0;
                for (j = SCREENHEIGHT - y[i]; j; j--)
                {
                    d[idx] = *(s++);
                    idx += (SCREENWIDTH / 2);
                }
                done = false;
            }
        }
    }

    return done;
}

int wipe_exitMelt()
{
    Z_Free(y);
    return 0;
}

int wipe_StartScreen()
{
    wipe_scr_start = screens[2];
    I_ReadScreen(wipe_scr_start);
    return 0;
}

int wipe_EndScreen()
{
    wipe_scr_end = screens[3];
    I_ReadScreen(wipe_scr_end);
    V_DrawBlock(0, 0, 0, SCREENWIDTH, SCREENHEIGHT, wipe_scr_start); // restore start scr.
    return 0;
}

int wipe_ScreenWipe(int ticks)
{
    int rc;

    // initial stuff
    if (!go)
    {
        go = 1;
        wipe_scr = screens[0];
        wipe_initMelt();
    }

    // do a piece of wipe-in
    V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);
    rc = wipe_doMelt(ticks);

    // final stuff
    if (rc)
    {
        go = 0;
        wipe_exitMelt();
    }

    return !go;
}
