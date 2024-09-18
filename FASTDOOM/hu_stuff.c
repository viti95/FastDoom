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
// DESCRIPTION:  Heads-up displays
//

#include <stdio.h>

#ifndef MAC
#include <conio.h>
#endif

#include "std_func.h"

#include "doomdef.h"

#include "z_zone.h"

#include "hu_stuff.h"
#include "hu_lib.h"
#include "w_wad.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "options.h"

//
// Locally used constants, shortcuts.
//
#define HU_TITLEHEIGHT 1
#define HU_TITLEX 0
#define HU_TITLEY (SCALED_SCREENHEIGHT - SCALED_SBARHEIGHT - hu_font[0]->height)
#define HU_TITLEY_FULL (HU_TITLEY + SCALED_SBARHEIGHT)

#define HU_INPUTTOGGLE 't'
#define HU_INPUTX HU_MSGX
#define HU_INPUTY (HU_MSGY + HU_MSGHEIGHT * (hu_font[0]->height + 1))
#define HU_INPUTWIDTH 64
#define HU_INPUTHEIGHT 1

patch_t *hu_font[HU_FONTSIZE];
static hu_textline_t w_title;
static hu_textline_t w_fps;

static byte message_on;
byte message_dontfuckwithme;
static byte message_nottobefuckedwith;

static hu_stext_t w_message;
static int message_counter;

extern int showMessages;
extern byte automapactive;

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void HU_Init(void)
{

    int i;
    int j;
    char buffer[9];

    // load the heads-up font
    j = HU_FONTSTART;
    for (i = 0; i < HU_FONTSIZE; i++)
    {
        sprintf(buffer, "STCFN%.3d", j++);
        hu_font[i] = (patch_t *)W_CacheLumpName(buffer, PU_STATIC);
    }
}
#endif

void HU_Start(void)
{

    int i;
    char *s = currentlevelname;

    message_on = 0;
    message_dontfuckwithme = 0;
    message_nottobefuckedwith = 0;

    // create the message widget
    HUlib_initSText(&w_message,
                    HU_MSGX, HU_MSGY, HU_MSGHEIGHT,
                    hu_font,
                    HU_FONTSTART, &message_on);

    // [JN] Create the FPS widget
    HUlib_initTextLine(&w_fps,
                       SCREENWIDTH - 24, HU_MSGY,
                       hu_font,
                       HU_FONTSTART);

    // create the map title widget
    HUlib_initTextLine(&w_title,
                       HU_TITLEX, HU_TITLEY,
                       hu_font,
                       HU_FONTSTART);

    while (*s)
        HUlib_addCharToTextLine(&w_title, *(s++));
}

void HU_DrawScreenFPS(void)
{
    static char str[4];
    char *f;
    int fpswhole, tmp;
    fpswhole = fps;

    f = str + sizeof(str) - 1;

    *f-- = '\0';          // NULL terminate

    // Manual simple unsigned itoa for the whole part
    while (1)
    {
        tmp = Div10(fpswhole);
        *--f = '0' + fpswhole - Mul10(tmp);
        if (tmp == 0)
            break;
        fpswhole = tmp;
    }

    HUlib_clearTextLine(&w_fps);
    while (*f)
    {
        HUlib_addCharToTextLine(&w_fps, *(f++));
    }
    HUlib_drawTextLine(&w_fps);
#if defined(USE_BACKBUFFER)
    updatestate |= I_MESSAGES;
#endif
}

void HU_DrawDebugCard2DigitsFPS(void)
{
    unsigned int outfps = fps;
    unsigned int outval = 0;
    unsigned int counter = 0;

    if (outfps > 99)
        outfps = 99;

    while (outfps)
    {
        outval |= (outfps % 10) << counter;
        outfps /= 10;
        counter += 4;
    }

    outp(debugCardPort, outval & 255);
}

void HU_DrawDebugCard4DigitsFPS(void)
{
    unsigned int outfps = fps;
    unsigned int outval = 0;
    unsigned int counter = 0;
    int port = debugCardPort;

    if (outfps > 9999)
        outfps = 9999;

    while (outfps)
    {
        outval |= (outfps % 10) << counter;
        outfps /= 10;
        counter += 4;
    }

    if (debugCardReverse)
    {
        outp(port, (outval >> 8) & 255);
        inp(port);
        inp(port);
        inp(port);
        inp(port);
        outp(port, outval & 255);
    }
    else
    {
        outp(port, outval & 255);
        inp(port);
        inp(port);
        inp(port);
        inp(port);
        outp(port, (outval >> 8) & 255);
    }
}

void HU_Drawer(void)
{
    HUlib_drawSText(&w_message);

    switch (showFPS)
    {
    case SCREEN_FPS:
        HU_DrawScreenFPS();
        break;
    case DEBUG_CARD_2D_FPS:
        HU_DrawDebugCard2DigitsFPS();
        break;
    case DEBUG_CARD_4D_FPS:
        HU_DrawDebugCard4DigitsFPS();
        break;
    case SCREEN_DC2D_FPS:
        HU_DrawScreenFPS();
        HU_DrawDebugCard2DigitsFPS();
        break;
    case SCREEN_DC4D_FPS:
        HU_DrawScreenFPS();
        HU_DrawDebugCard4DigitsFPS();
        break;
    }

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    if (automapactive)
        HUlib_drawTextLine(&w_title);
#endif
}

void HU_Erase(void)
{

    HUlib_eraseSText(&w_message);
    HUlib_eraseTextLine(&w_title);
    HUlib_eraseTextLine(&w_fps);
}

void HU_Ticker(void)
{

    int i, rc;
    char c;

    // tick down message counter if message is up
    if (message_counter && !--message_counter)
    {
        message_on = 0;
        message_nottobefuckedwith = 0;
    }

    if (showMessages || message_dontfuckwithme)
    {

        // display message if necessary
        if ((players.message && !message_nottobefuckedwith) || (players.message && message_dontfuckwithme))
        {
            HUlib_addMessageToSText(&w_message, 0, players.message);
            players.message = 0;
            message_on = 1;
            message_counter = HU_MSGTIMEOUT;
            message_nottobefuckedwith = message_dontfuckwithme;
            message_dontfuckwithme = 0;
        }

    } // else message_on = false;
}

#define QUEUESIZE 128
