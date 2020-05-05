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

#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <alloca.h>

#include "doomtype.h"
#include "doomstat.h"
#include "i_system.h"
#include "z_zone.h"

#include "w_wad.h"

//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t *lumpinfo;
int numlumps;

// Hash table for fast lookups
int *lumphash;

void **lumpcache;

void ExtractFileBase(char *path,
                     char *dest)
{
    char *src;
    int length;

    src = path + strlen(path) - 1;

    // back up until a \ or the start
    while (src != path && *(src - 1) != '\\' && *(src - 1) != '/')
    {
        src--;
    }

    // copy up to eight characters
    memset(dest, 0, 8);
    length = 0;

    while (*src && *src != '.')
    {
        if (++length == 9)
            I_Error("Filename base of %s >8 chars", path);

        *dest++ = toupper((int)*src++);
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
        singleinfo.size = LONG(filelength(handle));
        ExtractFileBase(filename, singleinfo.name);
        numlumps++;
    }
    else
    {
        // WAD file
        read(handle, &header, sizeof(header));
        if (strncmp(header.identification, "IWAD", 4))
        {
            // Homebrew levels?
            if (strncmp(header.identification, "PWAD", 4))
            {
                I_Error("Wad file %s doesn't have IWAD "
                        "or PWAD id\n",
                        filename);
            }

            modifiedgame = true;
        }
        header.numlumps = LONG(header.numlumps);
        header.infotableofs = LONG(header.infotableofs);
        length = header.numlumps * sizeof(filelump_t);
        fileinfo = alloca(length);
        lseek(handle, header.infotableofs, SEEK_SET);
        read(handle, fileinfo, length);
        numlumps += header.numlumps;
    }

    // Fill in lumpinfo
    lumpinfo = realloc(lumpinfo, numlumps * sizeof(lumpinfo_t));

    if (!lumpinfo)
        I_Error("Couldn't realloc lumpinfo");

    lump_p = &lumpinfo[startlump];

    storehandle = reloadname ? -1 : handle;

    for (i = startlump; i < numlumps; i++, lump_p++, fileinfo++)
    {
        lump_p->handle = storehandle;
        lump_p->position = LONG(fileinfo->filepos);
        lump_p->size = LONG(fileinfo->size);
        strncpy(lump_p->name, fileinfo->name, 8);
    }

    if (reloadname)
        close(handle);
}

//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
void W_Reload(void)
{
    wadinfo_t header;
    int lumpcount;
    lumpinfo_t *lump_p;
    unsigned i;
    int handle;
    int length;
    filelump_t *fileinfo;

    if (!reloadname)
        return;

    if ((handle = open(reloadname, O_RDONLY | O_BINARY)) == -1)
        I_Error("W_Reload: couldn't open %s", reloadname);

    read(handle, &header, sizeof(header));
    lumpcount = LONG(header.numlumps);
    header.infotableofs = LONG(header.infotableofs);
    length = lumpcount * sizeof(filelump_t);
    fileinfo = alloca(length);
    lseek(handle, header.infotableofs, SEEK_SET);
    read(handle, fileinfo, length);

    // Fill in lumpinfo
    lump_p = &lumpinfo[reloadlump];

    for (i = reloadlump;
         i < reloadlump + lumpcount;
         i++, lump_p++, fileinfo++)
    {
        if (lumpcache[i])
            Z_Free(lumpcache[i]);

        lump_p->position = LONG(fileinfo->filepos);
        lump_p->size = LONG(fileinfo->size);
    }

    close(handle);
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
    lumpinfo = malloc(1);

    for (; *filenames; filenames++)
        W_AddFile(*filenames);

    if (!numlumps)
        I_Error("W_InitFiles: no files found");

    // set up caching
    size = numlumps * sizeof(*lumpcache);
    lumpcache = malloc(size);

    if (!lumpcache)
        I_Error("Couldn't allocate lumpcache");

    memset(lumpcache, 0, size);

    W_GenerateHashTable();
}

//
// W_InitFile
// Just initialize from a single file.
//
void W_InitFile(char *filename)
{
    char *names[2];

    names[0] = filename;
    names[1] = NULL;
    W_InitMultipleFiles(names);
}

