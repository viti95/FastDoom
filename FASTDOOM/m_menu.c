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

// Data.
#include "sounds.h"

#include "m_menu.h"
#include "m_bench.h"

#include "options.h"

#include "version.h"

#include "am_map.h"

#include "i_file.h"
#include "i_gamma.h"

#if defined(MODE_13H)
#include "i_vga13h.h"
#endif

extern patch_t *hu_font[HU_FONTSIZE];
extern byte message_dontfuckwithme;

//
// defaulted values
//
int mouseSensitivity; // has default

// Show messages has default, 0 = off, 1 = on
int showMessages;

int sfxVolume;
int musicVolume;

// Blocky mode, has default, 0 = high, 1 = normal
int detailLevel;
int screenblocks; // has default

// temp for screenblocks (0-9)
int screenSize;

// -1 = no quicksave slot picked!
int quickSaveSlot;

// 1 = message to be printed
int messageToPrint;
// ...and here is the message string!
char *messageString;

// message x & y
int messageLastMenuActive;

// timed message = no input from user
byte messageNeedsInput;

void (*messageRoutine)(int response);

#define SAVESTRINGSIZE 24

// we are going to be entering a savegame string
int saveStringEnter;
int saveSlot;      // which slot to save in
int saveCharIndex; // which char we're editing
// old save description before edit
char saveOldString[SAVESTRINGSIZE];

byte menuactive;

#define SKULLXOFF -32
#define LINEHEIGHT 16

char savegamestrings[10][SAVESTRINGSIZE];

//
// MENU TYPEDEFS
//
typedef struct
{
    // 0 = no cursor here, 1 = ok, 2 = arrows ok
    char status;

    char name[10];
    char text[22];

    // choice = menu item #.
    // if status = 2,
    //   choice=0:leftarrow,1:rightarrow
    void (*routine)(int choice);
} menuitem_t;

typedef struct menu_s
{
    char numitems;          // # of menu items
    struct menu_s *prevMenu; // previous menu
    menuitem_t *menuitems;   // menu items
    void (*routine)();       // draw routine
    short x;
    short y;      // x,y of menu
    short lastOn; // last item user was on in menu
} menu_t;

short itemOn;           // menu item skull is on
short skullAnimCounter; // skull animation counter
byte whichSkull;        // which skull to draw

// graphic name of skulls
// warning: initializer-string for array of chars is too long
char skullName[2][9] = {"M_SKULL1", "M_SKULL2"};

// current menudef
menu_t *currentMenu;

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_QuitDOOM(int choice);

void M_ChangeMessages(int choice);
void M_ChangeSensitivity(int choice);
void M_Benchmark(int choice);
void M_SfxVol(int choice);
void M_MusicVol(int choice);
void M_ChangeDetail(int choice);
void M_ChangeVisplaneDetail(int choice);
void M_ChangeWallDetail(int choice);
void M_ChangeSpriteDetail(int choice);
void M_ChangePSpriteDetail(int choice);
void M_ChangeCPU(int choice);
void M_ChangeGamma(int choice);
void M_ChangeVsync(int choice);
void M_ChangeSkyDetail(int choice);
void M_ChangeInvisibleDetail(int choice);
void M_ChangeShowFPS(int choice);
void M_ChangeAutomapRT(int choice);
void M_ChangeSpriteCulling(int choice);
void M_ChangeMelting(int choice);
void M_ChangeBusSpeed(int choice);
void M_ChangeUncappedFPS(int choice);
void M_ChangeMono();
void M_SizeDisplay(int choice);
void M_Sound(int choice);
void M_Display(int choice);
void M_BenchmarkDemo1(int choice);
void M_BenchmarkDemo2(int choice);
void M_BenchmarkDemo3(int choice);
void M_BenchmarkDemo4(int choice);
void M_ReturnToOptions(int choice);
void M_ChangeBenchmarkType(int choice);

void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_QuickSave(void);
void M_QuickLoad(void);

void M_DrawMainMenu(void);
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
void M_DrawSound(void);
void M_DrawBenchmark(void);
void M_DrawBenchmarkResult(void);
void M_DrawBenchmarkCSV(void);
void M_DrawDisplay(void);
void M_DrawLoad(void);
void M_DrawSave(void);

void M_DrawSaveLoadBorder(int x, int y);
void M_DrawSaveLoadBorderText(int x, int y);
void M_DrawThermo(int x, int y, int thermWidth, int thermDot);
void M_DrawThermoText(int x, int y, int thermWidth, int thermDot);
void M_WriteText(int x, int y, char *string);
int M_StringWidth(char *string);
int M_StringHeight(char *string);
void M_StartControlPanel(void);
void M_StartMessage(char *string, void *routine, byte input);

//
// DOOM MENU
//
#define newgame 0
#define options 1
#define loadgame 2
#define savegame 3
#define quitdoom 4
#define main_end 5

menuitem_t MainMenu[] =
    {
        {1, "M_NGAME", "New game", M_NewGame},
        {1, "M_OPTION", "Options", M_Options},
        {1, "M_LOADG", "Load game", M_LoadGame},
        {1, "M_SAVEG", "Save game", M_SaveGame},
        {1, "M_QUITG", "Quit game", M_QuitDOOM}};

menu_t MainDef =
    {
        main_end,
        NULL,
        MainMenu,
        M_DrawMainMenu,
        97, 64,
        0};

//
// EPISODE SELECT
//
#define ep1 0
#define ep2 1
#define ep3 2
#define ep4 3
#define ep_end 4

menuitem_t EpisodeMenu[] =
    {
        {1, "M_EPI1", "Knee-Deep in the Dead", M_Episode},
        {1, "M_EPI2", "The Shores of Hell", M_Episode},
        {1, "M_EPI3", "Inferno", M_Episode},
        {1, "M_EPI4", "Thy Flesh Consumed", M_Episode}};

menu_t EpiDef =
    {
        ep_end,        // # of menu items
        &MainDef,      // previous menu
        EpisodeMenu,   // menuitem_t ->
        M_DrawEpisode, // drawing routine ->
        48, 63,        // x,y
        ep1            // lastOn
};

//
// NEW GAME
//
#define killthings 0
#define toorough 1
#define hurtme 2
#define violence 3
#define nightmare 4
#define newg_end 5

menuitem_t NewGameMenu[] =
    {
        {1, "M_JKILL", "I'm too young to die", M_ChooseSkill},
        {1, "M_ROUGH", "Hey, not too rough", M_ChooseSkill},
        {1, "M_HURT", "Hurt me plenty", M_ChooseSkill},
        {1, "M_ULTRA", "Ultra-violence", M_ChooseSkill},
        {1, "M_NMARE", "NIGHTMARE!", M_ChooseSkill}};

menu_t NewDef =
    {
        newg_end,      // # of menu items
        &EpiDef,       // previous menu
        NewGameMenu,   // menuitem_t ->
        M_DrawNewGame, // drawing routine ->
        48, 63,        // x,y
        hurtme         // lastOn
};

//
// OPTIONS MENU
//
#define gamma 0
#define vsync 1
#define detail 2
#define visplanes 3
#define columns 4
#define sprites 5
#define psprite 6
#define sky 7
#define invisible 8
#define showfps 9
#define automaprt 10
#define spriteculling 11
#define melting 12
#define bus_speed 13
#define cpu 14
#define uncapped_fps 15
#define display_end 16

#define endgame 0
#define messages 1
#define display 2
#define scrnsize 3
#define option_empty1 4
#define mousesens 5
#define option_empty2 6
#define soundvol 7
#define benchmark_option 8
#define opt_end 9

menuitem_t OptionsMenu[] =
    {
        {1, "M_ENDGAM", "End game", M_EndGame},
        {1, "M_MESSG", "Messages:", M_ChangeMessages},
        {1, "M_DISP", "Display", M_Display},
        {2, "M_SCRNSZ", "Screen size", M_SizeDisplay},
        {-1, "", ""},
        {2, "M_MSENS", "Mouse sensitivity", M_ChangeSensitivity},
        {-1, "", ""},
        {1, "M_SVOL", "Sound volume", M_Sound},
        {1, "", "Benchmark", M_Benchmark}};

menu_t OptionsDef =
    {
        opt_end,
        &MainDef,
        OptionsMenu,
        M_DrawOptions,
        60, 24,
        0};

menuitem_t DisplayMenu[] =
    {
        {2, "", "", M_ChangeGamma},
        {2, "", "", M_ChangeVsync},
        {2, "", "", M_ChangeDetail},
        {2, "", "", M_ChangeVisplaneDetail},
        {2, "", "", M_ChangeWallDetail},
        {2, "", "", M_ChangeSpriteDetail},
        {2, "", "", M_ChangePSpriteDetail},
        {2, "", "", M_ChangeSkyDetail},
        {2, "", "", M_ChangeInvisibleDetail},
        {2, "", "", M_ChangeShowFPS},
        {2, "", "", M_ChangeAutomapRT},
        {2, "", "", M_ChangeSpriteCulling},
        {2, "", "", M_ChangeMelting},
        {2, "", "", M_ChangeBusSpeed},
        {2, "", "", M_ChangeCPU},
        {2, "", "", M_ChangeUncappedFPS}};

menu_t DisplayDef =
    {
        display_end,
        &OptionsDef,
        DisplayMenu,
        M_DrawDisplay,
        60, 9,
        0};

//
// SOUND VOLUME MENU
//
#define sfx_vol 0
#define sfx_empty1 1
#define music_vol 2
#define sfx_empty2 3
#define monosound 4
#define sound_end 5

