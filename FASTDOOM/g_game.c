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

#include "g_game.h"

#include "s_sound.h"

#include "options.h"

#include "i_file.h"
#include "i_debug.h"

#define SAVEGAMESIZE 0x2c000
#define SAVESTRINGSIZE 24
#define DEMOMARKER 0x80
#define VERSION 109

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

int key_weaponprev;
int key_weaponnext;

/* action bound to each mouse button */
int mouseactions[3];

#define MAXPLMOVE (0x32 << 11)

#define TURBOTHRESHOLD 0x32

fixed_t forwardmove[2] = {0x19 << 11, 0x32 << 11};
fixed_t sidemove[2] = {0x18 << 11, 0x28 << 11};
fixed_t angleturn[3] = {640 << 16, 1280 << 16, 320 << 16}; // + slow turn

#define SLOWTURNTICS 6

#define NUMKEYS 256

byte gamekeydown[NUMKEYS];
int turnheld; // for accelerative turning

#define NUMMOUSEBUTTONS 3

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

char levelsfile[21] = "LEVELS\\";

// the order in which the weapons are cycled through
static const int weaponcycle[NUMWEAPONS] =
{
    wp_fist,
    wp_chainsaw,
    wp_pistol,
    wp_shotgun,
    wp_supershotgun,
    wp_chaingun,
    wp_missile,
    wp_plasma,
    wp_bfg
};

static const int weaponpos[NUMWEAPONS] =
{
    0, 2, 3, 5, 6, 7, 8, 1, 4
};

