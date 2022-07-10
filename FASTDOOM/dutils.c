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
//	Cheat sequence checking.
//

#include "dutils.h"
#include "i_system.h"
#include "z_zone.h"
#include "options.h"
#include "doomstat.h"

//
// CHEAT SEQUENCE PACKAGE
//

//
// Called in st_stuff module, which handles the input.
// Returns a 1 if the cheat was successful, 0 if failed.
//
byte cht_CheckCheat(cheatseq_t *cht)
{
    if (!cht->p)
        cht->p = cht->sequence; // initialize if first time

    if (*cht->p == 0)
        *(cht->p++) = current_ev->data1;
    else if (current_ev->data1 == *cht->p)
        cht->p++;
    else
        cht->p = cht->sequence;

    if (*cht->p == 1)
        cht->p++;
    else if (*cht->p == 0xff) // end of sequence character
    {
        cht->p = cht->sequence;
        return 1;
    }

    return 0;
}

void cht_GetParam(cheatseq_t *cht, char *buffer)
{

    unsigned char *p, c;

    p = cht->sequence;
    while (*(p++) != 1)
        ;

    do
    {
        c = *p;
        *(buffer++) = c;
        *(p++) = 0;
    } while (c && *p != 0xff);

    if (*p == 0xff)
        *buffer = 0;
}
