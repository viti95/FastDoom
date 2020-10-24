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

#include <stdio.h>
#include <string.h>

//
// Global parameters/defines.
//
// DOOM version
enum
{
    VERSION = 109
};

#define EXE_VERSION_1_9 0
#define EXE_VERSION_ULTIMATE 1
#define EXE_VERSION_FINAL 2
#define EXE_VERSION_FINAL2 3

#define EXE_VERSION EXE_VERSION_ULTIMATE

//
// For resize of screen, at start of game.
// It will not work dynamically, see visplanes.
//
#define BASE_WIDTH 320

// It is educational but futile to change this
//  scaling e.g. to 2. Drawing of status bar,
//  menues etc. is tied to the scale implied
//  by the graphics.
#define SCREEN_MUL 1
#define INV_ASPECT_RATIO 0.625 // 0.75, ideally

// Defines suck. C sucks.
// C++ might sucks for OOP, but it sure is a better C.
// So there.
#define SCREENWIDTH 320
//SCREEN_MUL*BASE_WIDTH //320
#define SCREENHEIGHT 200
//(int)(SCREEN_MUL*BASE_WIDTH*INV_ASPECT_RATIO) //200

#define FRACBITS 16
#define FRACUNIT (1 << FRACBITS)
typedef int fixed_t;
  
#define PI_F 3.14159265f
#define FIXED_TO_FLOAT(inp)       ((float)inp) / (1 << FRACBITS)
#define FLOAT_TO_FIXED(inp)       (fixed_t)(inp * (1 << FRACBITS))
#define ANGLE_TO_FLOAT(x)       (x * ((float)(PI_F / 4096.0f)))

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

fixed_t FixedMul(fixed_t a, fixed_t b);
#define FixedDiv(a,b) (((abs(a) >> 14) >= abs(b)) ? (((a) ^ (b)) >> 31) ^ MAXINT : FixedDiv2(a, b))
fixed_t FixedDiv2(fixed_t a, fixed_t b);
int Mul80(int value);
int Mul320(int value);
int Mul10(int value);
int Div1000(int value);
int Div10(int value);
int Div63(int value);
int Div101(int value);
int Mul47000(int value);
int Mul100(int value);
int Mul1000(int value);
int Div35(int value);
int DivSKULLSPEED(int value);
int Mod3(int value);
int Mod10(int value);
int Mod5(int value);
int Div100(int value);
int Div255(int value);
unsigned long Div51200(unsigned long value);
int Mod6(int value);
int Div45(int value);
int Mul819200(int value);
int Mul35(int value);
int Mul768(int value);
int Mul160(int value);
int Mul200(int value);
int Mul409(int value);
int Mul26843545(int value);
int Mul70(int value);
int Div70(int value);

#pragma aux FixedMul = \
    "imul ebx",        \
    "shrd eax,edx,16" parm[eax][ebx] value[eax] modify exact[eax edx]

#pragma aux FixedDiv2 =        \
    "cdq",                     \
    "shld edx,eax,16", \
    "sal eax,16",      \
    "idiv ebx" parm[eax][ebx] value[eax] modify exact[eax edx]

#pragma aux Mul80 = \
    "lea eax, [eax+eax*4]", \
    "shl eax, 4" parm[eax] value[eax] modify exact[eax]

#pragma aux Mul320 = \
    "lea eax, [eax+eax*4]", \
    "sal eax, 6" parm[eax] value[eax] modify exact[eax]

#pragma aux Mul10 = \
    "lea eax, [eax+eax*4]", \
    "add eax, eax" parm[eax] value[eax] modify exact[eax]

#pragma aux Mul100 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 2" parm[eax] value[eax] modify exact[eax]

#pragma aux Mul1000 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 2" parm[eax] value[eax] modify exact[eax]

#pragma aux Div1000 = \
    "mov eax, 274877907", \
    "imul ecx", \
    "mov eax, edx", \
    "sar eax, 6", \
    "sar ecx, 31", \
    "sub eax, ecx" parm[ecx] value[eax] modify exact[eax ecx edx]

#pragma aux Div10 = \
    "mov eax, 1717986919", \
    "imul ecx", \
    "mov eax, edx", \
    "sar eax, 2", \
    "sar ecx, 31", \
    "sub eax, ecx" parm[ecx] value[eax] modify exact[eax ecx edx]

#pragma aux Div63 = \
    "mov edx, -2113396605", \
    "mov eax, ecx", \
    "imul edx", \
    "add edx, ecx", \
    "sar edx, 5", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

#pragma aux Div101 = \
    "mov edx, 680390859", \
    "mov eax, ecx", \
    "imul edx", \
    "sar edx, 4", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

#pragma aux Mul47000 = \
    "lea eax, [edx+edx]", \
    "add eax, edx", \
    "lea eax, [edx+eax*4]", \
    "lea eax, [eax+eax*8]", \
    "add eax, eax", \
    "add eax, edx", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 3" parm[edx] value[eax] modify exact[eax edx]

#pragma aux Div35 = \
    "mov edx, -368140053", \
    "mov eax, ecx", \
    "imul edx", \
    "add edx, ecx", \
    "sar edx, 5", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

#pragma aux DivSKULLSPEED = \
    "mov edx, 1717986919", \
    "mov eax, ecx", \
    "imul edx", \
    "sar edx, 19", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

