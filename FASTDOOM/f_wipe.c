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
#include "i_ibm.h"

#include "z_zone.h"
#include "i_system.h"
#include "v_video.h"
#include "m_misc.h"

#include "doomdef.h"

#include "f_wipe.h"

#include <conio.h>

#define GC_INDEX 0x3CE
#define GC_READMAP 4

//
//                       SCREEN WIPE PACKAGE
//

// when zero, stop the wipe
static byte go = 0;

byte *screen2;
byte *screen3;

void wipe_shittyColMajorXform(short *array)
{
    int y;
    int base_y = 0;
    short *dest;

    dest = (short *)Z_MallocUnowned(SCREENWIDTH * SCREENHEIGHT, PU_STATIC);

    for (y = 0; y < SCREENHEIGHT; y++)
    {

        int base_x = y;
        int x;

        for (x = 0; x < SCREENWIDTH / 16; x++)
        {
            dest[base_x] = array[base_y];
            dest[base_x + 200] = array[base_y + 1];
            dest[base_x + 400] = array[base_y + 2];
            dest[base_x + 600] = array[base_y + 3];
            dest[base_x + 800] = array[base_y + 4];
            dest[base_x + 1000] = array[base_y + 5];
            dest[base_x + 1200] = array[base_y + 6];
            dest[base_x + 1400] = array[base_y + 7];
            base_x += 1600;
            base_y += 8;
        }
    }

    CopyDWords(dest, array, (SCREENWIDTH * SCREENHEIGHT) / 4);
    //memcpy(array, dest, SCREENWIDTH * SCREENHEIGHT);

    Z_Free(dest);
}

static int *y;

void wipe_initMelt()
{
    int i;

    // copy start screen to main screen
    #if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
    CopyDWords(screen2, screen0, (SCREENWIDTH * SCREENHEIGHT) / 4);
    #endif
    #if defined(USE_BACKBUFFER)
    CopyDWords(screen2, backbuffer, (SCREENWIDTH * SCREENHEIGHT) / 4);
    #endif

    // makes this wipe faster (in theory)
    // to have stuff in column-major format
    wipe_shittyColMajorXform((short *)screen2);
    wipe_shittyColMajorXform((short *)screen3);

    // setup initial column positions
    // (y<0 => not ready to scroll yet)
    y = (int *)Z_MallocUnowned(SCREENWIDTH * sizeof(int), PU_STATIC);
    y[0] = -(M_Random & 15);
    for (i = 1; i < SCREENWIDTH; i++)
    {
        y[i] = y[i - 1] + M_Random_Mod3_Minus1;
        if (y[i] > 0)
            y[i] = 0;
        else if (y[i] == -16)
            y[i] = -15;
    }
}

byte wipe_doMelt(int ticks)
{
    int i;
    byte done = 1;

    if (noMelt)
        return 1;

    while (ticks--)
    {
        for (i = 0; i < SCREENWIDTH / 2; i++)
        {
            int y_val = y[i];
            if (y_val < 0)
            {
                y[i]++;
                done = 0;
            }
            else if (y_val < SCREENHEIGHT)
            {
                short *s, *d;
                int j, dy;
                int idx = 0;

                dy = (y_val < 16) ? y_val + 1 : 8;
                if (dy >= SCREENHEIGHT - y_val)
                    dy = SCREENHEIGHT - y_val;
                s = &((short *)screen3)[(200 * i) + y_val];
                #if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
                d = &((short *)screen0)[(160 * y_val) + i];
                #endif
                #if defined(USE_BACKBUFFER)
                d = &((short *)backbuffer)[(160 * y_val) + i];
                #endif
                for (j = dy; j; j--)
                {
                    d[idx] = *(s++);
                    idx += SCREENWIDTH / 2;
                }
                y_val += dy;
                s = &((short *)screen2)[(200 * i)];
                #if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
                d = &((short *)screen0)[(160 * y_val) + i];
                #endif
                #if defined(USE_BACKBUFFER)
                d = &((short *)backbuffer)[(160 * y_val) + i];
                #endif
                idx = 0;
                for (j = SCREENHEIGHT - y_val; j; j--)
                {
                    d[idx] = *(s++);
                    idx += (SCREENWIDTH / 2);
                }
                y[i] = y_val;
                done = 0;
            }
        }
    }

    return done;
}

