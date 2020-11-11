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
//	Networking stuff.
//

#ifndef __D_NET__
#define __D_NET__

#include "i_ibm.h"

#include "d_player.h"

//
// Network play related stuff.
// There is a data struct that stores network
//  communication related stuff, and another
//  one that defines the actual packets to
//  be transmitted.
//

// Networking and tick handling related.
#define BACKUPTICS 16

//
// Network packet data.
//
typedef struct
{
    byte starttic;
    byte numtics;

} doomdata_t;

// Create any new ticcmds and broadcast to other players.
void NetUpdate(void);

//? how many ticks to run?
void TryRunTics(void);

#endif