#define benchmark_select 0
#define benchmark_demo1 1
#define benchmark_demo2 2
#define benchmark_demo3 3
#define benchmark_demo4 4
#define benchmark_end 5

menuitem_t SoundMenu[] =
    {
        {2, "M_SFXVOL", "SFX volume", M_SfxVol},
        {-1, "", ""},
        {2, "M_MUSVOL", "Music volume", M_MusicVol},
        {-1, "", ""},
        {1, "", "", M_ChangeMono}};

menu_t SoundDef =
    {
        sound_end,
        &OptionsDef,
        SoundMenu,
        M_DrawSound,
        80, 64,
        0};

menuitem_t BenchmarkMenu[] =
    {
        {2, "", "", M_ChangeBenchmarkType},
        {1, "", "DEMO1", M_BenchmarkDemo1},
        {1, "", "DEMO2", M_BenchmarkDemo2},
        {1, "", "DEMO3", M_BenchmarkDemo3},
        {1, "", "DEMO4", M_BenchmarkDemo4}};

menuitem_t BenchmarkResultMenu[] =
    {
        {1, "", "", M_ReturnToOptions}};

menu_t BenchmarkDef =
    {
        benchmark_end,
        &OptionsDef,
        BenchmarkMenu,
        M_DrawBenchmark,
        80, 80,
        0};

menu_t BenchmarkResultDef =
    {
        1,
        &OptionsDef,
        BenchmarkResultMenu,
        M_DrawBenchmarkResult,
        60, 64,
        0};

menu_t BenchmarkCSVDef =
    {
        1,
        &OptionsDef,
        BenchmarkResultMenu,
        M_DrawBenchmarkCSV,
        60, 64,
        0};

//
// LOAD GAME MENU
//
#define load1 0
#define load2 1
#define load3 2
#define load4 3
#define load5 4
#define load6 5
#define load_end 6

menuitem_t LoadMenu[] =
    {
        {1, "", "", M_LoadSelect},
        {1, "", "", M_LoadSelect},
        {1, "", "", M_LoadSelect},
        {1, "", "", M_LoadSelect},
        {1, "", "", M_LoadSelect},
        {1, "", "", M_LoadSelect}};

menu_t LoadDef =
    {
        load_end,
        &MainDef,
        LoadMenu,
        M_DrawLoad,
        80, 54,
        0};

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[] =
    {
        {1, "", "", M_SaveSelect},
        {1, "", "", M_SaveSelect},
        {1, "", "", M_SaveSelect},
        {1, "", "", M_SaveSelect},
        {1, "", "", M_SaveSelect},
        {1, "", "", M_SaveSelect}};

menu_t SaveDef =
    {
        load_end,
        &MainDef,
        SaveMenu,
        M_DrawSave,
        80, 54,
        0};

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
void M_ReadSaveStrings(void)
{
    int handle;
    int count;
    int i;
    char name[256];

    for (i = 0; i < load_end; i++)
    {
        sprintf(name, savegamename, i);

        handle = open(name, O_RDONLY | 0, 0666);
        if (handle == -1)
        {
            strcpy(&savegamestrings[i][0], EMPTYSTRING);
            LoadMenu[i].status = 0;
            continue;
        }
        count = read(handle, &savegamestrings[i], SAVESTRINGSIZE);
        close(handle);
        LoadMenu[i].status = 1;
    }
}

//
// M_LoadGame & Cie.
//
void M_DrawLoad(void)
{
    int i;

#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(9, 3, "LOAD GAME");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(18, 3, "LOAD GAME");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(18, 7, "LOAD GAME");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    V_DrawPatchDirectCentered(72, 28, W_CacheLumpName("M_LOADG", PU_CACHE));
#endif

    for (i = 0; i < load_end; i++)
    {
#if defined(MODE_T4025) || defined(MODE_T4050)
        M_DrawSaveLoadBorderText(LoadDef.x / 8, (LoadDef.y + LINEHEIGHT * i) / 8 - 2);
        V_WriteCharDirect(LoadDef.x / 8 - 1, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        V_WriteTextDirect(LoadDef.x / 8, (LoadDef.y + LINEHEIGHT * i) / 8, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 8 + 24, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 8, (LoadDef.y + LINEHEIGHT * i) / 8);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8 - 2);
        V_WriteCharDirect(LoadDef.x / 4 - 1, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        V_WriteTextDirect(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 4 + 24, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + (LINEHEIGHT)*i) / 4 - 2);
        V_WriteCharDirect(LoadDef.x / 4 - 1, (LoadDef.y + LINEHEIGHT * i) / 4, '|');
        V_WriteTextDirect(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 4, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 4 + 24, (LoadDef.y + LINEHEIGHT * i) / 4, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 4);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
        M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT * i);
        M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
#endif
#if defined(MODE_Y_HALF)
        M_DrawSaveLoadBorder(LoadDef.x, (LoadDef.y / 2) + 1 + 8 * i);
        M_WriteText(LoadDef.x, LoadDef.y + 5 + LINEHEIGHT * i, savegamestrings[i]);
#endif
    }
}

//
// Draw border for the savegame description
//
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void M_DrawSaveLoadBorder(int x, int y)
{
    int i;

    V_DrawPatchDirectCentered(x - 8, y + 7, W_CacheLumpName("M_LSLEFT", PU_CACHE));

    for (i = 0; i < 24; i++)
    {
        V_DrawPatchDirectCentered(x, y + 7, W_CacheLumpName("M_LSCNTR", PU_CACHE));
        x += 8;
    }

    V_DrawPatchDirectCentered(x, y + 7, W_CacheLumpName("M_LSRGHT", PU_CACHE));
}
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
void M_DrawSaveLoadBorderText(int x, int y)
{
    int i;

    V_WriteCharDirect(x - 1, y + 1, 7);

    for (i = 0; i < 24; i++)
    {
        V_WriteCharDirect(x, y + 1, '-');
        x += 1;
    }

    V_WriteCharDirect(x, y + 1, 7);
}
#endif

//
// User wants to load this game
//
void M_LoadSelect(int choice)
{
    char name[256];

    sprintf(name, savegamename, choice);
    G_LoadGame(name);
    menuactive = 0;
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
}

//
// Selected from DOOM menu
//
void M_LoadGame(int choice)
{
    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings();
}