#pragma aux Mod3 = \
    "mov edx, 1431655766", \
    "mov eax, ecx", \
    "imul edx", \
    "mov eax, ecx", \
    "sar eax, 31", \
    "sub edx, eax", \
    "lea eax, [edx+edx]", \
    "add edx, eax", \
    "sub ecx, edx" parm[ecx] value[ecx] modify exact[eax ecx edx]

#pragma aux Mod10 = \
    "mov eax, 1717986919", \
    "imul ecx", \
    "mov eax, edx", \
    "sar eax, 2", \
    "mov edx, ecx", \
    "sar edx, 31", \
    "sub eax, edx", \
    "lea eax, [eax+eax*4]", \
    "add eax, eax", \
    "sub ecx, eax" parm[ecx] value[ecx] modify exact[eax ecx edx]

#pragma aux Mod5 = \
    "mov eax, 1717986919", \
    "imul ecx", \
    "mov eax, edx", \
    "sar eax, 1", \
    "mov edx, ecx", \
    "sar edx, 31", \
    "sub eax, edx", \
    "lea eax, [eax+eax*4]", \
    "sub ecx, eax" parm[ecx] value[ecx] modify exact[eax ecx edx]

#pragma aux Div100 = \
    "mov edx, 1374389535", \
    "mov eax, ecx", \
    "imul edx", \
    "sar edx, 5", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

#pragma aux Div255 = \
    "mov edx, -2139062143", \
    "mov eax, ecx", \
    "imul edx", \
    "add edx, ecx", \
    "sar edx, 7", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

#pragma aux Div51200 = \
    "mov ecx, 1374389535", \
    "mov eax, edx", \
    "mul ecx", \
    "shr edx, 14" parm[edx] value[edx] modify exact[eax ecx edx]

#pragma aux Mod6 = \
    "mov edx, 715827883", \
    "mov eax, ecx", \
    "imul edx", \
    "mov eax, ecx", \
    "sar eax, 31", \
    "sub edx, eax", \
    "lea eax, [edx+edx]", \
    "add edx, eax", \
    "add edx, edx", \
    "sub ecx, edx" parm[ecx] value[ecx] modify exact[eax ecx edx]

#pragma aux Div45 = \
    "mov edx, -1240768329", \
    "mov eax, ecx", \
    "imul edx", \
    "add edx, ecx", \
    "sar edx, 5", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

#pragma aux Mul819200 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 15" parm[eax] value[eax] modify exact[eax]

#pragma aux Mul35 = \
    "lea eax, [edx+edx*8]", \
    "sal eax, 2", \
    "sub eax, edx" parm[edx] value[eax] modify exact[eax edx]

#pragma aux Mul768 = \
    "lea eax, [edx+edx]", \
    "add eax, edx", \
    "sal eax, 8" parm[edx] value[eax] modify exact[eax edx]

#pragma aux Mul160 = \
    "lea eax, [eax+eax*4]", \
    "sal eax, 5" parm[eax] value[eax] modify exact[eax]

#pragma aux Mul200 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 3" parm[eax] value[eax] modify exact[eax]

#pragma aux Mul409 = \
    "lea eax, [edx+edx*4]", \
    "lea eax, [eax+eax*4]", \
    "add eax, eax", \
    "add eax, edx", \
    "lea eax, [edx+eax*8]" parm[edx] value[eax] modify exact[eax edx]

#pragma aux Mul26843545 = \
    "lea eax, [edx+edx]", \
    "add eax, edx", \
    "lea ecx, [edx+eax*4]", \
    "mov eax, ecx", \
    "sal eax, 6", \
    "sub eax, ecx", \
    "mov ecx, eax", \
    "sal ecx, 12", \
    "add eax, ecx", \
    "lea eax, [edx+eax*8]" parm[edx] value[eax] modify exact[eax ecx edx]

#pragma aux Mul70 = \
    "lea eax, [edx+edx*8]", \
    "sal eax, 2", \
    "sub eax, edx", \
    "add eax, eax" parm[edx] value[eax] modify exact[eax edx]

#pragma aux Div70 = \
    "mov edx, -368140053", \
    "mov eax, ecx", \
    "imul edx", \
    "add edx, ecx", \
    "sar edx, 6", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

#define SHORT(x) (x)
#define LONG(x) (x)

void CopyBytes(void *src, void *dest, int num_bytes);
#pragma aux CopyBytes = \
    "rep movsb" \
    parm [esi] [edi] [ecx] modify[edi esi ecx];

void CopyWords(void *src, void *dest, int num_words);
#pragma aux CopyWords =     \
    "rep    movsw"          \
    parm [esi] [edi] [ecx] modify[edi esi ecx];

void CopyDWords(void *src, void *dest, int num_dwords);
#pragma aux CopyDWords =     \
    "rep movsd"             \
    parm [esi] [edi] [ecx]  \
    modify [esi edi ecx];

void SetBytes(void *dest, unsigned char value, int num_bytes);
#pragma aux SetBytes = \
    "rep stosb" \
    parm [edi] [al] [ecx] \
    modify [edi ecx];

void SetWords(void *dest, short value, int num_words);
#pragma aux SetWords = \
    "rep stosw" \
    parm [edi] [ax] [ecx] \
    modify [edi ecx];

void SetDWords(void *dest, int value, int num_dwords);
#pragma aux SetDWords = \
    "rep stosd" \
    parm [edi] [eax] [ecx] \
    modify [edi ecx];

#endif // __DOOMDEF__