void wipe_exitMelt()
{
    Z_Free(y);
    Z_Free(screen2);
    Z_Free(screen3);
}

//
// wipe_ReadScreen
// Reads the screen currently displayed into a linear buffer.
//
#ifdef MODE_Y
void wipe_ReadScreen(byte *scr)
{
    int j;

    outp(GC_INDEX, GC_READMAP);

    outp(GC_INDEX + 1, 0);
    for (j = 0; j < SCREENWIDTH * SCREENHEIGHT / 4; j += 8)
    {
        int i = j * 4;
        scr[i] = currentscreen[j];
        scr[i + 4] = currentscreen[j + 1];
        scr[i + 8] = currentscreen[j + 2];
        scr[i + 12] = currentscreen[j + 3];
        scr[i + 16] = currentscreen[j + 4];
        scr[i + 20] = currentscreen[j + 5];
        scr[i + 24] = currentscreen[j + 6];
        scr[i + 28] = currentscreen[j + 7];
    }

    outp(GC_INDEX + 1, 1);
    for (j = 0; j < SCREENWIDTH * SCREENHEIGHT / 4; j += 8)
    {
        int i = j * 4;
        scr[i + 1] = currentscreen[j];
        scr[i + 5] = currentscreen[j + 1];
        scr[i + 9] = currentscreen[j + 2];
        scr[i + 13] = currentscreen[j + 3];
        scr[i + 17] = currentscreen[j + 4];
        scr[i + 21] = currentscreen[j + 5];
        scr[i + 25] = currentscreen[j + 6];
        scr[i + 29] = currentscreen[j + 7];
    }

    outp(GC_INDEX + 1, 2);
    for (j = 0; j < SCREENWIDTH * SCREENHEIGHT / 4; j += 8)
    {
        int i = j * 4;
        scr[i + 2] = currentscreen[j];
        scr[i + 6] = currentscreen[j + 1];
        scr[i + 10] = currentscreen[j + 2];
        scr[i + 14] = currentscreen[j + 3];
        scr[i + 18] = currentscreen[j + 4];
        scr[i + 22] = currentscreen[j + 5];
        scr[i + 26] = currentscreen[j + 6];
        scr[i + 30] = currentscreen[j + 7];
    }

    outp(GC_INDEX + 1, 3);
    for (j = 0; j < SCREENWIDTH * SCREENHEIGHT / 4; j += 8)
    {
        int i = j * 4;
        scr[i + 3] = currentscreen[j];
        scr[i + 7] = currentscreen[j + 1];
        scr[i + 11] = currentscreen[j + 2];
        scr[i + 15] = currentscreen[j + 3];
        scr[i + 19] = currentscreen[j + 4];
        scr[i + 23] = currentscreen[j + 5];
        scr[i + 27] = currentscreen[j + 6];
        scr[i + 31] = currentscreen[j + 7];
    }
}
#endif

#ifdef MODE_VBE2_DIRECT
void wipe_ReadScreen(byte *scr)
{
    CopyDWords(destscreen, scr, SCREENWIDTH * SCREENHEIGHT / 4);
}
#endif

#if defined(USE_BACKBUFFER)
void wipe_ReadScreen(byte *scr)
{
    CopyDWords(backbuffer, scr, SCREENWIDTH * SCREENHEIGHT / 4);
}
#endif

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void wipe_StartScreen()
{
    screen2 = (byte *)Z_MallocUnowned(SCREENWIDTH * SCREENHEIGHT, PU_STATIC);
    wipe_ReadScreen(screen2);
}
#endif

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void wipe_EndScreen()
{
    screen3 = (byte *)Z_MallocUnowned(SCREENWIDTH * SCREENHEIGHT, PU_STATIC);
    wipe_ReadScreen(screen3);
}
#endif

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
int wipe_ScreenWipe(int ticks)
{
    int rc;

    // initial stuff
    if (!go)
    {
        go = 1;
        wipe_initMelt();
    }

    // do a piece of wipe-in
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
    V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);
#endif

    rc = wipe_doMelt(ticks);

    // final stuff
    if (rc)
    {
        go = 0;
        wipe_exitMelt();
    }

    return !go;
}
#endif
