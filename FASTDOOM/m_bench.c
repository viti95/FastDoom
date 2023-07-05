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

void M_SetVsync(boolean value)
{
    waitVsync = value;
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

void M_SetShowFPS(int value)
{
    showFPS = value;
}

void M_SetSpriteCulling(boolean value)
{
    nearSprites = value;
}

void M_SetNoMelting(boolean value)
{
    noMelt = value;
}

void M_SetUncappedFPS(boolean value)
{
    uncappedFPS = value;
}

void M_SetSizeDisplay(int value)
{
    screenSize = value;
    screenblocks = value + 3;

    R_SetViewSize(screenblocks, detailLevel);
}

void M_SetCSV(boolean value)
{
    csv = value;
}

void M_UpdateSettingsPhils(void)
{
    switch (benchmark_number)
    {
    case 0:
        M_SetCSV(true);
        M_SetCPU(INTEL_486);
        M_SetVisplaneDetail(VISPLANES_NORMAL);
        M_SetSkyDetail(false);
        M_SetSpriteCulling(false);
        M_SetInvisibleDetail(INVISIBLE_NORMAL);
        M_SetShowFPS(false);
        M_SetNoMelting(false);

        // %2 -high -size 12 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetDetail(DETAIL_HIGH);
        M_SetSizeDisplay(9);
        break;
    case 1:
        // %2 -low -size 3 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetDetail(DETAIL_LOW);
        M_SetSizeDisplay(0);
        break;
    }
}

void M_UpdateSettingsQuick(void)
{
    switch (benchmark_number)
    {
    case 0:
        M_SetCSV(true);
        M_SetCPU(INTEL_486);
        M_SetVisplaneDetail(VISPLANES_NORMAL);
        M_SetSkyDetail(false);
        M_SetSpriteCulling(false);
        M_SetInvisibleDetail(INVISIBLE_NORMAL);
        M_SetShowFPS(false);
        M_SetNoMelting(true);
        M_SetSizeDisplay(7);

        // %2 -potato -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetDetail(DETAIL_POTATO);
        break;
    case 1:
        // %2 -low -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetDetail(DETAIL_LOW);
        break;
    case 2:
        // %2 -high -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetDetail(DETAIL_HIGH);
        break;
    }
}

void M_UpdateSettingsArch(void)
{
    switch (benchmark_number)
    {
    case 0:
        M_SetCSV(true);
        M_SetVisplaneDetail(VISPLANES_NORMAL);
        M_SetSkyDetail(false);
        M_SetSpriteCulling(false);
        M_SetInvisibleDetail(INVISIBLE_NORMAL);
        M_SetShowFPS(false);
        M_SetNoMelting(true);
        M_SetSizeDisplay(7);

        // %2 -high -size 10 -386sx -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetCPU(INTEL_386SX);
        break;
    case 1:
        // %2 -high -size 10 -386dx -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetCPU(INTEL_386DX);
        break;
    case 2:
        // %2 -high -size 10 -cy386 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetCPU(CYRIX_386DLC);
        break;
    case 3:
        // %2 -high -size 10 -cy486 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetCPU(CYRIX_486);
        break;
    case 4:
        // %2 -high -size 10 -i486 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetCPU(INTEL_486);
        break;
    case 5:
        // %2 -high -size 10 -umc486 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetCPU(UMC_GREEN_486);
        break;
    case 6:
        // %2 -high -size 10 -cy5x86 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetCPU(CYRIX_5X86);
        break;
    case 7:
        // %2 -high -size 10 -k5 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetCPU(AMD_K5);
        break;
    case 8:
        // %2 -high -size 10 -pentium -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetCPU(INTEL_PENTIUM);
        break;
    }
}

void M_UpdateSettingsNormal(void)
{
    switch (benchmark_number)
    {
    case 0:
        M_SetCSV(true);
        M_SetCPU(INTEL_486);
        M_SetSkyDetail(false);
        M_SetSpriteCulling(false);
        M_SetInvisibleDetail(INVISIBLE_NORMAL);
        M_SetShowFPS(false);
        M_SetNoMelting(true);
        M_SetSizeDisplay(7);

        //%2 -potato -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetDetail(DETAIL_POTATO);
        M_SetVisplaneDetail(VISPLANES_NORMAL);
        break;
    case 3:
        //%2 -low -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetDetail(DETAIL_LOW);
        M_SetVisplaneDetail(VISPLANES_NORMAL);
        break;
    case 6:
        //%2 -high -size 10 -defSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetDetail(DETAIL_HIGH);
        M_SetVisplaneDetail(VISPLANES_NORMAL);
        break;
    case 1:
    case 4:
    case 7:
        //%2 -potato -size 10 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        //%2 -low -size 10 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        //%2 -high -size 10 -flatSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetVisplaneDetail(VISPLANES_FLAT);
        break;
    case 2:
    case 5:
    case 8:
        //%2 -potato -size 10 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        //%2 -low -size 10 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        //%2 -high -size 10 -flatterSpan -defSky -far -defInv -nofps -nomelt -iwad %3 -timedemo %4 -csv
        M_SetVisplaneDetail(VISPLANES_FLATTER);
        break;
    }
}

void M_UpdateSettings(void)
{
    switch (benchmark_type)
    {
    case BENCHMARK_PHILS:
        M_UpdateSettingsPhils();
        break;
    case BENCHMARK_QUICK:
        M_UpdateSettingsQuick();
        break;
    case BENCHMARK_ARCH:
        M_UpdateSettingsArch();
        break;
    case BENCHMARK_NORMAL:
        M_UpdateSettingsNormal();
        break;
    }
}