//
//  M_SaveGame & Cie.
//
void M_DrawSave(void)
{
    int i;

#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(9, 3, "SAVE GAME");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(18, 3, "SAVE GAME");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(18, 7, "SAVE GAME");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    V_DrawPatchDirectCentered(72, 28, W_CacheLumpName("M_SAVEG", PU_CACHE));
#endif

    for (i = 0; i < load_end; i++)
    {
#if defined(MODE_T4025) || defined(MODE_T4050)
        M_DrawSaveLoadBorderText(LoadDef.x / 8, (LoadDef.y + LINEHEIGHT * i) / 8 - 2);
        V_WriteCharDirect(LoadDef.x / 8 - 1, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        V_WriteTextDirect(LoadDef.x / 8, (LoadDef.y + LINEHEIGHT * i) / 8, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 8 + 24, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 8, (LoadDef.y + LINEHEIGHT * i) / 8);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8 - 2);
        V_WriteCharDirect(LoadDef.x / 4 - 1, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        V_WriteTextDirect(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 4 + 24, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + (LINEHEIGHT)*i) / 4 - 2);
        V_WriteCharDirect(LoadDef.x / 4 - 1, (LoadDef.y + LINEHEIGHT * i) / 4, '|');
        V_WriteTextDirect(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 4, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 4 + 24, (LoadDef.y + LINEHEIGHT * i) / 4, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 4);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
        M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT * i);
        M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
#endif
#if defined(MODE_Y_HALF)
        M_DrawSaveLoadBorder(LoadDef.x, (LoadDef.y / 2) + 1 + 8 * i);
        M_WriteText(LoadDef.x, LoadDef.y + 5 + LINEHEIGHT * i, savegamestrings[i]);
#endif
    }

    if (saveStringEnter)
    {
#if defined(MODE_T4025) || defined(MODE_T4050)
        V_WriteTextDirect((LoadDef.x / 8) + strlen(savegamestrings[saveSlot]), (LoadDef.y + LINEHEIGHT * saveSlot) / 8, "_");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
        V_WriteTextDirect((LoadDef.x / 4) + strlen(savegamestrings[saveSlot]), (LoadDef.y + LINEHEIGHT * saveSlot) / 8, "_");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
        V_WriteTextDirect((LoadDef.x / 4) + strlen(savegamestrings[saveSlot]), (LoadDef.y + LINEHEIGHT * saveSlot) / 4, "_");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
        i = M_StringWidth(savegamestrings[saveSlot]);
        M_WriteText(LoadDef.x + i, LoadDef.y + LINEHEIGHT * saveSlot, "_");
#endif
#if defined(MODE_Y_HALF)
        i = M_StringWidth(savegamestrings[saveSlot]);
        M_WriteText(LoadDef.x + i, LoadDef.y + 5 + LINEHEIGHT * saveSlot, "_");
#endif
    }
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot)
{
    G_SaveGame(slot, savegamestrings[slot]);
    menuactive = 0;

    // PICK QUICKSAVE SLOT YET?
    if (quickSaveSlot == -2)
        quickSaveSlot = slot;
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
{
    // we are going to be intercepting all chars
    saveStringEnter = 1;

    saveSlot = choice;
    strcpy(saveOldString, savegamestrings[choice]);
    if (!strcmp(savegamestrings[choice], EMPTYSTRING))
        savegamestrings[choice][0] = 0;
    saveCharIndex = strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
//
void M_SaveGame(int choice)
{
    if (!usergame)
    {
        M_StartMessage(I_LoadTextProgram(39), NULL, 0);
        return;
    }

    if (gamestate != GS_LEVEL)
        return;

    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings();
}

//
//      M_QuickSave
//
char tempstring[80];

void M_QuickSaveResponse(int ch)
{
    if (ch == 'y')
    {
        M_DoSave(quickSaveSlot);
        S_StartSound(NULL, sfx_swtchx);
    }
}

void M_QuickSave(void)
{
    if (!usergame)
    {
        S_StartSound(NULL, sfx_oof);
        return;
    }

    if (gamestate != GS_LEVEL)
        return;

    if (quickSaveSlot < 0)
    {
        M_StartControlPanel();
        M_ReadSaveStrings();
        M_SetupNextMenu(&SaveDef);
        quickSaveSlot = -2; // means to pick a slot now
        return;
    }
    sprintf(tempstring, I_LoadTextProgram(40), savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickSaveResponse, 1);
}

//
// M_QuickLoad
//
void M_QuickLoadResponse(int ch)
{
    if (ch == 'y')
    {
        M_LoadSelect(quickSaveSlot);
        S_StartSound(NULL, sfx_swtchx);
    }
}

void M_QuickLoad(void)
{
    if (quickSaveSlot < 0)
    {
        M_StartMessage(I_LoadTextProgram(38), NULL, 0);
        return;
    }
    sprintf(tempstring, I_LoadTextProgram(41), savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickLoadResponse, 1);
}

//
// Change Sfx & Music volumes
//
void M_DrawSound(void)
{
#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(15, 4, "SOUND VOLUME");

    M_DrawThermoText(SoundDef.x / 8, (SoundDef.y + LINEHEIGHT * (sfx_vol + 1)) / 8, 16, sfxVolume);
    M_DrawThermoText(SoundDef.x / 8, (SoundDef.y + LINEHEIGHT * (music_vol + 1)) / 8, 16, musicVolume);

    V_WriteTextDirect(10, 16, "Mono Sound:");
    V_WriteTextDirect(20, 16, monoSound ? "ON" : "OFF");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(30, 4, "SOUND VOLUME");

    M_DrawThermoText(SoundDef.x / 4, (SoundDef.y + LINEHEIGHT * (sfx_vol + 1)) / 8, 16, sfxVolume);
    M_DrawThermoText(SoundDef.x / 4, (SoundDef.y + LINEHEIGHT * (music_vol + 1)) / 8, 16, musicVolume);

    V_WriteTextDirect(20, 16, "Mono Sound:");
    V_WriteTextDirect(40, 16, monoSound ? "ON" : "OFF");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(30, 8, "SOUND VOLUME");

    M_DrawThermoText(SoundDef.x / 4, (SoundDef.y + LINEHEIGHT * (sfx_vol + 1)) / 4, 16, sfxVolume);
    M_DrawThermoText(SoundDef.x / 4, (SoundDef.y + LINEHEIGHT * (music_vol + 1)) / 4, 16, musicVolume);

    V_WriteTextDirect(20, 32, "Mono Sound:");
    V_WriteTextDirect(40, 32, monoSound ? "ON" : "OFF");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    V_DrawPatchDirectCentered(60, 38, W_CacheLumpName("M_SVOL", PU_CACHE));

    M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (sfx_vol + 1), 16, sfxVolume);
    M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (music_vol + 1), 16, musicVolume);

    M_WriteText(82, 130, "MONO SOUND:");
    M_WriteText(164, 130, monoSound ? "ON" : "OFF");
#endif
}

void M_Sound(int choice)
{
    M_SetupNextMenu(&SoundDef);
}

void M_DrawBenchmark(void)
{
#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(10, 6, "===========================");
    V_WriteTextDirect(10, 7, "=     BENCHMARK  MENU     =");
    V_WriteTextDirect(10, 8, "===========================");

    V_WriteTextDirect(10, 10, "Type:");

    if (benchmark_type == 0)
    {
        V_WriteTextDirect(16, 10, "SINGLE");
    }
    else
    {
        V_WriteTextDirect(16, 10, benchmark_files[benchmark_type - 1]);
    }

    V_WriteTextDirect(10, 12, "DEMO1");
    V_WriteTextDirect(10, 14, "DEMO2");
    V_WriteTextDirect(10, 16, "DEMO3");
    V_WriteTextDirect(10, 18, "DEMO4");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(20, 6, "===========================");
    V_WriteTextDirect(20, 7, "=     BENCHMARK  MENU     =");
    V_WriteTextDirect(20, 8, "===========================");

    V_WriteTextDirect(20, 10, "Type:");

    if (benchmark_type == 0)
    {
        V_WriteTextDirect(26, 10, "SINGLE");
    }
    else
    {
        V_WriteTextDirect(26, 10, benchmark_files[benchmark_type - 1]);
    }

    V_WriteTextDirect(20, 12, "DEMO1");
    V_WriteTextDirect(20, 14, "DEMO2");
    V_WriteTextDirect(20, 16, "DEMO3");
    V_WriteTextDirect(20, 18, "DEMO4");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(20, 14, "===========================");
    V_WriteTextDirect(20, 15, "=                         =");
    V_WriteTextDirect(20, 16, "=     BENCHMARK  MENU     =");
    V_WriteTextDirect(20, 17, "=                         =");
    V_WriteTextDirect(20, 18, "===========================");

    V_WriteTextDirect(20, 20, "Type:");

    if (benchmark_type == 0)
    {
        V_WriteTextDirect(26, 20, "SINGLE");
    }
    else
    {
        V_WriteTextDirect(26, 20, benchmark_files[benchmark_type - 1]);
    }

    V_WriteTextDirect(20, 24, "DEMO1");
    V_WriteTextDirect(20, 28, "DEMO2");
    V_WriteTextDirect(20, 32, "DEMO3");
    V_WriteTextDirect(20, 36, "DEMO4");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    M_WriteText(82, 20, "===========================");
    M_WriteText(82, 36, "=  BENCHMARK MENU  =");
    M_WriteText(82, 52, "===========================");

    M_WriteText(82, 84, "TYPE:");

    if (benchmark_type == 0)
    {
        M_WriteText(128, 84, "SINGLE");
    }
    else
    {
        M_WriteText(128, 84, benchmark_files[benchmark_type - 1]);
    }

    M_WriteText(82, 100, "DEMO1");
    M_WriteText(82, 116, "DEMO2");
    M_WriteText(82, 132, "DEMO3");
    M_WriteText(82, 148, "DEMO4");
#endif
}

char strRealtics[21];
char strGametics[21];
char strFPS[21];

void M_ReturnToOptions(int choice)
{
    benchmark = false;
    benchmark_finished = false;
    M_SetupNextMenu(&OptionsDef);
}

void M_DrawBenchmarkResult(void)
{
    sprintf(strGametics, "%u", benchmark_gametics);
    sprintf(strRealtics, "%u", benchmark_realtics);
    sprintf(strFPS, "%u.%.3u", benchmark_resultfps / 1000, benchmark_resultfps % 1000);

#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(10, 6, "RESULT");
    V_WriteTextDirect(7, 8, "Gametics:");
    V_WriteTextDirect(17, 8, strGametics);
    V_WriteTextDirect(7, 10, "Realtics:");
    V_WriteTextDirect(17, 10, strRealtics);
    V_WriteTextDirect(12, 12, "FPS:");
    V_WriteTextDirect(17, 12, strFPS);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(20, 6, "RESULT");
    V_WriteTextDirect(15, 8, "Gametics:");
    V_WriteTextDirect(25, 8, strGametics);
    V_WriteTextDirect(15, 10, "Realtics:");
    V_WriteTextDirect(25, 10, strRealtics);
    V_WriteTextDirect(20, 12, "FPS:");
    V_WriteTextDirect(25, 12, strFPS);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(20, 14, "RESULT");
    V_WriteTextDirect(15, 16, "Gametics:");
    V_WriteTextDirect(25, 16, strGametics);
    V_WriteTextDirect(15, 18, "Realtics:");
    V_WriteTextDirect(25, 18, strRealtics);
    V_WriteTextDirect(20, 20, "FPS:");
    V_WriteTextDirect(25, 20, strFPS);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    M_WriteText(100, 40, "RESULT");
    M_WriteText(62, 68, "Gametics:");
    M_WriteText(142, 68, strGametics);
    M_WriteText(62, 84, "Realtics:");
    M_WriteText(142, 84, strRealtics);
    M_WriteText(100, 100, "FPS:");
    M_WriteText(142, 100, strFPS);
#endif
}

#define CSV_MESSAGE "Results saved on file BENCH.CSV"

void M_DrawBenchmarkCSV(void)
{
    if (benchmark_commandline)
        I_Error(9);

#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(6, 8, CSV_MESSAGE);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(15, 8, CSV_MESSAGE);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(15, 16, CSV_MESSAGE);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    M_WriteText(62, 68, CSV_MESSAGE);
#endif
}

