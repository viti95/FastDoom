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
//	DOOM selection menu, options, episode etc.
//	Sliders and icons. Kinda widget stuff.
//

#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "std_func.h"

#include "doomdef.h"
#include "dstrings.h"

#include "d_main.h"

#include "i_system.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"

#include "r_local.h"
#include "r_main.h"

#include "hu_stuff.h"

#include "g_game.h"

#include "m_misc.h"

#include "s_sound.h"

#include "doomstat.h"

#include "sounds.h"

#include "m_bench.h"

#include "options.h"

#include "i_ibm.h"

#if defined(MODE_13H)
#include "i_vga13h.h"
#endif

extern int detailLevel;
extern int screenblocks;
extern int screenblocks;
extern int screenSize;

void M_SetDetail(int value)
{
    detailLevel = value;
    R_SetViewSize(screenblocks, detailLevel);
}

void M_SetVisplaneDetail(int value)
{
    visplaneRender = value;
    R_SetViewSize(screenblocks, detailLevel);
}

void M_SetWallDetail(int value)
{
    wallRender = value;
    R_SetViewSize(screenblocks, detailLevel);
}

void M_SetSpriteDetail(int value)
{
    spriteRender = value;
    R_SetViewSize(screenblocks, detailLevel);
}

void M_SetPSpriteDetail(int value)
{
    pspriteRender = value;
    R_SetViewSize(screenblocks, detailLevel);
}

void M_SetSkyDetail(boolean value)
{
    flatSky = value;
    R_SetViewSize(screenblocks, detailLevel);
}

void M_SetCPU(int value)
{
    selectedCPU = value;
    R_ExecuteSetViewSize();
    R_SetViewSize(screenblocks, detailLevel);

#if defined(MODE_13H)
    I_UpdateFinishFunc();
#endif
}

void M_SetInvisibleDetail(int value)
{
    invisibleRender = value;

    if (invisibleRender == INVISIBLE_TRANSLUCENT)
        R_InitTintMap();
    else
        R_CleanupTintMap();

    R_SetViewSize(screenblocks, detailLevel);
}

void M_SetBusSpeed(boolean value)
{
    busSpeed = value;

#if defined(MODE_13H)
    I_UpdateFinishFunc();
#endif
}

void M_SetUncapped(boolean value)
{
    uncappedFPS = value;
    
    if (uncappedFPS)
    {
        highResTimer = gamestate == GS_LEVEL;
    } else {
        highResTimer = false;
    }

    I_SetHrTimerEnabled(highResTimer);
}

void M_SetSizeDisplay(int value)
{
    screenSize = value;
    screenblocks = value + 3;

    R_SetViewSize(screenblocks, detailLevel);
}

#define FILE_SEPARATOR ",\n"

int M_CheckValue(char *check, char *compare)
{
    if (strcasecmp(check, compare) == 0)
        return 1;

    return 0;
}

int M_GetNumericValue(char *value)
{
    return strtoul(value, NULL, 0);
}

