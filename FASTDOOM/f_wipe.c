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

#include "options.h"

#define BYTE0_USHORT(value)  (((unsigned char *)&value)[0])
#define BYTE1_USHORT(value)  (((unsigned char *)&value)[1])

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
    unsigned char y;
    short *ptrarray = array;
    short *dest = (short *)Z_MallocUnowned(SCREENWIDTH * SCREENHEIGHT, PU_STATIC);

    for (y = 0; y < SCREENHEIGHT; y++, dest -= 31999)
    {
        unsigned char x;

        for (x = 0; x < SCREENWIDTH / 16; x++, dest += 1600, ptrarray += 8)
        {
            *dest = ptrarray[0];
            *(dest + 200) = ptrarray[1];
            *(dest + 400) = ptrarray[2];
            *(dest + 600) = ptrarray[3];
            *(dest + 800) = ptrarray[4];
            *(dest + 1000) = ptrarray[5];
            *(dest + 1200) = ptrarray[6];
            *(dest + 1400) = ptrarray[7];
        }
    }

    dest -= 200;
    CopyDWords(dest, array, (SCREENWIDTH * SCREENHEIGHT) / 4);

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
    unsigned char i;
    unsigned short i200;
    byte done = 1;

    if (noMelt)
        return 1;

    while (ticks--)
    {
        for (i = 0, i200 = 0; i < SCREENWIDTH / 2; i++, i200 += 200)
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
                s = &((short *)screen3)[i200 + y_val];
                #if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
                d = &((short *)screen0)[Mul160(y_val) + i];
                #endif
                #if defined(USE_BACKBUFFER)
                d = &((short *)backbuffer)[Mul160(y_val) + i];
                #endif
                for (j = dy; j; j--)
                {
                    d[idx] = *(s++);
                    idx += SCREENWIDTH / 2;
                }
                y_val += dy;
                s = &((short *)screen2)[i200];
                #if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
                d = &((short *)screen0)[Mul160(y_val) + i];
                #endif
                #if defined(USE_BACKBUFFER)
                d = &((short *)backbuffer)[Mul160(y_val) + i];
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
#if defined(MODE_Y)
void wipe_ReadScreen(byte *scr)
{
    int j;

    outp(GC_INDEX, GC_READMAP);

    outp(GC_INDEX + 1, 0);
    for (j = 0; j < SCREENWIDTH * SCREENHEIGHT / 8; j += 4)
    {
        int i = j * 8;
        unsigned short value;

        value = currentscreen[j];
        scr[i] = BYTE0_USHORT(value);
        scr[i + 4] = BYTE1_USHORT(value);

        value = currentscreen[j + 1];
        scr[i + 8] = BYTE0_USHORT(value);
        scr[i + 12] = BYTE1_USHORT(value);

        value = currentscreen[j + 2];
        scr[i + 16] = BYTE0_USHORT(value);
        scr[i + 20] = BYTE1_USHORT(value);

        value = currentscreen[j + 3];
        scr[i + 24] = BYTE0_USHORT(value);
        scr[i + 28] = BYTE1_USHORT(value);
    }

    outp(GC_INDEX + 1, 1);
    for (j = 0; j < SCREENWIDTH * SCREENHEIGHT / 8; j += 4)
    {
        int i = j * 8;
        unsigned short value;

        value = currentscreen[j];
        scr[i + 1] = BYTE0_USHORT(value);
        scr[i + 5] = BYTE1_USHORT(value);

        value = currentscreen[j + 1];
        scr[i + 9] = BYTE0_USHORT(value);
        scr[i + 13] = BYTE1_USHORT(value);

        value = currentscreen[j + 2];
        scr[i + 17] = BYTE0_USHORT(value);
        scr[i + 21] = BYTE1_USHORT(value);

        value = currentscreen[j + 3];
        scr[i + 25] = BYTE0_USHORT(value);
        scr[i + 29] = BYTE1_USHORT(value);
    }

    outp(GC_INDEX + 1, 2);
    for (j = 0; j < SCREENWIDTH * SCREENHEIGHT / 8; j += 4)
    {
        int i = j * 8;
        unsigned short value;

        value = currentscreen[j];
        scr[i + 2] = BYTE0_USHORT(value);
        scr[i + 6] = BYTE1_USHORT(value);

        value = currentscreen[j + 1];
        scr[i + 10] = BYTE0_USHORT(value);
        scr[i + 14] = BYTE1_USHORT(value);

        value = currentscreen[j + 2];
        scr[i + 18] = BYTE0_USHORT(value);
        scr[i + 22] = BYTE1_USHORT(value);

        value = currentscreen[j + 3];
        scr[i + 26] = BYTE0_USHORT(value);
        scr[i + 30] = BYTE1_USHORT(value);
    }

    outp(GC_INDEX + 1, 3);
    for (j = 0; j < SCREENWIDTH * SCREENHEIGHT / 8; j += 4)
    {
        int i = j * 8;
        unsigned short value;

        value = currentscreen[j];
        scr[i + 3] = BYTE0_USHORT(value);
        scr[i + 7] = BYTE1_USHORT(value);

        value = currentscreen[j + 1];
        scr[i + 11] = BYTE0_USHORT(value);
        scr[i + 15] = BYTE1_USHORT(value);

        value = currentscreen[j + 2];
        scr[i + 19] = BYTE0_USHORT(value);
        scr[i + 23] = BYTE1_USHORT(value);

        value = currentscreen[j + 3];
        scr[i + 27] = BYTE0_USHORT(value);
        scr[i + 31] = BYTE1_USHORT(value);
    }
}
#endif

#if defined(MODE_VBE2_DIRECT)
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

    // final stuff
    if (wipe_doMelt(ticks))
    {
        go = 0;
        wipe_exitMelt();
    }

    return !go;
}
#endif