void M_ChangeBenchmarkType(int choice)
{
    switch (choice)
    {
    case 0:
        benchmark_type--;

        if (benchmark_type == -1)
            benchmark_type = benchmark_files_num;
        break;
    case 1:
        benchmark_type++;

        if (benchmark_type == benchmark_files_num + 1)
            benchmark_type = 0;
        break;
    }

    if (benchmark_type == 0)
    {
        csv = 0;
    }
    else
    {
        sprintf(benchmark_file, benchmark_files[benchmark_type - 1]);
        benchmark_total = D_FileGetFirstInteger(benchmark_file);
        csv = 1;
    }
}

void M_BenchmarkRunDemo(void)
{
    menuactive = 0;
    benchmark = true;
    benchmark_finished = false;

    M_UpdateSettings();

    if (benchmark_advanced && frametime == NULL)
    {
        unsigned int i;

        // Get tics from demo
        benchmark_total_tics = G_GetDemoTicks(demofile) + 10;

        // Alloc memory for frametimes
        frametime = (unsigned int *)Z_MallocUnowned(benchmark_total_tics * sizeof(unsigned int), PU_STATIC);

        for (i = 0; i < benchmark_total_tics; i++)
        {
            frametime[i] = 0;
        }

        frametime_position = 0;
    }

    G_TimeDemo(demofile);

    benchmark_starttic = gametic;
}

void M_BenchmarkDemo1(int choice)
{
    sprintf(demofile, "demo1");
    M_BenchmarkRunDemo();
}

void M_BenchmarkDemo2(int choice)
{
    sprintf(demofile, "demo2");
    M_BenchmarkRunDemo();
}

void M_BenchmarkDemo3(int choice)
{
    sprintf(demofile, "demo3");
    M_BenchmarkRunDemo();
}

void M_BenchmarkDemo4(int choice)
{
    sprintf(demofile, "demo4");
    M_BenchmarkRunDemo();
}

void M_Benchmark(int choice)
{
    M_SetupNextMenu(&BenchmarkDef);
}

void M_SfxVol(int choice)
{
    switch (choice)
    {
    case 0:
        if (sfxVolume)
            sfxVolume--;
        break;
    case 1:
        if (sfxVolume < 15)
            sfxVolume++;
        break;
    }

    S_SetSfxVolume(sfxVolume * 8);
}

void M_MusicVol(int choice)
{
    switch (choice)
    {
    case 0:
        if (musicVolume)
            musicVolume--;
        break;
    case 1:
        if (musicVolume < 15)
            musicVolume++;
        break;
    }

    S_SetMusicVolume(musicVolume * 17);
}

//
// M_DrawMainMenu
//
void M_DrawMainMenu(void)
{
#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(11, 5, "DOOM");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(23, 5, "DOOM");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(23, 10, "DOOM");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    V_DrawPatchDirectCentered(94, 2, W_CacheLumpName("M_DOOM", PU_CACHE));
#endif
}

//
// M_NewGame
//
void M_DrawNewGame(void)
{
#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(12, 2, "NEW GAME");
    V_WriteTextDirect(6, 4, "Choose skill level:");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(24, 2, "NEW GAME");
    V_WriteTextDirect(13, 4, "Choose skill level:");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(24, 2, "NEW GAME");
    V_WriteTextDirect(13, 9, "Choose skill level:");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    V_DrawPatchDirectCentered(96, 14, W_CacheLumpName("M_NEWG", PU_CACHE));
    V_DrawPatchDirectCentered(54, 38, W_CacheLumpName("M_SKILL", PU_CACHE));
#endif
}

void M_NewGame(int choice)
{
    if (gamemode == commercial)
        M_SetupNextMenu(&NewDef);
    else
        M_SetupNextMenu(&EpiDef);
}

//
//      M_Episode
//
int epi;

void M_DrawEpisode(void)
{
#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(13, 4, "WHICH EPISODE?");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(27, 4, "WHICH EPISODE?");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(27, 9, "WHICH EPISODE?");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    V_DrawPatchDirectCentered(54, 38, W_CacheLumpName("M_EPISOD", PU_CACHE));
#endif
}

void M_VerifyNightmare(int ch)
{
    if (ch != 'y')
        return;

    G_DeferedInitNew(nightmare, epi + 1, 1);
    menuactive = 0;
}

void M_ChooseSkill(int choice)
{
    if (choice == nightmare)
    {
        M_StartMessage(I_LoadTextProgram(37), M_VerifyNightmare, 1);
        return;
    }

    G_DeferedInitNew(choice, epi + 1, 1);
    menuactive = 0;
}

void M_Episode(int choice)
{
    epi = choice;
    M_SetupNextMenu(&NewDef);
}

//
// M_Options
//
const char msgNames[2][9] = {"M_MSGOFF", "M_MSGON"};
char gammamsg[7];
char playergammamsg[13];

void M_DrawOptions(void)
{
#if defined(MODE_T4025) || defined(MODE_T4050)
    V_WriteTextDirect(13, 1, "OPTIONS");
    V_WriteTextDirect((OptionsDef.x + 120) / 8, (OptionsDef.y + LINEHEIGHT * messages) / 8, showMessages == 0 ? "OFF" : "ON");
    M_DrawThermoText(OptionsDef.x / 8, (OptionsDef.y + LINEHEIGHT * (mousesens + 1)) / 8, 10, mouseSensitivity);
    M_DrawThermoText(OptionsDef.x / 8, (OptionsDef.y + LINEHEIGHT * (scrnsize + 1)) / 8, 10, screenSize);
    V_WriteTextDirect(7, 19, "Benchmark");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    V_WriteTextDirect(30, 1, "OPTIONS");
    V_WriteTextDirect((OptionsDef.x + 120) / 6, (OptionsDef.y + LINEHEIGHT * messages) / 8, showMessages == 0 ? "OFF" : "ON");
    M_DrawThermoText(OptionsDef.x / 4, (OptionsDef.y + LINEHEIGHT * (mousesens + 1)) / 8, 10, mouseSensitivity);
    M_DrawThermoText(OptionsDef.x / 4, (OptionsDef.y + LINEHEIGHT * (scrnsize + 1)) / 8, 10, screenSize);
    V_WriteTextDirect(15, 19, "Benchmark");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
    V_WriteTextDirect(30, 4, "OPTIONS");
    V_WriteTextDirect((OptionsDef.x + 120) / 6, (OptionsDef.y + LINEHEIGHT * messages) / 4, showMessages == 0 ? "OFF" : "ON");
    M_DrawThermoText(OptionsDef.x / 4, (OptionsDef.y + LINEHEIGHT * (mousesens + 1)) / 4, 10, mouseSensitivity);
    M_DrawThermoText(OptionsDef.x / 4, (OptionsDef.y + LINEHEIGHT * (scrnsize + 1)) / 4, 10, screenSize);
    V_WriteTextDirect(15, 38, "Benchmark");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    V_DrawPatchDirectCentered(108, 2, W_CacheLumpName("M_OPTTTL", PU_CACHE));
    V_DrawPatchDirectCentered(OptionsDef.x + 120, OptionsDef.y + LINEHEIGHT * messages, W_CacheLumpName((char *)msgNames[showMessages], PU_CACHE));
    M_DrawThermo(OptionsDef.x, OptionsDef.y + LINEHEIGHT * (mousesens + 1), 10, mouseSensitivity);
    M_DrawThermo(OptionsDef.x, OptionsDef.y + LINEHEIGHT * (scrnsize + 1), 10, screenSize);
    M_WriteText(OptionsDef.x + 1, OptionsDef.y + LINEHEIGHT * benchmark_option + 4, "BENCHMARK");
#endif

#if defined(MODE_Y_HALF)
    V_DrawPatchDirectCentered(108, 2, W_CacheLumpName("M_OPTTTL", PU_CACHE));
    V_DrawPatchDirectCentered(OptionsDef.x + 120, (OptionsDef.y / 2) + 16, W_CacheLumpName((char *)msgNames[showMessages], PU_CACHE));
    M_DrawThermo(OptionsDef.x, (OptionsDef.y / 2) - 1 + LINEHEIGHT * (mousesens + 2), 10, mouseSensitivity);
    M_DrawThermo(OptionsDef.x, (OptionsDef.y / 2) - 1 + LINEHEIGHT * (scrnsize + 2), 10, screenSize);
    M_WriteText(OptionsDef.x + 1, OptionsDef.y + LINEHEIGHT * benchmark_option + 4, "BENCHMARK");
#endif
}

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void M_DrawDisplayItem(int item, int position)
{
    int y = 12 + position * 16;

    switch (item)
    {
    case gamma:
        M_WriteText(58, y, "GAMMA LEVEL:");
        M_WriteText(214, y, gammamsg);
        break;
    case vsync:
        M_WriteText(58, y, "VSYNC:");
        M_WriteText(214, y, waitVsync ? "ON" : "OFF");
        break;
    case detail:
        M_WriteText(58, y, "DETAIL LEVEL:");
        M_WriteText(214, y, detailLevel == DETAIL_POTATO ? "POTATO" : detailLevel == DETAIL_LOW ? "LOW"
                                                                                                : "HIGH");
        break;
    case visplanes:
        M_WriteText(58, y, "VISPLANE RENDERING:");
        M_WriteText(214, y, (visplaneRender == VISPLANES_NORMAL) ? "FULL" : (visplaneRender == VISPLANES_FLAT) ? "FLAT"
                                                                                                               : "FLATTER");
        break;
    case columns:
        M_WriteText(58, y, "WALL RENDERING:");
        M_WriteText(214, y, (wallRender == WALL_NORMAL) ? "FULL" : (wallRender == WALL_FLAT) ? "FLAT"
                                                                                             : "FLATTER");
        break;

    case sprites:
        M_WriteText(58, y, "SPRITE RENDERING:");
        M_WriteText(214, y, (spriteRender == SPRITE_NORMAL) ? "FULL" : (spriteRender == SPRITE_FLAT) ? "FLAT"
                                                                                                     : "FLATTER");
        break;

    case psprite:
        M_WriteText(58, y, "PLAYER RENDERING:");
        M_WriteText(214, y, (pspriteRender == PSPRITE_NORMAL) ? "FULL" : (pspriteRender == PSPRITE_FLAT) ? "FLAT"
                                                                                                         : "FLATTER");
        break;

    case sky:
        M_WriteText(58, y, "SKY RENDERING:");
        M_WriteText(214, y, flatSky ? "FLAT" : "FULL");
        break;
    case invisible:
        M_WriteText(58, y, "INVISIBLE RENDERING:");
        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            M_WriteText(214, y, "FUZZY");
            break;
        case INVISIBLE_FLAT:
            M_WriteText(214, y, "FLAT");
            break;
        case INVISIBLE_FLAT_SATURN:
            M_WriteText(214, y, "FLAT SATURN");
            break;
        case INVISIBLE_SATURN:
            M_WriteText(214, y, "SATURN");
            break;
        case INVISIBLE_TRANSLUCENT:
            M_WriteText(214, y, "TRANSLUCENT");
            break;
        }
        break;
    case showfps:
        M_WriteText(58, y, "SHOW FPS:");
        switch (showFPS)
        {
        case NO_FPS:
            M_WriteText(214, y, "OFF");
            break;
        case SCREEN_FPS:
            M_WriteText(214, y, "SCREEN");
            break;
        case DEBUG_CARD_2D_FPS:
            M_WriteText(214, y, "DEBUG CARD 2N");
            break;
        case DEBUG_CARD_4D_FPS:
            M_WriteText(214, y, "DEBUG CARD 4N");
            break;
        case SCREEN_DC2D_FPS:
            M_WriteText(214, y, "SCREEN + DC2N");
            break;
        case SCREEN_DC4D_FPS:
            M_WriteText(214, y, "SCREEN + DC4N");
            break;
        }
        break;
    case automaprt:
        M_WriteText(58, y, "AUTOMAP UPDATE:");
        M_WriteText(214, y, automapRT ? "ON" : "OFF");
        break;
    case spriteculling:
        M_WriteText(58, y, "SPRITE CULLING:");
        M_WriteText(214, y, nearSprites ? "ON" : "OFF");
        break;
    case melting:
        M_WriteText(58, y, "MELTING LOAD EFFECT:");
        M_WriteText(214, y, noMelt ? "OFF" : "ON");
        break;
    case bus_speed:
        M_WriteText(58, y, "BUS SPEED:");
        M_WriteText(214, y, busSpeed ? "SLOW" : "FAST");
        break;
    case cpu:
        M_WriteText(58, y, "CPU RENDERER:");
        switch (selectedCPU)
        {
        case AUTO_CPU:
            M_WriteText(214, y, "AUTODETECT");
            break;
        case INTEL_386SX:
            M_WriteText(214, y, "INTEL 386SX");
            break;
        case INTEL_386DX:
            M_WriteText(214, y, "INTEL 386DX");
            break;
        case INTEL_486:
            M_WriteText(214, y, "INTEL 486");
            break;
        case CYRIX_386DLC:
            M_WriteText(214, y, "CYRIX 386DLC");
            break;
        case CYRIX_486:
            M_WriteText(214, y, "CYRIX 486");
            break;
        case UMC_GREEN_486:
            M_WriteText(214, y, "UMC 486");
            break;
        case CYRIX_5X86:
            M_WriteText(214, y, "CYRIX 5X86");
            break;
        case AMD_K5:
            M_WriteText(214, y, "AMD K5");
            break;
        case INTEL_PENTIUM:
            M_WriteText(214, y, "INTEL PENTIUM");
            break;
        }
        break;
    case uncapped_fps:
        M_WriteText(58, y, "UNCAPPED FPS:");
        M_WriteText(214, y, uncappedFPS ? "ON" : "OFF");
        break;
    }
}
#endif

