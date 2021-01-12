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

//
// Locally used constants, shortcuts.
//
#define HU_TITLE (mapnames[(gameepisode - 1) * 9 + gamemap - 1])
#define HU_TITLE2 (mapnames2[gamemap - 1])
#define HU_TITLEP (mapnamesp[gamemap - 1])
#define HU_TITLET (mapnamest[gamemap - 1])
#define HU_TITLEHEIGHT 1
#define HU_TITLEX 0
#define HU_TITLEY (167 - hu_font[0]->height)

#define HU_INPUTTOGGLE 't'
#define HU_INPUTX HU_MSGX
#define HU_INPUTY (HU_MSGY + HU_MSGHEIGHT * (hu_font[0]->height + 1))
#define HU_INPUTWIDTH 64
#define HU_INPUTHEIGHT 1

static player_t *plr;
patch_t *hu_font[HU_FONTSIZE];
static hu_textline_t w_title;
static hu_textline_t w_fps;
static boolean always_off = false;

static boolean message_on;
boolean message_dontfuckwithme;
static boolean message_nottobefuckedwith;

static hu_stext_t w_message;
static int message_counter;

extern int showMessages;
extern boolean automapactive;

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

char *mapnames[] = // DOOM shareware/registered/retail (Ultimate) names.
    {

        HUSTR_E1M1,
        HUSTR_E1M2,
        HUSTR_E1M3,
        HUSTR_E1M4,
        HUSTR_E1M5,
        HUSTR_E1M6,
        HUSTR_E1M7,
        HUSTR_E1M8,
        HUSTR_E1M9,

        HUSTR_E2M1,
        HUSTR_E2M2,
        HUSTR_E2M3,
        HUSTR_E2M4,
        HUSTR_E2M5,
        HUSTR_E2M6,
        HUSTR_E2M7,
        HUSTR_E2M8,
        HUSTR_E2M9,

        HUSTR_E3M1,
        HUSTR_E3M2,
        HUSTR_E3M3,
        HUSTR_E3M4,
        HUSTR_E3M5,
        HUSTR_E3M6,
        HUSTR_E3M7,
        HUSTR_E3M8,
        HUSTR_E3M9,

        HUSTR_E4M1,
        HUSTR_E4M2,
        HUSTR_E4M3,
        HUSTR_E4M4,
        HUSTR_E4M5,
        HUSTR_E4M6,
        HUSTR_E4M7,
        HUSTR_E4M8,
        HUSTR_E4M9,

        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL"};

char *mapnames2[] = // DOOM 2 map names.
    {
        HUSTR_1,
        HUSTR_2,
        HUSTR_3,
        HUSTR_4,
        HUSTR_5,
        HUSTR_6,
        HUSTR_7,
        HUSTR_8,
        HUSTR_9,
        HUSTR_10,
        HUSTR_11,

        HUSTR_12,
        HUSTR_13,
        HUSTR_14,
        HUSTR_15,
        HUSTR_16,
        HUSTR_17,
        HUSTR_18,
        HUSTR_19,
        HUSTR_20,

        HUSTR_21,
        HUSTR_22,
        HUSTR_23,
        HUSTR_24,
        HUSTR_25,
        HUSTR_26,
        HUSTR_27,
        HUSTR_28,
        HUSTR_29,
        HUSTR_30,
        HUSTR_31,
        HUSTR_32};

#if (EXE_VERSION >= EXE_VERSION_FINAL)
char *mapnamesp[] = // Plutonia WAD map names.
    {
        PHUSTR_1,
        PHUSTR_2,
        PHUSTR_3,
        PHUSTR_4,
        PHUSTR_5,
        PHUSTR_6,
        PHUSTR_7,
        PHUSTR_8,
        PHUSTR_9,
        PHUSTR_10,
        PHUSTR_11,

        PHUSTR_12,
        PHUSTR_13,
        PHUSTR_14,
        PHUSTR_15,
        PHUSTR_16,
        PHUSTR_17,
        PHUSTR_18,
        PHUSTR_19,
        PHUSTR_20,

        PHUSTR_21,
        PHUSTR_22,
        PHUSTR_23,
        PHUSTR_24,
        PHUSTR_25,
        PHUSTR_26,
        PHUSTR_27,
        PHUSTR_28,
        PHUSTR_29,
        PHUSTR_30,
        PHUSTR_31,
        PHUSTR_32};

char *mapnamest[] = // TNT WAD map names.
    {
        THUSTR_1,
        THUSTR_2,
        THUSTR_3,
        THUSTR_4,
        THUSTR_5,
        THUSTR_6,
        THUSTR_7,
        THUSTR_8,
        THUSTR_9,
        THUSTR_10,
        THUSTR_11,

        THUSTR_12,
        THUSTR_13,
        THUSTR_14,
        THUSTR_15,
        THUSTR_16,
        THUSTR_17,
        THUSTR_18,
        THUSTR_19,
        THUSTR_20,

        THUSTR_21,
        THUSTR_22,
        THUSTR_23,
        THUSTR_24,
        THUSTR_25,
        THUSTR_26,
        THUSTR_27,
        THUSTR_28,
        THUSTR_29,
        THUSTR_30,
        THUSTR_31,
        THUSTR_32};
#endif

const char *shiftxform;

const char english_shiftxform[] =
    {

        0,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31,
        ' ', '!', '"', '#', '$', '%', '&',
        '"', // shift-'
        '(', ')', '*', '+',
        '<', // shift-,
        '_', // shift--
        '>', // shift-.
        '?', // shift-/
        ')', // shift-0
        '!', // shift-1
        '@', // shift-2
        '#', // shift-3
        '$', // shift-4
        '%', // shift-5
        '^', // shift-6
        '&', // shift-7
        '*', // shift-8
        '(', // shift-9
        ':',
        ':', // shift-;
        '<',
        '+', // shift-=
        '>', '?', '@',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        '[', // shift-[
        '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
        ']', // shift-]
        '"', '_',
        '\'', // shift-`
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        '{', '|', '}', '~', 127};

void HU_Init(void)
{

    int i;
    int j;
    char buffer[9];

    shiftxform = english_shiftxform;

    // load the heads-up font
    j = HU_FONTSTART;
    for (i = 0; i < HU_FONTSIZE; i++)
    {
        sprintf(buffer, "STCFN%.3d", j++);
        hu_font[i] = (patch_t *)W_CacheLumpName(buffer, PU_STATIC);
    }
}

void HU_Start(void)
{

    int i;
    char *s;

    plr = &players;
    message_on = false;
    message_dontfuckwithme = false;
    message_nottobefuckedwith = false;

    // create the message widget
    HUlib_initSText(&w_message,
                    HU_MSGX, HU_MSGY, HU_MSGHEIGHT,
                    hu_font,
                    HU_FONTSTART, &message_on);

    // [JN] Create the FPS widget
    HUlib_initTextLine(&w_fps,
                       SCREENWIDTH - 48, HU_MSGY,
                       hu_font,
                       HU_FONTSTART);

    // create the map title widget
    HUlib_initTextLine(&w_title,
                       HU_TITLEX, HU_TITLEY,
                       hu_font,
                       HU_FONTSTART);

    if (commercial)
    {
#if (EXE_VERSION < EXE_VERSION_FINAL)
        s = HU_TITLE2;
#else
        if (plutonia)
        {
            s = HU_TITLEP;
        }
        else if (tnt)
        {
            s = HU_TITLET;
        }
        else
        {
            s = HU_TITLE2;
        }
#endif
    }
    else
    {
        s = HU_TITLE;
    }

    while (*s)
        HUlib_addCharToTextLine(&w_title, *(s++));
}

void HU_Drawer(void)
{
    static char str[32], *f;

    HUlib_drawSText(&w_message);

    if (showFPS)
    {
        sprintf(str, "%i.%01i", fps >> FRACBITS, Mul10(fps & 65535) >> FRACBITS);
        HUlib_clearTextLine(&w_fps);
        f = str;
        while (*f)
        {
            HUlib_addCharToTextLine(&w_fps, *(f++));
        }
        HUlib_drawTextLine(&w_fps, false);
    }

    if (automapactive)
        HUlib_drawTextLine(&w_title, false);
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
        message_on = false;
        message_nottobefuckedwith = false;
    }

    if (showMessages || message_dontfuckwithme)
    {

        // display message if necessary
        if ((plr->message && !message_nottobefuckedwith) || (plr->message && message_dontfuckwithme))
        {
            HUlib_addMessageToSText(&w_message, 0, plr->message);
            plr->message = 0;
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            message_nottobefuckedwith = message_dontfuckwithme;
            message_dontfuckwithme = 0;
        }

    } // else message_on = false;
}

#define QUEUESIZE 128

boolean HU_Responder(event_t *ev)
{

    static char lastmessage[HU_MAXLINELENGTH + 1];
    char *macromessage;
    boolean eatkey = false;
    static boolean shiftdown = false;
    static boolean altdown = false;
    unsigned char c;
    int i;

    static int num_nobrainers = 0;

    if (ev->data1 == KEY_RSHIFT)
    {
        shiftdown = ev->type == ev_keydown;
        return false;
    }
    else if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT)
    {
        altdown = ev->type == ev_keydown;
        return false;
    }

    if (ev->type != ev_keydown)
        return false;

    if (ev->data1 == HU_MSGREFRESH)
    {
        message_on = true;
        message_counter = HU_MSGTIMEOUT;
        eatkey = true;
    }

    return eatkey;
}
