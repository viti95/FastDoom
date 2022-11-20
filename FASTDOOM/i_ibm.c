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
//  IBM DOS VGA graphics and key/mouse.
//

#include <string.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "d_main.h"
#include "doomstat.h"
#include "r_local.h"
#include "sounds.h"
#include "i_system.h"
#include "i_sound.h"
#include "g_game.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "ns_dpmi.h"
#include "ns_task.h"
#include "doomdef.h"
#include "doomstat.h"
#include "ns_inter.h"

#include "am_map.h"

#include "std_func.h"

#include "options.h"

#include "math.h"

#if defined(MODE_CGA_AFH)
#include "cga_afh.h"
#endif

#if defined(MODE_CGA16)
#include "cga_16.h"
#endif

#if defined(MODE_EGA16)
#include "ega_16.h"
#endif

#if defined(MODE_VGA16)
#include "vga_16.h"
#endif

#if defined(MODE_13H)
#include "vga.h"
#include "vga_13h.h"
#endif

#if defined(MODE_PCP)
#include "pcp.h"
#endif

#if defined(MODE_ATI640)
#include "ati.h"
#endif

#if defined(MODE_CGA)
#include "cga.h"
#endif

#if defined(MODE_CVB)
#include "cga_cvbs.h"
#endif

#if defined(MODE_HERC)
#include "herc.h"
#endif

#if defined(MODE_CGA_BW)
#include "cga_bw.h"
#endif

#if defined(MODE_EGA640)
#include "ega_640.h"
#endif

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
#include "i_vesa.h"
#endif

//
// Macros
//

#define DPMI_INT 0x31
#define SBARHEIGHT 32

//
// Code
//

typedef struct
{
    unsigned edi, esi, ebp, reserved, ebx, edx, ecx, eax;
    unsigned short flags, es, ds, fs, gs, ip, cs, sp, ss;
} dpmiregs_t;

extern dpmiregs_t dpmiregs;

void I_ReadMouse(void);

extern int usemouse;

//
// Constants
//

#define SC_INDEX 0x3C4
#define SC_DATA 0x3C5
#define SC_RESET 0
#define SC_CLOCK 1
#define SC_MAPMASK 2
#define SC_CHARMAP 3
#define SC_MEMMODE 4

#define CRTC_INDEX 0x3D4
#define CRTC_DATA 0x3D5
#define CRTC_H_TOTAL 0
#define CRTC_H_DISPEND 1
#define CRTC_H_BLANK 2
#define CRTC_H_ENDBLANK 3
#define CRTC_H_RETRACE 4
#define CRTC_H_ENDRETRACE 5
#define CRTC_V_TOTAL 6
#define CRTC_OVERFLOW 7
#define CRTC_ROWSCAN 8
#define CRTC_MAXSCANLINE 9
#define CRTC_CURSORSTART 10
#define CRTC_CURSOREND 11
#define CRTC_STARTHIGH 12
#define CRTC_STARTLOW 13
#define CRTC_CURSORHIGH 14
#define CRTC_CURSORLOW 15
#define CRTC_V_RETRACE 16
#define CRTC_V_ENDRETRACE 17
#define CRTC_V_DISPEND 18
#define CRTC_OFFSET 19
#define CRTC_UNDERLINE 20
#define CRTC_V_BLANK 21
#define CRTC_V_ENDBLANK 22
#define CRTC_MODE 23
#define CRTC_LINECOMPARE 24

#define GC_INDEX 0x3CE
#define GC_DATA 0x3CF
#define GC_SETRESET 0
#define GC_ENABLESETRESET 1
#define GC_COLORCOMPARE 2
#define GC_DATAROTATE 3
#define GC_READMAP 4
#define GC_MODE 5
#define GC_MISCELLANEOUS 6
#define GC_COLORDONTCARE 7
#define GC_BITMASK 8

#define ATR_INDEX 0x3c0
#define ATR_MODE 16
#define ATR_OVERSCAN 17
#define ATR_COLORPLANEENABLE 18
#define ATR_PELPAN 19
#define ATR_COLORSELECT 20

#define STATUS_REGISTER_1 0x3da

#define PEL_WRITE_ADR 0x3c8
#define PEL_READ_ADR 0x3c7
#define PEL_DATA 0x3c9
#define PEL_MASK 0x3c6

#define SYNC_RESET 0
#define MAP_MASK 2
#define MEMORY_MODE 4

#define READ_MAP 4
#define GRAPHICS_MODE 5
#define MISCELLANOUS 6

#define MISC_OUTPUT 0x3C2

#define MAX_SCAN_LINE 9
#define UNDERLINE 0x14
#define MODE_CONTROL 0x17

#define VBLCOUNTER 34000 // hardware tics to a frame

#define TIMERINT 8
#define KEYBOARDINT 9

#define MOUSEB1 1
#define MOUSEB2 2
#define MOUSEB3 4

byte mousepresent;

int ticcount;
fixed_t fps;

// REGS stuff used for int calls
union REGS regs;
struct SREGS segregs;

#define KBDQUESIZE 32
byte keyboardque[KBDQUESIZE];
int kbdtail, kbdhead;

#define KEY_LSHIFT 0xfe

#define KEY_INS (0x80 + 0x52)
#define KEY_DEL (0x80 + 0x53)
#define KEY_PGUP (0x80 + 0x49)
#define KEY_PGDN (0x80 + 0x51)
#define KEY_HOME (0x80 + 0x47)
#define KEY_END (0x80 + 0x4f)

#define SC_RSHIFT 0x36
#define SC_LSHIFT 0x2a
void DPMIInt(int i);
void I_StartupSound(void);
void I_ShutdownSound(void);
void I_ShutdownTimer(void);

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA)
byte lut16colors[14 * 256];
byte *ptrlut16colors;
#endif

byte gammatable[5][256] =
    {
        {0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42, 42, 43, 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47, 47, 47, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51, 52, 52, 52, 52, 53, 53, 53, 53, 54, 54, 54, 54, 55, 55, 55, 55, 56, 56, 56, 56, 57, 57, 57, 57, 58, 58, 58, 58, 59, 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63},
        {0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 37, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42, 42, 43, 43, 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 46, 47, 47, 47, 47, 48, 48, 48, 48, 49, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51, 51, 52, 52, 52, 52, 53, 53, 53, 53, 53, 54, 54, 54, 54, 55, 55, 55, 55, 55, 56, 56, 56, 56, 57, 57, 57, 57, 57, 58, 58, 58, 58, 59, 59, 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63, 63},
        {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 36, 36, 37, 37, 37, 37, 38, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 42, 42, 42, 42, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 45, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58, 59, 59, 59, 59, 59, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63},
        {2, 3, 4, 4, 5, 6, 6, 7, 7, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 37, 38, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 45, 45, 45, 45, 45, 45, 46, 46, 46, 46, 46, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 53, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58, 58, 59, 59, 59, 59, 59, 59, 60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 61, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63},
        {4, 5, 7, 8, 9, 9, 10, 11, 12, 12, 13, 13, 14, 15, 15, 16, 16, 17, 17, 17, 18, 18, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 32, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 37, 38, 38, 38, 38, 38, 39, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 45, 45, 45, 45, 45, 45, 46, 46, 46, 46, 46, 46, 47, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 49, 50, 50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 52, 52, 53, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58, 58, 58, 58, 59, 59, 59, 59, 59, 59, 59, 60, 60, 60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 61, 61, 62, 62, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63, 63}};

