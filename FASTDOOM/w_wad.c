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
//	Handles WAD file header, directory, lump I/O.
//

#include "std_func.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include "doomtype.h"
#include "doomstat.h"
#include "i_system.h"
#include "z_zone.h"
#include "options.h"
#include "w_wad.h"

#define HASHTABLESIZE 4096

#ifdef MAC
#define O_BINARY 0
#endif

//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t *lumpinfo;
int numlumps;

// Hash table for fast lookups
short lumphash[HASHTABLESIZE];

void **lumpcache;

void ExtractFileBase(char *path, char *dest)
{
    char *src;

    src = path + strlen(path) - 1;

    // back up until a \ or the start
    while (src != path && *(src - 1) != '\\' && *(src - 1) != '/')
    {
        src--;
    }

    // copy up to eight characters
    memset(dest, 0, 8);

    while (*src && *src != '.')
    {
    	int t = (int)*src++;
		*dest++ = toupper(t);
    }
}

//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...

int reloadlump;
char *reloadname;

void W_AddFile(char *filename)
{
    wadinfo_t header;
    lumpinfo_t *lump_p;
    unsigned i;
    int handle;
    int length;
    int startlump;
    filelump_t *fileinfo;
    filelump_t *fileinfoptr = NULL;
    filelump_t singleinfo;
    int storehandle;

    // open the file and add to directory

    // handle reload indicator.
    if (filename[0] == '~')
    {
        filename++;
        reloadname = filename;
        reloadlump = numlumps;
    }

    if ((handle = open(filename, O_RDONLY | O_BINARY)) == -1)
    {
        printf("\tcouldn't open %s\n", filename);
        return;
    }

    printf("\tadding %s\n", filename);
    startlump = numlumps;

    if (strcmpi(filename + strlen(filename) - 3, "wad"))
    {
        // single lump file
        fileinfo = &singleinfo;
        singleinfo.filepos = 0;
        singleinfo.size = filelength(handle);
        ExtractFileBase(filename, singleinfo.name);
        numlumps++;
    }
    else
    {
        // WAD file
        read(handle, &header, sizeof(header));
        if (strncmp(header.identification, "IWAD", 4))
        {
            modifiedgame = true;
        }
        length = header.numlumps * sizeof(filelump_t);
        fileinfoptr = Z_MallocUnowned(length, PU_STATIC);
        fileinfo = fileinfoptr;
        lseek(handle, header.infotableofs, SEEK_SET);
        read(handle, fileinfo, length);
        numlumps += header.numlumps;
    }

    // Fill in lumpinfo
    lumpinfo = Z_ReallocUnowned(lumpinfo, numlumps * sizeof(lumpinfo_t), PU_STATIC);

    lump_p = &lumpinfo[startlump];

    storehandle = reloadname ? -1 : handle;

    for (i = startlump; i < numlumps; i++, lump_p++, fileinfo++)
    {
        lump_p->handle = storehandle;
        lump_p->position = fileinfo->filepos;
        lump_p->size = fileinfo->size;
        strncpy(lump_p->name, fileinfo->name, 8);
    }

    if (reloadname)
        close(handle);

    if (fileinfoptr != NULL)
        Z_Free(fileinfoptr);
}

//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles(char **filenames)
{
    int size;

    // open all the files, load headers, and count lumps
    numlumps = 0;

    // will be realloced as lumps are added
    lumpinfo = Z_MallocUnowned(1, PU_STATIC);

    for (; *filenames; filenames++)
        W_AddFile(*filenames);

    // set up caching
    size = numlumps * sizeof(*lumpcache);
    lumpcache = Z_MallocUnowned(size, PU_STATIC);

    memset(lumpcache, 0, size);

    W_GenerateHashTable();
}

// Hash function used for lump names.
unsigned int W_LumpNameHash(char *s)
{
    unsigned hash;
    (void)((hash = toupperint((int)(s[0])), s[1]) &&
           (hash = hash * 2 + toupperint((int)(s[1])), s[2]) &&
           (hash = hash * 2 + toupperint((int)(s[2])), s[3]) &&
           (hash = hash * 2 + toupperint((int)(s[3])), s[4]) &&
           (hash = hash * 2 + toupperint((int)(s[4])), s[5]) &&
           (hash = hash * 2 + toupperint((int)(s[5])), s[6]) &&
           (hash = hash * 2 + toupperint((int)(s[6])),
            hash = hash + toupperint((int)(s[7]))));
    return hash;
}

//
// W_GetNumForName
// Returns -1 if name not found.
//

short W_GetNumForName(char *name)
{
    short i;
    short hash;

    hash = W_LumpNameHash(name) & (short)(HASHTABLESIZE - 1);

    for (i = lumphash[hash]; i != -1; i = lumpinfo[i].next)
    {
        if (!strncasecmp(lumpinfo[i].name, name, 8))
        {
            return i;
        }
    }

    // TFB. Not found.

    return -1;
}

//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(int lump,
                void *dest)
{
    int c;
    lumpinfo_t *l;
    int handle;

    l = lumpinfo + lump;

    if (l->handle == -1)
    {
        // reloadable file, so use open / read / close
        handle = open(reloadname, O_RDONLY | O_BINARY);
    }
    else
        handle = l->handle;

    lseek(handle, l->position, SEEK_SET);
    c = read(handle, dest, l->size);

    if (l->handle == -1)
        close(handle);
}

//
// W_CacheLumpNum
//
void * W_CacheLumpNum(int lump, byte tag)
{
    byte *ptr = lumpcache[lump];

    if (!ptr)
    {
        // read the lump in
        ptr = Z_Malloc(W_LumpLength(lump), tag, &lumpcache[lump]);
        W_ReadLump(lump, ptr);
    }
    else
    {
        Z_ChangeTag(ptr, tag);
    }

    return ptr;
}

// Generate a hash table for fast lookups

void W_GenerateHashTable(void)
{
    int i;

    // Generate hash table
    if (numlumps > 0)
    {
        SetWords(lumphash, -1, HASHTABLESIZE);

        for (i = 0; i < numlumps; ++i)
        {
            unsigned int hash;

            hash = W_LumpNameHash(lumpinfo[i].name) & (int)(HASHTABLESIZE - 1);

            // Hook into the hash table

            lumpinfo[i].next = lumphash[hash];
            lumphash[hash] = i;
        }
    }

    // All done!
}
