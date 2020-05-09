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
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//	Remark: this was the only stuff that, according
//	 to John Carmack, might have been useful for
//	 Quake.
//

#ifndef __Z_ZONE__
#define __Z_ZONE__

#include <stdio.h>

//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
#define PU_STATIC 1   // static entire execution time
#define PU_SOUND 2    // static while playing
#define PU_MUSIC 3    // static while playing
#define PU_DAVE 4     // anything else Dave wants static
#define PU_LEVEL 50   // static until level exited
#define PU_LEVSPEC 51 // a special thinker in a level
// Tags >= 100 are purgable whenever needed.
#define PU_PURGELEVEL 100
#define PU_CACHE 101

void Z_Init(void);
void *Z_Malloc(int size, int tag, void *ptr);
void Z_Free(void *ptr);
void Z_FreeTags(int lowtag, int hightag);
void Z_CheckHeap(void);
void Z_ChangeTag(void *ptr, int tag);

typedef struct memblock_s
{
    int size;    // including the header and possibly tiny fragments
    void **user; // NULL if a free block
    int tag;     // purgelevel
    int id;      // should be ZONEID
    struct memblock_s *next;
    struct memblock_s *prev;
} memblock_t;

#endif