#if defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_COLOR_MDA)
void M_DrawDisplayItem(int item, int position)
{

#if defined(MODE_T8050) || defined(MODE_T8043)
#define M_X1 15
#define M_X2 45
    int y = 2 + position * 4;
#endif

#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
#define M_X1 15
#define M_X2 45
    int y = 1 + position * 2;
#endif

#if defined(MODE_T4025) || defined(MODE_T4050)
#define M_X1 6
#define M_X2 27
    int y = 1 + position * 2;
#endif

    switch (item)
    {
    case gamma:
        V_WriteTextDirect(M_X1, y, "GAMMA LEVEL:");
        V_WriteTextDirect(M_X2, y, gammamsg);
        break;
    case vsync:
        V_WriteTextDirect(M_X1, y, "VSYNC:");
        V_WriteTextDirect(M_X2, y, waitVsync ? "ON" : "OFF");
        break;
    case detail:
        V_WriteTextDirect(M_X1, y, "DETAIL LEVEL:");
        V_WriteTextDirect(M_X2, y, detailLevel == DETAIL_POTATO ? "POTATO" : detailLevel == DETAIL_LOW ? "LOW"
                                                                                                     : "HIGH");
        break;
    case visplanes:
        V_WriteTextDirect(M_X1, y, "VISPLANE RENDERING:");
        V_WriteTextDirect(M_X2, y, (visplaneRender == VISPLANES_NORMAL) ? "FULL" : (visplaneRender == VISPLANES_FLAT) ? "FLAT"
                                                                                                                    : "FLATTER");
        break;
    case columns:
        V_WriteTextDirect(M_X1, y, "WALL RENDERING:");
        V_WriteTextDirect(M_X2, y, (wallRender == WALL_NORMAL) ? "FULL" : (wallRender == WALL_FLAT) ? "FLAT"
                                                                                                  : "FLATTER");
        break;

    case sprites:
        V_WriteTextDirect(M_X1, y, "SPRITE RENDERING:");
        V_WriteTextDirect(M_X2, y, (spriteRender == SPRITE_NORMAL) ? "FULL" : (spriteRender == SPRITE_FLAT) ? "FLAT"
                                                                                                          : "FLATTER");
        break;

    case psprite:
        V_WriteTextDirect(M_X1, y, "PLAYER RENDERING:");
        V_WriteTextDirect(M_X2, y, (pspriteRender == PSPRITE_NORMAL) ? "FULL" : (pspriteRender == PSPRITE_FLAT) ? "FLAT"
                                                                                                              : "FLATTER");
        break;

    case sky:
        V_WriteTextDirect(M_X1, y, "SKY RENDERING:");
        V_WriteTextDirect(M_X2, y, flatSky ? "FLAT" : "FULL");
        break;
    case invisible:
        V_WriteTextDirect(M_X1, y, "INVISIBLE RENDERING:");
        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            V_WriteTextDirect(M_X2, y, "FUZZY");
            break;
        case INVISIBLE_FLAT:
            V_WriteTextDirect(M_X2, y, "FLAT");
            break;
        case INVISIBLE_FLAT_SATURN:
            V_WriteTextDirect(M_X2, y, "FLAT SATURN");
            break;
        case INVISIBLE_SATURN:
            V_WriteTextDirect(M_X2, y, "SATURN");
            break;
        case INVISIBLE_TRANSLUCENT:
            V_WriteTextDirect(M_X2, y, "TRANSLUCENT");
            break;
        }
        break;
    case showfps:
        V_WriteTextDirect(M_X1, y, "SHOW FPS:");
        switch (showFPS)
        {
        case NO_FPS:
            V_WriteTextDirect(M_X2, y, "OFF");
            break;
        case SCREEN_FPS:
            V_WriteTextDirect(M_X2, y, "SCREEN");
            break;
        case DEBUG_CARD_2D_FPS:
            V_WriteTextDirect(M_X2, y, "DEBUG CARD 2N");
            break;
        case DEBUG_CARD_4D_FPS:
            V_WriteTextDirect(M_X2, y, "DEBUG CARD 4N");
            break;
        case SCREEN_DC2D_FPS:
            V_WriteTextDirect(M_X2, y, "SCREEN + DC2N");
            break;
        case SCREEN_DC4D_FPS:
            V_WriteTextDirect(M_X2, y, "SCREEN + DC4N");
            break;
        }
        break;
    case automaprt:
        V_WriteTextDirect(M_X1, y, "AUTOMAP UPDATE:");
        V_WriteTextDirect(M_X2, y, automapRT ? "ON" : "OFF");
        break;
    case spriteculling:
        V_WriteTextDirect(M_X1, y, "SPRITE CULLING:");
        V_WriteTextDirect(M_X2, y, nearSprites ? "ON" : "OFF");
        break;
    case melting:
        V_WriteTextDirect(M_X1, y, "MELTING LOAD EFFECT:");
        V_WriteTextDirect(M_X2, y, noMelt ? "OFF" : "ON");
        break;
    case bus_speed:
        V_WriteTextDirect(M_X1, y, "BUS SPEED:");
        V_WriteTextDirect(M_X2, y, busSpeed ? "SLOW" : "FAST");
        break;
    case cpu:
        V_WriteTextDirect(M_X1, y, "CPU RENDERER:");
        switch (selectedCPU)
        {
        case AUTO_CPU:
            V_WriteTextDirect(M_X2, y, "AUTODETECT");
            break;
        case INTEL_386SX:
            V_WriteTextDirect(M_X2, y, "INTEL 386SX");
            break;
        case INTEL_386DX:
            V_WriteTextDirect(M_X2, y, "INTEL 386DX");
            break;
        case INTEL_486:
            V_WriteTextDirect(M_X2, y, "INTEL 486");
            break;
        case CYRIX_386DLC:
            V_WriteTextDirect(M_X2, y, "CYRIX 386DLC");
            break;
        case CYRIX_486:
            V_WriteTextDirect(M_X2, y, "CYRIX 486");
            break;
        case UMC_GREEN_486:
            V_WriteTextDirect(M_X2, y, "UMC 486");
            break;
        case CYRIX_5X86:
            V_WriteTextDirect(M_X2, y, "CYRIX 5X86");
            break;
        case AMD_K5:
            V_WriteTextDirect(M_X2, y, "AMD K5");
            break;
        case INTEL_PENTIUM:
            V_WriteTextDirect(M_X2, y, "INTEL PENTIUM");
            break;
        }
        break;
    case uncapped_fps:
        V_WriteTextDirect(M_X1, y, "UNCAPPED FPS:");
        V_WriteTextDirect(M_X2, y, uncappedFPS ? "ON" : "OFF");
        break;
    }
}
#endif