#if defined(MODE_Y) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
byte processedpalette[14 * 768];
#endif

byte scantokey[128] =
    {
        //  0           1       2       3       4       5       6       7
        //  8           9       A       B       C       D       E       F
        0, 27, '1', '2', '3', '4', '5', '6',
        '7', '8', '9', '0', '-', '=', KEY_BACKSPACE, 9, // 0
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
        'o', 'p', '[', ']', 13, KEY_RCTRL, 'a', 's', // 1
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        39, '`', KEY_LSHIFT, 92, 'z', 'x', 'c', 'v', // 2
        'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT, '*',
        KEY_RALT, ' ', 0, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, // 3
        KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0, 0, KEY_HOME,
        KEY_UPARROW, KEY_PGUP, '-', KEY_LEFTARROW, '5', KEY_RIGHTARROW, '+', KEY_END, // 4
        KEY_DOWNARROW, KEY_PGDN, KEY_INS, KEY_DEL, 0, 0, 0, KEY_F11,
        KEY_F12, 0, 0, 0, 0, 0, 0, 0, // 5
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, // 6
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0 // 7
};

#if defined(MODE_EGA80)
byte vrambuffer[16384];
#endif

#if defined(MODE_CGA_AFH) || defined(MODE_CGA16) || defined(MODE_EGA16) || defined(MODE_VGA16) || defined(MODE_13H) || defined(MODE_PCP) || defined(MODE_ATI640) || defined(MODE_CGA) || defined(MODE_CVB) || defined(MODE_HERC) || defined(MODE_CGA_BW) || defined(MODE_EGA640)
void I_ProcessPalette(byte *palette)
{
    #if defined(MODE_CGA_AFH)
    CGA_AFH_ProcessPalette(palette);
    #endif

    #if defined(MODE_CGA16)
    CGA_16_ProcessPalette(palette);
    #endif

    #if defined(MODE_EGA16)
    EGA_16_ProcessPalette(palette);
    #endif

    #if defined(MODE_VGA16)
    VGA_16_ProcessPalette(palette);
    #endif

    #if defined(MODE_13H)
    VGA_13H_ProcessPalette(palette);
    #endif

    #if defined(MODE_PCP)
    PCP_ProcessPalette(palette);
    #endif

    #if defined(MODE_ATI640)
    ATI640_ProcessPalette(palette);
    #endif

    #if defined(MODE_CGA)
    CGA_ProcessPalette(palette);
    #endif

    #if defined(MODE_CVB)
    CGA_CVBS_ProcessPalette(palette);
    #endif

    #if defined(MODE_HERC)
    HERC_ProcessPalette(palette);
    #endif

    #if defined(MODE_CGA_BW)
    CGA_BW_ProcessPalette(palette);
    #endif

    #if defined(MODE_EGA640)
    EGA_640_ProcessPalette(palette);
    #endif
}
#endif

#if defined(MODE_Y) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 768; i += 4, palette += 4)
    {
        processedpalette[i] = ptr[*palette];
        processedpalette[i + 1] = ptr[*(palette + 1)];
        processedpalette[i + 2] = ptr[*(palette + 2)];
        processedpalette[i + 3] = ptr[*(palette + 3)];
    }
}
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA)
const byte colors[48] = {
    0x00, 0x00, 0x00,  // 0
    0x00, 0x00, 0x2A,  // 1
    0x00, 0x2A, 0x00,  // 2
    0x00, 0x2A, 0x2A,  // 3
    0x2A, 0x00, 0x00,  // 4
    0x2A, 0x00, 0x2A,  // 5
    0x2A, 0x15, 0x00,  // 6
    0x2A, 0x2A, 0x2A,  // 7
    0x15, 0x15, 0x15,  // 8
    0x15, 0x15, 0x3F,  // 9
    0x15, 0x3F, 0x15,  // 10
    0x15, 0x3F, 0x3F,  // 11
    0x3F, 0x15, 0x15,  // 12
    0x3F, 0x15, 0x3F,  // 13
    0x3F, 0x3F, 0x15,  // 14
    0x3F, 0x3F, 0x3F}; // 15
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA)
void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++)
    {
        int distance;

        int r1, g1, b1;

        int bescolor;

        r1 = (int)ptr[*palette++];
        g1 = (int)ptr[*palette++];
        b1 = (int)ptr[*palette++];

        bestcolor = GetClosestColor(colors, 16, r1, g1, b1);

        lut16colors[i] = bestcolor;
    }
}
#endif

#if defined(MODE_MDA)
void I_ProcessPalette(byte *palette)
{
}
#endif

//
// I_SetPalette
// Palette source must use 8 bit RGB elements.
//
void I_SetPalette(int numpalette)
{
#if defined(MODE_HERC)
    HERC_SetPalette(numpalette);
#endif

#if defined(MODE_EGA640)
    EGA_640_SetPalette(numpalette);    
#endif

#if defined(MODE_CGA_BW)
    CGA_BW_SetPalette(numpalette);
#endif

#if defined(MODE_ATI640)
    ATI640_SetPalette(numpalette);
#endif

#if defined(MODE_PCP)
    PCP_SetPalette(numpalette);
#endif

#if defined(MODE_CGA16)
    CGA_16_SetPalette(numpalette);
#endif

#if defined(MODE_EGA16)
    EGA_16_SetPalette(numpalette);
#endif

#if defined(MODE_VGA16)
    VGA_16_SetPalette(numpalette);
#endif

#if defined(MODE_CGA_AFH)
    CGA_AFH_SetPalette(numpalette);
#endif

#if defined(MODE_13H)
    VGA_13H_SetPalette(numpalette);
#endif

#if defined(MODE_CVB)
    CGA_CVBS_SetPalette(numpalette);
#endif

#if defined(MODE_CGA)
    CGA_SetPalette(numpalette);
#endif

#if defined(MODE_Y) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
    {
        int i;
        int pos = Mul768(numpalette);

        outp(PEL_WRITE_ADR, 0);

        if (VGADACfix)
        {
            byte *ptrprocessedpalette = processedpalette + pos;
            for (i = 0; i < 768; i += 4)
            {
                outp(PEL_DATA, *(ptrprocessedpalette));
                outp(PEL_DATA, *(ptrprocessedpalette + 1));
                outp(PEL_DATA, *(ptrprocessedpalette + 2));
                outp(PEL_DATA, *(ptrprocessedpalette + 3));
                ptrprocessedpalette += 4;
            }
        }
        else
        {
            OutString(PEL_DATA, ((unsigned char *)processedpalette) + pos, 768);
        }
    }
#endif
}

//
// Graphics mode
//

#if defined(USE_BACKBUFFER)
int updatestate;
#endif
byte *pcscreen, *destscreen, *destview;
unsigned short *currentscreen;

#if defined(MODE_VBE2_DIRECT)
short page = 0;
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100)
unsigned short *textdestscreen = (unsigned short *)0xB8000;
byte textpage = 0;
#endif

#if defined(MODE_MDA)
unsigned short *textdestscreen = backbuffer;
#endif

//
// I_UpdateBox
//
#if defined(MODE_VBE2_DIRECT)
void I_UpdateBox(int x, int y, int w, int h)
{
    byte *dest;
    byte *source;
    int i;
    int offset = Mul320(y) + x;

    dest = destscreen + offset;
    source = screen0 + offset;

    for (i = y; i < y + h; i++)
    {
        CopyBytes(source, dest, w);
        dest += 320;
        source += 320;
    }
}

