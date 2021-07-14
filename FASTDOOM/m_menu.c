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

#include "hu_stuff.h"

#include "g_game.h"

#include "m_misc.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

#include "m_menu.h"



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

char endmsg[NUM_QUITMESSAGES][80] =
    {
        // DOOM1
        QUITMSG,
        "please don't leave, there's more\ndemons to toast!",
        "let's beat it -- this is turning\ninto a bloodbath!",
        "i wouldn't leave if i were you.\ndos is much worse.",
        "you're trying to say you like dos\nbetter than me, right?",
        "don't leave yet -- there's a\ndemon around that corner!",
        "ya know, next time you come in here\ni'm gonna toast ya.",
        "go ahead and leave. see if i care."};

char endmsg2[NUM_QUITMESSAGES][80] =
    {
        // QuitDOOM II messages
        QUITMSG,
        "you want to quit?\nthen, thou hast lost an eighth!",
        "don't go now, there's a \ndimensional shambler waiting\nat the dos prompt!",
        "get outta here and go back\nto your boring programs.",
        "if i were your boss, i'd \n deathmatch ya in a minute!",
        "look, bud. you leave now\nand you forfeit your body count!",
        "just leave. when you come\nback, i'll be waiting with a bat.",
        "you're lucky i don't smack\nyou for thinking about leaving."};

// we are going to be entering a savegame string
int saveStringEnter;
int saveSlot;      // which slot to save in
int saveCharIndex; // which char we're editing
// old save description before edit
char saveOldString[SAVESTRINGSIZE];

byte inhelpscreens;
byte menuactive;

#define SKULLXOFF -32
#define LINEHEIGHT 16

char savegamestrings[10][SAVESTRINGSIZE];

char endstring[160];

//
// MENU TYPEDEFS
//
typedef struct
{
    // 0 = no cursor here, 1 = ok, 2 = arrows ok
    short status;

    char name[10];
    char text[22];

    // choice = menu item #.
    // if status = 2,
    //   choice=0:leftarrow,1:rightarrow
    void (*routine)(int choice);

    // hotkey in menu
    char alphaKey;
} menuitem_t;

typedef struct menu_s
{
    short numitems;          // # of menu items
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
char skullName[2][/*8*/ 9] = {"M_SKULL1", "M_SKULL2"};

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
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_QuitDOOM(int choice);

void M_ChangeMessages(int choice);
void M_ChangeSensitivity(int choice);
void M_SfxVol(int choice);
void M_MusicVol(int choice);
void M_ChangeDetail();
void M_ChangeVisplaneDetail();
void M_ChangeVsync();
void M_ChangeSkyDetail();
void M_ChangeInvisibleDetail();
void M_ChangeShowFPS();
void M_ChangeSpriteCulling();
void M_ChangeMelting();
void M_ChangeUncappedFPS();
void M_ChangeMono();
void M_SizeDisplay(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);
void M_Display(int choice);

void M_FinishReadThis(int choice);
void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_QuickSave(void);
void M_QuickLoad(void);

void M_DrawMainMenu(void);
void M_DrawReadThis1(void);
void M_DrawReadThis2(void);
void M_DrawReadThisRetail(void);
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
void M_DrawSound(void);
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
#define readthis 4
#define quitdoom 5
#define main_end 6

menuitem_t MainMenu[] =
    {
        {1, "M_NGAME", "New game", M_NewGame, 'n'},
        {1, "M_OPTION", "Options", M_Options, 'o'},
        {1, "M_LOADG", "Load game", M_LoadGame, 'l'},
        {1, "M_SAVEG", "Save game", M_SaveGame, 's'},
        {1, "M_RDTHIS", "Read this!", M_ReadThis, 'r'},
        {1, "M_QUITG", "Quit game", M_QuitDOOM, 'q'}};

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
        {1, "M_EPI1", "Knee-Deep in the Dead", M_Episode, 'k'},
        {1, "M_EPI2", "The Shores of Hell", M_Episode, 't'},
        {1, "M_EPI3", "Inferno", M_Episode, 'i'},
        {1, "M_EPI4", "Thy Flesh Consumed", M_Episode, 't'}};

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
        {1, "M_JKILL", "I'm too young to die", M_ChooseSkill, 'i'},
        {1, "M_ROUGH", "Hey, not too rough", M_ChooseSkill, 'h'},
        {1, "M_HURT", "Hurt me plenty", M_ChooseSkill, 'h'},
        {1, "M_ULTRA", "Ultra-violence", M_ChooseSkill, 'u'},
        {1, "M_NMARE", "NIGHTMARE!", M_ChooseSkill, 'n'}};

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
#define vsync 0
#define detail 1
#define visplanes 2
#define sky 3
#define invisible 4
#define showfps 5
#define spriteculling 6
#define melting 7
#define uncappedfps 8
#define display_end 9

