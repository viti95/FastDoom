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
//	WAD I/O functions.
//

#ifndef __W_WAD__
#define __W_WAD__

//
// TYPES
//
typedef struct
{
    // Should be "IWAD" or "PWAD".
    char identification[4];
    int numlumps;
    int infotableofs;

} wadinfo_t;

typedef struct
{
    int filepos;
    int size;
    char name[8];

} filelump_t;

//
// WADFILE I/O related stuff.
//
typedef struct
{
    char name[8];
    int handle;
    int position;
    int size;
    // Used for hash table lookups
    int next;
} lumpinfo_t;

extern void **lumpcache;
extern lumpinfo_t *lumpinfo;
extern int numlumps;

void W_InitMultipleFiles(char **filenames);

/* Preload all lumps into cache (for systems with plenty of RAM). */
void W_PreloadAllLumps(void);

short W_GetNumForName(char *name);

#define W_LumpLength(lump) lumpinfo[lump].size

void W_ReadLump(int lump, void *dest);

void *W_CacheLumpNum(int lump, byte tag);

/* VITI95: OPTIMIZE: W_CachedLumpBase()
 * Gives "(base) + (offset)" for a WAD lump whose address is kept in (base),
 * reloading the lump into the cache only when (base) is not its current
 * address.  Z_Malloc() clears the lumpcache[] entry of a purged block, so
 * comparing the last used address with lumpcache[] detects a purged lump
 * without entering the cache at all.  Rendering loops use this to fetch the
 * lump of a texture column once per lump instead of once per column.
 * (base) must be an lvalue of type byte *, and PU_CACHE must be defined.
 */
#define W_CachedLumpBase(base, lump, offset)                                   \
    ((base) == lumpcache[(lump)]                                                \
     ? (base) + (offset)                                                        \
     : ((base) = (byte *)W_CacheLumpNum((lump), PU_CACHE)) + (offset))

#define W_CacheLumpName(name, tag) W_CacheLumpNum(W_GetNumForName((name)), (tag))

void W_GenerateHashTable(void);

extern unsigned int W_LumpNameHash(char *s);

#endif
