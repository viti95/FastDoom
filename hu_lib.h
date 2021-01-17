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
// DESCRIPTION:  none
//

#ifndef __HULIB__
#define __HULIB__

// We are referring to patches.
#include "r_defs.h"

// background and foreground screen numbers
// different from other modules.
#define BG 1
#define FG 0

// font stuff
#define HU_CHARERASE KEY_BACKSPACE

#define HU_MAXLINES 4
#define HU_MAXLINELENGTH 80

//
// Typedefs of widgets
//

// Text Line widget
//  (parent of Scrolling Text and Input Text widgets)
typedef struct
{
    // left-justified position of scrolling text window
    int x;
    int y;

    patch_t **f;                  // font
    int sc;                       // start character
    char l[HU_MAXLINELENGTH + 1]; // line of text
    int len;                      // current line length

    // whether this line needs to be udpated
    int needsupdate;

} hu_textline_t;

// Scrolling Text window widget
//  (child of Text Line widget)
typedef struct
{
    hu_textline_t l[HU_MAXLINES]; // text lines to draw
    int h;                        // height in lines
    int cl;                       // current line number

    // pointer to boolean stating whether to update window
    boolean *on;
    boolean laston; // last value of *->on.

} hu_stext_t;

//
// Widget creation, access, and update routines
//

//
// textline code
//

// clear a line of text
void HUlib_clearTextLine(hu_textline_t *t);

void HUlib_initTextLine(hu_textline_t *t, int x, int y, patch_t **f, int sc);

// returns success
boolean HUlib_addCharToTextLine(hu_textline_t *t, char ch);

// draws tline
void HUlib_drawTextLine(hu_textline_t *l);

// erases text line
void HUlib_eraseTextLine(hu_textline_t *l);

//
// Scrolling Text window widget routines
//

// ?
void HUlib_initSText(hu_stext_t *s,
                     int x,
                     int y,
                     int h,
                     patch_t **font,
                     int startchar,
                     boolean *on);

// add a new line
void HUlib_addLineToSText(hu_stext_t *s);

// ?
void HUlib_addMessageToSText(hu_stext_t *s,
                             char *prefix,
                             char *msg);

// draws stext
void HUlib_drawSText(hu_stext_t *s);

// erases all stext lines
void HUlib_eraseSText(hu_stext_t *s);

#endif