void I_UpdateBoxTransparent(int x, int y, int w, int h)
{
    byte *dest;
    byte *source;
    int i;
    int offset = Mul320(y) + x;

    dest = destscreen + offset;
    source = screen0 + offset;

    for (i = y; i < y + h; i++)
    {
        for (x = 0; x < w; x++)
        {
            if (source[x] != 251)
            {
                dest[x] = source[x];
            }
        }

        dest += 320;
        source += 320;
    }
}
#endif

#if defined(MODE_Y)
void I_UpdateBox(int x, int y, int w, int h)
{
    int i, j, k, count;
    int sp_x1, sp_x2;
    int poffset;
    int offset;
    int pstep;
    int step;
    byte *dest, *source;

    sp_x1 = x / 8;
    sp_x2 = (x + w) / 8;
    count = sp_x2 - sp_x1 + 1;
    step = SCREENWIDTH - count * 8;
    offset = Mul320(y) + sp_x1 * 8;
    poffset = offset / 4;
    pstep = step / 4;

    if (count & 1)
    {
        // 16-bit copy

        count = 2 * count;

        for (i = 0; i < 4; i++)
        {
            outp(SC_INDEX + 1, 1 << i);
            source = &screen0[offset + i];
            dest = destscreen + poffset;

            for (j = 0; j < h; j++)
            {
                k = dest + count;

                while (dest < k)
                {
                    *(unsigned short *)dest = (unsigned short)((*source) + ((*(source + 4)) << 8));
                    dest += 2;
                    source += 8;
                }

                source += step;
                dest += pstep;
            }
        }
    }
    else
    {
        // 32-bit copy

        count = 2 * count;

        for (i = 0; i < 4; i++)
        {
            outp(SC_INDEX + 1, 1 << i);
            source = &screen0[offset + i];
            dest = destscreen + poffset;

            for (j = 0; j < h; j++)
            {
                k = dest + count;

                while (dest < k)
                {
                    *(unsigned int *)dest = (unsigned int)((*source) + ((*(source + 4)) << 8) + ((*(source + 8)) << 16) + ((*(source + 12)) << 24));
                    dest += 4;
                    source += 16;
                }

                source += step;
                dest += pstep;
            }
        }
    }
}

void I_UpdateBoxTransparent(int x, int y, int w, int h)
{
    int i, j, k, count;
    int sp_x1, sp_x2;
    int poffset;
    int offset;
    int pstep;
    int step;
    byte *dest, *source;

    sp_x1 = x / 8;
    sp_x2 = (x + w) / 8;
    count = sp_x2 - sp_x1 + 1;
    step = SCREENWIDTH - count * 8;
    offset = Mul320(y) + sp_x1 * 8;
    poffset = offset / 4;
    pstep = step / 4;

    count *= 2;

    for (i = 0; i < 4; i++)
    {
        outp(SC_INDEX + 1, 1 << i);
        source = &screen0[offset + i];
        dest = destscreen + poffset;

        for (j = 0; j < h; j++)
        {
            k = dest + count;

            while (dest < k)
            {
                if (*source != 251)
                    *dest = *source;
                dest++;
                source += 4;
            }

            source += step;
            dest += pstep;
        }
    }
}
#endif

//
// I_UpdateNoBlit
//
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
int olddb[2][4];
void I_UpdateNoBlit(void)
{
    int realdr[4];

    // Set current screen
    currentscreen = (unsigned short *)destscreen;

    // Update dirtybox size
    realdr[BOXTOP] = dirtybox[BOXTOP];
    if (realdr[BOXTOP] < olddb[0][BOXTOP])
    {
        realdr[BOXTOP] = olddb[0][BOXTOP];
    }
    if (realdr[BOXTOP] < olddb[1][BOXTOP])
    {
        realdr[BOXTOP] = olddb[1][BOXTOP];
    }

    realdr[BOXRIGHT] = dirtybox[BOXRIGHT];
    if (realdr[BOXRIGHT] < olddb[0][BOXRIGHT])
    {
        realdr[BOXRIGHT] = olddb[0][BOXRIGHT];
    }
    if (realdr[BOXRIGHT] < olddb[1][BOXRIGHT])
    {
        realdr[BOXRIGHT] = olddb[1][BOXRIGHT];
    }

    realdr[BOXBOTTOM] = dirtybox[BOXBOTTOM];
    if (realdr[BOXBOTTOM] > olddb[0][BOXBOTTOM])
    {
        realdr[BOXBOTTOM] = olddb[0][BOXBOTTOM];
    }
    if (realdr[BOXBOTTOM] > olddb[1][BOXBOTTOM])
    {
        realdr[BOXBOTTOM] = olddb[1][BOXBOTTOM];
    }

    realdr[BOXLEFT] = dirtybox[BOXLEFT];
    if (realdr[BOXLEFT] > olddb[0][BOXLEFT])
    {
        realdr[BOXLEFT] = olddb[0][BOXLEFT];
    }
    if (realdr[BOXLEFT] > olddb[1][BOXLEFT])
    {
        realdr[BOXLEFT] = olddb[1][BOXLEFT];
    }

    // Leave current box for next update
    // CopyDWords(olddb[1], olddb[0], 4);
    // CopyDWords(dirtybox, olddb[1], 4);
    // memcpy(olddb[0], olddb[1], 16);
    // memcpy(olddb[1], dirtybox, 16);

    olddb[0][0] = olddb[1][0];
    olddb[0][1] = olddb[1][1];
    olddb[0][2] = olddb[1][2];
    olddb[0][3] = olddb[1][3];
    olddb[1][0] = dirtybox[0];
    olddb[1][1] = dirtybox[1];
    olddb[1][2] = dirtybox[2];
    olddb[1][3] = dirtybox[3];

    // Update screen
    if (realdr[BOXBOTTOM] <= realdr[BOXTOP])
    {
        int x, y, w, h;

        x = realdr[BOXLEFT];
        w = realdr[BOXRIGHT] - x + 1;

        y = realdr[BOXBOTTOM];
        h = realdr[BOXTOP] - y + 1;

        if (transparentmap)
            I_UpdateBoxTransparent(x, y, w, h);
        else
            I_UpdateBox(x, y, w, h);
    }
    // Clear box
    dirtybox[BOXTOP] = dirtybox[BOXRIGHT] = MININT;
    dirtybox[BOXBOTTOM] = dirtybox[BOXLEFT] = MAXINT;
}
#endif

//
// I_FinishUpdate
//

extern int screenblocks;

#if defined(MODE_EGA)
unsigned short lastlatch;
unsigned short vrambuffer[16000];

void EGA_DrawBackbuffer(void)
{
    byte *vram = (byte *)0xA0000;
    byte *ptrbackbuffer = backbuffer;
    unsigned short *ptrvrambuffer = vrambuffer;

    do
    {
        unsigned short fullvalue = 16 * 16 * 16 * ptrlut16colors[*ptrbackbuffer] +
                                   16 * 16 * ptrlut16colors[*(ptrbackbuffer + 1)] +
                                   16 * ptrlut16colors[*(ptrbackbuffer + 2)] +
                                   ptrlut16colors[*(ptrbackbuffer + 3)];

        if (*(ptrvrambuffer) != fullvalue)
        {
            unsigned short vramlut;

            *(ptrvrambuffer) = fullvalue;
            vramlut = fullvalue >> 4;

            if (lastlatch != vramlut)
            {
                lastlatch = vramlut;
                ReadMem((byte *)0xA3E80 + vramlut);
            }

            *(vram) = fullvalue;
        }

        vram += 1;
        ptrbackbuffer += 4;
        ptrvrambuffer += 1;
    } while (vram < (byte *)0xA3E80);
}
#endif

