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
//	Zone Memory Allocation. Neat.
//

#include <string.h>
#include "options.h"
#include "z_zone.h"
#include "i_system.h"
#include "doomdef.h"
#include "s_sound.h"

//
// ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// The rover can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
//

typedef struct
{
    // total bytes malloced, including header
    int size;

    // start / end cap for linked list
    memblock_t blocklist;

    memblock_t *rover;

} memzone_t;

memzone_t *mainzone;

//
// Z_Init
//
void Z_Init(void)
{
    memblock_t *block;
    int size;

    mainzone = (memzone_t *)I_ZoneBase(&size);
    mainzone->size = size;

    // set the entire zone to one free block
    mainzone->blocklist.next =
        mainzone->blocklist.prev =
            block = (memblock_t *)((byte *)mainzone + sizeof(memzone_t));

    mainzone->blocklist.user = (void *)mainzone;
    mainzone->blocklist.tag = PU_STATIC;
    mainzone->rover = block;

    block->prev = block->next = &mainzone->blocklist;

    // NULL indicates a free block.
    block->user = NULL;

    block->size = mainzone->size - sizeof(memzone_t);
}

//
// Z_Free
//
void Z_Free(void *ptr)
{
    memblock_t *block;
    memblock_t *other;

    block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));

    if (block->user > (void **)0x100)
    {
        // smaller values are not pointers
        // Note: OS-dependend?

        // clear the user's mark
        *block->user = 0;
    }

    // mark as free
    block->user = NULL;
    block->tag = 0;

    other = block->prev;

    if (!other->user)
    {
        // merge with previous free block
        other->size += block->size;
        other->next = block->next;
        other->next->prev = other;

        if (block == mainzone->rover)
            mainzone->rover = other;

        block = other;
    }

    other = block->next;
    if (!other->user)
    {
        // merge the next free block onto the end
        block->size += other->size;
        block->next = other->next;
        block->next->prev = block;

        if (other == mainzone->rover)
            mainzone->rover = block;
    }
}

//
// Z_Malloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.
//
#define MINFRAGMENT sizeof(memblock_t)

byte emergency = 0;

void *Z_Malloc(int size, byte tag, void *user)
{
    int extra;
    memblock_t *start;
    memblock_t *rover;
    memblock_t *newblock;
    memblock_t *base;

    size = (size + 3) & ~3;

    // scan through the block list,
    // looking for the first free block
    // of sufficient size,
    // throwing out any purgable blocks along the way.

    // account for size of block header
    size += sizeof(memblock_t);

    // if there is a free block behind the rover,
    //  back up over them
    base = mainzone->rover;

    if (!base->prev->user)
        base = base->prev;

    rover = base;
    start = base->prev;

    while (1)
    {
        if (base->size >= size && !base->user)
        {
            // Found a free block of sufficient size
            break;
        }
        if (rover->next == start)
        {
            // scanned all the way around the list
            if (emergency == 1)
                I_Error("Z_Malloc: failed on allocation of %i bytes", size);
            else
                return Z_MallocEmergency(size, tag, user);
        }
        if (rover->user)
        {
            if (rover->tag < PU_PURGELEVEL)
            {
                // hit a block that can't be purged,
                //  so move base past it
                base = rover = rover->next;
            }
            else
            {
                // free the rover block (adding the size to base)

                // the rover can be the base block
                base = base->prev;
                Z_Free((byte *)rover + sizeof(memblock_t));
                base = base->next;
                rover = base->next;
            }
        }
        else
            rover = rover->next;
    }

    // found a block big enough
    extra = base->size - size;

    if (extra > MINFRAGMENT)
    {
        // there will be a free fragment after the allocated block
        newblock = (memblock_t *)((byte *)base + size);
        newblock->size = extra;

        // NULL indicates free block.
        newblock->user = NULL;
        newblock->tag = 0;
        newblock->prev = base;
        newblock->next = base->next;
        newblock->next->prev = newblock;

        base->next = newblock;
        base->size = size;
    }

    // mark as an in use block
    base->user = user;
    *(void **)user = (void *)((byte *)base + sizeof(memblock_t));
    base->tag = tag;

    // next allocation will start looking here
    mainzone->rover = base->next;
    emergency = 0;

    return (void *)((byte *)base + sizeof(memblock_t));
}

void *Z_MallocUnowned(int size, byte tag)
{
    int extra;
    memblock_t *start;
    memblock_t *rover;
    memblock_t *newblock;
    memblock_t *base;

    size = (size + 3) & ~3;

    // scan through the block list,
    // looking for the first free block
    // of sufficient size,
    // throwing out any purgable blocks along the way.

    // account for size of block header
    size += sizeof(memblock_t);

    // if there is a free block behind the rover,
    //  back up over them
    base = mainzone->rover;

    if (!base->prev->user)
        base = base->prev;

    rover = base;
    start = base->prev;

    while (1)
    {
        if (base->size >= size && !base->user)
        {
            // Found a free block of sufficient size
            break;
        }
        if (rover->next == start)
        {
            // scanned all the way around the list
            if (emergency == 1)
                I_Error("Z_Malloc: failed on allocation of %i bytes", size);
            else
                return Z_MallocEmergencyUnowned(size, tag);
        }
        if (rover->user)
        {
            if (rover->tag < PU_PURGELEVEL)
            {
                // hit a block that can't be purged,
                //  so move base past it
                base = rover = rover->next;
            }
            else
            {
                // free the rover block (adding the size to base)

                // the rover can be the base block
                base = base->prev;
                Z_Free((byte *)rover + sizeof(memblock_t));
                base = base->next;
                rover = base->next;
            }
        }
        else
            rover = rover->next;
    };

    // found a block big enough
    extra = base->size - size;

    if (extra > MINFRAGMENT)
    {
        // there will be a free fragment after the allocated block
        newblock = (memblock_t *)((byte *)base + size);
        newblock->size = extra;

        // NULL indicates free block.
        newblock->user = NULL;
        newblock->tag = 0;
        newblock->prev = base;
        newblock->next = base->next;
        newblock->next->prev = newblock;

        base->next = newblock;
        base->size = size;
    }

    base->user = (void *)2;
    base->tag = tag;

    // next allocation will start looking here
    mainzone->rover = base->next;

    emergency = 0;
    return (void *)((byte *)base + sizeof(memblock_t));
}

void *Z_ReallocUnowned(void *ptr, int n, byte tag)
{
    void *p = Z_MallocUnowned(n, tag);
    if (ptr)
    {
        memblock_t *block = (memblock_t *)((byte *)ptr - MINFRAGMENT);
        memcpy(p, ptr, n <= block->size ? n : block->size);
        Z_Free(ptr);
    }
    return p;
}

// Cleanup as memory as possible and try to malloc again
void *Z_MallocEmergency(int size, byte tag, void *user)
{
    emergency = 1;
    S_ClearUnusedSounds();
    return Z_Malloc(size, tag, user);
}

void *Z_MallocEmergencyUnowned(int size, byte tag)
{
    emergency = 1;
    S_ClearUnusedSounds();
    return Z_MallocUnowned(size, tag);
}

//
// Z_FreeTags
//
void Z_FreeTags(byte lowtag, byte hightag)
{
    memblock_t *block;
    memblock_t *next;

    for (block = mainzone->blocklist.next;
         block != &mainzone->blocklist;
         block = next)
    {
        // get link before freeing
        next = block->next;

        // free block?
        if (!block->user)
            continue;

        if (block->tag >= lowtag && block->tag <= hightag)
            Z_Free((byte *)block + sizeof(memblock_t));
    }
}