#define endgame 0
#define messages 1
#define display 2
#define scrnsize 3
#define option_empty1 4
#define mousesens 5
#define option_empty2 6
#define soundvol 7
#define opt_end 8

menuitem_t OptionsMenu[] =
    {
        {1, "M_ENDGAM", "End game", M_EndGame, 'e'},
        {1, "M_MESSG", "Messages:", M_ChangeMessages, 'm'},
        {1, "M_DISP", "Display", M_Display, 'd'},
        {2, "M_SCRNSZ", "Screen size", M_SizeDisplay, 's'},
        {-1, "", "", 0},
        {2, "M_MSENS", "Mouse sensitivity", M_ChangeSensitivity, 'm'},
        {-1, "", "", 0},
        {1, "M_SVOL", "Sound volume", M_Sound, 's'}};

menu_t OptionsDef =
    {
        opt_end,
        &MainDef,
        OptionsMenu,
        M_DrawOptions,
        60, 37,
        0};

menuitem_t DisplayMenu[] =
    {
        {1, "", "", M_ChangeVsync, 's'},
        {1, "", "", M_ChangeDetail, 'g'},
        {1, "", "", M_ChangeVisplaneDetail, 'v'},
        {1, "", "", M_ChangeSkyDetail, 's'},
        {1, "", "", M_ChangeInvisibleDetail, 'i'},
        {1, "", "", M_ChangeShowFPS, 'f'},
        {1, "", "", M_ChangeSpriteCulling, 'c'},
        {1, "", "", M_ChangeMelting, 'm'},
        {1, "", "", M_ChangeUncappedFPS, 'u'}};

menu_t DisplayDef =
    {
        display_end,
        &OptionsDef,
        DisplayMenu,
        M_DrawDisplay,
        60, 21,
        0};

//
// Read This! MENU 1 & 2
//
#define rdthsempty1 0
#define read1_end 1

menuitem_t ReadMenu1[] =
    {
        {1, "", "", M_ReadThis2, 0}};

menu_t ReadDef1 =
    {
        read1_end,
        &MainDef,
        ReadMenu1,
        M_DrawReadThis1,
        280, 185,
        0};

#define rdthsempty2 0
#define read2_end 1

menuitem_t ReadMenu2[] =
    {
        {1, "", "", M_FinishReadThis, 0}};

menu_t ReadDef2 =
    {
        read2_end,
        &ReadDef1,
        ReadMenu2,
        M_DrawReadThis2,
        330, 175,
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

menuitem_t SoundMenu[] =
    {
        {2, "M_SFXVOL", "SFX volume", M_SfxVol, 's'},
        {-1, "", "", 0},
        {2, "M_MUSVOL", "Music volume", M_MusicVol, 'm'},
        {-1, "", "", 0},
        {1, "", "", M_ChangeMono, 'c'}};

menu_t SoundDef =
    {
        sound_end,
        &OptionsDef,
        SoundMenu,
        M_DrawSound,
        80, 64,
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
        {1, "", "", M_LoadSelect, '1'},
        {1, "", "", M_LoadSelect, '2'},
        {1, "", "", M_LoadSelect, '3'},
        {1, "", "", M_LoadSelect, '4'},
        {1, "", "", M_LoadSelect, '5'},
        {1, "", "", M_LoadSelect, '6'}};

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
        {1, "", "", M_SaveSelect, '1'},
        {1, "", "", M_SaveSelect, '2'},
        {1, "", "", M_SaveSelect, '3'},
        {1, "", "", M_SaveSelect, '4'},
        {1, "", "", M_SaveSelect, '5'},
        {1, "", "", M_SaveSelect, '6'}};

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
        sprintf(name, SAVEGAMENAME "%d.dsg", i);

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

#ifdef MODE_T25
    V_WriteTextDirect(18, 3, "LOAD GAME");
