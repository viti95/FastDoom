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

#include "std_func.h"
#include "i_system.h"
#include "dstrings.h"
#include "doomstat.h"
#include "options.h"
#include "i_debug.h"

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

int I_ReadTextLineFile(char *filename, int line_number, char *buffer, int max_length)
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

    return 1;
}
