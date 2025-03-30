//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 1993-2008 Raven Software
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
//  System interface for sound.
//

#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "std_func.h"
#include "i_system.h"
#include "dstrings.h"
#include "doomstat.h"
#include "options.h"
#include "i_debug.h"
#include "z_zone.h"

int I_GetFileSize(char *filename)
{
    int result;
    FILE *fp;

    fp = fopen(filename, "r");

    if (fp == NULL)
        return -1;

    fseek(fp, 0L, SEEK_END);
    result = ftell(fp);
    fclose(fp);

    return result;
}

int I_ReadTextFile(char *dest, char *filename, int size)
{
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return -1;

    fread(dest, 1, size, fp);
    fclose(fp);
    dest[size] = '\0';

    return 0;
}

char *I_LoadText(char *filename)
{
    int size;
    char *ptr;

    size = I_GetFileSize(filename);

    if (size == -1)
        return NULL;

    ptr = Z_MallocUnowned(size + 1, PU_CACHE);

    SetBytes(ptr, '\0', size + 1);
    I_ReadTextFile(ptr, filename, size);

    return ptr;
}

void I_ReplaceEscapedNewlines(char *str)
{
    char *read_ptr = str;
    char *write_ptr = str;

    while (*read_ptr)
    {
        if (read_ptr[0] == '\\' && read_ptr[1] == 'n')
        {
            *write_ptr++ = '\n';
            read_ptr += 2;
        }
        else
        {
            *write_ptr++ = *read_ptr++;
        }
    }
    *write_ptr = '\0';
}

int I_ReadTextLineFile(char *filename, int line_number, char *buffer, int max_length, char extended_mode)
{
    FILE *f;

    int line = 0;
    int i = 0;
    int c;

    f = fopen(filename, "r");
    if (f == NULL)
    {
        return -1;
    }

    while (line < line_number && (c = fgetc(f)) != EOF)
    {
        if (c == '\n')
            line++;
    }

    if (c == EOF)
    {
        fclose(f);
        return -1;
    }

    while (i < max_length - 1 && (c = fgetc(f)) != EOF && c != '\n')
    {
        buffer[i++] = (char)c;
    }

    buffer[i] = '\0';

    fclose(f);

    if (extended_mode)
        I_ReplaceEscapedNewlines(buffer);

    return 1;
}


char programtext[256];

char *I_LoadTextProgram(int number)
{
    SetDWords(programtext, 0, 256 / 4);

    I_ReadTextLineFile("TEXT\\PROG.TXT", number, programtext, 256, true);

    return programtext;
}

unsigned char *I_ReadBinaryStatic(char *file, int size)
{
    int c;
    int handle;
    unsigned char *ptr;

    ptr = Z_MallocUnowned(size, PU_STATIC);

    handle = open(file, O_RDONLY | O_BINARY);

    c = read(handle, ptr, size);

    close(handle);

    return ptr;
}