#endif
#ifdef MODE_T50
    V_WriteTextDirect(18, 7, "LOAD GAME");
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(72, 28, W_CacheLumpName("M_LOADG", PU_CACHE));
#endif

    for (i = 0; i < load_end; i++)
    {
#ifdef MODE_T25
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8 - 2);
        V_WriteCharDirect(LoadDef.x / 4 - 1, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        V_WriteTextDirect(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 4 + 24, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8);
#endif
#ifdef MODE_T50
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + (LINEHEIGHT)*i) / 4 - 2);
        V_WriteCharDirect(LoadDef.x / 4 - 1, (LoadDef.y + LINEHEIGHT * i) / 4, '|');
        V_WriteTextDirect(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 4, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 4 + 24, (LoadDef.y + LINEHEIGHT * i) / 4, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 4);
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
        M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT * i);
        M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
#endif
    }
}

//
// Draw border for the savegame description
//
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
void M_DrawSaveLoadBorder(int x, int y)
{
    int i;

    V_DrawPatchDirect(x - 8, y + 7, W_CacheLumpName("M_LSLEFT", PU_CACHE));

    for (i = 0; i < 24; i++)
    {
        V_DrawPatchDirect(x, y + 7, W_CacheLumpName("M_LSCNTR", PU_CACHE));
        x += 8;
    }

    V_DrawPatchDirect(x, y + 7, W_CacheLumpName("M_LSRGHT", PU_CACHE));
}
#endif

#if defined(MODE_T25) || defined(MODE_T50)
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

    sprintf(name, SAVEGAMENAME "%d.dsg", choice);
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

#ifdef MODE_T25
    V_WriteTextDirect(18, 3, "SAVE GAME");
#endif
#ifdef MODE_T50
    V_WriteTextDirect(18, 7, "SAVE GAME");
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(72, 28, W_CacheLumpName("M_SAVEG", PU_CACHE));
#endif

    for (i = 0; i < load_end; i++)
    {
#ifdef MODE_T25
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8 - 2);
        V_WriteCharDirect(LoadDef.x / 4 - 1, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        V_WriteTextDirect(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 4 + 24, (LoadDef.y + LINEHEIGHT * i) / 8, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 8);
#endif
#ifdef MODE_T50
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + (LINEHEIGHT)*i) / 4 - 2);
        V_WriteCharDirect(LoadDef.x / 4 - 1, (LoadDef.y + LINEHEIGHT * i) / 4, '|');
        V_WriteTextDirect(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 4, savegamestrings[i]);
        V_WriteCharDirect(LoadDef.x / 4 + 24, (LoadDef.y + LINEHEIGHT * i) / 4, '|');
        M_DrawSaveLoadBorderText(LoadDef.x / 4, (LoadDef.y + LINEHEIGHT * i) / 4);
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
        M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT * i);
        M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
#endif
    }

    if (saveStringEnter)
    {
#ifdef MODE_T25
        V_WriteTextDirect((LoadDef.x / 4) + strlen(savegamestrings[saveSlot]), (LoadDef.y + LINEHEIGHT * saveSlot) / 8, "_");
#endif
#ifdef MODE_T50
        V_WriteTextDirect((LoadDef.x / 4) + strlen(savegamestrings[saveSlot]), (LoadDef.y + LINEHEIGHT * saveSlot) / 4, "_");
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
        i = M_StringWidth(savegamestrings[saveSlot]);
        M_WriteText(LoadDef.x + i, LoadDef.y + LINEHEIGHT * saveSlot, "_");
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
        M_StartMessage(SAVEDEAD, NULL, 0);
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
    sprintf(tempstring, QSPROMPT, savegamestrings[quickSaveSlot]);
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
        M_StartMessage(QSAVESPOT, NULL, 0);
        return;
    }
    sprintf(tempstring, QLPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickLoadResponse, 1);
}

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
    inhelpscreens = 1;

#ifdef MODE_T25
    V_DrawPatchDirectText8025(0, 0, W_CacheLumpName("HELP2", PU_CACHE));
#endif
#ifdef MODE_T50
    V_DrawPatchDirectText8050(0, 0, W_CacheLumpName("HELP2", PU_CACHE));
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(0, 0, W_CacheLumpName("HELP2", PU_CACHE));
#endif
}

//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
    inhelpscreens = 1;

#ifdef MODE_T25
    V_DrawPatchDirectText8025(0, 0, W_CacheLumpName("HELP1", PU_CACHE));