#define MAX_ITEMS_DRAWN 10

void M_DrawDisplay(void)
{
    int i, j;
    int itemMin = itemOn - (MAX_ITEMS_DRAWN / 2);
    int itemMax = itemOn + (MAX_ITEMS_DRAWN / 2);

    if (itemMin < 0)
    {

        itemMin = 0;
        itemMax = itemMin + MAX_ITEMS_DRAWN;
    }
    else if (itemMax > display_end)
    {

        itemMax = display_end;
        itemMin = itemMax - MAX_ITEMS_DRAWN;
    }

    for (i = itemMin, j = 0; i < itemMax; i++, j++)
    {
        M_DrawDisplayItem(i, j);
    }
}

void M_Options(int choice)
{
    M_SetupNextMenu(&OptionsDef);
}

void M_Display(int choice)
{
    M_SetupNextMenu(&DisplayDef);
}

//
//      Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
    showMessages = 1 - showMessages;

    message_dontfuckwithme = 1;
}

//
// M_EndGame
//
void M_EndGameResponse(int ch)
{
    if (ch != 'y')
        return;

    currentMenu->lastOn = itemOn;
    menuactive = 0;
    D_StartTitle();
}

void M_EndGame(int choice)
{
    if (!usergame)
    {
        S_StartSound(NULL, sfx_oof);
        return;
    }

    M_StartMessage(I_LoadTextProgram(42), M_EndGameResponse, 1);
}

//
// M_QuitDOOM
//
int quitsounds[8] =
    {
        sfx_pldeth,
        sfx_dmpain,
        sfx_popain,
        sfx_slop,
        sfx_telept,
        sfx_posit1,
        sfx_posit3,
        sfx_sgtatk};

int quitsounds2[8] =
    {
        sfx_vilact,
        sfx_getpow,
        sfx_boscub,
        sfx_slop,
        sfx_skeswg,
        sfx_kntdth,
        sfx_bspact,
        sfx_sgtatk};

void M_QuitResponse(int ch)
{
    int i;

    if (ch != 'y')
        return;

    i = 105;

    if (gamemode == commercial)
        S_StartSound(NULL, quitsounds2[(gametic >> 2) & 7]);
    else
        S_StartSound(NULL, quitsounds[(gametic >> 2) & 7]);

#if !defined(MODE_HERC) && !defined(MODE_MDA) && !defined(MODE_INCOLOR) && !defined(MODE_COLOR_MDA)
    do
    {
        I_WaitSingleVBL();
    } while (i--);
#endif

    I_Quit();
}

unsigned char *endmsg;

void M_QuitDOOM(int choice)
{
    // We pick index 0 which is language sensitive,
    //  or one at random, between 1 and maximum number.
    if (gamemode == commercial)
    {
        I_LoadTextProgram(69 + 8 + ((gametic >> 2) % NUM_QUITMESSAGES));
    }
    else
    {
        I_LoadTextProgram(69 + ((gametic >> 2) % NUM_QUITMESSAGES));
    }

    endmsg = Z_MallocUnowned(strlen(programtext), PU_CACHE);
    strcpy(endmsg, programtext);

    M_StartMessage(endmsg, M_QuitResponse, 1);
}

void M_ChangeSensitivity(int choice)
{
    switch (choice)
    {
    case 0:
        if (mouseSensitivity)
            mouseSensitivity--;
        break;
    case 1:
        if (mouseSensitivity < 9)
            mouseSensitivity++;
        break;
    }
}

void M_ChangeDetail(int choice)
{
    switch (choice)
    {
    case 0:
        detailLevel--;

        if (detailLevel == -1)
            detailLevel = DETAIL_POTATO;
        break;
    case 1:
        detailLevel++;

        if (detailLevel == NUM_DETAIL)
            detailLevel = DETAIL_HIGH;
        break;
    }

    R_SetViewSize(screenblocks, detailLevel);
}

void M_ChangeVisplaneDetail(int choice)
{
    switch (choice)
    {
    case 0:
        visplaneRender--;

        if (visplaneRender == -1)
            visplaneRender = VISPLANES_FLATTER;
        break;
    case 1:
        visplaneRender++;

        if (visplaneRender == NUM_VISPLANESRENDER)
            visplaneRender = VISPLANES_NORMAL;
        break;
    }

    R_SetViewSize(screenblocks, detailLevel);
}

void M_ChangeWallDetail(int choice)
{
    switch (choice)
    {
    case 0:
        wallRender--;

        if (wallRender == -1)
            wallRender = WALL_FLATTER;
        break;
    case 1:
        wallRender++;

        if (wallRender == NUM_WALLRENDER)
            wallRender = WALL_NORMAL;
        break;
    }

    R_SetViewSize(screenblocks, detailLevel);
}

void M_ChangeSpriteDetail(int choice)
{
    switch (choice)
    {
    case 0:
        spriteRender--;

        if (spriteRender == -1)
            spriteRender = SPRITE_FLATTER;
        break;
    case 1:
        spriteRender++;

        if (spriteRender == NUM_SPRITERENDER)
            spriteRender = SPRITE_NORMAL;
        break;
    }

    R_SetViewSize(screenblocks, detailLevel);
}

void M_ChangePSpriteDetail(int choice)
{
    switch (choice)
    {
    case 0:
        pspriteRender--;

        if (pspriteRender == -1)
            pspriteRender = PSPRITE_FLATTER;
        break;
    case 1:
        pspriteRender++;

        if (pspriteRender == NUM_PSPRITERENDER)
            pspriteRender = PSPRITE_NORMAL;
        break;
    }

    R_SetViewSize(screenblocks, detailLevel);
}

#define MIN_GAMMA (-8)
#define MAX_GAMMA (24)

void M_ChangeGamma(int choice)
{
    fixed_t gammaval;

    switch (choice)
    {
    case 0:
        if (usegamma > MIN_GAMMA)
            usegamma--;
        break;
    case 1:
        if (usegamma < MAX_GAMMA)
            usegamma++;
        break;
    }

    memset(gammamsg, 0, sizeof(gammamsg));

    gammaval = FRACUNIT + FixedMul(TO_FIXED(usegamma),(FRACUNIT / 16));
    sprintf(gammamsg, "%i.%04i", gammaval >> FRACBITS, ((gammaval & 65535) * 10000) >> FRACBITS);

    I_SetGamma(usegamma);
    I_ProcessPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
    I_SetPalette(0);

}

void M_ChangeVsync(int choice)
{
    waitVsync = !waitVsync;
}

void M_ChangeSkyDetail(int choice)
{
    flatSky = !flatSky;

    R_SetViewSize(screenblocks, detailLevel);
}

void M_ChangeCPU(int choice)
{
    switch (choice)
    {
    case 0:
        selectedCPU--;

        if (selectedCPU == -1)
            selectedCPU = INTEL_PENTIUM;
        break;
    case 1:

        selectedCPU++;

        if (selectedCPU == NUM_CPU)
            selectedCPU = INTEL_386SX;
        break;
    }

    R_ExecuteSetViewSize();
#if defined(MODE_13H)
    I_UpdateFinishFunc();
#endif
}

void M_ChangeInvisibleDetail(int choice)
{
    switch (choice)
    {
    case 0:
        invisibleRender--;

        if (invisibleRender == -1)
            invisibleRender = INVISIBLE_TRANSLUCENT;
        break;
    case 1:
        invisibleRender++;

        if (invisibleRender == NUM_INVISIBLERENDER)
            invisibleRender = INVISIBLE_NORMAL;
    }

    if (invisibleRender == INVISIBLE_TRANSLUCENT)
        R_InitTintMap();
    else
        R_CleanupTintMap();

    R_SetViewSize(screenblocks, detailLevel);
}

void M_ChangeShowFPS(int choice)
{
    switch (choice)
    {
    case 0:
        showFPS--;

        if (showFPS == -1)
            showFPS = SCREEN_DC4D_FPS;
        break;
    case 1:
        showFPS++;

        if (showFPS == NUM_FPS)
            showFPS = NO_FPS;
    }
}

void M_ChangeAutomapRT(int choice)
{
    automapRT = !automapRT;

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    if (!automapRT)
        transparentmap = 0;
#endif
}

