//
// Copyright (C) 1993-1996 id Software, Inc.
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

#ifndef __I_FILE__
#define __I_FILE__

#define MAX_TEXT_SIZE_PROGRAM 640

extern char programtext[MAX_TEXT_SIZE_PROGRAM];

int I_ReadTextLineFile(char *filename, int line_number, char *buffer, int max_length, char extended_mode);
unsigned char *I_ReadBinaryStatic(char *file, int size);
char *I_LoadTextProgram(int number);
void I_GetProgFilePositionCache(void);

#endif