#endif
#ifdef MODE_T50
    V_DrawPatchDirectText8050(0, 0, W_CacheLumpName("HELP1", PU_CACHE));
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(0, 0, W_CacheLumpName("HELP1", PU_CACHE));
#endif
}

void M_DrawReadThisRetail(void)
{
    inhelpscreens = 1;

#ifdef MODE_T25
    V_DrawPatchDirectText8025(0, 0, W_CacheLumpName("HELP", PU_CACHE));
#endif
#ifdef MODE_T50
    V_DrawPatchDirectText8050(0, 0, W_CacheLumpName("HELP", PU_CACHE));
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(0, 0, W_CacheLumpName("HELP", PU_CACHE));
#endif
}

//
// Change Sfx & Music volumes
//
void M_DrawSound(void)
{
#ifdef MODE_T25
    V_WriteTextDirect(30, 4, "SOUND VOLUME");

    M_DrawThermoText(SoundDef.x / 4, (SoundDef.y + LINEHEIGHT * (sfx_vol + 1)) / 8, 16, sfxVolume);
    M_DrawThermoText(SoundDef.x / 4, (SoundDef.y + LINEHEIGHT * (music_vol + 1)) / 8, 16, musicVolume);

    V_WriteTextDirect(20, 16, "Mono Sound:");
    V_WriteTextDirect(40, 16, monoSound ? "ON" : "OFF");
#endif
#ifdef MODE_T50
    V_WriteTextDirect(30, 8, "SOUND VOLUME");

    M_DrawThermoText(SoundDef.x / 4, (SoundDef.y + LINEHEIGHT * (sfx_vol + 1)) / 4, 16, sfxVolume);
    M_DrawThermoText(SoundDef.x / 4, (SoundDef.y + LINEHEIGHT * (music_vol + 1)) / 4, 16, musicVolume);

    V_WriteTextDirect(20, 32, "Mono Sound:");
    V_WriteTextDirect(40, 32, monoSound ? "ON" : "OFF");
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(60, 38, W_CacheLumpName("M_SVOL", PU_CACHE));

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
#ifdef MODE_T25
    V_WriteTextDirect(23, 5, "DOOM");
#endif
#ifdef MODE_T50
    V_WriteTextDirect(23, 10, "DOOM");
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(94, 2, W_CacheLumpName("M_DOOM", PU_CACHE));
#endif
}

//
// M_NewGame
//
void M_DrawNewGame(void)
{
#ifdef MODE_T25
    V_WriteTextDirect(24, 2, "NEW GAME");
    V_WriteTextDirect(13, 4, "Choose skill level:");
#endif
#ifdef MODE_T50
    V_WriteTextDirect(24, 2, "NEW GAME");
    V_WriteTextDirect(13, 9, "Choose skill level:");
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(96, 14, W_CacheLumpName("M_NEWG", PU_CACHE));
    V_DrawPatchDirect(54, 38, W_CacheLumpName("M_SKILL", PU_CACHE));
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
#ifdef MODE_T25
    V_WriteTextDirect(27, 4, "WHICH EPISODE?");
#endif
#ifdef MODE_T50
    V_WriteTextDirect(27, 9, "WHICH EPISODE?");
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(54, 38, W_CacheLumpName("M_EPISOD", PU_CACHE));
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
        M_StartMessage(NIGHTMARE, M_VerifyNightmare, 1);
        return;
    }

    G_DeferedInitNew(choice, epi + 1, 1);
    menuactive = 0;
}

void M_Episode(int choice)
{
    if (gamemode == shareware && choice)
    {
        M_StartMessage(SWSTRING, NULL, 0);
        M_SetupNextMenu(&ReadDef1);
        return;
    }

    epi = choice;
    M_SetupNextMenu(&NewDef);
}

//
// M_Options
//
const char msgNames[2][9] = {"M_MSGOFF", "M_MSGON"};

