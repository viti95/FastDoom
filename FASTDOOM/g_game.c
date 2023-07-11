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
// DESCRIPTION:  none
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "i_random.h"

#include "doomdef.h"
#include "doomstat.h"

#include "z_zone.h"
#include "f_finale.h"
#include "m_misc.h"
#include "m_menu.h"
#include "i_system.h"
#include "r_main.h"

#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"

#include "d_main.h"

#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"

#include "p_local.h"

#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

// SKY handling - still the wrong place.
#include "r_data.h"
#include "r_sky.h"

#include "g_game.h"

#include "s_sound.h"

#include "options.h"

//#include "i_log.h"

#define SAVEGAMESIZE 0x2c000
#define SAVESTRINGSIZE 24

void G_CheckDemoStatus(void);
void G_ReadDemoTiccmd(ticcmd_t *cmd);
void G_WriteDemoTiccmd(ticcmd_t *cmd);
void G_PlayerReborn();
void G_InitNew(skill_t skill, int episode, int map);

void G_DoLoadLevel(void);
void G_DoNewGame(void);
void G_DoLoadGame(void);
void G_DoPlayDemo(void);
void G_DoCompleted(void);
void G_DoVictory(void);
void G_DoWorldDone(void);
void G_DoSaveGame(void);

byte gameaction;
gamestate_t gamestate;
skill_t gameskill;
boolean respawnmonsters;
int gameepisode;
int gamemap;

byte paused = 0;
byte sendpause; // send a pause event next tic
byte sendsave;  // send a save event next tic
byte usergame;  // ok to save / end game

byte timingdemo = 0; // if true, exit with report on completion
int starttime;       // for comparative timing purposes

byte viewactive;

boolean playeringame;
player_t players;
mobj_t *players_mo;

int gametic;
int totalkills, totalitems, totalsecret; // for intermission

char demoname[32];
byte demorecording = 0;
byte demoplayback = 0;
byte *demobuffer;
byte *demo_p;
byte *demoend;
byte singledemo = 0; // quit after playing a demo from cmdline

wbstartstruct_t wminfo; // parms for world map / intermission

//
// controls (have defaults)
//
int key_right;
int key_left;

int key_up;
int key_down;
int key_strafeleft;
int key_straferight;
int key_fire;
int key_use;
int key_strafe;
int key_speed;

int mousebfire;
int mousebstrafe;
int mousebforward;

#define MAXPLMOVE (forwardmove[1])

#define TURBOTHRESHOLD 0x32

fixed_t forwardmove[2] = {0x19, 0x32};
fixed_t sidemove[2] = {0x18, 0x28};
fixed_t angleturn[3] = {640, 1280, 320}; // + slow turn

#define SLOWTURNTICS 6

#define NUMKEYS 256

byte gamekeydown[NUMKEYS];
int turnheld; // for accelerative turning

byte mousearray[5];
byte *mousebuttons = &mousearray[1]; // allow [-1]

// mouse values are used once
int mousex;

int dclicktime2;
int dclickstate2;
int dclicks2;

int savegameslot;
char savedescription[32];