static int G_CycleWeapon(boolean forward)
{
    int i;
    int start;
    int pos;
    int step;
    int weapon;
    byte saw;
    byte nocomm;
    byte share;

    if (players.pendingweapon != wp_nochange)
    {
        start = players.pendingweapon;
    }
    else
    {
        start = players.readyweapon;
    }
    pos = weaponpos[start];

    saw = players.weaponowned[wp_chainsaw];
    nocomm = gamemode != commercial;
    share = gamemode == shareware;

    step = forward ? 1 : -1;

    for (i = 0; i < NUMWEAPONS; i++)
    {
        // wrap around with branches instead of a modulo
        pos += step;
        if (pos < 0)
        {
            pos += NUMWEAPONS;
        }
        else if (pos == NUMWEAPONS)
        {
            pos = 0;
        }

        weapon = weaponcycle[pos];

        if (players.weaponowned[weapon]
            && (weapon != wp_supershotgun || !nocomm)
            && ((weapon != wp_plasma && weapon != wp_bfg) || !share))
        {
            if (weapon == wp_fist && saw)
            {
                weapon = wp_chainsaw;
            }

            if (weapon != start)
            {
                return weapon;
            }
        }
    }

    return -1;
}

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
    byte keyr;
    byte keyl;
    byte mnext;
    byte mprev;
    byte knext;
    byte kprev;
    int nextweapon = -1;
    static byte prevmousestate[NUMMOUSEBUTTONS];
    static byte keynextstate;
    static byte keyprevstate;

    cmd->angleturn=0;
    cmd->buttons=0;

    strafe = gamekeydown[key_strafe];
    bstrafe = 0;
    speed = !autorun ^ !gamekeydown[key_speed];

    forward = 0;
    mnext = 0;
    mprev = 0;

    /* mouse buttons can trigger actions; mouse weapon switching
       is edge triggered */
    for (i = 0; i < NUMMOUSEBUTTONS; i++)
    {
        byte m;
        byte action;

        m = mousebuttons[i];
        action = mouseactions[i];

        if (m)
        {
            switch (action)
            {
            case MOUSE_ACT_FIRE:
                cmd->buttons |= BT_ATTACK;
                break;
            case MOUSE_ACT_FORWARD:
                forward += forwardmove[speed];
                break;
            case MOUSE_ACT_BACK:
                forward -= forwardmove[speed];
                break;
            case MOUSE_ACT_STRAFE:
                strafe = 1;
                bstrafe = 1;
                break;
            case MOUSE_ACT_USE:
                cmd->buttons |= BT_USE;
                break;
            case MOUSE_ACT_SPEED:
                speed = 1;
                break;
            default:
                break;
            }

            if (!prevmousestate[i])
            {
                if (action == MOUSE_ACT_WEAPONNEXT)
                    mnext = 1;
                else if (action == MOUSE_ACT_WEAPONPREV)
                    mprev = 1;
            }
        }

        prevmousestate[i] = m;
    }

    if (gamekeydown[key_up])
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

    cmd->forwardmove = forward;

    keyr = gamekeydown[key_right];
    keyl = gamekeydown[key_left];

    // use two stage accelerative turning
    // on the keyboard
    if (keyr || keyl)
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
        if (keyr)
        {
            side += sidemove[speed];
        }
        if (keyl)
        {
            side -= sidemove[speed];
        }
    }
    else
    {
        if (keyr)
            cmd->angleturn -= angleturn[tspeed];
        if (keyl)
            cmd->angleturn += angleturn[tspeed];
    }

    if (gamekeydown[key_straferight])
        side += sidemove[speed];
    if (gamekeydown[key_strafeleft])
        side -= sidemove[speed];

    // strafe double click
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
        side += mousex << 12;
    else
        cmd->angleturn -= mousex << 19;

    mousex = 0;

    if (side > MAXPLMOVE)
        side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
        side = -MAXPLMOVE;

    cmd->sidemove = side;

    // buttons

    if (gamekeydown[key_fire])
        cmd->buttons |= BT_ATTACK;

    if (gamekeydown[key_use])
    {
        cmd->buttons |= BT_USE;
    }

    // chainsaw overrides
    if (gamekeydown['1'])
        cmd->buttons |= BT_CHANGE;
    else if (gamekeydown['2'])
        cmd->buttons |= BT_CHANGE | (1 << BT_WEAPONSHIFT);
    else if (gamekeydown['3'])
        cmd->buttons |= BT_CHANGE | (2 << BT_WEAPONSHIFT);
    else if (gamekeydown['4'])
        cmd->buttons |= BT_CHANGE | (3 << BT_WEAPONSHIFT);
    else if (gamekeydown['5'])
        cmd->buttons |= BT_CHANGE | (4 << BT_WEAPONSHIFT);
    else if (gamekeydown['6'])
        cmd->buttons |= BT_CHANGE | (5 << BT_WEAPONSHIFT);
    else if (gamekeydown['7'])
        cmd->buttons |= BT_CHANGE | (6 << BT_WEAPONSHIFT);
    else if (gamekeydown['8'])
        cmd->buttons |= BT_CHANGE | (7 << BT_WEAPONSHIFT);

    // cycle weapons, keyboard edges take priority over mouse edges
    knext = gamekeydown[key_weaponnext];
    kprev = gamekeydown[key_weaponprev];

    if ((knext && !keynextstate) || mnext)
        nextweapon = G_CycleWeapon(true);
    else if ((kprev && !keyprevstate) || mprev)
        nextweapon = G_CycleWeapon(false);

    if (nextweapon != -1 && nextweapon != players.readyweapon)
        players.pendingweapon = nextweapon;

    keynextstate = knext;
    keyprevstate = kprev;

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
    int currentmapnum;

    if (gamemode == commercial)
    {
        currentmapnum = gamemap - 1;
    }else{
        currentmapnum = (gameepisode - 1) * 9 + gamemap - 1;
    }

    I_ReadTextLineFile(levelsfile, currentmapnum, currentlevelname, 40, 0);
    I_ReadTextLineFile(levelsfile, currentmapnum + 1, nextlevelname, 40, 0);


    // Reset interpolation state to values that will force no interpolation
    // on the first tick
    interpolation_weight = 0x10000;
    frametime_hrticks = 17;
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
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
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
        gamekeydown[current_ev->data1] = 1;
        return; // eat key down events

    case ev_keyup:
        gamekeydown[current_ev->data1] = 0;
        return; // always let key up events filter down

    case ev_mouse:
        mousebuttons[0] = current_ev->data1 & 1;
        mousebuttons[1] = current_ev->data1 & 2;
        mousebuttons[2] = current_ev->data1 & 4;
        mousex += FixedMulShortToInt(current_ev->data2, mouseSensitivityFP);
        return; // eat events
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

        // highResTimer is now set and I_SetHrTimerEnabled() called in TryRunTics()
        // when uncappedFPS is true and gamestate == GS_LEVEL  
    }

    // get commands, check consistancy,
    // and build new consistancy check
    cmd = &players.cmd;

    if (demoplayback) {
        G_ReadDemoTiccmd(cmd);
    } else {
        int buf;
        ticcmd_t *localcmd;

        buf = (gametic) & (BACKUPTICS - 1);
        localcmd = &localcmds[buf];
        cmd->forwardmove = localcmd->forwardmove;
        cmd->sidemove = localcmd->sidemove;
        cmd->angleturn = localcmd->angleturn;
        cmd->buttons = localcmd->buttons;
    }
        
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
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
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
const unsigned char pars[4][9] =
    {
        { 30/5, 75/5,120/5, 90/5,165/5,180/5,180/5, 30/5,165/5},
        { 90/5, 90/5, 90/5,120/5, 90/5,360/5,240/5, 30/5,170/5},
        { 90/5, 45/5, 90/5,150/5, 90/5, 90/5,165/5, 30/5,135/5},
        {165/5,255/5,135/5,150/5,180/5,390/5,135/5,360/5,180/5}};