void M_DrawOptions(void)
{
#ifdef MODE_T25
    V_WriteTextDirect(27, 2, "OPTIONS");
    V_WriteTextDirect((OptionsDef.x + 120) / 6, (OptionsDef.y + LINEHEIGHT * messages) / 8, showMessages == 0 ? "OFF" : "ON");
    M_DrawThermoText(OptionsDef.x / 4, (OptionsDef.y + LINEHEIGHT * (mousesens + 1)) / 8, 10, mouseSensitivity);
    M_DrawThermoText(OptionsDef.x / 4, (OptionsDef.y + LINEHEIGHT * (scrnsize + 1)) / 8, 9, screenSize);
#endif
#ifdef MODE_T50
    V_WriteTextDirect(27, 6, "OPTIONS");
    V_WriteTextDirect((OptionsDef.x + 120) / 6, (OptionsDef.y + LINEHEIGHT * messages) / 4, showMessages == 0 ? "OFF" : "ON");
    M_DrawThermoText(OptionsDef.x / 4, (OptionsDef.y + LINEHEIGHT * (mousesens + 1)) / 4, 10, mouseSensitivity);
    M_DrawThermoText(OptionsDef.x / 4, (OptionsDef.y + LINEHEIGHT * (scrnsize + 1)) / 4, 9, screenSize);
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(108, 15, W_CacheLumpName("M_OPTTTL", PU_CACHE));
    V_DrawPatchDirect(OptionsDef.x + 120, OptionsDef.y + LINEHEIGHT * messages, W_CacheLumpName((char *)msgNames[showMessages], PU_CACHE));
    M_DrawThermo(OptionsDef.x, OptionsDef.y + LINEHEIGHT * (mousesens + 1), 10, mouseSensitivity);
    M_DrawThermo(OptionsDef.x, OptionsDef.y + LINEHEIGHT * (scrnsize + 1), 9, screenSize);
#endif
}

void M_DrawDisplay(void)
{
    //V_DrawPatchDirect(54, 15, 0, W_CacheLumpName("M_DISOPT", PU_CACHE));

#ifdef MODE_T25
    V_WriteTextDirect(15, 2, "VSync:");
    V_WriteTextDirect(45, 2, waitVsync ? "ON" : "OFF");

    V_WriteTextDirect(15, 4, "Detail level:");
    V_WriteTextDirect(45, 4, detailLevel == 2 ? "POTATO" : detailLevel == 1 ? "LOW"
                                                                            : "HIGH");

    V_WriteTextDirect(15, 6, "Visplane rendering:");
    V_WriteTextDirect(45, 6, (!untexturedSurfaces && !flatSurfaces) ? "FULL" : untexturedSurfaces ? "FLAT"
                                                                                                  : "FLATTER");

    V_WriteTextDirect(15, 8, "Sky rendering:");
    V_WriteTextDirect(45, 8, flatSky ? "FLAT" : "FULL");

    V_WriteTextDirect(15, 10, "Invisible rendering:");
    V_WriteTextDirect(45, 10, (!saturnShadows && !flatShadows) ? "FUZZY" : flatShadows ? "FLAT"
                                                                                       : "SEGA SATURN");

    V_WriteTextDirect(15, 12, "Show FPS:");
    V_WriteTextDirect(45, 12, showFPS ? "ON" : "OFF");

    V_WriteTextDirect(15, 14, "Sprite culling:");
    V_WriteTextDirect(45, 14, nearSprites ? "ON" : "OFF");

    V_WriteTextDirect(15, 16, "Melting load effect:");
    V_WriteTextDirect(45, 16, noMelt ? "OFF" : "ON");

    V_WriteTextDirect(15, 18, "Uncapped framerate:");
    V_WriteTextDirect(45, 18, uncappedFPS ? "ON" : "OFF");
#endif
#ifdef MODE_T50
    V_WriteTextDirect(15, 5, "VSync:");
    V_WriteTextDirect(45, 5, waitVsync ? "ON" : "OFF");

    V_WriteTextDirect(15, 9, "Detail level:");
    V_WriteTextDirect(45, 9, detailLevel == 2 ? "POTATO" : detailLevel == 1 ? "LOW"
                                                                            : "HIGH");

    V_WriteTextDirect(15, 13, "Visplane rendering:");
    V_WriteTextDirect(45, 13, (!untexturedSurfaces && !flatSurfaces) ? "FULL" : untexturedSurfaces ? "FLAT"
                                                                                                   : "FLATTER");

    V_WriteTextDirect(15, 17, "Sky rendering:");
    V_WriteTextDirect(45, 17, flatSky ? "FLAT" : "FULL");

    V_WriteTextDirect(15, 21, "Invisible rendering:");
    V_WriteTextDirect(45, 21, (!saturnShadows && !flatShadows) ? "FUZZY" : flatShadows ? "FLAT"
                                                                                       : "SEGA SATURN");

    V_WriteTextDirect(15, 25, "Show FPS:");
    V_WriteTextDirect(45, 25, showFPS ? "ON" : "OFF");

    V_WriteTextDirect(15, 29, "Sprite culling:");
    V_WriteTextDirect(45, 29, nearSprites ? "ON" : "OFF");

    V_WriteTextDirect(15, 33, "Melting load effect:");
    V_WriteTextDirect(45, 33, noMelt ? "OFF" : "ON");

    V_WriteTextDirect(15, 37, "Uncapped framerate:");
    V_WriteTextDirect(45, 37, uncappedFPS ? "ON" : "OFF");
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    M_WriteText(58, 24, "VSYNC:");
    M_WriteText(214, 24, waitVsync ? "ON" : "OFF");

    M_WriteText(58, 40, "DETAIL LEVEL:");
    M_WriteText(214, 40, detailLevel == 2 ? "POTATO" : detailLevel == 1 ? "LOW"
                                                                        : "HIGH");

    M_WriteText(58, 56, "VISPLANE RENDERING:");
    M_WriteText(214, 56, (!untexturedSurfaces && !flatSurfaces) ? "FULL" : untexturedSurfaces ? "FLAT"
                                                                                              : "FLATTER");

    M_WriteText(58, 72, "SKY RENDERING:");
    M_WriteText(214, 72, flatSky ? "FLAT" : "FULL");

    M_WriteText(58, 88, "INVISIBLE RENDERING:");
    M_WriteText(214, 88, (!saturnShadows && !flatShadows) ? "FUZZY" : flatShadows ? "FLAT"
                                                                                  : "SEGA SATURN");

    M_WriteText(58, 104, "SHOW FPS:");
    M_WriteText(214, 104, showFPS ? "ON" : "OFF");

    M_WriteText(58, 120, "SPRITE CULLING:");
    M_WriteText(214, 120, nearSprites ? "ON" : "OFF");

    M_WriteText(58, 136, "MELTING LOAD EFFECT:");
    M_WriteText(214, 136, noMelt ? "OFF" : "ON");

    M_WriteText(58, 152, "UNCAPPED FRAMERATE:");
    M_WriteText(214, 152, uncappedFPS ? "ON" : "OFF");
#endif
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
    // warning: unused parameter `int choice'
    choice = 0;
    showMessages = 1 - showMessages;

    if (!showMessages)
        players.message = MSGOFF;
    else
        players.message = MSGON;

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
    choice = 0;
    if (!usergame)
    {
        S_StartSound(NULL, sfx_oof);
        return;
    }

    M_StartMessage(ENDGAME, M_EndGameResponse, 1);
}

