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

#define FILE_SEPARATOR ",\n"

int M_CheckValue(char *check, char *compare)
{
    if (strcasecmp(check, compare) == 0)
        return 1;

    return 0;
}

void M_ChangeValueFile(unsigned int position, char *token)
{
    int value;

    switch (position)
    {
    // Detail
    case 0:
        if (M_CheckValue(token, "high"))
            detailLevel = DETAIL_HIGH;
        if (M_CheckValue(token, "low"))
            detailLevel = DETAIL_LOW;
        if (M_CheckValue(token, "potato"))
            detailLevel = DETAIL_POTATO;

        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Size
    case 1:
        value = strtoul(token, NULL, 0);

        screenSize = value;
        screenblocks = value + 3;

        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Visplanes
    case 2:
        if (M_CheckValue(token, "default"))
            visplaneRender = VISPLANES_NORMAL;
        if (M_CheckValue(token, "flat"))
            visplaneRender = VISPLANES_FLAT;
        if (M_CheckValue(token, "flatter"))
            visplaneRender = VISPLANES_FLATTER;

        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Walls
    case 3:
        if (M_CheckValue(token, "default"))
            wallRender = WALL_NORMAL;
        if (M_CheckValue(token, "flat"))
            wallRender = WALL_FLAT;
        if (M_CheckValue(token, "flatter"))
            wallRender = WALL_FLATTER;

        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Sprites
    case 4:
        if (M_CheckValue(token, "default"))
            spriteRender = SPRITE_NORMAL;
        if (M_CheckValue(token, "flat"))
            spriteRender = SPRITE_FLAT;
        if (M_CheckValue(token, "flatter"))
            spriteRender = SPRITE_FLATTER;

        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Player Sprites
    case 5:
        if (M_CheckValue(token, "default"))
            pspriteRender = PSPRITE_NORMAL;
        if (M_CheckValue(token, "flat"))
            pspriteRender = PSPRITE_FLAT;
        if (M_CheckValue(token, "flatter"))
            pspriteRender = PSPRITE_FLATTER;

        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Sky
    case 6:
        if (M_CheckValue(token, "default"))
            flatSky = false;
        if (M_CheckValue(token, "flat"))
            flatSky = true;

        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Invisible
    case 7:
        if (M_CheckValue(token, "default"))
            invisibleRender = INVISIBLE_NORMAL;
        if (M_CheckValue(token, "saturn"))
            invisibleRender = INVISIBLE_SATURN;
        if (M_CheckValue(token, "flatsaturn"))
            invisibleRender = INVISIBLE_FLAT_SATURN;
        if (M_CheckValue(token, "translucent"))
            invisibleRender = INVISIBLE_TRANSLUCENT;
        if (M_CheckValue(token, "flat"))
            invisibleRender = INVISIBLE_FLAT;

        if (invisibleRender == INVISIBLE_TRANSLUCENT)
            R_InitTintMap();
        else
            R_CleanupTintMap();

        R_SetViewSize(screenblocks, detailLevel);
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
            uncappedFPS = false;
        if (M_CheckValue(token, "uncapped"))
            uncappedFPS = true;

        if (uncappedFPS)
        {
            highResTimer = gamestate == GS_LEVEL;
        }
        else
        {
            highResTimer = false;
        }

        I_SetHrTimerEnabled(highResTimer);
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
            selectedCPU = INTEL_386SX;
        if (M_CheckValue(token, "386dx"))
            selectedCPU = INTEL_386DX;
        if (M_CheckValue(token, "i486"))
            selectedCPU = INTEL_486;
        if (M_CheckValue(token, "pentium"))
            selectedCPU = INTEL_PENTIUM_P5_P54C;
        if (M_CheckValue(token, "pentiumP54CS"))
            selectedCPU = INTEL_PENTIUM_P54CS;
        if (M_CheckValue(token, "pentiumMMX"))
            selectedCPU = INTEL_PENTIUM_MMX;
        if (M_CheckValue(token, "k5"))
            selectedCPU = AMD_K5;
        if (M_CheckValue(token, "k6"))
            selectedCPU = AMD_K6;
        if (M_CheckValue(token, "cy386"))
            selectedCPU = CYRIX_386DLC;
        if (M_CheckValue(token, "cy486"))
            selectedCPU = CYRIX_486;
        if (M_CheckValue(token, "cy5x86"))
            selectedCPU = CYRIX_5X86;
        if (M_CheckValue(token, "cy6x86"))
            selectedCPU = CYRIX_6X86;
        if (M_CheckValue(token, "cy6x86mx"))
            selectedCPU = CYRIX_6X86MX;
        if (M_CheckValue(token, "umc486"))
            selectedCPU = UMC_GREEN_486;
        if (M_CheckValue(token, "winchip"))
            selectedCPU = IDT_WINCHIP;
        if (M_CheckValue(token, "mp6"))
            selectedCPU = RISE_MP6;

        R_ExecuteSetViewSize();
        R_SetViewSize(screenblocks, detailLevel);

#if defined(MODE_13H)
        I_UpdateFinishFunc();
#endif
        break;
    // Bus Speed
    case 13:
        if (M_CheckValue(token, "slow"))
            busSpeed = 1;
        if (M_CheckValue(token, "fast"))
            busSpeed = 0;

#if defined(MODE_13H)
        I_UpdateFinishFunc();
#endif
        break;
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

void M_UpdateSettings(void)
{
    if (benchmark_type > 0)
        M_ProcessBenchmarkFile(benchmark_file, benchmark_number);
}