// DOOM II Par Times
const unsigned char cpars[32] =
    {
         30/5, 90/5,120/5,120/5, 90/5,150/5,120/5,120/5,270/5, 90/5, //  1-10
        210/5,150/5,150/5,150/5,210/5,150/5,420/5,150/5,210/5,150/5, // 11-20
        240/5,150/5,180/5,150/5,150/5,300/5,330/5,420/5,300/5,180/5, // 21-30
        120/5, 30/5                                                  // 31-32
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

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
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
        wminfo.partime = Mul175(cpars[gamemap - 1]);
    else
        wminfo.partime = Mul175(pars[gameepisode - 1][gamemap - 1]);

    wminfo.plyr.in = true;
    wminfo.plyr.skills = players.killcount;
    wminfo.plyr.sitems = players.itemcount;
    wminfo.plyr.ssecret = players.secretcount;
    wminfo.plyr.stime = leveltime;

    gamestate = GS_INTERMISSION;
    viewactive = 0;
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
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
    singletics = false;
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
    *save_p++;
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
        I_Error(2);

    // done
    Z_Free(savebuffer);

    if (setsizeneeded)
        R_ExecuteSetViewSize();

// draw the pattern into the back screen
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
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

int G_CalculateSaveGameSize(void)
{
    thinker_t *th;
    int i;
    int savesize = 0;

    // G_DoSaveGame
    savesize += SAVESTRINGSIZE;
    savesize += VERSIONSIZE;
    savesize += 11;

    // P_ArchivePlayers
    savesize += 3; // PADSAVEP
    savesize += sizeof(player_t);

    // P_ArchiveWorld
    savesize += numsectors * 14;
    savesize += numlines * (6 + (2 * 10));

    // P_ArchiveThinkers
    for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
        if (th->function.acp1 == (actionf_p1)P_MobjThinker || th->function.acp1 == (actionf_p1)P_MobjBrainlessThinker || th->function.acp1 == (actionf_p1)P_MobjTicklessThinker)
		{
            savesize += 1;
            savesize += 3; // PADSAVEP
            savesize += sizeof(mobj_t);
		}
    }
    savesize += 1;

    // P_ArchiveSpecials
    for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
        if (th->function.acv == (actionf_v)NULL)
		{
			for (i = 0; i < MAXCEILINGS; i++)
				if (activeceilings[i] == (ceiling_t *)th)
					break;

			if (i < MAXCEILINGS)
			{
                savesize += 1;
                savesize += 3; // PADSAVEP
                savesize += sizeof(ceiling_t);
			}
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
		{
            savesize += 1;
            savesize += 3; // PADSAVEP
            savesize += sizeof(ceiling_t);
            continue;
		}

		if (th->function.acp1 == (actionf_p1)T_VerticalDoor)
		{
            savesize += 1;
            savesize += 3; // PADSAVEP
            savesize += sizeof(vldoor_t);
            continue;
		}

		if (th->function.acp1 == (actionf_p1)T_MoveFloor)
		{
            savesize += 1;
            savesize += 3; // PADSAVEP
            savesize += sizeof(floormove_t);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_PlatRaise)
		{
            savesize += 1;
            savesize += 3; // PADSAVEP
            savesize += sizeof(plat_t);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_LightFlash)
		{
            savesize += 1;
            savesize += 3; // PADSAVEP
            savesize += sizeof(lightflash_t);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
		{
            savesize += 1;
            savesize += 3; // PADSAVEP
            savesize += sizeof(strobe_t);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_Glow)
		{
            savesize += 1;
            savesize += 3; // PADSAVEP
            savesize += sizeof(glow_t);
			continue;
		}
    }

    savesize += 1;
    return savesize;
}

void G_DoSaveGame(void)
{
    char name[100];
    char name2[VERSIONSIZE];
    char *description;
    int length;
    int i;
    byte *savebuffer;

    int requiredmemsize;

    sprintf(name, savegamename, savegameslot);
    description = savedescription;

    requiredmemsize = G_CalculateSaveGameSize();

    savebuffer = (byte *)Z_MallocUnowned(requiredmemsize, PU_STATIC);

    save_p = savebuffer;

    CopyBytes(description, save_p, SAVESTRINGSIZE);
    save_p += SAVESTRINGSIZE;

    SetBytes(name2, 0, sizeof(name2));
    sprintf(name2, "version %i", VERSION);
    CopyBytes(name2, save_p, VERSIONSIZE);
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

    if (length > requiredmemsize)
        I_Error(3);

    M_WriteFile(name, savebuffer, length);
    gameaction = ga_nothing;
    savedescription[0] = 0;

    players.message = GGSAVED;

    Z_Free(savebuffer);

// draw the pattern into the back screen
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
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
    singletics = false;
    demoplayback = 0;
    respawnparm = false;
    fastparm = false;
    G_InitNew(d_skill, d_episode, d_map);
    gameaction = ga_nothing;
}

// The sky texture to be used instead of the F_SKY1 dummy.
extern short skytexture;

#define levelfilepos 7

void G_GetLevelsFileName(void)
{

    int length = strlen(iwadname);

    memcpy(levelsfile + levelfilepos, iwadname, length + 1);

    levelsfile[levelfilepos + length - 3] = 'T';
    levelsfile[levelfilepos + length - 2] = 'X';
    levelsfile[levelfilepos + length - 1] = 'T';
}


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
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
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

void G_ReadDemoTiccmd(ticcmd_t *cmd)
{
    if (*demo_p == DEMOMARKER)
    {
        // end of demo data stream
        G_CheckDemoStatus();
        return;
    }
    cmd->forwardmove = ((signed char)*demo_p++) << 11;
    cmd->sidemove = ((signed char)*demo_p++) << 11;
    cmd->angleturn = ((unsigned char)*demo_p++) << (8+16);
    cmd->buttons = (unsigned char)*demo_p++;
}

void G_WriteDemoTiccmd(ticcmd_t *cmd)
{
    if (gamekeydown['q']) // press q to end demo recording
        G_CheckDemoStatus();
    *demo_p++ = cmd->forwardmove >> 11;
    *demo_p++ = cmd->sidemove >> 11;
    *demo_p++ = (cmd->angleturn + 128) >> (8+16);
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
    *demo_p++ = 0;
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

unsigned int G_GetDemoTicks(char *demofile)
{
    unsigned int count;

    demobuffer = demo_p = W_CacheLumpName(demofile, PU_STATIC);

    // G_DoPlayDemo
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;
    *demo_p++;

    // Loop CMD ticks
    while(*demo_p != DEMOMARKER)
    {
        count++;
        *demo_p++;
        *demo_p++;
        *demo_p++;
        *demo_p++;
    }

    Z_ChangeTag(demobuffer, PU_CACHE);

    return count;
}

void G_DoPlayDemo(void)
{
    skill_t skill;
    int i, episode, map;

    gameaction = ga_nothing;
    demobuffer = demo_p = W_CacheLumpName(defdemoname, PU_STATIC);
    if (*demo_p++ != VERSION)
    {
        I_Error(4);
    }

    skill = *demo_p++;
    episode = *demo_p++;
    map = *demo_p++;
    *demo_p++;
    respawnparm = *demo_p++;
    fastparm = *demo_p++;
    *demo_p++;
    *demo_p++;

    *demo_p++;
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

#define SLK_FILE "BENCH.SLK"

//
// SLK_WriteQuoted: writes a string to the log, quoting any embedded
// semicolons by doubling them (";;"), as required by the SYLK spec
//
static void SLK_WriteQuoted(FILE *logFile, char *text)
{
    while (*text)
    {
        if (*text == ';')
            fputc(';', logFile);
        fputc(*text++, logFile);
    }
}

//
// SLK_CellStr: writes a SYLK C record with a string value (K"<string>")
//
static void SLK_CellStr(FILE *logFile, int row, int col, char *text)
{
    fprintf(logFile, "C;Y%d;X%d;K\"", row, col);
    SLK_WriteQuoted(logFile, text);
    fprintf(logFile, "\"\n");
}

//
// SLK_CellNum: writes a SYLK C record with a decimal value (K<decimal>)
//
static void SLK_CellNum(FILE *logFile, int row, int col, unsigned int value)
{
    fprintf(logFile, "C;Y%d;X%d;K%u.%03u\n", row, col, value / 1000, value % 1000);
}

//
// SLK_CellInt: writes a SYLK C record with an integer value (K<integer>)
//
static void SLK_CellInt(FILE *logFile, int row, int col, unsigned int value)
{
    fprintf(logFile, "C;Y%d;X%d;K%u\n", row, col, value);
}

void G_CreateSLK(void)
{
    FILE *fptr;
    fptr = fopen(SLK_FILE, "r");
    if (fptr == NULL) // if file does not exist, create it
    {
        fptr = fopen(SLK_FILE, "w+");
        // ID record (first record)
        fprintf(fptr, "ID;PFD;N;\n");

        // Header row
        SLK_CellStr(fptr, 1, 1, "executable");
        SLK_CellStr(fptr, 1, 2, "arch");
        SLK_CellStr(fptr, 1, 3, "detail");
        SLK_CellStr(fptr, 1, 4, "size");
        SLK_CellStr(fptr, 1, 5, "visplanes");
        SLK_CellStr(fptr, 1, 6, "walls");
        SLK_CellStr(fptr, 1, 7, "sprites");
        SLK_CellStr(fptr, 1, 8, "psprite");
        SLK_CellStr(fptr, 1, 9, "sky");
        SLK_CellStr(fptr, 1, 10, "objects");
        SLK_CellStr(fptr, 1, 11, "transparent_columns");
        SLK_CellStr(fptr, 1, 12, "iwad");
        SLK_CellStr(fptr, 1, 13, "demo");
        SLK_CellStr(fptr, 1, 14, "gametics");
        SLK_CellStr(fptr, 1, 15, "realtics");
        SLK_CellStr(fptr, 1, 16, "fps");
        SLK_CellStr(fptr, 1, 17, "onepercentlow");
        SLK_CellStr(fptr, 1, 18, "dotonepercentlow");

        // E record (end of file)
        fprintf(fptr, "E\n");
    }
    fclose(fptr);
}

//
// Names for the export, indexed by the cpu_t / render enums
//
static const char *const cpunames[NUM_CPU] =
{
    "386sx", "386dx", "intel486", "pentium", "pentiump54cs", "pentiummmx",
    "pentiumii", "k5", "k6", "cyrix386", "cyrix486", "cyrix5x86",
    "cyrix6x86", "cyrix6x86mx", "umc486", "winchip", "mp6"
};

static const char *const detailnames[NUM_DETAIL] = { "high", "low", "potato" };

// Shared by visplanes, walls, sprites and player sprites
static const char *const rendernames[NUM_VISPLANESRENDER] = { "normal", "flat", "flatter" };

static const char *const invisiblenames[NUM_INVISIBLERENDER] =
{
    "normal", "flat", "flatsaturn", "saturn", "translucent"
};

void G_SaveSLKResult(unsigned int gametics, unsigned int realtics, unsigned int resultfps, unsigned int onepercentlow, unsigned int dotonepercentlow)
{
    char *buffer;
    char *line;
    int row, col;
    long size;
    FILE *logFile;

    // Load the existing file (if any) so it can be rewritten with the new
    // result and a trailing E record (which must be the last record)
    buffer = 0;
    logFile = fopen(SLK_FILE, "r");
    if (logFile)
    {
        fseek(logFile, 0, SEEK_END);
        size = ftell(logFile);
        fseek(logFile, 0, SEEK_SET);
        buffer = Z_MallocUnowned(size + 1, PU_STATIC);
        if (buffer)
        {
            fread(buffer, 1, size, logFile);
            buffer[size] = 0;
        }
        fclose(logFile);
    }

    logFile = fopen(SLK_FILE, "w+");
    if (logFile)
    {
        // ID record (first record)
        fprintf(logFile, "ID;PFD;N;\n");

        // Existing cell records, tracking the last row in use
        row = 0;
        for (line = buffer; line && *line;)
        {
            char *d;
            char *next = strchr(line, '\n');
            int r = 0;

            if (next)
                *next = 0;

            if (line[0] == 'C' && line[1] == ';')
            {
                // Parse row number from "C;Y<row>;..."
                for (d = line + 3; *d >= '0' && *d <= '9'; d++)
                    r = r * 10 + (*d - '0');
                if (r > row)
                    row = r;

                fprintf(logFile, "%s\n", line);
            }

            line = next ? next + 1 : line + strlen(line);
        }

        // New result row
        row++;
        col = 0;

        // Executable
        SLK_CellStr(logFile, row, ++col, myargv[0]);

        // Architecture
        if (selectedCPU >= 0 && selectedCPU < NUM_CPU)
            SLK_CellStr(logFile, row, ++col, cpunames[selectedCPU]);
        else
            SLK_CellStr(logFile, row, ++col, "");

        // Detail
        SLK_CellStr(logFile, row, ++col, detailnames[detailshift]);

        // Screen size
        SLK_CellInt(logFile, row, ++col, screenblocks);

        // Visplanes
        SLK_CellStr(logFile, row, ++col, rendernames[visplaneRender]);

        // Walls
        SLK_CellStr(logFile, row, ++col, rendernames[wallRender]);

        // Sprites
        SLK_CellStr(logFile, row, ++col, rendernames[spriteRender]);

        // Player sprite
        SLK_CellStr(logFile, row, ++col, rendernames[pspriteRender]);

        // Sky
        SLK_CellStr(logFile, row, ++col, flatSky ? "flat" : "normal");

        // Objects
        SLK_CellStr(logFile, row, ++col, nearSprites ? "near" : "normal");

        // Transparent objects
        SLK_CellStr(logFile, row, ++col, invisiblenames[invisibleRender]);

        // IWAD
        SLK_CellStr(logFile, row, ++col, iwadfile);

        // Demo
        SLK_CellStr(logFile, row, ++col, demofile);

        // Gametics, Realtics
        SLK_CellInt(logFile, row, ++col, gametics);
        SLK_CellInt(logFile, row, ++col, realtics);

        // FPS, 1% low FPS, 0.1% low FPS
        SLK_CellNum(logFile, row, ++col, resultfps);
        SLK_CellNum(logFile, row, ++col, onepercentlow);
        SLK_CellNum(logFile, row, ++col, dotonepercentlow);

        // E record (end of file)
        fprintf(logFile, "E\n");

        fclose(logFile);
    }

    Z_Free(buffer);
}

#define FRAMETIME_FILE "FTIME.SLK"

void G_CreateFrametime(void)
{
    FILE *fptr;
    fptr = fopen(FRAMETIME_FILE, "r");
    if (fptr == NULL) // if file does not exist, create it
    {
        fptr = fopen(FRAMETIME_FILE, "w+");
        // ID record (first record)
        fprintf(fptr, "ID;PFD;N;\n");

        // Header row
        SLK_CellStr(fptr, 1, 1, "frame");
        SLK_CellStr(fptr, 1, 2, "milliseconds");
        SLK_CellStr(fptr, 1, 3, "ms_per_frame");

        // E record (end of file)
        fprintf(fptr, "E\n");
    }
    fclose(fptr);
}

void G_SaveFrametimeResult(unsigned int start, unsigned int count)
{
    char *buffer;
    char *line;
    int row;
    long size;
    unsigned int counter = 0;
    unsigned int i;
    FILE *logFile;

    // Load the existing file (if any) so it can be rewritten with the new
    // results and a trailing E record (which must be the last record)
    buffer = 0;
    logFile = fopen(FRAMETIME_FILE, "r");
    if (logFile)
    {
        fseek(logFile, 0, SEEK_END);
        size = ftell(logFile);
        fseek(logFile, 0, SEEK_SET);
        buffer = Z_MallocUnowned(size + 1, PU_STATIC);
        if (buffer)
        {
            fread(buffer, 1, size, logFile);
            buffer[size] = 0;
        }
        fclose(logFile);
    }

    logFile = fopen(FRAMETIME_FILE, "w+");
    if (logFile)
    {
        // ID record (first record)
        fprintf(logFile, "ID;PFD;N;\n");

        // Existing cell records, tracking the last row in use
        row = 0;
        for (line = buffer; line && *line;)
        {
            char *d;
            char *next = strchr(line, '\n');
            int r = 0;

            if (next)
                *next = 0;

            if (line[0] == 'C' && line[1] == ';')
            {
                // Parse row number from "C;Y<row>;..."
                for (d = line + 3; *d >= '0' && *d <= '9'; d++)
                    r = r * 10 + (*d - '0');
                if (r > row)
                    row = r;

                fprintf(logFile, "%s\n", line);
            }

            line = next ? next + 1 : line + strlen(line);
        }

        // New frametime rows (frametime holds cumulative times in millis,
        // 1/1000 sec, so milliseconds per frame is the delta between
        // consecutive entries divided by 10)
        for (i = start; i < count; i++)
        {
            unsigned int ms;

            row++;
            SLK_CellInt(logFile, row, 1, counter);
            SLK_CellInt(logFile, row, 2, frametime[i]);
            ms = (i > 0) ? frametime[i] - frametime[i - 1] : frametime[i];
            SLK_CellInt(logFile, row, 3, ms);
            counter++;
        }

        // E record (end of file)
        fprintf(logFile, "E\n");

        fclose(logFile);
    }

    Z_Free(buffer);
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

        if (export)
        {
            G_CreateSLK();

            if (benchmark_advanced)
            {
                unsigned int i, j;
                unsigned int temp;
                unsigned int prev;
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

                // Convert cumulative times to per-frame durations (must
                // happen before the sort below)
                prev = fix_start ? frametime[fix_start - 1] : 0;
                for (i = fix_start; i < frametime_position; i++)
                {
                    temp = frametime[i];
                    frametime[i] = temp - prev;
                    prev = temp;
                }

                // Sort per-frame durations (higher values are worse),
                // omitting the first frame (load data) and any frame before
                // the benchmark
                for (i = fix_start; i < frametime_position; i++)
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

                // Calculate 1% low frametimes (over the benchmark frames only)

                onepercentlow_num = (frametime_position - fix_start) / 100; // 1% Low

                if (onepercentlow_num == 0)
                    onepercentlow_num++;

                for (i = fix_start; i < onepercentlow_num + fix_start; i++)
                {
                    onepercentlow_ms += frametime[i];
                }

                onepercentlow_ms *= 1000; // millis -> microseconds
                onepercentlow_ms /= onepercentlow_num; // Average us 1% low

                if (onepercentlow_ms == 0)
                    onepercentlow_ms = 1; // Avoid division by zero

                onepercentlow_fps = 1000000000u / onepercentlow_ms; // us -> millifps

                // Calculate 0.1% low frametimes (over the benchmark frames only)
                dotonepercentlow_num = (frametime_position - fix_start) / 1000; // 0.1% Low

                if (dotonepercentlow_num == 0)
                    dotonepercentlow_num++;

                for (i = fix_start; i < dotonepercentlow_num + fix_start; i++)
                {
                    dotonepercentlow_ms += frametime[i];
                }

                dotonepercentlow_ms *= 1000; // millis -> microseconds
                dotonepercentlow_ms /= dotonepercentlow_num; // Average us 0.1% low

                if (dotonepercentlow_ms == 0)
                    dotonepercentlow_ms = 1; // Avoid division by zero

                dotonepercentlow_fps = 1000000000u / dotonepercentlow_ms; // us -> millifps

                G_SaveSLKResult(gametics, realtics, resultfps, onepercentlow_fps, dotonepercentlow_fps);

                // Cleanup frametimes
                frametime_position = 0;

                for (i = 0; i < benchmark_total_tics; i++)
                {
                    frametime[i] = 0;
                }
            }
            else
            {
                G_SaveSLKResult(gametics, realtics, resultfps, 0, 0);
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
            I_Error(17, gametics, realtics, resultfps / 1000, resultfps % 1000);
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
        D_AdvanceDemo();
        return;
    }

    if (demorecording)
    {
        *demo_p++ = DEMOMARKER;
        M_WriteFile(demoname, demobuffer, demo_p - demobuffer);
        Z_Free(demobuffer);
        demorecording = 0;
        I_Error(18, demoname);
    }

    return;
}