//
// M_ReadThis
//
void M_ReadThis(int choice)
{
    choice = 0;

    switch (gamemode)
    {
    case shareware:
    case registered:
        M_SetupNextMenu(&ReadDef1);
        break;
    case retail:
        M_SetupNextMenu(&ReadDef2);
        break;
    case commercial:
        M_SetupNextMenu(&ReadDef2);
        break;
    }
}

void M_ReadThis2(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef2);
}

void M_FinishReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&MainDef);
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

    #ifndef MODE_HERC
    do
    {
        I_WaitSingleVBL();
    } while (i--);
    #endif

    I_Quit();
}

void M_QuitDOOM(int choice)
{
    // We pick index 0 which is language sensitive,
    //  or one at random, between 1 and maximum number.
    if (gamemode == commercial)
    {
        sprintf(endstring, "%s\n\n" DOSY, endmsg2[(gametic >> 2) % NUM_QUITMESSAGES]);
    }
    else
    {
        sprintf(endstring, "%s\n\n" DOSY, endmsg[(gametic >> 2) % NUM_QUITMESSAGES]);
    }

    M_StartMessage(endstring, M_QuitResponse, 1);
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

void M_ChangeDetail()
{
    detailLevel++;

    if (detailLevel == 3)
        detailLevel = 0;

    R_SetViewSize(screenblocks, detailLevel);

    switch (detailLevel)
    {
    case 0:
        players.message = DETAILHI;
        break;
    case 1:
        players.message = DETAILLO;
        break;
    case 2:
        players.message = DETAILPO;
        break;
    }
}

void M_ChangeVisplaneDetail()
{
    if (!untexturedSurfaces && !flatSurfaces)
    {
        flatSurfaces = false;
        untexturedSurfaces = true;
    }
    else if (untexturedSurfaces)
    {
        untexturedSurfaces = false;
        flatSurfaces = true;
    }
    else
    {
        flatSurfaces = false;
        untexturedSurfaces = false;
    }

    R_SetViewSize(screenblocks, detailLevel);

    if (!untexturedSurfaces && !flatSurfaces)
    {
        players.message = "FULL VISPLANES";
    }
    else if (untexturedSurfaces)
    {
        players.message = "FLAT VISPLANES";
    }
    else
    {
        players.message = "FLATTER VISPLANES";
    }
}

void M_ChangeVsync()
{
    waitVsync = !waitVsync;

    if (waitVsync)
    {
        players.message = "VSYNC ENABLED";
    }
    else
    {
        players.message = "VSYNC DISABLED";
    }
}

void M_ChangeSkyDetail()
{
    flatSky = !flatSky;

    R_SetViewSize(screenblocks, detailLevel);

    if (flatSky)
    {
        players.message = "FLAT COLOR SKY";
    }
    else
    {
        players.message = "FULL SKY";
    }
}

void M_ChangeInvisibleDetail()
{
    if (!flatShadows && !saturnShadows)
    {
        saturnShadows = false;
        flatShadows = true;
    }
    else if (flatShadows)
    {
        flatShadows = false;
        saturnShadows = true;
    }
    else
    {
        flatShadows = false;
        saturnShadows = false;
    }

    R_SetViewSize(screenblocks, detailLevel);

    if (!flatShadows && !saturnShadows)
    {
        players.message = "FULL INVISIBILITY";
    }
    else if (flatShadows)
    {
        players.message = "FLAT INVISIBILITY";
    }
    else
    {
        players.message = "SEGA SATURN INVISIBILITY";
    }
}

void M_ChangeShowFPS()
{
    showFPS = !showFPS;
}

void M_ChangeSpriteCulling()
{
    nearSprites = !nearSprites;

    if (nearSprites)
    {
        players.message = "SPRITE CULLING ON";
    }
    else
    {
        players.message = "SPRITE CULLING OFF";
    }
}

void M_ChangeMelting()
{
    noMelt = !noMelt;

    if (noMelt)
    {
        players.message = "MELTING SCREEN LOAD EFFECT OFF";
    }
    else
    {
        players.message = "MELTING SCREEN LOAD EFFECT ON";
    }
}

void M_ChangeUncappedFPS()
{
    uncappedFPS = !uncappedFPS;

    if (uncappedFPS)
    {
        players.message = "UNCAPPED FRAMERATE ON";
    }
    else
    {
        players.message = "35 FPS LIMIT ON";
    }
}

void M_ChangeMono()
{
    monoSound = !monoSound;

    if (monoSound)
    {
        players.message = "MONO SOUND";
    }
    else
    {
        players.message = "STEREO SOUND";
    }
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
        if (screenSize < 8)
        {
            screenblocks++;
            screenSize++;
        }
        break;
    }

    R_SetViewSize(screenblocks, detailLevel);
}

