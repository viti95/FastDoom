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
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include
//    this header file.
//   In practice, things are a bit messy.
//

#ifndef __D_STATE__
#define __D_STATE__

// We need globally shared data structures,
//  for defining the global state variables.
#include "doomdata.h"
#include "d_net.h"

// We need the playr data structure as well.
#include "d_player.h"

#include "options.h"



// ------------------------
// Command line parameters.
//
extern boolean respawnparm; // checkparm of -respawn
extern boolean fastparm;    // checkparm of -fast

extern boolean flatSky;
extern int invisibleRender;
extern int visplaneRender;
extern int wallRender;
extern int spriteRender;
extern int pspriteRender;
extern int selectedCPU;
extern int showFPS;
extern int automapRT;
extern int debugCardPort;
extern boolean debugCardReverse;
extern boolean nearSprites;
extern boolean monoSound;
extern boolean noMelt;

extern boolean reverseStereo;

extern boolean forceHighDetail;
extern boolean forceLowDetail;
extern boolean forcePotatoDetail;
extern int forceScreenSize;

#if defined(TEXT_MODE)
extern boolean CGAcard;
#endif

#if defined(MODE_CGA)
extern boolean CGApalette1;
#endif

#if defined(MODE_CGA512)
extern unsigned char CGAmodel;
#endif

#if defined(MODE_CGA16) || defined(MODE_CGA_AFH)
extern boolean snowfix;
#endif

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
extern int forceVesaBitsPerPixel;
extern int forceVesaNonLinear;
#endif

#ifdef SUPPORTS_HERCULES_AUTOMAP
extern boolean HERCmap;
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
extern boolean VGADACfix;
#endif

#if defined(TEXT_MODE)
extern boolean videoPageFix;
#endif

extern boolean ignoreSoundChecks;

extern boolean xtCompat;

extern boolean csv;
extern boolean disableDemo;

extern boolean busSpeed;
extern boolean waitVsync;
extern boolean uncappedFPS;
extern boolean highResTimer;

// Set if homebrew PWAD stuff has been added.
extern boolean modifiedgame;

extern unsigned char complevel;

// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern skill_t startskill;
extern int startepisode;
extern int startmap;

// Selected by user.
extern skill_t gameskill;
extern int gameepisode;
extern int gamemap;

extern gamemode_t gamemode;
extern gamemission_t gamemission;

extern char currentlevelname[40];
extern char nextlevelname[40];

// Nightmare mode flag, single player.
extern boolean respawnmonsters;

// Flag: true only if started as net deathmatch.
// An enum might handle altdeath/cooperative better.
extern boolean deathmatch;

// -------------------------
// Internal parameters for sound rendering.
// These have been taken from the DOS version,
//  but are not (yet) supported with Linux
//  (e.g. no sound volume adjustment with menu.

// These are not used, but should be (menu).
// From m_menu.c:
//  Sound FX volume has default, 0 - 15
//  Music volume has default, 0 - 15
// These are multiplied by 8.
//extern int snd_SfxVolume;      // maximum volume for sound
//extern int snd_MusicVolume;    // maximum volume for music

// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
// Ideally, this would use indices found
//  in: /usr/include/linux/soundcard.h
extern int snd_MusicDevice;
extern int snd_SfxDevice;
extern int snd_MidiDevice;
// Config file? Same disclaimer as above.
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;
extern int snd_DesiredMidiDevice;

// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern boolean statusbaractive;

extern byte automapactive; // In AutoMap mode?
extern byte menuactive;    // Menu overlayed?
extern byte paused;        // Game Pause?

extern byte viewactive;

extern struct ev_s *current_ev;

extern int viewwindowx;
extern int viewwindowy;
extern int viewheight;
extern int viewheightminusone;
extern int viewheightshift;
extern int viewheightopt;
extern int viewheight32;
extern int viewwidth;
extern int viewwidthhalf;
extern int viewwidthlimit;
extern int automapheight;
extern int scaledviewwidth;

#if defined(MODE_13H) || defined(MODE_VBE2)
extern int endscreen;
extern int startscreen;
#endif

// -------------------------------------
// Scores, rating.
// Statistics on a given map, for intermission.
//
extern int totalkills;
extern int totalitems;
extern int totalsecret;

// Timer, for scores.
extern int leveltime;     // tics in game play for par

// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern byte usergame;

//?
extern byte demoplayback;
extern byte demorecording;
extern byte timingdemo;

// Quit after playing a demo from cmdline.
extern byte singledemo;

//?
extern gamestate_t gamestate;

//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern int gametic;

// Bookkeeping on players - state.
extern player_t players;
extern mobj_t *players_mo;

// Intermission stats.
// Parameters for world map / intermission.
extern wbstartstruct_t wminfo;

// LUT of ammunition limits for each kind.
// This doubles with BackPack powerup item.
extern int maxammo[NUMAMMO];

//-----------------------------------------
// Internal parameters, used for engine.
//

// File handling stuff.
extern char basedefault[13];
extern char sbkfile[13];
extern char iwadfile[13];
extern char demofile[13];
extern char savegamename[14];

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern gamestate_t wipegamestate;

extern int mouseSensitivity;
extern fixed_t mouseSensitivityFP;
extern fixed_t sensitivityTable[10];
//?
// debug flag to cancel adaptiveness
extern boolean singletics;
extern boolean benchmark;
extern boolean benchmark_finished;
extern boolean benchmark_commandline;
extern unsigned int benchmark_resultfps;
extern unsigned int benchmark_gametics;
extern unsigned int benchmark_realtics;
extern unsigned int benchmark_starttic;
extern unsigned int benchmark_type;
extern unsigned int benchmark_number;
extern boolean benchmark_advanced;
extern char benchmark_file[20];
extern int benchmark_total;
extern char **benchmark_files;
extern unsigned int benchmark_files_num;
extern unsigned int benchmark_total_tics;

// Needed to store the number of the dummy sky flat.
// Used for rendering,
//  as well as tracking projectiles etc.
extern short skyflatnum;
extern short skytexture;

extern ticcmd_t localcmds[BACKUPTICS];

extern int maketic;

extern int autorun;

#endif