#if defined(MODE_EGAW1)
unsigned short lastlatch;
unsigned short vrambuffer[8000];

void EGAW1_DrawBackbuffer(void)
{
    byte *vram = (byte *)0xA0000;
    byte *ptrbackbuffer = backbuffer;
    unsigned short *ptrvrambuffer = vrambuffer;

    do
    {
        unsigned short fullvalue = 16 * 16 * 16 * ptrlut16colors[*ptrbackbuffer] +
                                   16 * 16 * ptrlut16colors[*(ptrbackbuffer + 2)] +
                                   16 * ptrlut16colors[*(ptrbackbuffer + 4)] +
                                   ptrlut16colors[*(ptrbackbuffer + 6)];

        if (*(ptrvrambuffer) != fullvalue)
        {
            unsigned short vramlut;

            *(ptrvrambuffer) = fullvalue;
            vramlut = fullvalue >> 4;

            if (lastlatch != vramlut)
            {
                lastlatch = vramlut;
                ReadMem((byte *)0xA1F40 + vramlut);
            }

            *(vram) = fullvalue;
        }

        vram += 1;
        ptrbackbuffer += 8;
        ptrvrambuffer += 1;
    } while (vram < (byte *)0xA1F40);
}
#endif

#if defined(MODE_EGA80)
void EGA80_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xA0000;
    byte *ptrvrambuffer = vrambuffer;
    byte *ptrbackbuffer = backbuffer;

    do
    {
        byte tmp;

        tmp = ptrlut16colors[*ptrbackbuffer];
        if (tmp != *(ptrvrambuffer))
        {
            *(vram) = tmp;
            *(ptrvrambuffer) = tmp;
        }

        tmp = ptrlut16colors[*(ptrbackbuffer + 4)];
        if (tmp != *(ptrvrambuffer + 1))
        {
            *(vram + 1) = tmp;
            *(ptrvrambuffer + 1) = tmp;
        }

        tmp = ptrlut16colors[*(ptrbackbuffer + 8)];
        if (tmp != *(ptrvrambuffer + 2))
        {
            *(vram + 2) = tmp;
            *(ptrvrambuffer + 2) = tmp;
        }

        tmp = ptrlut16colors[*(ptrbackbuffer + 12)];
        if (tmp != *(ptrvrambuffer + 3))
        {
            *(vram + 3) = tmp;
            *(ptrvrambuffer + 3) = tmp;
        }

        vram += 4;
        ptrvrambuffer += 4;
        ptrbackbuffer += 16;
    } while (vram < (unsigned char *)0xA3E80);
}
#endif

#if defined(MODE_V2)
void V2_DrawBackbuffer(void)
{
    byte *ptrdestscreen;
    int pos;

    outp(SC_INDEX + 1, 1 << 0);

    ptrdestscreen = destscreen + 15 * 80 + 15;
    pos = 319;

    do
    {
        unsigned char x;

        for (x = 0; x < 10; x++)
        {
            ptrdestscreen[0] = backbuffer[pos];
            ptrdestscreen[1] = backbuffer[pos + 1280];
            ptrdestscreen[2] = backbuffer[pos + 2560];
            ptrdestscreen[3] = backbuffer[pos + 3840];
            ptrdestscreen[4] = backbuffer[pos + 5120];

            pos += 1280 * 5;
            ptrdestscreen += 5;
        }

        pos -= (1280 * 50 + 1);
        ptrdestscreen += 30;
    } while (pos > -1);

    outp(SC_INDEX + 1, 1 << 1);

    ptrdestscreen = destscreen + 15 * 80 + 15;
    pos = 639;

    do
    {
        unsigned char x;

        for (x = 0; x < 10; x++)
        {
            ptrdestscreen[0] = backbuffer[pos];
            ptrdestscreen[1] = backbuffer[pos + 1280];
            ptrdestscreen[2] = backbuffer[pos + 2560];
            ptrdestscreen[3] = backbuffer[pos + 3840];
            ptrdestscreen[4] = backbuffer[pos + 5120];

            pos += 1280 * 5;
            ptrdestscreen += 5;
        }

        pos -= (1280 * 50 + 1);
        ptrdestscreen += 30;
    } while (pos > 319);

    outp(SC_INDEX + 1, 1 << 2);

    ptrdestscreen = destscreen + 15 * 80 + 15;
    pos = 959;

    do
    {
        unsigned char x;

        for (x = 0; x < 10; x++)
        {
            ptrdestscreen[0] = backbuffer[pos];
            ptrdestscreen[1] = backbuffer[pos + 1280];
            ptrdestscreen[2] = backbuffer[pos + 2560];
            ptrdestscreen[3] = backbuffer[pos + 3840];
            ptrdestscreen[4] = backbuffer[pos + 5120];

            pos += 1280 * 5;
            ptrdestscreen += 5;
        }

        pos -= (1280 * 50 + 1);
        ptrdestscreen += 30;
    } while (pos > 639);

    outp(SC_INDEX + 1, 1 << 3);

    ptrdestscreen = destscreen + 15 * 80 + 15;
    pos = 1279;

    do
    {
        unsigned char x;

        for (x = 0; x < 10; x++)
        {
            ptrdestscreen[0] = backbuffer[pos];
            ptrdestscreen[1] = backbuffer[pos + 1280];
            ptrdestscreen[2] = backbuffer[pos + 2560];
            ptrdestscreen[3] = backbuffer[pos + 3840];
            ptrdestscreen[4] = backbuffer[pos + 5120];

            pos += 1280 * 5;
            ptrdestscreen += 5;
        }

        pos -= (1280 * 50 + 1);
        ptrdestscreen += 30;
    } while (pos > 959);

    outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0xc);

    // Next plane
    if (destscreen == (byte *)0xA7000)
        destscreen = (byte *)0xA0000;
    else
        destscreen += 0x7000;
}
#endif

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
static struct VBE_VbeInfoBlock vbeinfo;
static struct VBE_ModeInfoBlock vbemode;
unsigned short vesavideomode = 0xFFFF;
int vesalinear = -1;
char *vesavideoptr;
#endif