//
//      Menu Functions
//
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
void M_DrawThermo(int x, int y, int thermWidth, int thermDot)
{
    int xx;
    int i;

    xx = x;
    V_DrawPatchDirect(xx, y, W_CacheLumpName("M_THERML", PU_CACHE));
    xx += 8;
    for (i = 0; i < thermWidth; i++)
    {
        V_DrawPatchDirect(xx, y, W_CacheLumpName("M_THERMM", PU_CACHE));
        xx += 8;
    }
    V_DrawPatchDirect(xx, y, W_CacheLumpName("M_THERMR", PU_CACHE));

    V_DrawPatchDirect((x + 8) + thermDot * 8, y, W_CacheLumpName("M_THERMO", PU_CACHE));
}
#endif

#if defined(MODE_T25) || defined(MODE_T50)
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
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
            w += hu_font[c]->width;
#endif
#if defined(MODE_T25) || defined(MODE_T50)
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
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    int height = hu_font[0]->height;
#endif
#if defined(MODE_T25) || defined(MODE_T50)
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
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
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
        V_DrawPatchDirect(cx, cy, hu_font[c]);
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
const char gammamsg[5][26] =
    {
        GAMMALVL0,
        GAMMALVL1,
        GAMMALVL2,
        GAMMALVL3,
        GAMMALVL4};

