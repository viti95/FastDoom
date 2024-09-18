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
// DESCRIPTION:
//  Internally used data structures for virtually everything,
//   key definitions, lots of other stuff.
//

#ifndef __DOOMDEF__
#define __DOOMDEF__

//
// Global parameters/defines.
//
// DOOM version
enum
{
    VERSION = 109
};

#define COMPLEVEL_DOOM              2
#define COMPLEVEL_ULTIMATE_DOOM     3
#define COMPLEVEL_FINAL_DOOM        4

// Game mode handling - identify IWAD version
//  to handle IWAD dependend animations etc.
typedef enum
{
  shareware,	// DOOM 1 shareware, E1, M9
  registered,	// DOOM 1 registered, E3, M27
  commercial,	// DOOM 2 retail, E1 M34
  retail,	    // DOOM 1 retail, E4, M36
  indetermined	// Well, no IWAD found.
} gamemode_t;

// Mission packs - might be useful for TC stuff?
typedef enum
{
  doom,		    // DOOM 1
  doom2,	    // DOOM 2
  pack_tnt,	    // TNT mission pack
  pack_plut,	// Plutonia pack
  none
} gamemission_t;

// Defines suck. C sucks.
// C++ might sucks for OOP, but it sure is a better C.
// So there.
// If we don't override the screen resolution, use the ubiqitous 320x200
#if !defined(SCREENWIDTH) || !defined(SCREENHEIGHT)
#define SCREENWIDTH 320
//SCREEN_MUL*BASE_WIDTH //320
#define SCREENHEIGHT 200
#endif // !defined(SCREENWIDTH) || !defined(SCREENHEIGHT)

#if SCREENWIDTH == 320 && SCREENHEIGHT == 100
#define MulScreenWidth(x) Mul320(x)
#define MulScreenWidthHalf(x) Mul160(x)
#define MulScreenWidthQuarter(x) Mul80(x)
#define MulScreenWidthEighth(x) Mul40(x)
#define PIXEL_SCALING 1
#define ASPECTRATIO16x10

#elif SCREENWIDTH == 320 && (SCREENHEIGHT == 200 || SCREENHEIGHT == 240)
#define MulScreenWidth(x) Mul320(x)
#define MulScreenWidthHalf(x) Mul160(x)
#define MulScreenWidthQuarter(x) Mul80(x)
#define MulScreenWidthEighth(x) Mul40(x)
#define PIXEL_SCALING 1
#if SCREENHEIGHT == 200
#define ASPECTRATIO16x10
#elif SCREENHEIGHT == 240
#define ASPECTRATIO4x3
#endif

#elif SCREENWIDTH == 400 && SCREENHEIGHT == 300
#define MulScreenWidth(x) Mul400(x)
#define MulScreenWidthHalf(x) Mul200(x)
#define MulScreenWidthQuarter(x) Mul100(x)
#define MulScreenWidthEighth(x) Mul50(x)
#define ASPECTRATIO4x3
#define PIXEL_SCALING 1


#elif SCREENWIDTH == 512 && SCREENHEIGHT == 384
#define MulScreenWidth(x) Mul512(x)
#define MulScreenWidthHalf(x) Mul256(x)
#define MulScreenWidthQuarter(x) Mul128(x)
#define MulScreenWidthEighth(x) Mul64(x)
#define ASPECTRATIO4x3
#define PIXEL_SCALING 1

#elif SCREENWIDTH == 640 && (SCREENHEIGHT == 400 || SCREENHEIGHT == 480)
#define MulScreenWidth(x) Mul640(x)
#define MulScreenWidthHalf(x) Mul320(x)
#define MulScreenWidthQuarter(x) Mul160(x)
#define MulScreenWidthEighth(x) Mul80(x)
#define PIXEL_SCALING 2
#if SCREENHEIGHT == 400
#define ASPECTRATIO16x10
#elif SCREENHEIGHT == 480
#define ASPECTRATIO4x3
#endif

#elif SCREENWIDTH == 800 && SCREENHEIGHT == 600
#define MulScreenWidth(x) Mul800(x)
#define MulScreenWidthHalf(x) Mul400(x)
#define MulScreenWidthQuarter(x) Mul200(x)
#define MulScreenWidthEighth(x) Mul100(x)
#define ASPECTRATIO4x3
#define PIXEL_SCALING 2