void I_FinishUpdate(void)
{
    static int fps_counter, fps_starttime, fps_nextcalculation;
    int opt1, opt2;

#if !defined(MODE_HERC) && !defined(MODE_MDA)
    if (waitVsync)
        I_WaitSingleVBL();
#endif

#if defined(MODE_MDA)
    CopyDWords(backbuffer, 0xB0000, 1000);
#endif

#if defined(MODE_T4025) || defined(MODE_T4050)
    // Change video page
    regs.h.ah = 0x05;
    regs.h.al = textpage;
    regs.h.bh = 0x00;
    regs.h.bl = 0x00;
    int386(0x10, &regs, &regs);

    if (textpage == 2)
    {
        textpage = 0;
        textdestscreen = (unsigned short *)0xB8000;
    }
    else
    {
        textpage++;
        textdestscreen += 1024;
    }
#endif

#if defined(MODE_T8025)
    // Change video page
    regs.h.ah = 0x05;
    regs.h.al = textpage;
    regs.h.bh = 0x00;
    regs.h.bl = 0x00;
    int386(0x10, &regs, &regs);

    if (textpage == 2)
    {
        textpage = 0;
        textdestscreen = (unsigned short *)0xB8000;
    }
    else
    {
        textpage++;
        textdestscreen += 2048;
    }
#endif

#if defined(MODE_T8043) || defined(MODE_T8086)
    // Change video page
    regs.h.ah = 0x05;
    regs.h.al = textpage;
    regs.h.bh = 0x00;
    regs.h.bl = 0x00;
    int386(0x10, &regs, &regs);

    if (textpage == 2)
    {
        textpage = 0;
        textdestscreen = (unsigned short *)0xB8000;
    }
    else
    {
        textpage++;
        textdestscreen += 3568;
    }
#endif

#if defined(MODE_T8050) || defined(MODE_T80100)
    // Change video page
    regs.h.ah = 0x05;
    regs.h.al = textpage;
    regs.h.bh = 0x00;
    regs.h.bl = 0x00;
    int386(0x10, &regs, &regs);

    if (textpage == 2)
    {
        textpage = 0;
        textdestscreen = (unsigned short *)0xB8000;
    }
    else
    {
        textpage++;
        if (videoPageFix)
            textdestscreen += 4000;
        else
            textdestscreen += 4128;
    }
#endif
#if defined(MODE_Y)
    outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0xc);

    // Next plane
    if (destscreen == (byte *)0xA8000)
        destscreen = (byte *)0xA0000;
    else
        destscreen += 0x4000;
#endif
#if defined(MODE_VBE2_DIRECT)
    VBE_SetDisplayStart_Y(page);

    if (page == 400)
    {
        page = 0;
        destscreen -= 2 * 320 * 200;
    }
    else
    {
        page += 200;
        destscreen += 320 * 200;
    }
#endif
#if defined(MODE_13H)
    VGA_13H_DrawBackbuffer();
#endif

#if defined(MODE_VBE2)
    if (updatestate & I_FULLSCRN)
    {
        CopyDWords(backbuffer, pcscreen, SCREENHEIGHT * SCREENWIDTH / 4);
        updatestate = I_NOUPDATE; // clear out all draw types
    }
    if (updatestate & I_FULLVIEW)
    {
        if (updatestate & I_MESSAGES && screenblocks > 7)
        {
            int i;
            for (i = 0; i < endscreen; i += SCREENWIDTH)
            {
                CopyDWords(backbuffer + i, pcscreen + i, SCREENWIDTH / 4);
            }
            updatestate &= ~(I_FULLVIEW | I_MESSAGES);
        }
        else
        {
            int i;
            for (i = startscreen; i < endscreen; i += SCREENWIDTH)
            {
                CopyDWords(backbuffer + i, pcscreen + i, SCREENWIDTH / 4);
            }
            updatestate &= ~I_FULLVIEW;
        }
    }
    if (updatestate & I_STATBAR)
    {
        CopyDWords(backbuffer + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), pcscreen + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), SCREENWIDTH * SBARHEIGHT / 4);
        updatestate &= ~I_STATBAR;
    }
    if (updatestate & I_MESSAGES)
    {
        CopyDWords(backbuffer, pcscreen, (SCREENWIDTH * 28) / 4);
        updatestate &= ~I_MESSAGES;
    }
#endif
#if defined(MODE_HERC)
    HERC_DrawBackbuffer();
#endif
#if defined(MODE_CGA)
    CGA_DrawBackbuffer();
#endif
#if defined(MODE_CGA_BW)
    CGA_BW_DrawBackbuffer();
#endif
#if defined(MODE_CGA16)
    if (snowfix)
        CGA_16_DrawBackbuffer_Snow();
    else
        CGA_16_DrawBackbuffer();
#endif
#if defined(MODE_CGA_AFH)
    if (snowfix)
        CGA_AFH_DrawBackbuffer_Snow();
    else
        CGA_AFH_DrawBackbuffer();
#endif
#if defined(MODE_EGA16)
    EGA_16_DrawBackbuffer();
#endif
#if defined(MODE_EGA80)
    EGA80_DrawBackbuffer();
#endif
#if defined(MODE_EGA)
    EGA_DrawBackbuffer();
#endif
#if defined(MODE_EGAW1)
    EGAW1_DrawBackbuffer();
#endif
#if defined(MODE_VGA16)
    VGA_16_DrawBackbuffer();
#endif
#if defined(MODE_EGA640)
    EGA_640_DrawBackbuffer();
#endif
#if defined(MODE_ATI640)
    ATI640_DrawBackbuffer();
#endif
#if defined(MODE_PCP)
    PCP_DrawBackbuffer();
#endif
#if defined(MODE_CVB)
    CGA_CVBS_DrawBackbuffer();
#endif
#if defined(MODE_V2)
    V2_DrawBackbuffer();
#endif

    if (showFPS)
    {
        if (fps_counter == 0)
        {
            fps_starttime = ticcount;
        }

        fps_counter++;

        // store a value and/or draw when data is ok:
        if (fps_counter > (TICRATE * 2) && fps_nextcalculation < ticcount)
        {
            // in case of a very fast system, this will limit the sampling
            // minus 1!, exactly 35 FPS when measeraring for a longer time.
            opt1 = Mul35(fps_counter - 1) << FRACBITS;
            opt2 = (ticcount - fps_starttime) << FRACBITS;
            fps = (opt1 >> 14 >= opt2) ? ((opt1 ^ opt2) >> 31) ^ MAXINT : FixedDiv2(opt1, opt2);
            fps_nextcalculation = ticcount + 12;
            fps_counter = 0; // flush old data
        }
    }
}

// Test VGA REP OUTSB capability
#if defined(MODE_Y) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
void I_TestFastSetPalette(void)
{
    if (!VGADACfix)
    {
        byte test_palette[768];
        unsigned short x;
        byte y;

        // Initialize test palette
        for (x = 0; x < 768; x++)
        {
            test_palette[x] = x & 63;
        }

        // Write test palette using REP STOSB
        outp(PEL_WRITE_ADR, 0);
        OutString(PEL_DATA, test_palette, 768);

        // Read palette from VGA card
        // and compare results
        outp(PEL_READ_ADR, 0);
        for (x = 0; x < 768; x++)
        {
            byte read_data = inp(PEL_DATA);

            if (read_data != test_palette[x])
            {
                VGADACfix = true;
                return;
            }
        }
    }
}
#endif

//
// I_InitGraphics
//
#ifdef SUPPORTS_HERCULES_AUTOMAP
void I_InitHerculesHalfMode(void)
{
    byte Graph_640x400[12] = {0x03, 0x34, 0x28, 0x2A, 0x47, 0x69, 0x00, 0x64, 0x65, 0x02, 0x03, 0x0A};
    int i;

    outp(0x03BF, Graph_640x400[0]);
    for (i = 0; i < 10; i++)
    {
        outp(0x03B4, i);
        outp(0x03B5, Graph_640x400[i + 1]);
    }
    outp(0x03B8, Graph_640x400[11]);

    SetDWords((byte *)0xB0000, 0, 8192);
}
#endif