byte M_Responder(event_t *ev)
{
    int ch;
    int i;
    static int lasty = 0;
    static int lastx = 0;

    ch = -1;

    if (ev->type == ev_keydown)
    {
        ch = ev->data1;
    }

    if (ch == -1)
        return 0;

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
            if (automapactive)
                return 0;
            M_SizeDisplay(0);
            S_StartSound(NULL, sfx_stnmov);
            return 1;

        case KEY_EQUALS: // Screen size up
            if (automapactive)
                return 0;
            M_SizeDisplay(1);
            S_StartSound(NULL, sfx_stnmov);
            return 1;

        case KEY_F1: // Help key
            M_StartControlPanel();

            if (gamemode == retail)
                currentMenu = &ReadDef2;
            else
                currentMenu = &ReadDef1;

            itemOn = 0;
            S_StartSound(NULL, sfx_swtchn);
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
            M_ChangeDetail();
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
        case KEY_F11: // gamma toggle
            usegamma++;
            if (usegamma > 4)
                usegamma = 0;
            players.message = (char *)gammamsg[usegamma];
            I_ProcessPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
            I_SetPalette(0);
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
            if ((gamemode == shareware || gamemode == registered) && currentMenu == &EpiDef)
            {
                if (itemOn + 1 > currentMenu->numitems - 2)
                    itemOn = 0;
                else
                    itemOn++;
            }
            else
            {
                if (itemOn + 1 > currentMenu->numitems - 1)
                    itemOn = 0;
                else
                    itemOn++;
            }

            S_StartSound(NULL, sfx_pstop);
        } while (currentMenu->menuitems[itemOn].status == -1);
        return 1;

    case KEY_UPARROW:
        do
        {
            if ((gamemode == shareware || gamemode == registered) && currentMenu == &EpiDef)
            {
                if (!itemOn)
                    itemOn = currentMenu->numitems - 2;
                else
                    itemOn--;
            }
            else
            {
                if (!itemOn)
                    itemOn = currentMenu->numitems - 1;
                else
                    itemOn--;
            }

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

    default:
        for (i = itemOn + 1; i < currentMenu->numitems; i++)
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(NULL, sfx_pstop);
                return 1;
            }
        for (i = 0; i <= itemOn; i++)
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(NULL, sfx_pstop);
                return 1;
            }
        break;
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
    short max;
    char string[40];
    int start;

    inhelpscreens = 0;

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

#ifdef MODE_T25
            V_WriteTextDirect(x / 4, y / 8, string);
#endif
#ifdef MODE_T50
            V_WriteTextDirect(x / 4, y / 4, string);
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
            M_WriteText(x, y, string);
#endif

#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
            y += hu_font[0]->height;
#endif
#if defined(MODE_T25) || defined(MODE_T50)
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
    max = currentMenu->numitems;

    if ((gamemode == shareware || gamemode == registered) && currentMenu == &EpiDef){
        max -= 1;
    }

    for (i = 0; i < max; i++)
    {
        if (currentMenu->menuitems[i].name[0])
        {
#ifdef MODE_T25
            V_WriteTextDirect(x / 4, y / 8, currentMenu->menuitems[i].text);
#endif
#ifdef MODE_T50
            V_WriteTextDirect(x / 4, y / 4, currentMenu->menuitems[i].text);
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
            V_DrawPatchDirect(x, y, W_CacheLumpName(currentMenu->menuitems[i].name, PU_CACHE));
#endif
        }

        y += LINEHEIGHT;
    }

// DRAW SKULL
#ifdef MODE_T25
    V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 8 + itemOn * 2, whichSkull + 1);
#endif
#ifdef MODE_T50
    V_WriteCharDirect(currentMenu->x / 4 - 3, currentMenu->y / 4 + itemOn * 4, whichSkull + 1);
#endif
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_VBE2)
    V_DrawPatchDirect(x + SKULLXOFF, currentMenu->y - 5 + itemOn * LINEHEIGHT, W_CacheLumpName(skullName[whichSkull], PU_CACHE));
#endif
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

    if (gamemode == commercial)
    {
        MainMenu[readthis] = MainMenu[quitdoom];
        MainDef.numitems--;
        MainDef.y += 8;
        NewDef.prevMenu = &MainDef;
        ReadDef1.routine = M_DrawReadThisRetail;
        ReadDef1.x = 330;
        ReadDef1.y = 165;
        ReadMenu1[0].routine = M_FinishReadThis;
    }
}
