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
//	Archiving: SaveGame I/O.
//	Thinker, Ticker.
//

#include "z_zone.h"
#include "p_local.h"

#include "doomstat.h"

int leveltime;

//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//

// Both the head and tail of the thinker list.
thinker_t thinkercap;

//
// P_InitThinkers
//
void P_InitThinkers(void)
{
    thinkercap.prev = thinkercap.next = &thinkercap;
}

//
// P_RunThinkers
//
void P_RunThinkers(void)
{
    thinker_t *currentthinker;

    currentthinker = thinkercap.next;
    while (currentthinker != &thinkercap)
    {
        if (currentthinker->function.acv == (actionf_v)(-1))
        {
            // time to remove it
            currentthinker->next->prev = currentthinker->prev;
            currentthinker->prev->next = currentthinker->next;
            Z_Free(currentthinker);
            currentthinker = currentthinker->next;
            continue;
        }
        else if (currentthinker->function.acp1 == 0 || currentthinker->function.acp1 == (actionf_p1)P_MobjTicklessThinker)
        {
            currentthinker = currentthinker->next;
            continue;
        }

        currentthinker->function.acp1(currentthinker);

        currentthinker = currentthinker->next;
    }
}

//
// P_Ticker
//

void P_Ticker(void)
{
    int i;

    // run the tic, pause if in menu and at least one tic has been run
    if (paused || (menuactive && !demoplayback && players.viewz != 1))
        return;

    P_PlayerThink();

    P_RunThinkers();
    P_UpdateSpecials();

    // for par times
    leveltime++;
}