void I_InitGraphics(void)
{
#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
    int mode;
#endif

#ifdef SUPPORTS_HERCULES_AUTOMAP
    if (HERCmap)
        I_InitHerculesHalfMode();
#endif

#if defined(MODE_CGA_AFH)
    CGA_AFH_InitGraphics();
#endif

#if defined(MODE_CGA16)
    CGA_16_InitGraphics();
#endif

#if defined(MODE_EGA16)
    EGA_16_InitGraphics();
#endif

#if defined(MODE_VGA16)
    VGA_16_InitGraphics();
#endif

#if defined(MODE_T4025) || defined(MODE_T4050)
    // Set 40x25 color mode
    regs.h.ah = 0x00;
    regs.h.al = 0x01;
    int386(0x10, &regs, &regs);

    // Disable cursor
    regs.h.ah = 0x01;
    regs.h.ch = 0x3F;
    int386(0x10, &regs, &regs);

    // CGA Disable blink
    if (CGAcard)
    {
        I_DisableCGABlink();
    }
    else
    {
        // Disable blinking
        regs.h.ah = 0x10;
        regs.h.al = 0x03;
        regs.h.bl = 0x00;
        regs.h.bh = 0x00;
        int386(0x10, &regs, &regs);
    }

    textdestscreen = (unsigned short *)0xB8000;
    textpage = 0;
#endif
#if defined(MODE_T8025)
    // Set 80x25 color mode
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
    int386(0x10, &regs, &regs);

    // Disable cursor
    regs.h.ah = 0x01;
    regs.h.ch = 0x3F;
    int386(0x10, &regs, &regs);

    // CGA Disable blink
    if (CGAcard)
    {
        I_DisableCGABlink();
    }
    else
    {
        // Disable blinking
        regs.h.ah = 0x10;
        regs.h.al = 0x03;
        regs.h.bl = 0x00;
        regs.h.bh = 0x00;
        int386(0x10, &regs, &regs);
    }

    textdestscreen = (unsigned short *)0xB8000;
    textpage = 0;
#endif
#if defined(MODE_MDA)
    // Set 80x25 color mode
    regs.h.ah = 0x00;
    regs.h.al = 0x07;
    int386(0x10, &regs, &regs);

    // Disable MDA blink
    I_DisableMDABlink();
#endif
#if defined(MODE_T8050) || defined(MODE_T80100) || defined(MODE_T8043) || defined(MODE_T8086)
    // Set 80x25 color mode
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
    int386(0x10, &regs, &regs);

    // Change font size to 8x8
    regs.h.ah = 0x11;
    regs.h.al = 0x12;
    regs.h.bh = 0;
    regs.h.bl = 0;
    int386(0x10, &regs, &regs);

    // Disable cursor
    regs.h.ah = 0x01;
    regs.h.ch = 0x3F;
    int386(0x10, &regs, &regs);

    // Disable blinking
    regs.h.ah = 0x10;
    regs.h.al = 0x03;
    regs.h.bl = 0x00;
    regs.h.bh = 0x00;
    int386(0x10, &regs, &regs);

    textdestscreen = (unsigned short *)0xB8000;
    textpage = 0;
#endif
#if defined(MODE_Y)
    regs.w.ax = 0x13;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = (byte *)0xA0000;
    currentscreen = (unsigned short *)0xA0000;
    destscreen = (byte *)0xA4000;

    outp(SC_INDEX, SC_MEMMODE);
    outp(SC_INDEX + 1, (inp(SC_INDEX + 1) & ~8) | 4);
    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1) & ~0x13);
    outp(GC_INDEX, GC_MISCELLANEOUS);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1) & ~2);
    outpw(SC_INDEX, 0xf02);
    SetDWords(pcscreen, 0, 0x4000);
    outp(CRTC_INDEX, CRTC_UNDERLINE);
    outp(CRTC_INDEX + 1, inp(CRTC_INDEX + 1) & ~0x40);
    outp(CRTC_INDEX, CRTC_MODE);
    outp(CRTC_INDEX + 1, inp(CRTC_INDEX + 1) | 0x40);
    outp(GC_INDEX, GC_READMAP);
#endif

#if defined(MODE_V2)
    regs.w.ax = 0x13;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = (byte *)0xA0000;
    currentscreen = (unsigned short *)0xA0000;
    destscreen = (byte *)0xA7000;

    //
    // switch to linear, non-chain4 mode
    //
    outp(SC_INDEX, SYNC_RESET);
    outp(SC_DATA, 1);

    outp(SC_INDEX, MEMORY_MODE);
    outp(SC_DATA, (inp(SC_DATA) & ~0x08) | 0x04);
    outp(GC_INDEX, GRAPHICS_MODE);
    outp(GC_DATA, (inp(GC_DATA) & ~0x10) | 0x00);
    outp(GC_INDEX, MISCELLANOUS);
    outp(GC_DATA, (inp(GC_DATA) & ~0x02) | 0x00);

    outpw(SC_INDEX, 0xf02);
    SetDWords(pcscreen, 0, 0x4000);

    outp(MISC_OUTPUT, 0xA3); // 350-scan-line scan rate

    outp(SC_INDEX, SYNC_RESET);
    outp(SC_DATA, 3);

    //
    // unprotect CRTC0 through CRTC0
    //
    outp(CRTC_INDEX, 0x11);
    outp(CRTC_DATA, (inp(CRTC_DATA) & ~0x80) | 0x00);

    //
    // stop scanning each line twice
    //
    outp(CRTC_INDEX, MAX_SCAN_LINE);
    outp(CRTC_DATA, (inp(CRTC_DATA) & ~0x1F) | 0x00);

    //
    // change the CRTC from doubleword to byte mode
    //
    outp(CRTC_INDEX, UNDERLINE);
    outp(CRTC_DATA, (inp(CRTC_DATA) & ~0x40) | 0x00);
    outp(CRTC_INDEX, MODE_CONTROL);
    outp(CRTC_DATA, (inp(CRTC_DATA) & ~0x00) | 0x40);

    //
    // set the vertical counts for 350-scan-line mode
    //
    outp(CRTC_INDEX, 0x06);
    outp(CRTC_INDEX + 1, 0xBF);
    outp(CRTC_INDEX, 0x07);
    outp(CRTC_INDEX + 1, 0x1F);
    outp(CRTC_INDEX, 0x10);
    outp(CRTC_INDEX + 1, 0x83);
    outp(CRTC_INDEX, 0x11);
    outp(CRTC_INDEX + 1, 0x85);
    outp(CRTC_INDEX, 0x12);
    outp(CRTC_INDEX + 1, 0x5D);
    outp(CRTC_INDEX, 0x15);
    outp(CRTC_INDEX + 1, 0x63);
    outp(CRTC_INDEX, 0x16);
    outp(CRTC_INDEX + 1, 0xBA);

    outp(SC_INDEX, MAP_MASK);
    outp(GC_INDEX, READ_MAP);
#endif
#if defined(MODE_13H)
    VGA_13H_InitGraphics();
#endif
#if defined(MODE_CGA)
    CGA_InitGraphics();
#endif

#if defined(MODE_CGA_BW)
    CGA_BW_InitGraphics();
#endif

#if defined(MODE_PCP)
    PCP_InitGraphics();
#endif

