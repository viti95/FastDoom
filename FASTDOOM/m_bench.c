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
extern int screenSize;

#define FILE_SEPARATOR ",\n"

//
// Benchmark option tables
//

static const char *detailNames[] = { "high", "low", "potato" };
static const int detailValues[] = { DETAIL_HIGH, DETAIL_LOW, DETAIL_POTATO };

static const char *modeNames[] = { "default", "flat", "flatter" };
static const int visplaneValues[] = { VISPLANES_NORMAL, VISPLANES_FLAT, VISPLANES_FLATTER };
static const int wallValues[] = { WALL_NORMAL, WALL_FLAT, WALL_FLATTER };
static const int spriteValues[] = { SPRITE_NORMAL, SPRITE_FLAT, SPRITE_FLATTER };
static const int pspriteValues[] = { PSPRITE_NORMAL, PSPRITE_FLAT, PSPRITE_FLATTER };

static const char *skyNames[] = { "default", "flat" };
static const char *invisNames[] = { "default", "saturn", "flatsaturn", "translucent", "flat" };
static const int invisValues[] = { INVISIBLE_NORMAL, INVISIBLE_SATURN, INVISIBLE_FLAT_SATURN, INVISIBLE_TRANSLUCENT, INVISIBLE_FLAT };
static const char *nearNames[] = { "far", "near" };
static const char *fpsNames[] = { "nofps", "fps" };
static const char *capNames[] = { "capped", "uncapped" };
static const char *meltNames[] = { "nomelt", "melt" };
static const char *busNames[] = { "slow", "fast" };
static const int offOnValues[] = { false, true };
static const int onOffValues[] = { true, false };

static const char *cpuNames[] = {
    "386sx", "386dx", "i486", "pentium", "pentiumP54CS", "pentiumMMX",
    "pentiumII", "k5", "k6", "cy386", "cy486", "cy5x86", "cy6x86",
    "cy6x86mx", "umc486", "winchip", "mp6"
};
static const int cpuValues[] = {
    INTEL_386SX, INTEL_386DX, INTEL_486, INTEL_PENTIUM_P5_P54C,
    INTEL_PENTIUM_P54CS, INTEL_PENTIUM_MMX, INTEL_PENTIUM_II,
    AMD_K5, AMD_K6, CYRIX_386DLC, CYRIX_486, CYRIX_5X86,
    CYRIX_6X86, CYRIX_6X86MX, UMC_GREEN_486, IDT_WINCHIP, RISE_MP6
};

//
// Set var to the value matching token, do nothing if not found.
// Returns the matched value, or -1 if not found.
// All benchmark variables are int sized (boolean is an int enum).
//

static int M_SetValue(void *var, char *token, const char **names, const int *values, int count)
{
    int i;

    for (i = 0; i < count; i++)
        if (strcasecmp(token, names[i]) == 0)
        {
            *(int *)var = values[i];
            return values[i];
        }

    return -1;
}

void M_ChangeValueFile(unsigned int position, char *token)
{
    int value;

    switch (position)
    {
    // Detail
    case 0:
        M_SetValue(&detailLevel, token, detailNames, detailValues, 3);
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
        M_SetValue(&visplaneRender, token, modeNames, visplaneValues, 3);
        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Walls
    case 3:
        M_SetValue(&wallRender, token, modeNames, wallValues, 3);
        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Sprites
    case 4:
        M_SetValue(&spriteRender, token, modeNames, spriteValues, 3);
        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Player Sprites
    case 5:
        M_SetValue(&pspriteRender, token, modeNames, pspriteValues, 3);
        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Sky
    case 6:
        M_SetValue(&flatSky, token, skyNames, offOnValues, 2);
        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Invisible
    case 7:
        M_SetValue(&invisibleRender, token, invisNames, invisValues, 5);

        if (invisibleRender == INVISIBLE_TRANSLUCENT)
            R_InitTintMap();
        else
            R_CleanupTintMap();

        R_SetViewSize(screenblocks, detailLevel);
        break;
    // Sprite culling
    case 8:
        M_SetValue(&nearSprites, token, nearNames, offOnValues, 2);
        break;
    // Show FPS
    case 9:
        M_SetValue(&showFPS, token, fpsNames, offOnValues, 2);
        break;
    // Uncapped FPS
    case 10:
        if (M_SetValue(&uncappedFPS, token, capNames, offOnValues, 2) != -1)
        {
            highResTimer = uncappedFPS && (gamestate == GS_LEVEL);
            I_SetHrTimerEnabled(highResTimer);
        }
        break;
    // Melting
    case 11:
        M_SetValue(&noMelt, token, meltNames, onOffValues, 2);
        break;
    // CPU
    case 12:
        M_SetValue(&selectedCPU, token, cpuNames, cpuValues, 17);
        R_ExecuteSetViewSize();
        R_SetViewSize(screenblocks, detailLevel);
#if defined(MODE_13H)
        I_UpdateFinishFunc();
#endif
        break;
    // Bus Speed
    case 13:
        M_SetValue(&busSpeed, token, busNames, onOffValues, 2);
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