int autorun;

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
void G_BuildTiccmd(ticcmd_t *cmd)
{
    int i;
    byte strafe;
    byte bstrafe;
    int speed;
    int tspeed;
    int forward;
    int side;

    SetBytes(cmd, 0, sizeof(*cmd));

    strafe = gamekeydown[key_strafe] || mousebuttons[mousebstrafe];
    speed = !autorun ^ !gamekeydown[key_speed];

    forward = 0;

    if (gamekeydown[key_up] || mousebuttons[mousebforward])
    {
        forward += forwardmove[speed];
    }
    if (gamekeydown[key_down])
    {
        forward -= forwardmove[speed];
    }

    if (forward > MAXPLMOVE)
        forward = MAXPLMOVE;
    else if (forward < -MAXPLMOVE)
        forward = -MAXPLMOVE;

    cmd->forwardmove += forward;

    // use two stage accelerative turning
    // on the keyboard
    if (gamekeydown[key_right] || gamekeydown[key_left])
    {
        turnheld += 1;

        if (turnheld < SLOWTURNTICS)
            tspeed = 2; // slow turn
        else
            tspeed = speed;
    }
    else
    {
        turnheld = 0;
        tspeed = 2; // slow turn
    }

    side = 0;

    // let movement keys cancel each other out
    if (strafe)
    {
        if (gamekeydown[key_right])
        {
            side += sidemove[speed];
        }
        if (gamekeydown[key_left])
        {
            side -= sidemove[speed];
        }
    }
    else
    {
        if (gamekeydown[key_right])
            cmd->angleturn -= angleturn[tspeed];
        if (gamekeydown[key_left])
            cmd->angleturn += angleturn[tspeed];
    }

    if (gamekeydown[key_straferight])
        side += sidemove[speed];
    if (gamekeydown[key_strafeleft])
        side -= sidemove[speed];

    // strafe double click
    bstrafe = mousebuttons[mousebstrafe];
    if (bstrafe != dclickstate2 && dclicktime2 > 1)
    {
        dclickstate2 = bstrafe;
        dclicks2 += dclickstate2 != 0;

        if (dclicks2 == 2)
        {
            cmd->buttons |= BT_USE;
            dclicks2 = 0;
        }
        else
            dclicktime2 = 0;
    }
    else
    {
        dclicktime2 += 1;
        if (dclicktime2 > 20)
        {
            dclicks2 = 0;
            dclickstate2 = 0;
        }
    }

    if (strafe)
        side += mousex * 2;
    else
        cmd->angleturn -= mousex * 0x8;

    mousex = 0;

    if (side > MAXPLMOVE)
        side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
        side = -MAXPLMOVE;

    cmd->sidemove += side;

    // buttons

    if (gamekeydown[key_fire] || mousebuttons[mousebfire])
        cmd->buttons |= BT_ATTACK;

    if (gamekeydown[key_use])
    {
        cmd->buttons |= BT_USE;
    }

    // chainsaw overrides
    for (i = 0; i < NUMWEAPONS - 1; i++)
        if (gamekeydown['1' + i])
        {
            cmd->buttons |= BT_CHANGE;
            cmd->buttons |= i << BT_WEAPONSHIFT;
            break;
        }

    // special buttons
    if (sendpause)
    {
        sendpause = 0;
        cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

    if (sendsave)
    {
        sendsave = 0;
        cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot << BTS_SAVESHIFT);
    }
}

//
// G_DoLoadLevel
//
extern gamestate_t wipegamestate;

void G_DoLoadLevel(void)
{
    // DOOM determines the sky texture to be used
    // depending on the current episode, and the game version.
    if (gamemode == commercial)
    {
        skytexture = R_TextureNumForName("SKY3");
        if (gamemap < 12)
            skytexture = R_TextureNumForName("SKY1");
        else if (gamemap < 21)
            skytexture = R_TextureNumForName("SKY2");
    }

    if (wipegamestate == GS_LEVEL)
        wipegamestate = -1; // force a wipe

    gamestate = GS_LEVEL;

    if (players.playerstate == PST_DEAD)
        players.playerstate = PST_REBORN;

    P_SetupLevel(gameepisode, gamemap, 0, gameskill);
    starttime = ticcount;
    gameaction = ga_nothing;

    // clear cmd building stuff
    memset(gamekeydown, 0, sizeof(gamekeydown));
    mousex = 0;
    sendpause = sendsave = paused = 0;
    memset(mousebuttons, 0, sizeof(mousebuttons));

    if (gamemap == 8 && gamemission == doom)
    {
        // BOSSES
        snd_clipping = S_CLIPPING_DIST_BOSS;
    }
    else
    {
        snd_clipping = S_CLIPPING_DIST;
    }
}

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
void G_Responder(void)
{
    // any other key pops up menu if in demos
    if (gameaction == ga_nothing && !singledemo && (demoplayback || gamestate == GS_DEMOSCREEN))
    {
        if (current_ev->type == ev_keydown || (current_ev->type == ev_mouse && current_ev->data1))
        {
            M_StartControlPanel();
            return;
        }
        return;
    }

    if (gamestate == GS_LEVEL)
    {
        ST_Responder(current_ev); // status window ate it
#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
        if (AM_Responder())
            return; // automap ate it
#endif
    }

    if (gamestate == GS_FINALE)
    {
        if (F_Responder())
            return; // finale ate the event
    }

    switch (current_ev->type)
    {
    case ev_keydown:
        if (current_ev->data1 == KEY_PAUSE)
        {
            sendpause = 1;
            return;
        }
        if (current_ev->data1 < NUMKEYS)
            gamekeydown[current_ev->data1] = 1;
        return; // eat key down events

    case ev_keyup:
        if (current_ev->data1 < NUMKEYS)
            gamekeydown[current_ev->data1] = 0;
        return; // always let key up events filter down

    case ev_mouse:
        mousebuttons[0] = current_ev->data1 & 1;
        mousebuttons[1] = current_ev->data1 & 2;
        mousebuttons[2] = current_ev->data1 & 4;
        mousex = Div10(current_ev->data2 * (mouseSensitivity + 5));
        return; // eat events

    default:
        break;
    }

    return;
}