void M_ChangeValueFile(unsigned int position, char *token)
{
    switch (position)
    {
    // Detail
    case 0:
        if (M_CheckValue(token, "high"))
            M_SetDetail(DETAIL_HIGH);
        if (M_CheckValue(token, "low"))
            M_SetDetail(DETAIL_LOW);
        if (M_CheckValue(token, "potato"))
            M_SetDetail(DETAIL_POTATO);
        break;
    // Size
    case 1:
        M_SetSizeDisplay(M_GetNumericValue(token));
        break;
    // Visplanes
    case 2:
        if (M_CheckValue(token, "default"))
            M_SetVisplaneDetail(VISPLANES_NORMAL);
        if (M_CheckValue(token, "flat"))
            M_SetVisplaneDetail(VISPLANES_FLAT);
        if (M_CheckValue(token, "flatter"))
            M_SetVisplaneDetail(VISPLANES_FLATTER);
        break;
    // Walls
    case 3:
        if (M_CheckValue(token, "default"))
            M_SetWallDetail(WALL_NORMAL);
        if (M_CheckValue(token, "flat"))
            M_SetWallDetail(WALL_FLAT);
        if (M_CheckValue(token, "flatter"))
            M_SetWallDetail(WALL_FLATTER);
        break;
    // Sprites
    case 4:
        if (M_CheckValue(token, "default"))
            M_SetSpriteDetail(SPRITE_NORMAL);
        if (M_CheckValue(token, "flat"))
            M_SetSpriteDetail(SPRITE_FLAT);
        if (M_CheckValue(token, "flatter"))
            M_SetSpriteDetail(SPRITE_FLATTER);
        break;
    // Player Sprites
    case 5:
        if (M_CheckValue(token, "default"))
            M_SetPSpriteDetail(PSPRITE_NORMAL);
        if (M_CheckValue(token, "flat"))
            M_SetPSpriteDetail(PSPRITE_FLAT);
        if (M_CheckValue(token, "flatter"))
            M_SetPSpriteDetail(PSPRITE_FLATTER);
        break;
    // Sky
    case 6:
        if (M_CheckValue(token, "default"))
            M_SetSkyDetail(false);
        if (M_CheckValue(token, "flat"))
            M_SetSkyDetail(true);
        break;
    // Invisible
    case 7:
        if (M_CheckValue(token, "default"))
            M_SetInvisibleDetail(INVISIBLE_NORMAL);
        if (M_CheckValue(token, "saturn"))
            M_SetInvisibleDetail(INVISIBLE_SATURN);
        if (M_CheckValue(token, "flatsaturn"))
            M_SetInvisibleDetail(INVISIBLE_FLAT_SATURN);
        if (M_CheckValue(token, "translucent"))
            M_SetInvisibleDetail(INVISIBLE_TRANSLUCENT);
        if (M_CheckValue(token, "flat"))
            M_SetInvisibleDetail(INVISIBLE_FLAT);
    // Sprite culling
    case 8:
        if (M_CheckValue(token, "far"))
            nearSprites = false;
        if (M_CheckValue(token, "near"))
            nearSprites = true;
        break;
    // Show FPS
    case 9:
        if (M_CheckValue(token, "nofps"))
            showFPS = false;
        if (M_CheckValue(token, "fps"))
            showFPS = true;
        break;
    // Uncapped FPS
    case 10:
        if (M_CheckValue(token, "capped"))
            M_SetUncapped(false);
        if (M_CheckValue(token, "uncapped"))
            M_SetUncapped(true);
    // Melting
    case 11:
        if (M_CheckValue(token, "nomelt"))
            noMelt = true;
        if (M_CheckValue(token, "melt"))
            noMelt = false;
        break;
    // CPU
    case 12:
        if (M_CheckValue(token, "386sx"))
            M_SetCPU(INTEL_386SX);
        if (M_CheckValue(token, "386dx"))
            M_SetCPU(INTEL_386DX);
        if (M_CheckValue(token, "i486"))
            M_SetCPU(INTEL_486);
        if (M_CheckValue(token, "pentium"))
            M_SetCPU(INTEL_PENTIUM);
        if (M_CheckValue(token, "k5"))
            M_SetCPU(AMD_K5);
        if (M_CheckValue(token, "cy386"))
            M_SetCPU(CYRIX_386DLC);
        if (M_CheckValue(token, "cy486"))
            M_SetCPU(CYRIX_486);
        if (M_CheckValue(token, "cy5x86"))
            M_SetCPU(CYRIX_5X86);
        if (M_CheckValue(token, "umc486"))
            M_SetCPU(UMC_GREEN_486);
        break;
    // Bus Speed
    case 13:
        if (M_CheckValue(token, "slow"))
            M_SetBusSpeed(1);
        if (M_CheckValue(token, "fast"))
            M_SetBusSpeed(0);
    }
}

void M_ParseBenchmarkLine(char *line)
{
    unsigned int count = 0;
    char *token = strtok(line, FILE_SEPARATOR);
    while (token != NULL)
    {
        M_ChangeValueFile(count, token);
        token = strtok(NULL, FILE_SEPARATOR);
        count++;
    }
}

int M_ProcessBenchmarkFile(const char *filename, int lineNumber)
{
    char buffer[1024];
    int currentLine = 0;

    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        return 0;
    }

    fgets(buffer, sizeof(buffer), file); // Skip first line

    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        if (currentLine == lineNumber)
        {
            M_ParseBenchmarkLine(buffer);
            break;
        }
        currentLine++;
    }

    fclose(file);

    return 1;
}

void M_UpdateSettingsFile(void)
{
    M_ProcessBenchmarkFile(benchmark_file, benchmark_number);
}

void M_UpdateSettings(void)
{
    if (benchmark_type > 0)
        M_UpdateSettingsFile();
}