#elif SCREENWIDTH == 1024 && SCREENHEIGHT == 768
#define MulScreenWidth(x) Mul1024(x)
#define MulScreenWidthHalf(x) Mul512(x)
#define MulScreenWidthQuarter(x) Mul256(x)
#define MulScreenWidthEighth(x) Mul128(x)
#define ASPECTRATIO4x3
#define PIXEL_SCALING 3

#elif SCREENWIDTH == 1280 && (SCREENHEIGHT == 800 || SCREENHEIGHT == 1024)
#define MulScreenWidth(x) Mul1280(x)
#define MulScreenWidthHalf(x) Mul640(x)
#define MulScreenWidthQuarter(x) Mul320(x)
#define MulScreenWidthEighth(x) Mul160(x)
#define PIXEL_SCALING 4
#if SCREENHEIGHT == 800
#define ASPECTRATIO16x10
#elif SCREENHEIGHT == 1024
#define ASPECTRATIO5x4
#endif

#else
#error "Defined Screen Resolution is not supported"
#endif

#define ORIGINAL_SCREENWIDTH 320
#define ORIGINAL_SCREENHEIGHT 200
#define SCALED_SCREENWIDTH (SCREENWIDTH / PIXEL_SCALING)
#define SCALED_SCREENHEIGHT (SCREENHEIGHT / PIXEL_SCALING)

#if defined(MODE_Y_HALF)
    #define SBARHEIGHT (32 / 2)
#else
    #define SBARHEIGHT (32 * PIXEL_SCALING)
#endif

#define SBARWIDTH (320 * PIXEL_SCALING)
#define SCALED_SBARHEIGHT (SBARHEIGHT / PIXEL_SCALING)
#define SCALED_SBARWIDTH (SBARWIDTH / PIXEL_SCALING)

// For screen resolution that don't scale perfectly, we need a centering
// offset to make sure the menu/ui/etc. is centered on the screen.
// Evertything is based on 320x200 which is the native positioning of
// everything.
// Note that this gets reduced to a constant at compile time
#define CENTERING_OFFSET_X ((SCALED_SCREENWIDTH-ORIGINAL_SCREENWIDTH) / 2)

#if SCREENHEIGHT < ORIGINAL_SCREENHEIGHT
#define CENTERING_OFFSET_Y 0
#else
#define CENTERING_OFFSET_Y ((SCALED_SCREENHEIGHT-ORIGINAL_SCREENHEIGHT) / 2)
#endif

#if defined(ASPECTRATIO16x10)
#define SKY_SCALE 100
#elif defined(ASPECTRATIO4x3)
#define SKY_SCALE 80
#elif defined(ASPECTRATIO5x4)
#define SKY_SCALE 75
#endif

// The maximum number of players, multiplayer/networking.
#define MAXPLAYERS 4

// State updates, number of tics / second.
#define TICRATE 35

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo.
typedef enum
{
    GS_LEVEL,
    GS_INTERMISSION,
    GS_FINALE,
    GS_DEMOSCREEN
} gamestate_t;

//
// Difficulty/skill settings/filters.
//

// Skill flags.
#define MTF_EASY 1
#define MTF_NORMAL 2
#define MTF_HARD 4

// Deaf monsters/do not react to sound.
#define MTF_AMBUSH 8

typedef enum
{
    sk_baby,
    sk_easy,
    sk_medium,
    sk_hard,
    sk_nightmare
} skill_t;

//
// Key cards.
//
typedef enum
{
    it_bluecard,
    it_yellowcard,
    it_redcard,
    it_blueskull,
    it_yellowskull,
    it_redskull,

    NUMCARDS

} card_t;

// The defined weapons,
//  including a marker indicating
//  user has not changed weapon.
typedef enum
{
    wp_fist,
    wp_pistol,
    wp_shotgun,
    wp_chaingun,
    wp_missile,
    wp_plasma,
    wp_bfg,
    wp_chainsaw,
    wp_supershotgun,

    NUMWEAPONS,

    // No pending weapon change.
    wp_nochange

} weapontype_t;

// Ammunition types defined.
typedef enum
{
    am_clip,  // Pistol / chaingun ammo.
    am_shell, // Shotgun / double barreled shotgun.
    am_cell,  // Plasma rifle, BFG.
    am_misl,  // Missile launcher.
    NUMAMMO,
    am_noammo // Unlimited for chainsaw / fist.

} ammotype_t;