#if defined(MODE_EGA)
    {
        unsigned int pos1 = 0;
        unsigned int pos2 = 0;
        unsigned int pos3 = 0;
        unsigned int counter = 0;
        byte *basevram;

        regs.w.ax = 0x0E;
        int386(0x10, (union REGS *)&regs, &regs);
        pcscreen = destscreen = (byte *)0xA0000;

        basevram = (byte *)0xA3E80; // Init at ending of viewable screen

        // Step 1
        // Copy all possible combinations to the VRAM

        outp(0x3C4, 0x02);
        for (pos1 = 0; pos1 < 16; pos1++)
        {
            for (pos2 = 0; pos2 < 16; pos2++)
            {
                for (pos3 = 0; pos3 < 16; pos3++)
                {
                    for (counter = 0; counter < 4; counter++)
                    {
                        byte bitstatuspos1;
                        byte bitstatuspos2;
                        byte bitstatuspos3;

                        byte final;

                        outp(0x3C5, 1 << counter); // Change plane

                        bitstatuspos1 = (pos1 >> counter) & 1;
                        bitstatuspos2 = (pos2 >> counter) & 1;
                        bitstatuspos3 = (pos3 >> counter) & 1;

                        final = bitstatuspos1 << 6 | bitstatuspos1 << 7 |
                                bitstatuspos2 << 4 | bitstatuspos2 << 5 |
                                bitstatuspos3 << 2 | bitstatuspos3 << 3;
                        *basevram = final;
                    }
                    basevram++;
                }
            }
        }

        // Step 2

        // Write Mode 2
        outp(0x3CE, 0x05);
        outp(0x3CF, 0x02);

        // Write to all 4 planes
        outp(0x3C4, 0x02);
        outp(0x3C5, 0x0F);

        // Set Bit Mask to use the latch registers
        outp(0x3CE, 0x08);
        outp(0x3CF, 0x03);

        // Set logical operation to OR
        outp(0x3CE, 0x03);
        outp(0x3CF, 0x10);
    }
#endif
#if defined(MODE_EGAW1)
    {
        unsigned int pos1 = 0;
        unsigned int pos2 = 0;
        unsigned int pos3 = 0;
        unsigned int counter = 0;
        byte *basevram;

        regs.w.ax = 0x0D;
        int386(0x10, (union REGS *)&regs, &regs);
        pcscreen = destscreen = (byte *)0xA0000;

        basevram = (byte *)0xA1F40; // Init at ending of viewable screen

        // Step 1
        // Copy all possible combinations to the VRAM

        outp(0x3C4, 0x02);
        for (pos1 = 0; pos1 < 16; pos1++)
        {
            for (pos2 = 0; pos2 < 16; pos2++)
            {
                for (pos3 = 0; pos3 < 16; pos3++)
                {
                    for (counter = 0; counter < 4; counter++)
                    {
                        byte bitstatuspos1;
                        byte bitstatuspos2;
                        byte bitstatuspos3;

                        byte final;

                        outp(0x3C5, 1 << counter); // Change plane

                        bitstatuspos1 = (pos1 >> counter) & 1;
                        bitstatuspos2 = (pos2 >> counter) & 1;
                        bitstatuspos3 = (pos3 >> counter) & 1;

                        final = bitstatuspos1 << 6 | bitstatuspos1 << 7 |
                                bitstatuspos2 << 4 | bitstatuspos2 << 5 |
                                bitstatuspos3 << 2 | bitstatuspos3 << 3;
                        *basevram = final;
                    }
                    basevram++;
                }
            }
        }

        // Step 2

        // Write Mode 2
        outp(0x3CE, 0x05);
        outp(0x3CF, 0x02);

        // Write to all 4 planes
        outp(0x3C4, 0x02);
        outp(0x3C5, 0x0F);

        // Set Bit Mask to use the latch registers
        outp(0x3CE, 0x08);
        outp(0x3CF, 0x03);

        // Set logical operation to OR
        outp(0x3CE, 0x03);
        outp(0x3CF, 0x10);
    }
#endif
#if defined(MODE_EGA80)
    regs.w.ax = 0x0E;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xA0000;

    // Write Mode 2
    outp(0x3CE, 0x05);
    outp(0x3CF, 0x02);

    // Write to all 4 planes
    outp(0x3C4, 0x02);
    outp(0x3C5, 0x0F);

    // Set Bit Mask to omit the latch registers
    outp(0x3CE, 0x08);
    outp(0x3CF, 0xFF);

    SetDWords(vrambuffer, 0, 4096);
#endif
#if defined(MODE_EGA640)
    EGA_640_InitGraphics();
#endif
#if defined(MODE_ATI640)
    ATI640_InitGraphics();
#endif
#if defined(MODE_CVB)
    CGA_CVBS_InitGraphics();
#endif
#if defined(MODE_HERC)
    HERC_InitGraphics();
#endif

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
    VBE_Init();

    // Get VBE info
    VBE_Controller_Information(&vbeinfo);

    // Get VBE modes
    for (mode = 0; vbeinfo.VideoModePtr[mode] != 0xffff; mode++)
    {
        VBE_Mode_Information(vbeinfo.VideoModePtr[mode], &vbemode);
        if (vbemode.XResolution == 320 && vbemode.YResolution == 200 && vbemode.BitsPerPixel == 8)
        {
            vesavideomode = vbeinfo.VideoModePtr[mode];
            vesalinear = VBE_IsModeLinear(vesavideomode);
            break;
        }
    }

    // If a VESA compatible 320x200 8bpp mode is found, use it!
    if (vesavideomode != 0xFFFF)
    {
        VBE_SetMode(vesavideomode, vesalinear, 1);

        if (vesalinear == 1)
        {
            pcscreen = destscreen = VBE_GetVideoPtr(vesavideomode);
        }
        else
        {
            pcscreen = destscreen = (char *)0xA0000;
        }

        // Force 6 bits resolution per color
        VBE_SetDACWidth(6);
    }
    else
    {
        I_Error("Compatible VESA 2.0 video mode not found! (320x200 8bpp required)");
    }
#endif

#if defined(MODE_Y) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
    I_TestFastSetPalette();
#endif

#if defined(MODE_13H)
    VGA_TestFastSetPalette();
#endif

    I_ProcessPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
    I_SetPalette(0);
}

//
// I_ShutdownGraphics
//
void I_ShutdownGraphics(void)
{
#if defined(MODE_HERC)
    HERC_ShutdownGraphics();
#endif

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
    VBE_Done();
#endif

    regs.w.ax = 3;
    int386(0x10, &regs, &regs); // back to text mode
}

//
// I_StartTic
//
// called by D_DoomLoop
// called before processing each tic in a frame
// can call D_PostEvent
// asyncronous interrupt functions should maintain private ques that are
// read by the syncronous functions to be converted into events
//

#define SC_UPARROW 0x48
#define SC_DOWNARROW 0x50
#define SC_LEFTARROW 0x4b
#define SC_RIGHTARROW 0x4d

