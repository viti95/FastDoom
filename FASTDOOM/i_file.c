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
#include "i_file.h"

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

#define TOTAL_LINES 227
unsigned int linePosition[TOTAL_LINES];

void I_GetProgFilePositionCache()
{

    unsigned int index = 1;
    int c;
    unsigned int position = 0;

    FILE *f = fopen("TEXT\\PROG.TXT", "rb");

    if (f == NULL)
    {
        return;
    }

    linePosition[0] = 0;

    while ((c = fgetc(f)) != EOF)
    {
        if (c == '\n')
        {
            if (index < TOTAL_LINES)
            {
                linePosition[index++] = position + 1;
            }
            else
            {
                break;
            }
        }
        position++;
    }

    fclose(f);
}

int I_ReadTextLineFileCache(char *filename, int line_number, char *buffer, int max_length, char extended_mode)
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

    fseek(f, linePosition[line_number], SEEK_SET);

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

char programtext[MAX_TEXT_SIZE_PROGRAM];
int lastText = -1;

char *I_LoadTextProgram(int number)
{
    if (lastText == number)
        return programtext;

    SetDWords(programtext, 0, MAX_TEXT_SIZE_PROGRAM / 4);

    I_ReadTextLineFileCache("TEXT\\PROG.TXT", number, programtext, MAX_TEXT_SIZE_PROGRAM, true);

    lastText = number;

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