//
// W_NumLumps
//
int W_NumLumps(void)
{
    return numlumps;
}

// Hash function used for lump names.
unsigned int W_LumpNameHash(char *s)
{
  unsigned hash;
  (void) ((hash =        toupper(s[0]), s[1]) &&
          (hash = hash*3+toupper(s[1]), s[2]) &&
          (hash = hash*2+toupper(s[2]), s[3]) &&
          (hash = hash*2+toupper(s[3]), s[4]) &&
          (hash = hash*2+toupper(s[4]), s[5]) &&
          (hash = hash*2+toupper(s[5]), s[6]) &&
          (hash = hash*2+toupper(s[6]),
           hash = hash*2+toupper(s[7]))
         );
  return hash;
}

//
// W_GetNumForName
// Returns -1 if name not found.
//

int W_GetNumForName(char *name)
{
    int i;

    // Do we have a hash table yet?

    if (lumphash != NULL)
    {
        int hash;

        // We do! Excellent.

        hash = W_LumpNameHash(name) % numlumps;

        for (i = lumphash[hash]; i != -1; i = lumpinfo[i].next)
        {
            if (!strncasecmp(lumpinfo[i].name, name, 8))
            {
                return i;
            }
        }
    }
    else
    {
        // We don't have a hash table generate yet. Linear search :-(
        //
        // scan backwards so patch lump files take precedence

        for (i = numlumps - 1; i >= 0; --i)
        {
            if (!strncasecmp(lumpinfo[i].name, name, 8))
            {
                return i;
            }
        }
    }

    // TFB. Not found.

    return -1;
}

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength(int lump)
{
    if (lump >= numlumps)
        I_Error("W_LumpLength: %i >= numlumps", lump);

    return lumpinfo[lump].size;
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

    if (lump >= numlumps)
        I_Error("W_ReadLump: %i >= numlumps", lump);

    l = lumpinfo + lump;

    I_BeginRead();

    if (l->handle == -1)
    {
        // reloadable file, so use open / read / close
        if ((handle = open(reloadname, O_RDONLY | O_BINARY)) == -1)
            I_Error("W_ReadLump: couldn't open %s", reloadname);
    }
    else
        handle = l->handle;

    lseek(handle, l->position, SEEK_SET);
    c = read(handle, dest, l->size);

    if (c < l->size)
        I_Error("W_ReadLump: only read %i of %i on lump %i",
                c, l->size, lump);

    if (l->handle == -1)
        close(handle);

    I_EndRead();
}

//
// W_CacheLumpNum
//
void *
W_CacheLumpNum(int lump,
               int tag)
{
    byte *ptr;

    if ((unsigned)lump >= numlumps)
        I_Error("W_CacheLumpNum: %i >= numlumps", lump);

    if (!lumpcache[lump])
    {
        // read the lump in

        ptr = Z_Malloc(W_LumpLength(lump), tag, &lumpcache[lump]);
        W_ReadLump(lump, lumpcache[lump]);
    }
    else
    {
        Z_ChangeTag(lumpcache[lump], tag);
    }

    return lumpcache[lump];
}

//
// W_CacheLumpName
//
void *
W_CacheLumpName(char *name,
                int tag)
{
    return W_CacheLumpNum(W_GetNumForName(name), tag);
}

// Generate a hash table for fast lookups

void W_GenerateHashTable(void)
{
    int i;

    // Free the old hash table, if there is one:
    if (lumphash != NULL)
    {
        Z_Free(lumphash);
    }

    // Generate hash table
    if (numlumps > 0)
    {
        lumphash = Z_Malloc(sizeof(int) * numlumps, PU_STATIC, NULL);

        for (i = 0; i < numlumps; ++i)
        {
            lumphash[i] = -1;
        }

        for (i = 0; i < numlumps; ++i)
        {
            unsigned int hash;

            hash = W_LumpNameHash(lumpinfo[i].name) % numlumps;

            // Hook into the hash table

            lumpinfo[i].next = lumphash[hash];
            lumphash[hash] = i;
        }
    }

    // All done!
}