void I_StartTic(void)
{
    int k;
    event_t ev;

    if (mousepresent)
        I_ReadMouse();

    //
    // keyboard events
    //
    while (kbdtail < kbdhead)
    {
        k = keyboardque[kbdtail & (KBDQUESIZE - 1)];
        kbdtail++;

        // extended keyboard shift key bullshit
        if ((k & 0x7f) == SC_LSHIFT || (k & 0x7f) == SC_RSHIFT)
        {
            if (keyboardque[(kbdtail - 2) & (KBDQUESIZE - 1)] == 0xe0)
            {
                continue;
            }
            k &= 0x80;
            k |= SC_RSHIFT;
        }

        if (k == 0xe0 || keyboardque[(kbdtail - 2) & (KBDQUESIZE - 1)] == 0xe1)
        {
            continue; // special / pause keys, pause key bullshit
        }

        if (k == 0xc5 && keyboardque[(kbdtail - 2) & (KBDQUESIZE - 1)] == 0x9d)
        {
            ev.type = ev_keydown;
            ev.data1 = KEY_PAUSE;
            D_PostEvent(&ev);
            continue;
        }

        ev.type = (k & 0x80) != 0;
        k &= 0x7f;

        switch (k)
        {
        case SC_UPARROW:
            ev.data1 = KEY_UPARROW;
            break;
        case SC_DOWNARROW:
            ev.data1 = KEY_DOWNARROW;
            break;
        case SC_LEFTARROW:
            ev.data1 = KEY_LEFTARROW;
            break;
        case SC_RIGHTARROW:
            ev.data1 = KEY_RIGHTARROW;
            break;
        default:
            ev.data1 = scantokey[k];
            break;
        }
        D_PostEvent(&ev);
    }
}

//
// Timer interrupt
//

//
// I_TimerISR
//
void I_TimerISR(task *task)
{
    ticcount++;
}

//
// Keyboard
//

void(__interrupt __far *oldkeyboardisr)() = NULL;

//
// I_KeyboardISR
//

void __interrupt I_KeyboardISR(void)
{
    // Get the scan code

    keyboardque[kbdhead & (KBDQUESIZE - 1)] = inp(0x60);
    kbdhead++;

    // acknowledge the interrupt

    outp(0x20, 0x20);
}

//
// I_StartupKeyboard
//
void I_StartupKeyboard(void)
{
    oldkeyboardisr = _dos_getvect(KEYBOARDINT);
    _dos_setvect(0x8000 | KEYBOARDINT, I_KeyboardISR);
}

void I_ShutdownKeyboard(void)
{
    if (oldkeyboardisr)
        _dos_setvect(KEYBOARDINT, oldkeyboardisr);
    *(short *)0x41c = *(short *)0x41a; // clear bios key buffer
}

//
// Mouse
//

int I_ResetMouse(void)
{
    regs.w.ax = 0; // reset
    int386(0x33, &regs, &regs);
    return regs.w.ax;
}

//
// StartupMouse
//

void I_StartupMouse(void)
{
    //
    // General mouse detection
    //
    mousepresent = 0;
    if (M_CheckParm("-nomouse") || !usemouse)
    {
        return;
    }

    if (I_ResetMouse() != 0xffff)
    {
        printf("Mouse: not present\n", 0);
        return;
    }
    printf("Mouse: detected\n", 0);

    mousepresent = 1;
}

//
// ShutdownMouse
//
void I_ShutdownMouse(void)
{
    if (!mousepresent)
    {
        return;
    }

    I_ResetMouse();
}

//
// I_ReadMouse
//
void I_ReadMouse(void)
{
    event_t ev;

    //
    // mouse events
    //

    ev.type = ev_mouse;

    SetBytes(&dpmiregs, 0, sizeof(dpmiregs));
    dpmiregs.eax = 3; // read buttons / position
    DPMIInt(0x33);
    ev.data1 = dpmiregs.ebx;

    dpmiregs.eax = 11; // read counters
    DPMIInt(0x33);
    ev.data2 = (short)dpmiregs.ecx;

    D_PostEvent(&ev);
}

//
// DPMI stuff
//

#define REALSTACKSIZE 1024

dpmiregs_t dpmiregs;

unsigned realstackseg;

//
// I_StartupDPMI
//
byte *I_AllocLow(int length);

void I_StartupDPMI(void)
{
    extern char __begtext;
    extern char ___Argc;

    //
    // allocate a decent stack for real mode ISRs
    //
    realstackseg = (int)I_AllocLow(1024) >> 4;

    //
    // lock the entire program down
    //

    DPMI_LockMemory(&__begtext, &___Argc - &__begtext);
}

//
// I_Init
// hook interrupts and set graphics mode
//
void I_Init(void)
{
    int p;
    printf("I_StartupDPMI\n");
    I_StartupDPMI();
    printf("I_StartupMouse\n");
    I_StartupMouse();
    printf("I_StartupKeyboard\n");
    I_StartupKeyboard();
    printf("I_StartupSound\n");
    I_StartupSound();
}

//
// I_Shutdown
// return to default system state
//
void I_Shutdown(void)
{
    I_ShutdownGraphics();
    I_ShutdownSound();
    I_ShutdownTimer();
    I_ShutdownMouse();
    I_ShutdownKeyboard();
}

//
// I_Error
//
void I_Error(char *error, ...)
{
    va_list argptr;

    I_Shutdown();
    va_start(argptr, error);
    vprintf(error, argptr);
    va_end(argptr);
    printf("\n");
    exit(1);
}

//
// I_Quit
//
// Shuts down net game, saves defaults, prints the exit text message,
// goes to text mode, and exits.
//
void I_Quit(void)
{
    byte *scr;

    if (demorecording)
    {
        G_CheckDemoStatus();
    }

    M_SaveDefaults();
    scr = (byte *)W_CacheLumpName("ENDOOM", PU_CACHE);
    I_Shutdown();
#if defined(MODE_HERC) || defined(MODE_MDA)
    CopyDWords(scr, (void *)0xb0000, (80 * 25 * 2) / 4);
#else
    CopyDWords(scr, (void *)0xb8000, (80 * 25 * 2) / 4);
#endif
    regs.w.ax = 0x0200;
    regs.h.bh = 0;
    regs.h.dl = 0;
    regs.h.dh = 23;
    int386(0x10, (union REGS *)&regs, &regs); // Set text pos
    printf("\n");

    exit(0);
}

//
// I_ZoneBase
//
byte *I_ZoneBase(int *size)
{
    int meminfo[32];
    int heap;
    byte *ptr;

    SetDWords(meminfo, 0, sizeof(meminfo) / 4);
    segread(&segregs);
    segregs.es = segregs.ds;
    regs.w.ax = 0x500; // get memory info
    regs.x.edi = (int)&meminfo;
    int386x(0x31, &regs, &regs, &segregs);

    heap = meminfo[0];
    printf("DPMI memory: %d Kb", heap >> 10);

    do
    {
        heap -= 0x60000; // leave 384kb alone
        if (heap > 0x800000 && !unlimitedRAM)
        {
            heap = 0x800000;
        }
        ptr = malloc(heap);
    } while (!ptr);

    printf(", %d Kb allocated for zone\n", heap >> 10);

    *size = heap;
    return ptr;
}

//
// I_AllocLow
//
byte *I_AllocLow(int length)
{
    byte *mem;

    // DPMI call 100h allocates DOS memory
    segread(&segregs);
    regs.w.ax = 0x0100; // DPMI allocate DOS memory
    regs.w.bx = (length + 15) / 16;
    int386(DPMI_INT, &regs, &regs);
    // segment = regs.w.ax;
    // selector = regs.w.dx;

    mem = (void *)((regs.x.eax & 0xFFFF) << 4);

    memset(mem, 0, length);
    return mem;
}

//
// DPMIInt
//
void DPMIInt(int i)
{
    dpmiregs.ss = realstackseg;
    dpmiregs.sp = REALSTACKSIZE - 4;

    segread(&segregs);
    regs.w.ax = 0x300;
    regs.w.bx = i;
    regs.w.cx = 0;
    regs.x.edi = (unsigned)&dpmiregs;
    segregs.es = segregs.ds;
    int386x(DPMI_INT, &regs, &regs, &segregs);
}