//
// G_DoReborn
//
void G_DoReborn(int playernum)
{
    // reload the level from scratch
    gameaction = ga_loadlevel;
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker(void)
{
    int i;
    int buf;
    ticcmd_t *cmd;

    // do player reborns if needed
    if (players.playerstate == PST_REBORN)
        G_DoReborn(0);

    // do things to change the game state
    while (gameaction != ga_nothing)
    {
        switch (gameaction)
        {
        case ga_loadlevel:
            G_DoLoadLevel();
            break;
        case ga_newgame:
            G_DoNewGame();
            break;
        case ga_loadgame:
            G_DoLoadGame();
            break;
        case ga_savegame:
            G_DoSaveGame();
            break;
        case ga_playdemo:
            G_DoPlayDemo();
            break;
        case ga_completed:
            G_DoCompleted();
            break;
        case ga_victory:
            F_StartFinale();
            break;
        case ga_worlddone:
            G_DoWorldDone();
            break;
        case ga_nothing:
            break;
        }
    }

    // get commands, check consistancy,
    // and build new consistancy check
    buf = (gametic) & (BACKUPTICS - 1);

    cmd = &players.cmd;

    CopyBytes(&localcmds[buf], cmd, sizeof(ticcmd_t));
    // memcpy(cmd, &localcmds[buf], sizeof(ticcmd_t));

    if (demoplayback)
        G_ReadDemoTiccmd(cmd);
    if (demorecording)
        G_WriteDemoTiccmd(cmd);

    // check for special buttons
    if (players.cmd.buttons & BT_SPECIAL)
    {
        switch (players.cmd.buttons & BT_SPECIALMASK)
        {
        case BTS_PAUSE:
            paused ^= 1;
            if (paused)
                S_PauseMusic();
            else
                S_ResumeMusic();
            break;

        case BTS_SAVEGAME:
            savegameslot = (players.cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT;
            gameaction = ga_savegame;
            break;
        }
    }

    // do main actions
    switch (gamestate)
    {
    case GS_LEVEL:
        P_Ticker();
        ST_Ticker();
#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
        AM_Ticker();
#endif
        HU_Ticker();
        break;

    case GS_INTERMISSION:
        WI_Ticker();
        break;

    case GS_FINALE:
        F_Ticker();
        break;

    case GS_DEMOSCREEN:
        D_PageTicker();
        break;
    }
}

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel()
{
    player_t *p;

    p = &players;

    memset(p->powers, 0, sizeof(p->powers));
    memset(p->cards, 0, sizeof(p->cards));
    p->mo->flags &= ~MF_SHADOW; // cancel invisibility
    p->extralight = 0;          // cancel gun flashes
    p->fixedcolormap = 0;       // cancel ir gogles
    p->damagecount = 0;         // no palette changes
    p->bonuscount = 0;
}

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn()
{
    player_t *p;
    int i;
    int killcount;
    int itemcount;
    int secretcount;

    killcount = players.killcount;
    itemcount = players.itemcount;
    secretcount = players.secretcount;

    p = &players;
    memset(p, 0, sizeof(*p));

    players.killcount = killcount;
    players.itemcount = itemcount;
    players.secretcount = secretcount;

    p->usedown = p->attackdown = true; // don't do anything immediately
    p->playerstate = PST_LIVE;
    p->health = MAXHEALTH;
    p->readyweapon = p->pendingweapon = wp_pistol;
    p->weaponowned[wp_fist] = true;
    p->weaponowned[wp_pistol] = true;
    p->ammo[am_clip] = 50;

    p->maxammo[0] = maxammo[0];
    p->maxammo[1] = maxammo[1];
    p->maxammo[2] = maxammo[2];
    p->maxammo[3] = maxammo[3];
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
void P_SpawnPlayer(mapthing_t *mthing);

// DOOM Par Times
short pars[4][10] =
    {
        {0},
        {0, 30, 75, 120, 90, 165, 180, 180, 30, 165},
        {0, 90, 90, 90, 120, 90, 360, 240, 30, 170},
        {0, 90, 45, 90, 150, 90, 90, 165, 30, 135}};

// DOOM II Par Times
short cpars[32] =
    {
        30, 90, 120, 120, 90, 150, 120, 120, 270, 90,     //  1-10
        210, 150, 150, 150, 210, 150, 420, 150, 210, 150, // 11-20
        240, 150, 180, 150, 150, 300, 330, 420, 300, 180, // 21-30
        120, 30                                           // 31-32
};

//
// G_DoCompleted
//
byte secretexit;
extern char *pagename;

void G_ExitLevel(void)
{
    secretexit = 0;
    gameaction = ga_completed;
}

// Here's for the german edition.
void G_SecretExitLevel(void)
{
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ((gamemode == commercial) && (W_GetNumForName("MAP31") < 0))
        secretexit = 0;
    else
        secretexit = 1;
    gameaction = ga_completed;
}

void G_DoCompleted(void)
{
    int i;

    gameaction = ga_nothing;

    G_PlayerFinishLevel(); // take away cards and stuff

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    if (automapactive)
        AM_Stop();
#endif

    if (gamemode != commercial)
        switch (gamemap)
        {
        case 8:
            gameaction = ga_victory;
            return;
        case 9:
            players.didsecret = true;
            break;
        }

    wminfo.didsecret = players.didsecret;
    wminfo.epsd = gameepisode - 1;
    wminfo.last = gamemap - 1;

    // wminfo.next is 0 biased, unlike gamemap
    if (gamemode == commercial)
    {
        if (secretexit)
            switch (gamemap)
            {
            case 15:
                wminfo.next = 30;
                break;
            case 31:
                wminfo.next = 31;
                break;
            }
        else
            switch (gamemap)
            {
            case 31:
            case 32:
                wminfo.next = 15;
                break;
            default:
                wminfo.next = gamemap;
            }
    }
    else
    {
        if (secretexit)
            wminfo.next = 8; // go to secret level
        else if (gamemap == 9)
        {
            // returning from secret level
            switch (gameepisode)
            {
            case 1:
                wminfo.next = 3;
                break;
            case 2:
                wminfo.next = 5;
                break;
            case 3:
                wminfo.next = 6;
                break;
            case 4:
                wminfo.next = 2;
                break;
            }
        }
        else
            wminfo.next = gamemap; // go to next level
    }

    wminfo.maxkills = totalkills;
    wminfo.maxitems = totalitems;
    wminfo.maxsecret = totalsecret;
    if (gamemode == commercial)
        wminfo.partime = Mul35(cpars[gamemap - 1]);
    else
        wminfo.partime = Mul35(pars[gameepisode][gamemap]);

    wminfo.plyr.in = true;
    wminfo.plyr.skills = players.killcount;
    wminfo.plyr.sitems = players.itemcount;
    wminfo.plyr.ssecret = players.secretcount;
    wminfo.plyr.stime = leveltime;

    gamestate = GS_INTERMISSION;
    viewactive = 0;
#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    automapactive = 0;
#endif

    WI_Start(&wminfo);
}

//
// G_WorldDone
//
void G_WorldDone(void)
{
    gameaction = ga_worlddone;

    if (secretexit)
        players.didsecret = true;

    if (gamemode == commercial)
    {
        switch (gamemap)
        {
        case 15:
        case 31:
            if (!secretexit)
                break;
        case 6:
        case 11:
        case 20:
        case 30:
            F_StartFinale();
            break;
        }
    }
}

void G_DoWorldDone(void)
{
    gamestate = GS_LEVEL;
    gamemap = wminfo.next + 1;
    G_DoLoadLevel();
    gameaction = ga_nothing;
    viewactive = 1;
}

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
extern byte setsizeneeded;

char savename[256];

void G_LoadGame(char *name)
{
    strcpy(savename, name);
    gameaction = ga_loadgame;
}

#define VERSIONSIZE 16

void G_DoLoadGame(void)
{
    int length;
    int i;
    int a, b, c;
    char vcheck[VERSIONSIZE];
    byte *savebuffer;

    gameaction = ga_nothing;

    length = M_ReadFile(savename, &savebuffer);
    save_p = savebuffer + SAVESTRINGSIZE;

    // skip the description field
    memset(vcheck, 0, sizeof(vcheck));
    sprintf(vcheck, "version %i", VERSION);
    if (strcmp((char *)save_p, vcheck))
        return; // bad version
    save_p += VERSIONSIZE;

    gameskill = *save_p++;
    gameepisode = *save_p++;
    gamemap = *save_p++;
    playeringame = *save_p++;
    *save_p++;
    *save_p++;
    *save_p++;

    // load a base level
    G_InitNew(gameskill, gameepisode, gamemap);

    // get the times
    a = *save_p++;
    b = *save_p++;
    c = *save_p++;
    leveltime = (a << 16) + (b << 8) + c;

    // dearchive all the modifications
    P_UnArchivePlayers();
    P_UnArchiveWorld();
    P_UnArchiveThinkers();
    P_UnArchiveSpecials();

    if (*save_p != 0x1d)
        I_Error("Bad savegame");

    // done
    Z_Free(savebuffer);

    if (setsizeneeded)
        R_ExecuteSetViewSize();

// draw the pattern into the back screen
#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    R_FillBackScreen();
#endif
}

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void G_SaveGame(int slot,
                char *description)
{
    savegameslot = slot;
    strcpy(savedescription, description);
    sendsave = 1;
}

void G_DoSaveGame(void)
{
    char name[100];
    char name2[VERSIONSIZE];
    char *description;
    int length;
    int i;
    byte *savebuffer;

    sprintf(name, SAVEGAMENAME "%d.dsg", savegameslot);
    description = savedescription;

    savebuffer = (byte *)Z_MallocUnowned(SAVEGAMESIZE, PU_STATIC);

    save_p = savebuffer;

    CopyBytes(description, save_p, SAVESTRINGSIZE);
    // memcpy(save_p, description, SAVESTRINGSIZE);
    save_p += SAVESTRINGSIZE;
    SetBytes(name2, 0, sizeof(name2));
    sprintf(name2, "version %i", VERSION);
    CopyBytes(name2, save_p, VERSIONSIZE);
    // memcpy(save_p, name2, VERSIONSIZE);
    save_p += VERSIONSIZE;

    *save_p++ = gameskill;
    *save_p++ = gameepisode;
    *save_p++ = gamemap;
    *save_p++ = true;
    *save_p++ = false;
    *save_p++ = false;
    *save_p++ = false;
    *save_p++ = leveltime >> 16;
    *save_p++ = leveltime >> 8;
    *save_p++ = leveltime;

    P_ArchivePlayers();
    P_ArchiveWorld();
    P_ArchiveThinkers();
    P_ArchiveSpecials();

    *save_p++ = 0x1d; // consistancy marker

    length = save_p - savebuffer;
    if (length > SAVEGAMESIZE)
        I_Error("Savegame buffer overrun");
    M_WriteFile(name, savebuffer, length);
    gameaction = ga_nothing;
    savedescription[0] = 0;

    players.message = GGSAVED;

    Z_Free(savebuffer);

// draw the pattern into the back screen
#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    R_FillBackScreen();
#endif
}

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, playeringame[] should be set.
//
skill_t d_skill;
int d_episode;
int d_map;

void G_DeferedInitNew(skill_t skill,
                      int episode,
                      int map)
{
    d_skill = skill;
    d_episode = episode;
    d_map = map;
    gameaction = ga_newgame;
}

void G_DoNewGame(void)
{
    demoplayback = 0;
    respawnparm = false;
    fastparm = false;
    // nomonsters = false;
    G_InitNew(d_skill, d_episode, d_map);
    gameaction = ga_nothing;
}

// The sky texture to be used instead of the F_SKY1 dummy.
extern short skytexture;

void G_InitNew(skill_t skill,
               int episode,
               int map)
{
    int i;

    if (paused)
    {
        paused = 0;
        S_ResumeMusic();
    }

    if (skill > sk_nightmare)
        skill = sk_nightmare;

    if (episode < 1)
        episode = 1;

    if (gamemode == retail)
    {
        if (episode > 4)
            episode = 4;
    }
    else if (gamemode == shareware)
    {
        if (episode > 1)
            episode = 1; // only start episode 1 on shareware
    }
    else
    {
        if (episode > 3)
            episode = 3;
    }

    if (map < 1)
        map = 1;

    if ((map > 9) && (gamemode != commercial))
        map = 9;

    prndindex = 0;

    if (skill == sk_nightmare || respawnparm)
        respawnmonsters = true;
    else
        respawnmonsters = false;

    if (fastparm || (skill == sk_nightmare && gameskill != sk_nightmare))
    {
        for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
            states[i].tics >>= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 20 * FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 20 * FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 20 * FRACUNIT;
    }
    else if (skill != sk_nightmare && gameskill == sk_nightmare)
    {
        for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
            states[i].tics <<= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 15 * FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 10 * FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 10 * FRACUNIT;
    }

    // force players to be initialized upon first level load
    players.playerstate = PST_REBORN;

    usergame = 1; // will be set false if a demo
    paused = 0;
    demoplayback = 0;
#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
    automapactive = 0;
#endif
    viewactive = 1;
    gameepisode = episode;
    gamemap = map;
    gameskill = skill;

    viewactive = 1;

    // set the sky map for the episode
    if (gamemode == commercial)
    {
        skytexture = R_TextureNumForName("SKY3");
        if (gamemap < 12)
            skytexture = R_TextureNumForName("SKY1");
        else if (gamemap < 21)
            skytexture = R_TextureNumForName("SKY2");
    }
    else
        switch (episode)
        {
        case 1:
            skytexture = R_TextureNumForName("SKY1");
            break;
        case 2:
            skytexture = R_TextureNumForName("SKY2");
            break;
        case 3:
            skytexture = R_TextureNumForName("SKY3");
            break;
        case 4: // Special Edition sky
            skytexture = R_TextureNumForName("SKY4");
            break;
        }

    G_DoLoadLevel();
}

//
// DEMO RECORDING
//
#define DEMOMARKER 0x80

void G_ReadDemoTiccmd(ticcmd_t *cmd)
{
    if (*demo_p == DEMOMARKER)
    {
        // end of demo data stream
        G_CheckDemoStatus();
        return;
    }
    cmd->forwardmove = ((signed char)*demo_p++);
    cmd->sidemove = ((signed char)*demo_p++);
    cmd->angleturn = ((unsigned char)*demo_p++) << 8;
    cmd->buttons = (unsigned char)*demo_p++;
}

void G_WriteDemoTiccmd(ticcmd_t *cmd)
{
    if (gamekeydown['q']) // press q to end demo recording
        G_CheckDemoStatus();
    *demo_p++ = cmd->forwardmove;
    *demo_p++ = cmd->sidemove;
    *demo_p++ = (cmd->angleturn + 128) >> 8;
    *demo_p++ = cmd->buttons;
    demo_p -= 4;
    if (demo_p > demoend - 16)
    {
        // no more space
        G_CheckDemoStatus();
        return;
    }

    G_ReadDemoTiccmd(cmd); // make SURE it is exactly the same
}

//
// G_RecordDemo
//
void G_RecordDemo(char *name)
{
    int i;
    int maxsize;

    usergame = 0;
    strcpy(demoname, name);
    strcat(demoname, ".lmp");
    maxsize = 0x20000;
    i = M_CheckParm("-maxdemo");
    if (i && i < myargc - 1)
        maxsize = atoi(myargv[i + 1]) * 1024;
    demobuffer = Z_MallocUnowned(maxsize, PU_STATIC);
    demoend = demobuffer + maxsize;

    demorecording = 1;
}

void G_BeginRecording(void)
{
    int i;

    demo_p = demobuffer;

    *demo_p++ = VERSION;
    *demo_p++ = gameskill;
    *demo_p++ = gameepisode;
    *demo_p++ = gamemap;
    *demo_p++ = false;
    *demo_p++ = respawnparm;
    *demo_p++ = fastparm;
    *demo_p++ = nomonsters;
    *demo_p++ = 0;

    *demo_p++ = true;
    *demo_p++ = false;
    *demo_p++ = false;
    *demo_p++ = false;
}

//
// G_PlayDemo
//

char *defdemoname;

void G_DeferedPlayDemo(char *name)
{
    if (!disableDemo)
    {
        defdemoname = name;
        gameaction = ga_playdemo;
    }
}

void G_DoPlayDemo(void)
{
    skill_t skill;
    int i, episode, map;

    gameaction = ga_nothing;
    demobuffer = demo_p = W_CacheLumpName(defdemoname, PU_STATIC);
    if (*demo_p++ != VERSION)
    {
        I_Error("Demo is from a different game version!");
    }

    skill = *demo_p++;
    episode = *demo_p++;
    map = *demo_p++;
    *demo_p++;
    respawnparm = *demo_p++;
    fastparm = *demo_p++;
    nomonsters = *demo_p++;
    *demo_p++;

    playeringame = *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;

    // don't spend a lot of time in loadlevel
    G_InitNew(skill, episode, map);

    usergame = 0;
    demoplayback = 1;
}

//
// G_TimeDemo
//
void G_TimeDemo(char *name)
{
    timingdemo = 1;
    singletics = true;

    defdemoname = name;
    gameaction = ga_playdemo;
}

/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

#define CSV_COLUMN ","
#define CSV_DECIMAL "."
#define CSV_FILE "BENCH.CSV"

void G_CreateCSV(void)
{
    FILE *fptr;
    fptr = fopen(CSV_FILE, "r");
    if (fptr == NULL) // if file does not exist, create it
    {
        fptr = fopen(CSV_FILE, "w+");
        fprintf(fptr, "executable" CSV_COLUMN "arch" CSV_COLUMN "detail" CSV_COLUMN "size" CSV_COLUMN "visplanes" CSV_COLUMN "sky" CSV_COLUMN "objects" CSV_COLUMN "transparent_columns" CSV_COLUMN "iwad" CSV_COLUMN "demo" CSV_COLUMN "gametics" CSV_COLUMN "realtics" CSV_COLUMN "fps" CSV_COLUMN "onepercentlow" CSV_COLUMN "dotonepercentlow\n");
        fclose(fptr);
    }
    fclose(fptr);
}

void G_SaveCSVResult(unsigned int gametics, unsigned int realtics, unsigned int resultfps, unsigned int onepercentlow, unsigned int dotonepercentlow)
{
    FILE *logFile = fopen(CSV_FILE, "a");
    if (logFile)
    {
        // Executable
        fprintf(logFile, "%s" CSV_COLUMN, myargv[0]);

        // Architecture
        switch (selectedCPU)
        {
        case INTEL_386SX:
            fprintf(logFile, "386sx");
            break;
        case INTEL_386DX:
            fprintf(logFile, "386dx");
            break;
        case INTEL_486:
            fprintf(logFile, "intel486");
            break;
        case INTEL_PENTIUM:
            fprintf(logFile, "pentium");
            break;
        case CYRIX_386DLC:
            fprintf(logFile, "cyrix386");
            break;
        case CYRIX_486:
            fprintf(logFile, "cyrix486");
            break;
        case CYRIX_5X86:
            fprintf(logFile, "cyrix5x86");
            break;
        case UMC_GREEN_486:
            fprintf(logFile, "umc486");
            break;
        case AMD_K5:
            fprintf(logFile, "k5");
            break;
        }

        fprintf(logFile, CSV_COLUMN);

        // Detail
        switch (detailshift)
        {
        case DETAIL_HIGH:
            fprintf(logFile, "high");
            break;
        case DETAIL_LOW:
            fprintf(logFile, "low");
            break;
        case DETAIL_POTATO:
            fprintf(logFile, "potato");
            break;
        }

        fprintf(logFile, CSV_COLUMN);

        // Screen size
        fprintf(logFile, "%i" CSV_COLUMN, screenblocks);

        // Visplanes
        switch (visplaneRender)
        {
        case VISPLANES_NORMAL:
            fprintf(logFile, "normal");
            break;
        case VISPLANES_FLAT:
            fprintf(logFile, "flat");
            break;
        case VISPLANES_FLATTER:
            fprintf(logFile, "flatter");
            break;
        }

        fprintf(logFile, CSV_COLUMN);

        // Sky
        if (flatSky)
            fprintf(logFile, "flat");
        else
            fprintf(logFile, "normal");

        fprintf(logFile, CSV_COLUMN);

        // Objects
        if (nearSprites)
            fprintf(logFile, "near");
        else
            fprintf(logFile, "normal");

        fprintf(logFile, CSV_COLUMN);

        // Transparent objects
        switch (invisibleRender)
        {
        case INVISIBLE_NORMAL:
            fprintf(logFile, "normal");
            break;
        case INVISIBLE_FLAT:
            fprintf(logFile, "flat");
            break;
        case INVISIBLE_FLAT_SATURN:
            fprintf(logFile, "flatsaturn");
            break;
        case INVISIBLE_SATURN:
            fprintf(logFile, "saturn");
            break;
        case INVISIBLE_TRANSLUCENT:
            fprintf(logFile, "translucent");
            break;
        }

        fprintf(logFile, CSV_COLUMN);

        // IWAD
        fprintf(logFile, "%s" CSV_COLUMN, iwadfile);

        // Demo
        fprintf(logFile, "%s" CSV_COLUMN, demofile);

        // Gametics, Realtics, FPS
        fprintf(logFile, "%i" CSV_COLUMN "%u" CSV_COLUMN "%u" CSV_DECIMAL "%.3u" CSV_COLUMN, gametics, realtics, resultfps / 1000, resultfps % 1000);

        // 1% low FPS
        fprintf(logFile, "%u" CSV_DECIMAL "%.3u" CSV_COLUMN, onepercentlow / 1000, onepercentlow % 1000);
        
        // 0.1% low FPS
        fprintf(logFile, "%u" CSV_DECIMAL "%.3u\n", dotonepercentlow / 1000, dotonepercentlow % 1000);

        fclose(logFile);
    }
}

#define FRAMETIME_FILE "FTIME.CSV"

void G_CreateFrametime(void)
{
    FILE *fptr;
    fptr = fopen(FRAMETIME_FILE, "r");
    if (fptr == NULL) // if file does not exist, create it
    {
        fptr = fopen(FRAMETIME_FILE, "w+");
        fprintf(fptr, "frame" CSV_COLUMN "milliseconds\n");
        fclose(fptr);
    }
    fclose(fptr);
}

void G_SaveFrametimeResult(unsigned int start, unsigned int count)
{
    FILE *logFile = fopen(FRAMETIME_FILE, "a");

    if (logFile)
    {
        unsigned int counter = 0;
        unsigned int i;

        for (i = start; i < count; i++)
        {
            fprintf(logFile, "%u" CSV_COLUMN "%u\n", counter, frametime[i]);
            counter++;
        }

        fclose(logFile);
    }
}

void G_CheckDemoStatus(void)
{
    unsigned int realtics;
    unsigned int resultfps;
    unsigned int gametics;

    if (timingdemo)
    {
        if (benchmark)
        {
            benchmark_realtics = ticcount - starttime;
            benchmark_gametics = gametic - benchmark_starttic;
            benchmark_resultfps = (35 * 1000 * (unsigned int)benchmark_gametics) / (unsigned int)benchmark_realtics;
            gametics = benchmark_gametics;
            realtics = benchmark_realtics;
            resultfps = benchmark_resultfps;
        }
        else
        {
            gametics = gametic;
            realtics = ticcount - starttime;
            resultfps = (35 * 1000 * (unsigned int)gametic) / (unsigned int)realtics;
        }

        if (csv)
        {
            G_CreateCSV();

            if (benchmark_advanced)
            {
                unsigned int i, j;
                unsigned int temp;
                unsigned int onepercentlow_ms = 0;
                unsigned int onepercentlow_fps = 0;
                unsigned int onepercentlow_num = 0;

                unsigned int dotonepercentlow_ms = 0;
                unsigned int dotonepercentlow_fps = 0;
                unsigned int dotonepercentlow_num = 0;

                unsigned int fix_start = 0;

                fix_start = frametime_position - benchmark_gametics + 1;

                G_CreateFrametime();
                G_SaveFrametimeResult(fix_start - 1, frametime_position);

                // Sort array (higher values are worse)
                for (i = 0; i < frametime_position; i++)
                {
                    for (j = i + 1; j < frametime_position; j++)
                    {
                        if (frametime[i] < frametime[j])
                        {
                            temp = frametime[i];
                            frametime[i] = frametime[j];
                            frametime[j] = temp;
                        }
                    }
                }

                // Calculate 1% low frametimes

                onepercentlow_num = frametime_position / 100; // 1% Low

                if (onepercentlow_num == 0)
                    onepercentlow_num++;

                for (i = fix_start; i < onepercentlow_num + fix_start; i++) // Omit first frame (load data)
                {
                    onepercentlow_ms += frametime[i];
                }

                onepercentlow_ms *= 1000;
                onepercentlow_ms /= onepercentlow_num; // Average ms 1% low
                onepercentlow_fps = 1000000000u / onepercentlow_ms;

                // Calculate 0.1% low frametimes
                dotonepercentlow_num = frametime_position / 1000; // 0.1% Low

                if (dotonepercentlow_num == 0)
                    dotonepercentlow_num++;

                for (i = fix_start; i < dotonepercentlow_num + fix_start; i++) // Omit first frame (load data)
                {
                    dotonepercentlow_ms += frametime[i];
                }

                dotonepercentlow_ms *= 1000;
                dotonepercentlow_ms /= dotonepercentlow_num; // Average ms 0.1% low
                dotonepercentlow_fps = 1000000000u / dotonepercentlow_ms;

                G_SaveCSVResult(gametics, realtics, resultfps, onepercentlow_fps, dotonepercentlow_fps);

                // Cleanup frametimes
                frametime_position = 0;
                
                for (i = 0; i < 20000; i++)
                {
                    frametime[i] = 0;
                }
            }
            else
            {
                G_SaveCSVResult(gametics, realtics, resultfps, 0, 0);
            }
        }

        if (benchmark)
        {
            timingdemo = 0;
            singletics = false;

            benchmark_finished = true;
        }
        else
        {
            I_Error("Timed %u gametics in %u realtics. FPS: %u.%.3u", gametics, realtics, resultfps / 1000, resultfps % 1000);
        }
    }

    if (demoplayback)
    {
        if (singledemo)
            I_Quit();

        Z_ChangeTag(demobuffer, PU_CACHE);
        demoplayback = 0;
        respawnparm = false;
        fastparm = false;
        nomonsters = false;
        D_AdvanceDemo();
        return;
    }

    if (demorecording)
    {
        *demo_p++ = DEMOMARKER;
        M_WriteFile(demoname, demobuffer, demo_p - demobuffer);
        Z_Free(demobuffer);
        demorecording = 0;
        I_Error("Demo %s recorded", demoname);
    }

    return;
}