void M_ChangeSpriteCulling(int choice)
{
    nearSprites = !nearSprites;
}

void M_ChangeMelting(int choice)
{
    noMelt = !noMelt;
}

void M_ChangeBusSpeed(int choice)
{
    busSpeed = !busSpeed;

#if defined(MODE_13H)
    I_UpdateFinishFunc();
#endif
}

void I_SetHrTimerEnabled(int enabled);

void M_ChangeUncappedFPS(int choice)
{
    uncappedFPS = !uncappedFPS;

    if (uncappedFPS) 
    {
        highResTimer = gamestate == GS_LEVEL;
    } else {
        highResTimer = false;
    }

    I_SetHrTimerEnabled(highResTimer);
}

void M_ChangeMono()
{
    monoSound = !monoSound;
}

void M_SizeDisplay(int choice)
{
    switch (choice)
    {
    case 0:
        if (screenSize > 0)
        {
            screenblocks--;
            screenSize--;
        }
        break;
    case 1:
        if (screenSize < 9)
        {
            screenblocks++;
            screenSize++;
        }
        break;
    }

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    if (screenSize < 7)
        transparentmap = 0;
#endif

    R_SetViewSize(screenblocks, detailLevel);
}

//
//      Menu Functions
//
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void M_DrawThermo(int x, int y, int thermWidth, int thermDot)
{
    int xx;
    int i;
    xx = x;
    V_DrawPatchDirectCentered(xx, y, W_CacheLumpName("M_THERML", PU_CACHE));
    xx += 8;
    for (i = 0; i < thermWidth; i++)
    {
        V_DrawPatchDirectCentered(xx, y, W_CacheLumpName("M_THERMM", PU_CACHE));
        xx += 8;
    }
    V_DrawPatchDirectCentered(xx, y, W_CacheLumpName("M_THERMR", PU_CACHE));

    V_DrawPatchDirectCentered((x + 8) + thermDot * 8, y, W_CacheLumpName("M_THERMO", PU_CACHE));
}
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
void M_DrawThermoText(int x, int y, int thermWidth, int thermDot)
{
    int xx;
    int i;

    xx = x;
    V_WriteTextDirect(xx, y, "[");
    xx += 1;
    for (i = 0; i < thermWidth; i++)
    {
        V_WriteTextDirect(xx, y, "-");
        xx += 1;
    }
    V_WriteTextDirect(xx, y, "]");

    V_WriteTextDirect((x + 1) + thermDot, y, "|");
}
#endif

void M_StartMessage(char *string, void *routine, byte input)
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageString = string;
    messageRoutine = routine;
    messageNeedsInput = input;
    menuactive = 1;
    return;
}

//
// Find string width from hu_font chars
//
int M_StringWidth(char *string)
{
    int i;
    int w = 0;
    int c;

    for (i = 0; i < strlen(string); i++)
    {
        c = toupper(string[i]) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
            w += 4;
        else
        {
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
            w += hu_font[c]->width;
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
            w += 8;
#endif
        }
    }

    return w;
}

//
//      Find string height from hu_font chars
//
int M_StringHeight(char *string)
{
    int i;
    int h;
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    int height = hu_font[0]->height;
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
    int height = 8;
#endif

    h = height;
    for (i = 0; i < strlen(string); i++)
        if (string[i] == '\n')
            h += height;

    return h;
}

//
//      Write a string using the hu_font
//
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void M_WriteText(int x, int y, char *string)
{
    int w;
    char *ch;
    int c;
    int cx;
    int cy;

    ch = string;
    cx = x;
    cy = y;

    while (1)
    {
        c = *ch++;
        if (!c)
            break;
        if (c == '\n')
        {
            cx = x;
            cy += 12;
            continue;
        }

        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        w = hu_font[c]->width;
        if (cx + w > SCREENWIDTH)
            break;
        V_DrawPatchDirectCentered(cx, cy, hu_font[c]);
        cx += w;
    }
}
#endif

//
// CONTROL PANEL
//

//
// M_Responder
//

byte M_Responder(void)
{
    int ch;
    int i;

    if (current_ev->type == ev_keydown)
    {
        ch = current_ev->data1;
    }
    else
    {
        return 0;
    }

    // Save Game string input
    if (saveStringEnter)
    {
        switch (ch)
        {
        case KEY_BACKSPACE:
            if (saveCharIndex > 0)
            {
                saveCharIndex--;
                savegamestrings[saveSlot][saveCharIndex] = 0;
            }
            break;

        case KEY_ESCAPE:
            saveStringEnter = 0;
            strcpy(&savegamestrings[saveSlot][0], saveOldString);
            break;

        case KEY_ENTER:
            saveStringEnter = 0;
            if (savegamestrings[saveSlot][0])
                M_DoSave(saveSlot);
            break;

        default:
            ch = toupper(ch);
            if (ch != 32)
                if (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE)
                    break;
            if (ch >= 32 && ch <= 127 &&
                saveCharIndex < SAVESTRINGSIZE - 1 &&
                M_StringWidth(savegamestrings[saveSlot]) < (SAVESTRINGSIZE - 2) * 8)
            {
                savegamestrings[saveSlot][saveCharIndex++] = ch;
                savegamestrings[saveSlot][saveCharIndex] = 0;
            }
            break;
        }
        return 1;
    }

    // Take care of any messages that need input
    if (messageToPrint)
    {
        if (messageNeedsInput == 1 && !(ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE))
            return 0;

        menuactive = messageLastMenuActive;
        messageToPrint = 0;
        if (messageRoutine)
            messageRoutine(ch);

        menuactive = 0;
        S_StartSound(NULL, sfx_swtchx);
        return 1;
    }

    // F-Keys
    if (!menuactive)
        switch (ch)
        {
        case KEY_MINUS: // Screen size down
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
            if (automapactive)
                return 0;
#endif
            M_SizeDisplay(0);
            S_StartSound(NULL, sfx_stnmov);
            return 1;

        case KEY_EQUALS: // Screen size up
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
            if (automapactive)
                return 0;
#endif
            M_SizeDisplay(1);
            S_StartSound(NULL, sfx_stnmov);
            return 1;

        case KEY_F1: // FastDoom key
            M_StartMessage(I_LoadTextProgram(46), NULL, 0);
            S_StartSound(NULL, sfx_oof);
            return 1;

        case KEY_F2: // Save
            M_StartControlPanel();
            S_StartSound(NULL, sfx_swtchn);
            M_SaveGame(0);
            return 1;

        case KEY_F3: // Load
            M_StartControlPanel();
            S_StartSound(NULL, sfx_swtchn);
            M_LoadGame(0);
            return 1;

        case KEY_F4: // Sound Volume
            M_StartControlPanel();
            currentMenu = &SoundDef;
            itemOn = sfx_vol;
            S_StartSound(NULL, sfx_swtchn);
            return 1;

        case KEY_F5: // Detail toggle
            M_ChangeDetail(1);

            switch (detailLevel)
            {
            case DETAIL_HIGH:
                players.message = DETAILHI;
                break;
            case DETAIL_LOW:
                players.message = DETAILLO;
                break;
            case DETAIL_POTATO:
                players.message = DETAILPO;
                break;
            }

            S_StartSound(NULL, sfx_swtchn);
            return 1;

        case KEY_F6: // Quicksave
            S_StartSound(NULL, sfx_swtchn);
            M_QuickSave();
            return 1;

        case KEY_F7: // End game
            S_StartSound(NULL, sfx_swtchn);
            M_EndGame(0);
            return 1;

        case KEY_F8: // Toggle messages
            M_ChangeMessages(0);

            if (!showMessages)
                players.message = MSGOFF;
            else
                players.message = MSGON;

            S_StartSound(NULL, sfx_swtchn);
            return 1;

        case KEY_F9: // Quickload
            S_StartSound(NULL, sfx_swtchn);
            M_QuickLoad();
            return 1;

        case KEY_F10: // Quit DOOM
            S_StartSound(NULL, sfx_swtchn);
            M_QuitDOOM(0);
            return 1;
        case KEY_F11: // Change gamma
            if (usegamma == MAX_GAMMA)
                usegamma = -1;

            M_ChangeGamma(1);

            memset(playergammamsg, 0, sizeof(playergammamsg));
            sprintf(playergammamsg, "GAMMA %s", gammamsg);

            players.message = playergammamsg;

            return 1;
        case KEY_F12: // Autorun
            autorun = !autorun;
            if (autorun)
                players.message = AUTORUNON;
            else
                players.message = AUTORUNOFF;
            return 1;
        }

    // Pop-up menu?
    if (!menuactive)
    {
        if (ch == KEY_ESCAPE)
        {
            M_StartControlPanel();
            S_StartSound(NULL, sfx_swtchn);
            return 1;
        }
        return 0;
    }

    // Keys usable within menu
    switch (ch)
    {
    case KEY_DOWNARROW:
        do
        {
            if (itemOn + 1 > currentMenu->numitems - 1)
                itemOn = 0;
            else
                itemOn++;
            
            S_StartSound(NULL, sfx_pstop);
        } while (currentMenu->menuitems[itemOn].status == -1);
        return 1;

    case KEY_UPARROW:
        do
        {
            if (!itemOn)
                itemOn = currentMenu->numitems - 1;
            else
                itemOn--;
            
            S_StartSound(NULL, sfx_pstop);
        } while (currentMenu->menuitems[itemOn].status == -1);
        return 1;

    case KEY_LEFTARROW:
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(0);
        }
        return 1;

    case KEY_RIGHTARROW:
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(1);
        }
        return 1;

    case KEY_ENTER:
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status)
        {
            currentMenu->lastOn = itemOn;
            if (currentMenu->menuitems[itemOn].status == 2)
            {
                currentMenu->menuitems[itemOn].routine(1); // right arrow
                S_StartSound(NULL, sfx_stnmov);
            }
            else
            {
                currentMenu->menuitems[itemOn].routine(itemOn);
                S_StartSound(NULL, sfx_pistol);
            }
        }
        return 1;

    case KEY_ESCAPE:
        currentMenu->lastOn = itemOn;
        menuactive = 0;
        S_StartSound(NULL, sfx_swtchx);
        return 1;

    case KEY_BACKSPACE:
        currentMenu->lastOn = itemOn;
        if (currentMenu->prevMenu)
        {
            currentMenu = currentMenu->prevMenu;
            itemOn = currentMenu->lastOn;
            S_StartSound(NULL, sfx_swtchn);
        }
        return 1;
    }

    return 0;
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
    // intro might call this repeatedly
    if (menuactive)
        return;

    menuactive = 1;
    currentMenu = &MainDef;       // JDC
    itemOn = currentMenu->lastOn; // JDC
}