// Power up artifacts.
typedef enum
{
    pw_invulnerability,
    pw_strength,
    pw_invisibility,
    pw_ironfeet,
    pw_allmap,
    pw_infrared,
    NUMPOWERS

} powertype_t;

//
// Power up durations,
//  how many seconds till expiration,
//  assuming TICRATE is 35 ticks/second.
//
typedef enum
{
    INVULNTICS = (30 * TICRATE),
    INVISTICS = (60 * TICRATE),
    INFRATICS = (120 * TICRATE),
    IRONTICS = (60 * TICRATE)

} powerduration_t;

//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//
#define KEY_RIGHTARROW 0xae
#define KEY_LEFTARROW 0xac
#define KEY_UPARROW 0xad
#define KEY_DOWNARROW 0xaf
#define KEY_ESCAPE 27
#define KEY_ENTER 13
#define KEY_TAB 9
#define KEY_F1 (0x80 + 0x3b)
#define KEY_F2 (0x80 + 0x3c)
#define KEY_F3 (0x80 + 0x3d)
#define KEY_F4 (0x80 + 0x3e)
#define KEY_F5 (0x80 + 0x3f)
#define KEY_F6 (0x80 + 0x40)
#define KEY_F7 (0x80 + 0x41)
#define KEY_F8 (0x80 + 0x42)
#define KEY_F9 (0x80 + 0x43)
#define KEY_F10 (0x80 + 0x44)
#define KEY_F11 (0x80 + 0x57)
#define KEY_F12 (0x80 + 0x58)

#define KEY_BACKSPACE 127
#define KEY_PAUSE 0xff

#define KEY_EQUALS 0x3d
#define KEY_MINUS 0x2d

#define KEY_RSHIFT (0x80 + 0x36)
#define KEY_RCTRL (0x80 + 0x1d)
#define KEY_RALT (0x80 + 0x38)

#define KEY_LALT KEY_RALT

typedef enum
{
    INVISIBLE_NORMAL,
    INVISIBLE_FLAT,
    INVISIBLE_FLAT_SATURN,
    INVISIBLE_SATURN,
    INVISIBLE_TRANSLUCENT,
    NUM_INVISIBLERENDER
} invisiblerender_t;

typedef enum
{
    VISPLANES_NORMAL,
    VISPLANES_FLAT,
    VISPLANES_FLATTER,
    NUM_VISPLANESRENDER
} visplanerender_t;

typedef enum
{
    WALL_NORMAL,
    WALL_FLAT,
    WALL_FLATTER,
    NUM_WALLRENDER
} wallrender_t;

typedef enum
{
    SPRITE_NORMAL,
    SPRITE_FLAT,
    SPRITE_FLATTER,
    NUM_SPRITERENDER
} spriterender_t;

typedef enum
{
    PSPRITE_NORMAL,
    PSPRITE_FLAT,
    PSPRITE_FLATTER,
    NUM_PSPRITERENDER
} pspriterender_t;

typedef enum
{
    DETAIL_HIGH,
    DETAIL_LOW,
    DETAIL_POTATO,
    NUM_DETAIL
} detail_t;

typedef enum
{
    AUTO_CPU = -1,
    INTEL_386SX = 0,
    INTEL_386DX,
    CYRIX_386DLC,
    CYRIX_486,
    INTEL_486,
    UMC_GREEN_486,
    CYRIX_5X86,
    AMD_K5,
    INTEL_PENTIUM,
    NUM_CPU
} cpu_t;

typedef enum
{
    NO_FPS,
    SCREEN_FPS,
    DEBUG_CARD_2D_FPS,
    DEBUG_CARD_4D_FPS,
    SCREEN_DC2D_FPS,
    SCREEN_DC4D_FPS,
    NUM_FPS
} fps_t;

typedef enum
{
    BENCHMARK_SINGLE,
    BENCHMARK_PHILS,
    BENCHMARK_QUICK,
    BENCHMARK_NORMAL,
    BENCHMARK_ARCH,
    BENCHMARK_FILE,
    NUM_BENCHMARK
} benchmark_t;

#endif // __DOOMDEF__
