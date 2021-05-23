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
//

#ifndef __M_MISC__
#define __M_MISC__

#include "doomdef.h"
#include "doomtype.h"
#include "fastmath.h"

//
// MISC
//
extern int myargc;
extern char **myargv;

extern int numChannels;

// Returns the position of the given parameter
// in the arg list (0 if not found).
int M_CheckParm(char *check);
int M_CheckParmOptional(char *check, int *variable);
int M_CheckParmDisable(char *check, int *variable);

byte M_WriteFile(char const *name, void *source, int length);

int M_ReadFile(char const *name,
               byte **buffer);

// Bounding box coordinate storage.
enum
{
    BOXTOP,
    BOXBOTTOM,
    BOXLEFT,
    BOXRIGHT
}; // bbox coordinates

#ifdef MODE_Y
// Bounding box functions.
void M_AddToBox(fixed_t *box, fixed_t x, fixed_t y);
#endif

void M_LoadDefaults(void);

void M_SaveDefaults(void);

#endif