#define BENCHMARK_QUICK_LAST 3
#define BENCHMARK_NORMAL_LAST 9

void M_ShowBenchmarkCSVMessage(void)
{
    M_StartControlPanel();
    itemOn = 0;
    currentMenu = &BenchmarkCSVDef;
}

void M_FinishBenchmark(void)
{
    if (benchmark_type == 0)
    {
        if (benchmark_commandline)
        {
            benchmark_number = 0;
            M_ShowBenchmarkCSVMessage();
        }
        else
        {
            M_StartControlPanel();
            itemOn = 0;
            currentMenu = &BenchmarkResultDef;
        }
    }
    else
    {
        benchmark_number++;

        if (benchmark_number == benchmark_total)
        {
            benchmark_number = 0;
            M_ShowBenchmarkCSVMessage();
        }
        else
        {
            M_BenchmarkRunDemo();
        }
    }
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
    static short x;
    static short y;
    short i;
    char string[40];
    int start;

    // Horiz. & Vertically center string and print it.
    if (messageToPrint)
    {
        start = 0;
        y = 100 - M_StringHeight(messageString) / 2;
        while (*(messageString + start))
        {
            for (i = 0; i < strlen(messageString + start); i++)
                if (*(messageString + start + i) == '\n')
                {
                    memset(string, 0, 40);
                    strncpy(string, messageString + start, i);
                    start += i + 1;
                    break;
                }

            if (i == strlen(messageString + start))
            {
                strcpy(string, messageString + start);
                start += i;
            }

            x = 160 - M_StringWidth(string) / 2;

#if defined(MODE_T4025) || defined(MODE_T4050)
            V_WriteTextDirect(x / 8, y / 8, string);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
            V_WriteTextDirect(x / 4, y / 8, string);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
            V_WriteTextDirect(x / 4, y / 4, string);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
            M_WriteText(x, y, string);
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
            y += hu_font[0]->height;
#endif
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
            y += 8;
#endif
        }
        return;
    }

    if (!menuactive)
        return;

    if (currentMenu->routine)
        currentMenu->routine(); // call Draw routine

    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;
    for (i = 0; i < currentMenu->numitems; i++)
    {
        if (currentMenu->menuitems[i].name[0])
        {
#if defined(MODE_T4025) || defined(MODE_T4050)
            V_WriteTextDirect(x / 8, y / 8, currentMenu->menuitems[i].text);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
            V_WriteTextDirect(x / 4, y / 8, currentMenu->menuitems[i].text);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
            V_WriteTextDirect(x / 4, y / 4, currentMenu->menuitems[i].text);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
            V_DrawPatchDirectCentered(x, y, W_CacheLumpName(currentMenu->menuitems[i].name, PU_CACHE));
#endif
        }

        y += LINEHEIGHT;
    }

    if (currentMenu == &DisplayDef)
    {
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
        if (itemOn - (MAX_ITEMS_DRAWN / 2) < 0)
            V_DrawPatchDirectCentered(x + SKULLXOFF, currentMenu->y - 5 + itemOn * LINEHEIGHT, W_CacheLumpName(skullName[whichSkull], PU_CACHE));
        else if (itemOn + (MAX_ITEMS_DRAWN / 2) > display_end)
            V_DrawPatchDirectCentered(x + SKULLXOFF, currentMenu->y - 5 + (itemOn - (display_end - MAX_ITEMS_DRAWN)) * LINEHEIGHT, W_CacheLumpName(skullName[whichSkull], PU_CACHE));
        else
            V_DrawPatchDirectCentered(x + SKULLXOFF, currentMenu->y - 5 + (MAX_ITEMS_DRAWN / 2) * LINEHEIGHT, W_CacheLumpName(skullName[whichSkull], PU_CACHE));
#endif
#if defined(MODE_Y_HALF)
        if (itemOn - (MAX_ITEMS_DRAWN / 2) < 0)
            V_DrawPatchDirectCentered(x + SKULLXOFF, (currentMenu->y / 2) - 2 + itemOn * LINEHEIGHT, W_CacheLumpName(skullName[whichSkull], PU_CACHE));
        else if (itemOn + (MAX_ITEMS_DRAWN / 2) > display_end)
            V_DrawPatchDirectCentered(x + SKULLXOFF, (currentMenu->y / 2) - 2 + (itemOn - (display_end - MAX_ITEMS_DRAWN)) * LINEHEIGHT, W_CacheLumpName(skullName[whichSkull], PU_CACHE));
        else
            V_DrawPatchDirectCentered(x + SKULLXOFF, (currentMenu->y / 2) - 2 + (MAX_ITEMS_DRAWN / 2) * LINEHEIGHT, W_CacheLumpName(skullName[whichSkull], PU_CACHE));
#endif
#if defined(MODE_T4025) || defined(MODE_T4050)
        if (itemOn - (MAX_ITEMS_DRAWN / 2) < 0)
            V_WriteCharDirect(currentMenu->x / 8 - 3, currentMenu->y / 8 + itemOn * 2, whichSkull + 1);
        else if (itemOn + (MAX_ITEMS_DRAWN / 2) > display_end)
            V_WriteCharDirect(currentMenu->x / 8 - 3, currentMenu->y / 8 + (itemOn - (display_end - MAX_ITEMS_DRAWN)) * 2, whichSkull + 1);
        else
            V_WriteCharDirect(currentMenu->x / 8 - 3, currentMenu->y / 8 + (MAX_ITEMS_DRAWN / 2) * 2, whichSkull + 1);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
        if (itemOn - (MAX_ITEMS_DRAWN / 2) < 0)
            V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 8 + itemOn * 2, whichSkull + 1);
        else if (itemOn + (MAX_ITEMS_DRAWN / 2) > display_end)
            V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 8 + (itemOn - (display_end - MAX_ITEMS_DRAWN)) * 2, whichSkull + 1);
        else
            V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 8 + (MAX_ITEMS_DRAWN / 2) * 2, whichSkull + 1);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
        if (itemOn - (MAX_ITEMS_DRAWN / 2) < 0)
            V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 4 + itemOn * 4, whichSkull + 1);
        else if (itemOn + (MAX_ITEMS_DRAWN / 2) > display_end)
            V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 4 + (itemOn - (display_end - MAX_ITEMS_DRAWN)) * 4, whichSkull + 1);
        else
            V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 4 + (MAX_ITEMS_DRAWN / 2) * 4, whichSkull + 1);
#endif
    }
    else
    {

// DRAW SKULL
#if defined(MODE_T4025) || defined(MODE_T4050)
        V_WriteCharDirect(currentMenu->x / 8 - 3, currentMenu->y / 8 + itemOn * 2, whichSkull + 1);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA) || defined(MODE_COLOR_MDA)
        V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 8 + itemOn * 2, whichSkull + 1);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
        V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 4 + itemOn * 4, whichSkull + 1);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
        V_DrawPatchDirectCentered(x + SKULLXOFF, currentMenu->y - 5 + itemOn * LINEHEIGHT, W_CacheLumpName(skullName[whichSkull], PU_CACHE));
#endif
#if defined(MODE_Y_HALF)
        V_DrawPatchDirectCentered(x + SKULLXOFF, (currentMenu->y / 2) - 2 + itemOn * LINEHEIGHT, W_CacheLumpName(skullName[whichSkull], PU_CACHE));
#endif
    }
}

//
// M_Ticker
//
void M_Ticker(void)
{
    if (--skullAnimCounter <= 0)
    {
        whichSkull ^= 1;
        skullAnimCounter = 8;
    }
}

//
// M_Init
//
void M_Init(void)
{
    fixed_t gammaval = FRACUNIT + FixedMul(TO_FIXED(usegamma),(FRACUNIT / 16));

    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    whichSkull = 0;
    skullAnimCounter = 10;
    screenSize = screenblocks - 3;
    messageToPrint = 0;
    messageString = NULL;
    messageLastMenuActive = menuactive;
    quickSaveSlot = -1;

    // Modify menus for episode selection on shareware/registered DOOM
    if (gamemode == shareware)
    {
        EpiDef.numitems -= 3;
    }
    else if (gamemode == registered)
    {
        EpiDef.numitems -= 1;
    }

    sprintf(gammamsg, "%i.%04i", gammaval >> FRACBITS, ((gammaval & 65535) * 10000) >> FRACBITS);
}
