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

#define BYTE0_USHORT(value) (((unsigned char *)&value)[0])
#define BYTE1_USHORT(value) (((unsigned char *)&value)[1])

#define BYTE0_UINT(value) (((unsigned char *)&value)[0])
#define BYTE1_UINT(value) (((unsigned char *)&value)[1])
#define BYTE2_UINT(value) (((unsigned char *)&value)[2])
#define BYTE3_UINT(value) (((unsigned char *)&value)[3])

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

void I_StartupNet(void);
void I_ShutdownNet(void);
void I_ReadExternDriver(void);

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

#define CRTCOFF (_inbyte(STATUS_REGISTER_1) & 1)
#define CLI _disable()
#define STI _enable()

#define _outbyte(x, y) (outp(x, y))
#define _outhword(x, y) (outpw(x, y))

#define _inbyte(x) (inp(x))
#define _inhword(x) (inpw(x))

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

#if defined(MODE_VGA16) || defined(MODE_CGA16) || defined(MODE_EGA16) || defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_CVB) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA10)
byte lut16colors[14 * 256];
byte *ptrlut16colors;
#endif

#if defined(MODE_PCP)
unsigned short lut16colors[14 * 256];
unsigned short *ptrlut16colors;
#endif

#if defined(MODE_ATI640)
unsigned short lutcolors[14 * 512];
unsigned short *ptrlutcolors;
#endif

#if defined(MODE_CGA136) || defined(MODE_VGA136) || defined(MODE_EGA136)
byte lut136colors[14 * 256];
byte *ptrlut136colors;
#endif

#if defined(MODE_EGA)
byte lutcolor[4][14 * 256];
unsigned int pospalette;
#endif

#if defined(MODE_CGA)
byte lut4colors[14 * 256];
byte *ptrlut4colors;
#endif

#if defined(MODE_HERC)
byte lutcolors[14 * 1024];
byte *ptrlutcolors;
#endif

#if defined(MODE_CGA_BW) || defined(MODE_EGA640)
byte lutcolors[14 * 512];
byte *ptrlutcolors;
#endif

byte gammatable[5][256] =
    {
        {0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42, 42, 43, 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47, 47, 47, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51, 52, 52, 52, 52, 53, 53, 53, 53, 54, 54, 54, 54, 55, 55, 55, 55, 56, 56, 56, 56, 57, 57, 57, 57, 58, 58, 58, 58, 59, 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63},
        {0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 37, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42, 42, 43, 43, 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 46, 47, 47, 47, 47, 48, 48, 48, 48, 49, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51, 51, 52, 52, 52, 52, 53, 53, 53, 53, 53, 54, 54, 54, 54, 55, 55, 55, 55, 55, 56, 56, 56, 56, 57, 57, 57, 57, 57, 58, 58, 58, 58, 59, 59, 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63, 63},
        {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 36, 36, 37, 37, 37, 37, 38, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 42, 42, 42, 42, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 45, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58, 59, 59, 59, 59, 59, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63},
        {2, 3, 4, 4, 5, 6, 6, 7, 7, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 37, 38, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 45, 45, 45, 45, 45, 45, 46, 46, 46, 46, 46, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 53, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58, 58, 59, 59, 59, 59, 59, 59, 60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 61, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63},
        {4, 5, 7, 8, 9, 9, 10, 11, 12, 12, 13, 13, 14, 15, 15, 16, 16, 17, 17, 17, 18, 18, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 32, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 37, 38, 38, 38, 38, 38, 39, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 45, 45, 45, 45, 45, 45, 46, 46, 46, 46, 46, 46, 47, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 49, 50, 50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 52, 52, 53, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58, 58, 58, 58, 59, 59, 59, 59, 59, 59, 59, 60, 60, 60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 61, 61, 62, 62, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63, 63}};

#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
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

#if defined(MODE_HERC)
byte vrambuffer[32768];
#endif

#if defined(MODE_CGA16) || defined(MODE_CGA136) || defined(MODE_EGA16) || defined(MODE_EGA136) || defined(MODE_EGA80) || defined(MODE_EGAW1)
byte vrambuffer[16384];
#endif

#if defined(MODE_EGA10)
unsigned short vrambuffer[16384];
#endif

#ifdef MODE_CGA_BW
unsigned short vrambuffer[16384];
#endif

#ifdef MODE_EGA
unsigned short vrambuffer[4][3][8192]; // Color, Plane, VRAM
#endif

#ifdef MODE_EGA640
byte vrambufferR1[16384];
byte vrambufferG1[16384];
byte vrambufferB1[16384];
byte vrambufferI1[16384];
byte vrambufferR2[16384];
byte vrambufferG2[16384];
byte vrambufferB2[16384];
byte vrambufferI2[16384];
byte vrambufferR3[16384];
byte vrambufferG3[16384];
byte vrambufferB3[16384];
byte vrambufferI3[16384];
#endif

#ifdef MODE_PCP
byte vrambuffer[32768];
#endif

#ifdef MODE_CGA
unsigned short vrambuffer[16384];
#endif

#ifdef MODE_ATI640
byte vrambuffer[65536];
#endif

#ifdef MODE_CVB
byte vrambuffer[16384];
#endif

#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
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

#if defined(MODE_CGA_BW)
void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 512; i += 2, palette += 3)
    {
        byte r, g, b;

        r = ptr[*palette];
        g = ptr[*(palette + 1)];
        b = ptr[*(palette + 2)];

        if (r + g + b > 32)
        {
            lutcolors[i] = 0xFF;
        }
        else
        {
            lutcolors[i] = 0x00;
        }

        if (r + g + b > 64)
        {
            lutcolors[i + 1] = 0xFF;
        }
        else
        {
            lutcolors[i + 1] = 0x00;
        }
    }
}
#endif

#if defined(MODE_HERC)
void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 1024; i += 4, palette += 3)
    {
        byte r, g, b;

        r = ptr[*palette];
        g = ptr[*(palette + 1)];
        b = ptr[*(palette + 2)];

        if (r + g + b > 38)
        {
            lutcolors[i] = 0xFF;
        }
        else
        {
            lutcolors[i] = 0x00;
        }

        if (r + g + b > 115)
        {
            lutcolors[i + 1] = 0xFF;
        }
        else
        {
            lutcolors[i + 1] = 0x00;
        }

        if (r + g + b > 155)
        {
            lutcolors[i + 2] = 0xFF;
        }
        else
        {
            lutcolors[i + 2] = 0x00;
        }

        if (r + g + b > 77)
        {
            lutcolors[i + 3] = 0xFF;
        }
        else
        {
            lutcolors[i + 3] = 0x00;
        }
    }
}
#endif

#if defined(MODE_VGA16) || defined(MODE_CGA16) || defined(MODE_EGA16) || defined(MODE_EGA640) || defined(MODE_EGA) || defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_EGA80) || defined(MODE_EGAW1)
const byte colors[48] = {
    0x00, 0x00, 0x00, // 0
    0x00, 0x00, 0x2A, // 1
    0x00, 0x2A, 0x00, // 2
    0x00, 0x2A, 0x2A, // 3
    0x2A, 0x00, 0x00, // 4
    0x2A, 0x00, 0x2A, // 5
    0x2A, 0x15, 0x00, // 6
    0x2A, 0x2A, 0x2A, // 7
    0x15, 0x15, 0x15, // 8
    0x15, 0x15, 0x3F, // 9
    0x15, 0x3F, 0x15, // 10
    0x15, 0x3F, 0x3F, // 11
    0x3F, 0x15, 0x15, // 12
    0x3F, 0x15, 0x3F, // 13
    0x3F, 0x3F, 0x15, // 14
    0x3F, 0x3F, 0x3F}; // 15
#endif

#if defined(MODE_EGA10)
const byte colors[30] = {
    0x00, 0x00, 0x00, // 0
    0x00, 0x00, 0x2A, // 1
    0x00, 0x2A, 0x00, // 2
    0x00, 0x2A, 0x2A, // 3
    0x2A, 0x00, 0x00, // 4
    0x2A, 0x15, 0x00, // 6
    0x2A, 0x2A, 0x2A, // 7 
    0x15, 0x15, 0x15, // 8
    0x15, 0x15, 0x3F, // 9
    0x3F, 0x3F, 0x15}; // 14
#endif

#if defined(MODE_CGA136) || defined(MODE_VGA136) || defined(MODE_EGA136)
const byte colors[4 * 122] = {
    0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x1E,
    0x02, 0x00, 0x1E, 0x00,
    0x03, 0x00, 0x1E, 0x1E,
    0x04, 0x1E, 0x00, 0x00,
    0x05, 0x1E, 0x00, 0x1E,
    0x06, 0x1E, 0x0F, 0x00,
    0x07, 0x1E, 0x1E, 0x1E,
    0x08, 0x0F, 0x0F, 0x0F,
    0x09, 0x0F, 0x0F, 0x2D,
    0x0A, 0x0F, 0x2D, 0x0F,
    0x0B, 0x0F, 0x2D, 0x2D,
    0x0C, 0x2D, 0x0F, 0x0F,
    0x0D, 0x2D, 0x0F, 0x2D,
    0x0E, 0x2D, 0x2D, 0x0F,
    0x0F, 0x2D, 0x2D, 0x2D,
    0x11, 0x00, 0x00, 0x2A,
    0x13, 0x00, 0x1E, 0x2A,
    0x15, 0x1E, 0x00, 0x2A,
    0x16, 0x1E, 0x0F, 0x1E,
    0x17, 0x1E, 0x1E, 0x2A,
    0x18, 0x0F, 0x0F, 0x21,
    0x19, 0x0F, 0x0F, 0x36,
    0x1A, 0x0F, 0x2D, 0x21,
    0x1B, 0x0F, 0x2D, 0x36,
    0x1C, 0x2D, 0x0F, 0x21,
    0x1D, 0x2D, 0x0F, 0x36,
    0x1E, 0x2D, 0x2D, 0x21,
    0x1F, 0x2D, 0x2D, 0x36,
    0x22, 0x00, 0x2A, 0x00,
    0x23, 0x00, 0x2A, 0x1E,
    0x24, 0x1E, 0x1E, 0x00,
    0x26, 0x1E, 0x21, 0x00,
    0x27, 0x1E, 0x2A, 0x1E,
    0x28, 0x0F, 0x21, 0x0F,
    0x29, 0x0F, 0x21, 0x2D,
    0x2A, 0x0F, 0x36, 0x0F,
    0x2B, 0x0F, 0x36, 0x2D,
    0x2C, 0x2D, 0x21, 0x0F,
    0x2D, 0x2D, 0x21, 0x2D,
    0x2E, 0x2D, 0x36, 0x0F,
    0x2F, 0x2D, 0x36, 0x2D,
    0x33, 0x00, 0x2A, 0x2A,
    0x36, 0x1E, 0x21, 0x1E,
    0x37, 0x1E, 0x2A, 0x2A,
    0x38, 0x0F, 0x21, 0x21,
    0x39, 0x0F, 0x21, 0x36,
    0x3A, 0x0F, 0x36, 0x21,
    0x3B, 0x0F, 0x36, 0x36,
    0x3C, 0x2D, 0x21, 0x21,
    0x3D, 0x2D, 0x21, 0x36,
    0x3E, 0x2D, 0x36, 0x21,
    0x3F, 0x2D, 0x36, 0x36,
    0x44, 0x2A, 0x00, 0x00,
    0x45, 0x2A, 0x00, 0x1E,
    0x46, 0x2A, 0x0F, 0x00,
    0x47, 0x2A, 0x1E, 0x1E,
    0x48, 0x21, 0x0F, 0x0F,
    0x49, 0x21, 0x0F, 0x2D,
    0x4A, 0x21, 0x2D, 0x0F,
    0x4B, 0x21, 0x2D, 0x2D,
    0x4C, 0x36, 0x0F, 0x0F,
    0x4D, 0x36, 0x0F, 0x2D,
    0x4E, 0x36, 0x2D, 0x0F,
    0x4F, 0x36, 0x2D, 0x2D,
    0x55, 0x2A, 0x00, 0x2A,
    0x56, 0x2A, 0x0F, 0x1E,
    0x57, 0x2A, 0x1E, 0x2A,
    0x58, 0x21, 0x0F, 0x21,
    0x59, 0x21, 0x0F, 0x36,
    0x5A, 0x21, 0x2D, 0x21,
    0x5B, 0x21, 0x2D, 0x36,
    0x5C, 0x36, 0x0F, 0x21,
    0x5D, 0x36, 0x0F, 0x36,
    0x5E, 0x36, 0x2D, 0x21,
    0x5F, 0x36, 0x2D, 0x36,
    0x66, 0x2A, 0x15, 0x00,
    0x67, 0x2A, 0x21, 0x1E,
    0x68, 0x21, 0x15, 0x0F,
    0x69, 0x21, 0x15, 0x2D,
    0x6A, 0x21, 0x2F, 0x0F,
    0x6B, 0x21, 0x2F, 0x2D,
    0x6C, 0x36, 0x15, 0x0F,
    0x6D, 0x36, 0x15, 0x2D,
    0x6E, 0x36, 0x2F, 0x0F,
    0x6F, 0x36, 0x2F, 0x2D,
    0x77, 0x2A, 0x2A, 0x2A,
    0x78, 0x21, 0x21, 0x21,
    0x79, 0x21, 0x21, 0x36,
    0x7A, 0x21, 0x36, 0x21,
    0x7B, 0x21, 0x36, 0x36,
    0x7C, 0x36, 0x21, 0x21,
    0x7D, 0x36, 0x21, 0x36,
    0x7E, 0x36, 0x36, 0x21,
    0x7F, 0x36, 0x36, 0x36,
    0x88, 0x15, 0x15, 0x15,
    0x89, 0x15, 0x15, 0x2F,
    0x8A, 0x15, 0x2F, 0x15,
    0x8B, 0x15, 0x2F, 0x2F,
    0x8C, 0x2F, 0x15, 0x15,
    0x8D, 0x2F, 0x15, 0x2F,
    0x8E, 0x2F, 0x2F, 0x15,
    0x8F, 0x2F, 0x2F, 0x2F,
    0x99, 0x15, 0x15, 0x3F,
    0x9B, 0x15, 0x2F, 0x3F,
    0x9D, 0x2F, 0x15, 0x3F,
    0x9F, 0x2F, 0x2F, 0x3F,
    0xAA, 0x15, 0x3F, 0x15,
    0xAB, 0x15, 0x3F, 0x2F,
    0xAE, 0x2F, 0x3F, 0x15,
    0xAF, 0x2F, 0x3F, 0x2F,
    0xBB, 0x15, 0x3F, 0x3F,
    0xBF, 0x2F, 0x3F, 0x3F,
    0xCC, 0x3F, 0x15, 0x15,
    0xCD, 0x3F, 0x15, 0x2F,
    0xCE, 0x3F, 0x2F, 0x15,
    0xCF, 0x3F, 0x2F, 0x2F,
    0xDD, 0x3F, 0x15, 0x3F,
    0xDF, 0x3F, 0x2F, 0x3F,
    0xEE, 0x3F, 0x3F, 0x15,
    0xEF, 0x3F, 0x3F, 0x2F,
    0xFF, 0x3F, 0x3F, 0x3F};
#endif

#if defined(MODE_ATI640)
const byte colors[48] = { // Color      R G B I     G R I B
    0x00, 0x00, 0x00,     // Black      0 0 0 0     0 0 0 0
    0x00, 0x2A, 0x00,     // Green      0 1 0 0     1 0 0 0
    0x2A, 0x00, 0x00,     // Red        1 0 0 0     0 1 0 0
    0x2A, 0x15, 0x00,     // Brown      1 1 0 0     1 1 0 0
    0x15, 0x15, 0x15,     // Gray       0 0 0 1     0 0 1 0
    0x15, 0x3F, 0x15,     // L green    0 1 0 1     1 0 1 0
    0x3F, 0x15, 0x15,     // L red      1 0 0 1     0 1 1 0
    0x3F, 0x3F, 0x15,     // Yellow     1 1 0 1     1 1 1 0
    0x00, 0x00, 0x2A,     // Blue       0 0 1 0     0 0 0 1
    0x00, 0x2A, 0x2A,     // Cyan       0 1 1 0     1 0 0 1
    0x2A, 0x00, 0x2A,     // Magenta    1 0 1 0     0 1 0 1
    0x2A, 0x2A, 0x2A,     // L Gray     1 1 1 0     1 1 0 1
    0x15, 0x15, 0x3F,     // L blue     0 0 1 1     0 0 1 1
    0x15, 0x3F, 0x3F,     // L cyan     0 1 1 1     1 0 1 1
    0x3F, 0x15, 0x3F,     // L magenta  1 0 1 1     0 1 1 1
    0x3F, 0x3F, 0x3F};    // White      1 1 1 1     1 1 1 1
#endif

#ifdef MODE_CVB
const byte colors[48] = { // standard IBM CGA
    0x00, 0x00, 0x00,
    0x00, 0x18, 0x06,
    0x09, 0x0a, 0x2f,
    0x04, 0x26, 0x37,
    0x20, 0x03, 0x15,
    0x1c, 0x1c, 0x1c,
    0x2c, 0x0e, 0x3f,
    0x26, 0x2a, 0x3f,
    0x12, 0x12, 0x00,
    0x0f, 0x2e, 0x00,
    0x1c, 0x1c, 0x1c,
    0x18, 0x3b, 0x21,
    0x37, 0x15, 0x04,
    0x35, 0x31, 0x07,
    0x3f, 0x20, 0x3a,
    0x3f, 0x3f, 0x3f};
/*const byte colors[48] = {  // ATi Small Wonder
    0x00, 0x00, 0x00,
    0x22, 0x04, 0x00,
    0x06, 0x15, 0x00,
    0x26, 0x1f, 0x00,
    0x03, 0x0f, 0x23,
    0x16, 0x17, 0x15,
    0x00, 0x2b, 0x00,
    0x1a, 0x38, 0x00,
    0x18, 0x01, 0x36,
    0x3f, 0x07, 0x31,
    0x14, 0x15, 0x13,
    0x3f, 0x22, 0x0e,
    0x0a, 0x13, 0x3f,
    0x39, 0x1e, 0x3f,
    0x07, 0x32, 0x3f,
    0x3f, 0x3f, 0x3f};*/
#endif

#ifdef MODE_PCP
const byte colors[48] = {
    0x00, 0x00, 0x00,
    0x00, 0x2A, 0x00,
    0x2A, 0x00, 0x00,
    0x2A, 0x15, 0x00,
    0x15, 0x15, 0x15,
    0x15, 0x3F, 0x15,
    0x3F, 0x15, 0x15,
    0x3F, 0x3F, 0x15,
    0x00, 0x00, 0x2A,
    0x00, 0x2A, 0x2A,
    0x2A, 0x00, 0x2A,
    0x2A, 0x2A, 0x2A,
    0x15, 0x15, 0x3F,
    0x15, 0x3F, 0x3F,
    0x3F, 0x15, 0x3F,
    0x3F, 0x3F, 0x3F};
#endif

#ifdef MODE_CGA
const byte colors[12] = {
    0x00, 0x00, 0x00,
    0x00, 0x2A, 0x00,
    0x2A, 0x00, 0x00,
    0x2A, 0x15, 0x00};
#endif

#if defined(MODE_VGA16) || defined(MODE_CGA16) || defined(MODE_EGA16) || defined(MODE_CGA) || defined(MODE_EGA640) || defined(MODE_EGA) || defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_PCP) || defined(MODE_CVB) || defined(MODE_ATI640) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA10)

int I_SQRT(int x)
{
    int start = 1, end = x / 2, ans;

    if (x == 0 || x == 1)
        return x;

    while (start <= end)
    {
        int mid = (start + end) / 2;

        if (mid * mid == x)
            return mid;

        if (mid <= x / mid)
        {
            start = mid + 1;
            ans = mid;
        }
        else
            end = mid - 1;
    }
    return ans;
}

#endif

#if defined(MODE_EGA640) || defined(MODE_ATI640)

int I_GetClosestColor(int r1, int g1, int b1)
{
    int i;

    int result;

    int distance;
    int best_difference = MAXINT;

    for (i = 0; i < 16; i++)
    {
        int r2, g2, b2;
        int cR, cG, cB;
        int pos = i * 3;

        r2 = (int)colors[pos];
        cR = abs(r2 - r1);

        g2 = (int)colors[pos + 1];
        cG = abs(g2 - g1);

        b2 = (int)colors[pos + 2];
        cB = abs(b2 - b1);

        distance = cR + cG + cB;

        if (distance == 0)
        {
            return i;
        }

        distance = I_SQRT(distance);

        if (best_difference > distance)
        {
            best_difference = distance;
            result = i;
        }
    }

    return result;
}
#endif

#if defined(MODE_EGA640)
void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 512; i += 2, palette += 3)
    {
        unsigned char color;
        int r, g, b;
        int r2, g2, b2;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        r2 = (r * 1) / 3 + r;
        g2 = (g * 1) / 3 + g;
        b2 = (b * 1) / 3 + b;

        color = I_GetClosestColor(r2, g2, b2);
        color |= color << 4;

        lutcolors[i] = color;

        r2 = (r * 2) / 3 + r;
        g2 = (g * 2) / 3 + g;
        b2 = (b * 2) / 3 + b;

        color = I_GetClosestColor(r2, g2, b2);
        color |= color << 4;

        lutcolors[i + 1] = color;
    }
}
#endif

#if defined(MODE_ATI640)
void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 512; i += 2, palette += 3)
    {
        unsigned char color;
        unsigned short value;
        unsigned short value2;
        int r, g, b;
        int r2, g2, b2;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        r2 = (r * 1) / 3 + r;
        g2 = (g * 1) / 3 + g;
        b2 = (b * 1) / 3 + b;

        color = I_GetClosestColor(r2, g2, b2);
        value = color & 12;
        value = value | value >> 2 | value << 2 | value << 4;
        value <<= 8;

        value2 = color & 3;
        value2 = value2 | value2 << 2 | value2 << 4 | value2 << 6;
        value2;

        lutcolors[i] = value | value2;

        r2 = (r * 2) / 3 + r;
        g2 = (g * 2) / 3 + g;
        b2 = (b * 2) / 3 + b;

        color = I_GetClosestColor(r2, g2, b2);
        value = color & 12;
        value = value | value >> 2 | value << 2 | value << 4;
        value <<= 8;

        value2 = color & 3;
        value2 = value2 | value2 << 2 | value2 << 4 | value2 << 6;
        value2;

        lutcolors[i + 1] = value | value2;
    }
}
#endif

#if defined(MODE_VGA16) || defined(MODE_CGA16) || defined(MODE_EGA16) || defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_CVB) || defined(MODE_EGA80) || defined(MODE_EGAW1)
void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++)
    {
        int distance;

        int r1, g1, b1;

        int best_difference = MAXINT;

        r1 = (int)ptr[*palette++];
        g1 = (int)ptr[*palette++];
        b1 = (int)ptr[*palette++];

        for (j = 0; j < 16; j++)
        {
            int r2, g2, b2;
            int cR, cG, cB;
            int pos = j * 3;

            r2 = (int)colors[pos];
            cR = abs(r2 - r1);

            g2 = (int)colors[pos + 1];
            cG = abs(g2 - g1);

            b2 = (int)colors[pos + 2];
            cB = abs(b2 - b1);

            distance = cR + cG + cB;

            if (distance == 0)
            {
                lut16colors[i] = j;
                break;
            }

            distance = I_SQRT(distance);

            if (best_difference > distance)
            {
                best_difference = distance;
                lut16colors[i] = j;
            }
        }
    }
}
#endif

#if defined(MODE_EGA10)
void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++)
    {
        int distance;

        int r1, g1, b1;

        int best_difference = MAXINT;

        r1 = (int)ptr[*palette++];
        g1 = (int)ptr[*palette++];
        b1 = (int)ptr[*palette++];

        for (j = 0; j < 10; j++)
        {
            int r2, g2, b2;
            int cR, cG, cB;
            int pos = j * 3;

            r2 = (int)colors[pos];
            cR = abs(r2 - r1);

            g2 = (int)colors[pos + 1];
            cG = abs(g2 - g1);

            b2 = (int)colors[pos + 2];
            cB = abs(b2 - b1);

            distance = cR + cG + cB;

            if (distance == 0)
            {
                lut16colors[i] = j;
                break;
            }

            distance = I_SQRT(distance);

            if (best_difference > distance)
            {
                best_difference = distance;
                lut16colors[i] = j;
            }
        }
    }
}

unsigned int EGA10ColorRemap(unsigned int input)
{
    switch(input)
    {
        case 5:
            return 6;
        case 6:
            return 7;
        case 7:
            return 8;
        case 8:
            return 9;
        case 9:
            return 14;
        default:
            return input;
    }
}
#endif

#if defined(MODE_MDA)
void I_ProcessPalette(byte *palette)
{
}
#endif

#if defined(MODE_CGA136) || defined(MODE_VGA136) || defined(MODE_EGA136)
void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++)
    {
        int distance;

        int r1, g1, b1;

        int best_difference = MAXINT;

        r1 = (int)ptr[*palette++];
        g1 = (int)ptr[*palette++];
        b1 = (int)ptr[*palette++];

        for (j = 0; j < 122; j++)
        {
            int r2, g2, b2;
            int cR, cG, cB;
            int pos = j * 4;

            r2 = (int)colors[pos + 1];
            cR = abs(r2 - r1);

            g2 = (int)colors[pos + 2];
            cG = abs(g2 - g1);

            b2 = (int)colors[pos + 3];
            cB = abs(b2 - b1);

            distance = cR + cG + cB;

            if (distance == 0)
            {
                lut136colors[i] = colors[pos];
                break;
            }

            if (best_difference > distance)
            {
                best_difference = distance;
                lut136colors[i] = colors[pos];
            }
        }
    }
}
#endif

#if defined(MODE_PCP)
void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++)
    {
        int distance;

        int r1, g1, b1;

        int best_difference = MAXINT;

        r1 = (int)ptr[*palette++];
        g1 = (int)ptr[*palette++];
        b1 = (int)ptr[*palette++];

        for (j = 0; j < 16; j++)
        {
            int r2, g2, b2;
            int cR, cG, cB;
            int pos = j * 3;

            r2 = (int)colors[pos];
            cR = abs(r2 - r1);

            g2 = (int)colors[pos + 1];
            cG = abs(g2 - g1);

            b2 = (int)colors[pos + 2];
            cB = abs(b2 - b1);

            distance = cR + cG + cB;

            if (distance == 0)
            {
                unsigned short value;
                unsigned short value2;

                value = j & 12;
                value = value | value >> 2 | value << 2 | value << 4;
                value <<= 8;

                value2 = j & 3;
                value2 = value2 | value2 << 2 | value2 << 4 | value2 << 6;
                value2;

                lut16colors[i] = value | value2;
                break;
            }

            distance = I_SQRT(distance);

            if (best_difference > distance)
            {
                unsigned short value;
                unsigned short value2;

                value = j & 12;
                value = value | value >> 2 | value << 2 | value << 4;
                value <<= 8;

                value2 = j & 3;
                value2 = value2 | value2 << 2 | value2 << 4 | value2 << 6;
                value2;

                lut16colors[i] = value | value2;
            }
        }
    }
}
#endif

#if defined(MODE_EGA)
void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++)
    {
        int distance;

        int r1, g1, b1;

        int best_difference = MAXINT;

        r1 = (int)ptr[*palette++];
        g1 = (int)ptr[*palette++];
        b1 = (int)ptr[*palette++];

        for (j = 0; j < 16; j++)
        {
            int r2, g2, b2;
            int cR, cG, cB;
            int pos = j * 3;

            r2 = (int)colors[pos];
            cR = abs(r2 - r1);

            g2 = (int)colors[pos + 1];
            cG = abs(g2 - g1);

            b2 = (int)colors[pos + 2];
            cB = abs(b2 - b1);

            distance = cR + cG + cB;

            if (distance == 0)
            {
                // R
                if (j & 8)
                {
                    lutcolor[3][i] = 0xFF;
                }
                else
                {
                    lutcolor[3][i] = 0x00;
                }

                // G
                if (j & 4)
                {
                    lutcolor[2][i] = 0xFF;
                }
                else
                {
                    lutcolor[2][i] = 0x00;
                }

                // B
                if (j & 2)
                {
                    lutcolor[1][i] = 0xFF;
                }
                else
                {
                    lutcolor[1][i] = 0x00;
                }

                // I
                if (j & 1)
                {
                    lutcolor[0][i] = 0xFF;
                }
                else
                {
                    lutcolor[0][i] = 0x00;
                }

                break;
            }

            distance = I_SQRT(distance);

            if (best_difference > distance)
            {
                best_difference = distance;

                // R
                if (j & 8)
                {
                    lutcolor[3][i] = 0xFF;
                }
                else
                {
                    lutcolor[3][i] = 0x00;
                }

                // G
                if (j & 4)
                {
                    lutcolor[2][i] = 0xFF;
                }
                else
                {
                    lutcolor[2][i] = 0x00;
                }

                // B
                if (j & 2)
                {
                    lutcolor[1][i] = 0xFF;
                }
                else
                {
                    lutcolor[1][i] = 0x00;
                }

                // I
                if (j & 1)
                {
                    lutcolor[0][i] = 0xFF;
                }
                else
                {
                    lutcolor[0][i] = 0x00;
                }
            }
        }
    }
}
#endif

#if defined(MODE_CGA)
void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++)
    {
        int distance;

        int r1, g1, b1;

        int best_difference = MAXINT;

        r1 = (int)ptr[*palette++];
        g1 = (int)ptr[*palette++];
        b1 = (int)ptr[*palette++];

        for (j = 0; j < 4; j++)
        {
            int r2, g2, b2;
            int cR, cG, cB;
            int pos = j * 3;

            r2 = (int)colors[pos];
            cR = abs(r2 - r1);

            g2 = (int)colors[pos + 1];
            cG = abs(g2 - g1);

            b2 = (int)colors[pos + 2];
            cB = abs(b2 - b1);

            distance = cR + cG + cB;

            if (distance == 0)
            {
                lut4colors[i] = j | j << 2 | j << 4 | j << 6;
                break;
            }

            distance = I_SQRT(distance);

            if (best_difference > distance)
            {
                best_difference = distance;
                lut4colors[i] = j | j << 2 | j << 4 | j << 6;
            }
        }
    }
}
#endif

//
// I_SetPalette
// Palette source must use 8 bit RGB elements.
//
void I_SetPalette(int numpalette)
{
#if defined(MODE_HERC)
    ptrlutcolors = lutcolors + numpalette * 1024;
#endif

#if defined(MODE_CGA_BW) || defined(MODE_EGA640)
    ptrlutcolors = lutcolors + numpalette * 512;
#endif

#if defined(MODE_ATI640)
    ptrlutcolors = lutcolors + numpalette * 512;
#endif

#if defined(MODE_PCP)
    ptrlut16colors = lut16colors + numpalette * 256;
#endif

#if defined(MODE_VGA16) || defined(MODE_CGA16) || defined(MODE_EGA16) || defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100) || defined(MODE_CVB) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA10)
    ptrlut16colors = lut16colors + numpalette * 256;
#endif

#if defined(MODE_CGA136) || defined(MODE_VGA136) || defined(MODE_EGA136)
    ptrlut136colors = lut136colors + numpalette * 256;
#endif

#if defined(MODE_CGA)
    ptrlut4colors = lut4colors + numpalette * 256;
#endif

#if defined(MODE_EGA)
    pospalette = numpalette * 256;
#endif

#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
    {
        int i;
        int pos = Mul768(numpalette);

        _outbyte(PEL_WRITE_ADR, 0);

        if (VGADACfix)
        {
            byte *ptrprocessedpalette = processedpalette + pos;
            for (i = 0; i < 768; i += 4)
            {
                _outbyte(PEL_DATA, *(ptrprocessedpalette));
                _outbyte(PEL_DATA, *(ptrprocessedpalette + 1));
                _outbyte(PEL_DATA, *(ptrprocessedpalette + 2));
                _outbyte(PEL_DATA, *(ptrprocessedpalette + 3));
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

#if defined(MODE_EGA) || defined(MODE_EGA640)
byte page = 0;
#endif

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
#ifdef MODE_VBE2_DIRECT
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

#ifdef MODE_Y
void I_UpdateBox(int x, int y, int w, int h)
{
    int i, j, k, count;
    int sp_x1, sp_x2;
    int poffset;
    int offset;
    int pstep;
    int step;
    byte *dest, *source;

    outp(SC_INDEX, SC_MAPMASK);

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

    outp(SC_INDEX, SC_MAPMASK);

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

#ifdef MODE_CGA_BW
void CGA_BW_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB8000;
    unsigned short *ptrvrambuffer = vrambuffer;
    byte *ptrbackbuffer = backbuffer;

    do
    {
        unsigned char x = 80;

        do
        {
            unsigned short *ptr;
            unsigned short finalcolor;

            // Process two pixels at the same time (16-bit)
            ptr = ptrlutcolors + *(ptrbackbuffer)*2;
            finalcolor = *ptr & 0x8040;
            ptr = ptrlutcolors + *(ptrbackbuffer + 1) * 2;
            finalcolor |= *ptr & 0x2010;
            ptr = ptrlutcolors + *(ptrbackbuffer + 2) * 2;
            finalcolor |= *ptr & 0x0804;
            ptr = ptrlutcolors + *(ptrbackbuffer + 3) * 2;
            finalcolor |= *ptr & 0x0201;

            if (finalcolor != *ptrvrambuffer)
            {
                *ptrvrambuffer = finalcolor;
                *vram = BYTE0_USHORT(finalcolor) | BYTE1_USHORT(finalcolor);
            }

            ptr = ptrlutcolors + *(ptrbackbuffer + 320) * 2;
            finalcolor = *ptr & 0x8040;
            ptr = ptrlutcolors + *(ptrbackbuffer + 321) * 2;
            finalcolor |= *ptr & 0x2010;
            ptr = ptrlutcolors + *(ptrbackbuffer + 322) * 2;
            finalcolor |= *ptr & 0x0804;
            ptr = ptrlutcolors + *(ptrbackbuffer + 323) * 2;
            finalcolor |= *ptr & 0x0201;

            if (finalcolor != *(ptrvrambuffer + 0x2000))
            {
                *(ptrvrambuffer + 0x2000) = finalcolor;
                *(vram + 0x2000) = BYTE0_USHORT(finalcolor) | BYTE1_USHORT(finalcolor);
            }

            ptrbackbuffer += 4;
            ptrvrambuffer++;
            vram++;
            x--;

        } while (x > 0);

        ptrbackbuffer += 320;
    } while (vram < 0xB9F40);
}
#endif

#ifdef MODE_HERC
void HERC_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB0000;
    byte *ptrvrambuffer = vrambuffer;
    byte *ptrbackbuffer = backbuffer;

    do
    {
        unsigned char x = 80;

        do
        {
            unsigned int *ptr;
            unsigned int finalcolor;
            byte tmp;

            // Process four pixels at the same time (32-bit)
            ptr = ptrlutcolors + *(ptrbackbuffer)*4;
            finalcolor = *ptr & 0x80408040;
            ptr = ptrlutcolors + *(ptrbackbuffer + 1) * 4;
            finalcolor |= *ptr & 0x20102010;
            ptr = ptrlutcolors + *(ptrbackbuffer + 2) * 4;
            finalcolor |= *ptr & 0x08040804;
            ptr = ptrlutcolors + *(ptrbackbuffer + 3) * 4;
            finalcolor |= *ptr & 0x02010201;

            tmp = BYTE0_UINT(finalcolor) | BYTE1_UINT(finalcolor);

            if (tmp != *ptrvrambuffer)
            {
                *vram = tmp;
                *ptrvrambuffer = tmp;
            }

            tmp = BYTE2_UINT(finalcolor) | BYTE3_UINT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x2000))
            {
                *(vram + 0x2000) = tmp;
                *(ptrvrambuffer + 0x2000) = tmp;
            }

            ptr = ptrlutcolors + *(ptrbackbuffer + 320) * 4;
            finalcolor = *ptr & 0x80408040;
            ptr = ptrlutcolors + *(ptrbackbuffer + 321) * 4;
            finalcolor |= *ptr & 0x20102010;
            ptr = ptrlutcolors + *(ptrbackbuffer + 322) * 4;
            finalcolor |= *ptr & 0x08040804;
            ptr = ptrlutcolors + *(ptrbackbuffer + 323) * 4;
            finalcolor |= *ptr & 0x02010201;

            tmp = BYTE0_UINT(finalcolor) | BYTE1_UINT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x4000))
            {
                *(vram + 0x4000) = tmp;
                *(ptrvrambuffer + 0x4000) = tmp;
            }

            tmp = BYTE2_UINT(finalcolor) | BYTE3_UINT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x6000))
            {
                *(vram + 0x6000) = tmp;
                *(ptrvrambuffer + 0x6000) = tmp;
            }

            ptrbackbuffer += 4;
            ptrvrambuffer++;
            vram++;
            x--;

        } while (x > 0);

        ptrbackbuffer += 320;
    } while (vram < 0xB1F40);
}
#endif

#ifdef MODE_CGA16
void CGA16_DrawBackbuffer_Snow(void)
{
    unsigned char *vram = (unsigned char *)0xB8001;
    unsigned char line = 80;
    byte *ptrbackbuffer = backbuffer;
    byte *ptrvrambuffer = vrambuffer;

    do
    {
        unsigned char tmp = ptrlut16colors[*ptrbackbuffer] << 4 | ptrlut16colors[*(ptrbackbuffer + 2)];

        if (tmp != *ptrvrambuffer)
        {
            I_WaitCGA();
            *vram = tmp;
            *ptrvrambuffer = tmp;
        }

        vram += 2;
        ptrvrambuffer += 2;
        ptrbackbuffer += 4;

        line--;
        if (line == 0)
        {
            line = 80;
            ptrbackbuffer += 320;
        }
    } while (vram < (unsigned char *)0xBBE80);
}

void CGA16_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB8001;
    unsigned char line = 80;
    byte *ptrbackbuffer = backbuffer;
    byte *ptrvrambuffer = vrambuffer;

    do
    {
        unsigned char tmp = ptrlut16colors[*ptrbackbuffer] << 4 | ptrlut16colors[*(ptrbackbuffer + 2)];

        if (tmp != *ptrvrambuffer)
        {
            *vram = tmp;
            *ptrvrambuffer = tmp;
        }

        vram += 2;
        ptrvrambuffer += 2;
        ptrbackbuffer += 4;

        line--;
        if (line == 0)
        {
            line = 80;
            ptrbackbuffer += 320;
        }
    } while (vram < (unsigned char *)0xBBE80);
}
#endif

#ifdef MODE_EGA16
void EGA16_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB8501;
    unsigned char line = 80;
    byte *ptrbackbuffer = backbuffer;
    byte *ptrvrambuffer = vrambuffer;

    do
    {
        unsigned char tmp = ptrlut16colors[*ptrbackbuffer] << 4 | ptrlut16colors[*(ptrbackbuffer + 2)];

        if (tmp != *ptrvrambuffer)
        {
            *vram = tmp;
            *ptrvrambuffer = tmp;
        }

        vram += 2;
        ptrvrambuffer += 2;
        ptrbackbuffer += 4;

        line--;
        if (line == 0)
        {
            line = 80;
            ptrbackbuffer += 320;
        }
    } while (vram < (unsigned char *)0xBC380);
}
#endif

#ifdef MODE_CGA136
void CGA136_DrawBackbuffer_Snow(void)
{
    unsigned char *vram = (unsigned char *)0xB8001;
    byte *ptrbackbuffer = backbuffer;
    byte *ptrvrambuffer = vrambuffer;
    unsigned char line = 20;

    do
    {
        unsigned char tmp = ptrlut136colors[*ptrbackbuffer];

        if (tmp != *ptrvrambuffer)
        {
            I_WaitCGA();
            *vram = tmp;
            *ptrvrambuffer = tmp;
        }

        tmp = ptrlut136colors[*(ptrbackbuffer + 4)];

        if (tmp != *(ptrvrambuffer + 2))
        {
            I_WaitCGA();
            *(vram + 2) = tmp;
            *(ptrvrambuffer + 2) = tmp;
        }

        tmp = ptrlut136colors[*(ptrbackbuffer + 8)];

        if (tmp != *(ptrvrambuffer + 4))
        {
            I_WaitCGA();
            *(vram + 4) = tmp;
            *(ptrvrambuffer + 4) = tmp;
        }

        tmp = ptrlut136colors[*(ptrbackbuffer + 12)];

        if (tmp != *(ptrvrambuffer + 6))
        {
            I_WaitCGA();
            *(vram + 6) = tmp;
            *(ptrvrambuffer + 6) = tmp;
        }

        vram += 8;
        ptrbackbuffer += 16;
        ptrvrambuffer += 8;

        line--;
        if (line == 0)
        {
            line = 20;
            ptrbackbuffer += 320;
        }
    } while (vram < (unsigned char *)0xBBE80);
}

void CGA136_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB8001;
    byte *ptrbackbuffer = backbuffer;
    byte *ptrvrambuffer = vrambuffer;
    unsigned char line = 20;

    do
    {
        unsigned char tmp = ptrlut136colors[*ptrbackbuffer];

        if (tmp != *ptrvrambuffer)
        {
            *vram = tmp;
            *ptrvrambuffer = tmp;
        }

        tmp = ptrlut136colors[*(ptrbackbuffer + 4)];

        if (tmp != *(ptrvrambuffer + 2))
        {
            *(vram + 2) = tmp;
            *(ptrvrambuffer + 2) = tmp;
        }

        tmp = ptrlut136colors[*(ptrbackbuffer + 8)];

        if (tmp != *(ptrvrambuffer + 4))
        {
            *(vram + 4) = tmp;
            *(ptrvrambuffer + 4) = tmp;
        }

        tmp = ptrlut136colors[*(ptrbackbuffer + 12)];

        if (tmp != *(ptrvrambuffer + 6))
        {
            *(vram + 6) = tmp;
            *(ptrvrambuffer + 6) = tmp;
        }

        vram += 8;
        ptrbackbuffer += 16;
        ptrvrambuffer += 8;

        line--;
        if (line == 0)
        {
            line = 20;
            ptrbackbuffer += 320;
        }
    } while (vram < (unsigned char *)0xBBE80);
}
#endif

#ifdef MODE_EGA136
void EGA136_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB8501;
    byte *ptrbackbuffer = backbuffer;
    byte *ptrvrambuffer = vrambuffer;
    unsigned char line = 20;

    do
    {
        unsigned char tmp = ptrlut136colors[*ptrbackbuffer];

        if (tmp != *ptrvrambuffer)
        {
            *vram = tmp;
            *ptrvrambuffer = tmp;
        }

        tmp = ptrlut136colors[*(ptrbackbuffer + 4)];

        if (tmp != *(ptrvrambuffer + 2))
        {
            *(vram + 2) = tmp;
            *(ptrvrambuffer + 2) = tmp;
        }

        tmp = ptrlut136colors[*(ptrbackbuffer + 8)];

        if (tmp != *(ptrvrambuffer + 4))
        {
            *(vram + 4) = tmp;
            *(ptrvrambuffer + 4) = tmp;
        }

        tmp = ptrlut136colors[*(ptrbackbuffer + 12)];

        if (tmp != *(ptrvrambuffer + 6))
        {
            *(vram + 6) = tmp;
            *(ptrvrambuffer + 6) = tmp;
        }

        vram += 8;
        ptrbackbuffer += 16;
        ptrvrambuffer += 8;

        line--;
        if (line == 0)
        {
            line = 20;
            ptrbackbuffer += 320;
        }
    } while (vram < (unsigned char *)0xBC380);
}
#endif

#ifdef MODE_EGA10

unsigned short LUTmul10[10] = {
    0,
    10,
    20,
    30,
    40,
    50,
    60,
    70,
    80,
    90
};

unsigned short LUTmul100[10] = {
    0,
    100,
    200,
    300,
    400,
    500,
    600,
    700,
    800,
    900
};

unsigned short LUTmul1000[10] = {
    0,
    1000,
    2000,
    3000,
    4000,
    5000,
    6000,
    7000,
    8000,
    9000
};

unsigned short lastlatch = 0;

void EGA10_DrawBackbuffer(void)
{
    byte *vram = (byte *)0xA0000;
    unsigned short *ptrvrambuffer = vrambuffer;
    byte *ptrbackbuffer = backbuffer;

    do
    {
        unsigned short value = ptrlut16colors[*ptrbackbuffer] + 
                               LUTmul10[ptrlut16colors[*(ptrbackbuffer + 1)]] + 
                               LUTmul100[ptrlut16colors[*(ptrbackbuffer + 2)]] + 
                               LUTmul1000[ptrlut16colors[*(ptrbackbuffer + 3)]];

        // Avoid accessing to VRAM whenever possible
        if (*(ptrvrambuffer) != value)
        {
            *(ptrvrambuffer) = value;

            // If the latch has already a good value, use it!
            if (lastlatch != value)
            {
                // Read + write
                byte latch;

                latch = *((byte *)0xA3E80 + value); // Read block into latches
                *(vram) = latch;                    // Copy from latches
                lastlatch = value;                  // Update new latches
            }
            else
            {
                // Write
                *(vram) = 0; // Just copy from latches
            }
        }

        vram += 1;
        ptrbackbuffer += 4;
        ptrvrambuffer += 1;
    } while (vram < (byte *)0xA3E80);
}
#endif

#ifdef MODE_EGAW1
byte lastlatch = 0;

void EGAW1_DrawBackbuffer(void)
{
    byte *vram = (byte *)0xA0000;
    byte *ptrvrambuffer = vrambuffer;
    byte *ptrbackbuffer = backbuffer;

    byte latch;

    do
    {
        byte pos1 = ptrlut16colors[*ptrbackbuffer];
        byte pos2 = ptrlut16colors[*(ptrbackbuffer + 2)];

        byte value = pos1 | pos2 << 4;

        // Avoid accessing to VRAM whenever possible
        if (*ptrvrambuffer != value)
        {
            // If the latch has already a good value, use it!
            if (lastlatch != value)
            {
                // Read + write
                latch = *((byte *)0xA3E80 + value); // Read block into latches
                *(vram) = latch;                    // Copy from latches
                lastlatch = value;                  // Update new latches
            }
            else
            {
                // Write
                *(vram) = latch; // Just copy from latches
            }
            *ptrvrambuffer = value;
        }

        vram += 1;
        ptrbackbuffer += 4;
        ptrvrambuffer += 1;
    } while (vram < (byte *)0xA3E80);
}
#endif

#ifdef MODE_EGA80
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

#ifdef MODE_VGA16
void VGA16_DrawBackbuffer(void)
{
    unsigned char *vram;
    byte *ptrbackbuffer = backbuffer;

    for (vram = (unsigned char *)0xB8001; vram < (unsigned char *)0xBFD00; vram += 2, ptrbackbuffer += 4)
    {
        *vram = ptrlut16colors[*ptrbackbuffer] << 4 | ptrlut16colors[*(ptrbackbuffer + 2)];
    }
}
#endif

#ifdef MODE_VGA136
void VGA136_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB8001;
    byte *ptrbackbuffer = backbuffer;

    do
    {
        *vram = ptrlut136colors[*ptrbackbuffer];
        *(vram + 2) = ptrlut136colors[*(ptrbackbuffer + 4)];
        *(vram + 4) = ptrlut136colors[*(ptrbackbuffer + 8)];
        *(vram + 6) = ptrlut136colors[*(ptrbackbuffer + 12)];

        vram += 8;
        ptrbackbuffer += 16;
    } while (vram < (unsigned char *)0xBFD00);
}
#endif

#ifdef MODE_ATI640
void ATI640_DrawBackbuffer(void)
{
    int x;
    unsigned char *vram = (unsigned char *)0xB0000;
    byte *ptrvrambuffer = vrambuffer;
    unsigned int base = 0;

    for (base = 0; base < SCREENHEIGHT * 320; base += 960)
    {
        for (x = 0; x < 160; x++, base += 2, vram++, ptrvrambuffer++)
        {
            unsigned short color;
            unsigned short finalcolor;
            byte tmp;

            color = ptrlutcolors[backbuffer[base] * 2];
            finalcolor = color & 0xC0C0;

            color = ptrlutcolors[backbuffer[base] * 2 + 1];
            finalcolor |= color & 0x3030;

            color = ptrlutcolors[backbuffer[base + 1] * 2];
            finalcolor |= color & 0x0C0C;

            color = ptrlutcolors[backbuffer[base + 1] * 2 + 1];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer))
            {
                *(vram) = tmp;
                *(ptrvrambuffer) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x8000))
            {
                *(vram + 0x8000) = tmp;
                *(ptrvrambuffer + 0x8000) = tmp;
            }

            color = ptrlutcolors[backbuffer[base + 320] * 2];
            finalcolor = color & 0xC0C0;

            color = ptrlutcolors[backbuffer[base + 320] * 2 + 1];
            finalcolor |= color & 0x3030;

            color = ptrlutcolors[backbuffer[base + 321] * 2];
            finalcolor |= color & 0x0C0C;

            color = ptrlutcolors[backbuffer[base + 321] * 2 + 1];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x2000))
            {
                *(vram + 0x2000) = tmp;
                *(ptrvrambuffer + 0x2000) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0xA000))
            {
                *(vram + 0xA000) = tmp;
                *(ptrvrambuffer + 0xA000) = tmp;
            }

            color = ptrlutcolors[backbuffer[base + 640] * 2];
            finalcolor = color & 0xC0C0;

            color = ptrlutcolors[backbuffer[base + 640] * 2 + 1];
            finalcolor |= color & 0x3030;

            color = ptrlutcolors[backbuffer[base + 641] * 2];
            finalcolor |= color & 0x0C0C;

            color = ptrlutcolors[backbuffer[base + 641] * 2 + 1];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x4000))
            {
                *(vram + 0x4000) = tmp;
                *(ptrvrambuffer + 0x4000) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0xC000))
            {
                *(vram + 0xC000) = tmp;
                *(ptrvrambuffer + 0xC000) = tmp;
            }

            color = ptrlutcolors[backbuffer[base + 960] * 2];
            finalcolor = color & 0xC0C0;

            color = ptrlutcolors[backbuffer[base + 960] * 2 + 1];
            finalcolor |= color & 0x3030;

            color = ptrlutcolors[backbuffer[base + 961] * 2];
            finalcolor |= color & 0x0C0C;

            color = ptrlutcolors[backbuffer[base + 961] * 2 + 1];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x6000))
            {
                *(vram + 0x6000) = tmp;
                *(ptrvrambuffer + 0x6000) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0xE000))
            {
                *(vram + 0xE000) = tmp;
                *(ptrvrambuffer + 0xE000) = tmp;
            }
        }
    }
}
#endif

#ifdef MODE_EGA640
void EGA640_DrawBackbuffer(void)
{
    unsigned short i;
    byte *backbufferptr;

    byte *vrambufferR;
    byte *vrambufferG;
    byte *vrambufferB;
    byte *vrambufferI;

    if (destscreen == 0xA0000)
    {
        vrambufferR = vrambufferR1;
        vrambufferG = vrambufferG1;
        vrambufferB = vrambufferB1;
        vrambufferI = vrambufferI1;
    }
    else if (destscreen == 0xA4000)
    {
        vrambufferR = vrambufferR2;
        vrambufferG = vrambufferG2;
        vrambufferB = vrambufferB2;
        vrambufferI = vrambufferI2;
    }
    else
    {
        vrambufferR = vrambufferR3;
        vrambufferG = vrambufferG3;
        vrambufferB = vrambufferB3;
        vrambufferI = vrambufferI3;
    }

    // Red
    outp(0x3C5, 1 << (3 & 0x03));

    for (i = 0, backbufferptr = backbuffer; i < 2 * SCREENWIDTH * SCREENHEIGHT / 8; i++, backbufferptr += 4)
    {
        unsigned char color;
        unsigned char tmpColor;

        color = ptrlutcolors[*(backbufferptr)*2];
        tmpColor = color & 0x80;

        color = ptrlutcolors[*(backbufferptr)*2 + 1];
        tmpColor |= (color & 0x80) >> 1;

        color = ptrlutcolors[*(backbufferptr + 1) * 2];
        tmpColor |= (color & 0x80) >> 2;

        color = ptrlutcolors[*(backbufferptr + 1) * 2 + 1];
        tmpColor |= (color & 0x80) >> 3;

        color = ptrlutcolors[*(backbufferptr + 2) * 2];
        tmpColor |= color & 0x08;

        color = ptrlutcolors[*(backbufferptr + 2) * 2 + 1];
        tmpColor |= (color & 0x08) >> 1;

        color = ptrlutcolors[*(backbufferptr + 3) * 2];
        tmpColor |= (color & 0x08) >> 2;

        color = ptrlutcolors[*(backbufferptr + 3) * 2 + 1];
        tmpColor |= (color & 0x08) >> 3;

        if (tmpColor != vrambufferR[i])
        {
            destscreen[i] = tmpColor;
            vrambufferR[i] = tmpColor;
        }
    }

    // Green
    outp(0x3C5, 1 << (2 & 0x03));

    for (i = 0, backbufferptr = backbuffer; i < 2 * SCREENWIDTH * SCREENHEIGHT / 8; i++, backbufferptr += 4)
    {
        unsigned char color;
        unsigned char tmpColor;

        color = ptrlutcolors[*(backbufferptr)*2];
        tmpColor = (color & 0x40) << 1;

        color = ptrlutcolors[*(backbufferptr)*2 + 1];
        tmpColor |= color & 0x40;

        color = ptrlutcolors[*(backbufferptr + 1) * 2];
        tmpColor |= (color & 0x40) >> 1;

        color = ptrlutcolors[*(backbufferptr + 1) * 2 + 1];
        tmpColor |= (color & 0x40) >> 2;

        color = ptrlutcolors[*(backbufferptr + 2) * 2];
        tmpColor |= (color & 0x04) << 1;

        color = ptrlutcolors[*(backbufferptr + 2) * 2 + 1];
        tmpColor |= color & 0x04;

        color = ptrlutcolors[*(backbufferptr + 3) * 2];
        tmpColor |= (color & 0x04) >> 1;

        color = ptrlutcolors[*(backbufferptr + 3) * 2 + 1];
        tmpColor |= (color & 0x04) >> 2;

        if (tmpColor != vrambufferG[i])
        {
            destscreen[i] = tmpColor;
            vrambufferG[i] = tmpColor;
        }
    }

    // Blue
    outp(0x3C5, 1 << (1 & 0x03));

    for (i = 0, backbufferptr = backbuffer; i < 2 * SCREENWIDTH * SCREENHEIGHT / 8; i++, backbufferptr += 4)
    {
        unsigned char color;
        unsigned char tmpColor;

        color = ptrlutcolors[*(backbufferptr)*2];
        tmpColor = (color & 0x20) << 2;

        color = ptrlutcolors[*(backbufferptr)*2 + 1];
        tmpColor |= (color & 0x20) << 1;

        color = ptrlutcolors[*(backbufferptr + 1) * 2];
        tmpColor |= color & 0x20;

        color = ptrlutcolors[*(backbufferptr + 1) * 2 + 1];
        tmpColor |= (color & 0x20) >> 1;

        color = ptrlutcolors[*(backbufferptr + 2) * 2];
        tmpColor |= (color & 0x02) << 2;

        color = ptrlutcolors[*(backbufferptr + 2) * 2 + 1];
        tmpColor |= (color & 0x02) << 1;

        color = ptrlutcolors[*(backbufferptr + 3) * 2];
        tmpColor |= color & 0x02;

        color = ptrlutcolors[*(backbufferptr + 3) * 2 + 1];
        tmpColor |= (color & 0x02) >> 1;

        if (tmpColor != vrambufferB[i])
        {
            destscreen[i] = tmpColor;
            vrambufferB[i] = tmpColor;
        }
    }

    // Intensity
    outp(0x3C5, 1 << (0 & 0x03));

    for (i = 0, backbufferptr = backbuffer; i < 2 * SCREENWIDTH * SCREENHEIGHT / 8; i++, backbufferptr += 4)
    {
        unsigned char color;
        unsigned char tmpColor;

        color = ptrlutcolors[*(backbufferptr)*2];
        tmpColor = (color & 0x10) << 3;

        color = ptrlutcolors[*(backbufferptr)*2 + 1];
        tmpColor |= (color & 0x10) << 2;

        color = ptrlutcolors[*(backbufferptr + 1) * 2];
        tmpColor |= (color & 0x10) << 1;

        color = ptrlutcolors[*(backbufferptr + 1) * 2 + 1];
        tmpColor |= color & 0x10;

        color = ptrlutcolors[*(backbufferptr + 2) * 2];
        tmpColor |= (color & 0x01) << 3;

        color = ptrlutcolors[*(backbufferptr + 2) * 2 + 1];
        tmpColor |= (color & 0x01) << 2;

        color = ptrlutcolors[*(backbufferptr + 3) * 2];
        tmpColor |= (color & 0x01) << 1;

        color = ptrlutcolors[*(backbufferptr + 3) * 2 + 1];
        tmpColor |= color & 0x01;

        if (tmpColor != vrambufferI[i])
        {
            destscreen[i] = tmpColor;
            vrambufferI[i] = tmpColor;
        }
    }

    // Change video page
    outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0xc);

    // Next plane
    if (destscreen == 0xA8000)
        destscreen = 0xA0000;
    else
        destscreen += 0x4000;
}
#endif

#ifdef MODE_EGA
void EGA_DrawBackbuffer(void)
{
    byte *backbufferptr;
    unsigned short *ptrvrambuffer;
    int i;

    // Each plane
    for (i = 0; i < 4; i++)
    {
        byte *ptrlutcolor = lutcolor[i] + pospalette;

        outp(0x3C5, 1 << i);

        backbufferptr = backbuffer;
        ptrvrambuffer = vrambuffer[i][page];

        do
        {
            unsigned short color;
            unsigned short tmpColor;

            BYTE1_USHORT(tmpColor) = ptrlutcolor[*(backbufferptr)];
            BYTE0_USHORT(tmpColor) = ptrlutcolor[*(backbufferptr + 1)];
            tmpColor &= 0x8040;

            BYTE1_USHORT(color) = ptrlutcolor[*(backbufferptr + 2)];
            BYTE0_USHORT(color) = ptrlutcolor[*(backbufferptr + 3)];
            tmpColor |= color & 0x2010;

            BYTE1_USHORT(color) = ptrlutcolor[*(backbufferptr + 4)];
            BYTE0_USHORT(color) = ptrlutcolor[*(backbufferptr + 5)];
            tmpColor |= color & 0x0804;

            BYTE1_USHORT(color) = ptrlutcolor[*(backbufferptr + 6)];
            BYTE0_USHORT(color) = ptrlutcolor[*(backbufferptr + 7)];
            tmpColor |= color & 0x0201;

            if (tmpColor != *ptrvrambuffer)
            {
                *ptrvrambuffer = tmpColor;
                *destscreen = BYTE0_USHORT(tmpColor) | BYTE1_USHORT(tmpColor);
            }

            destscreen++;
            ptrvrambuffer++;
            backbufferptr += 8;

        } while (backbufferptr < backbuffer + SCREENWIDTH * SCREENHEIGHT);

        backbufferptr = backbuffer;
        destscreen -= SCREENWIDTH * SCREENHEIGHT / 8;
    }

    // Change video page
    outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0xc);

    // Next plane
    if (page == 2)
    {
        page = 0;
        destscreen = (byte *)0xA0000;
    }
    else
    {
        page++;
        destscreen += 0x2000;
    }
}
#endif

#ifdef MODE_CVB
void CVBS_DrawBackbuffer(void)
{

    unsigned char *vram = (unsigned char *)0xB8000;
    unsigned short base = 0;
    byte *ptrvrambuffer = vrambuffer;

    for (base = 0; base < SCREENHEIGHT * 320;)
    {
        unsigned char x;

        for (x = 0; x < SCREENWIDTH / 4; x++, base += 4, vram++, ptrvrambuffer++)
        {
            unsigned char color0, color1;
            byte tmp;

            color0 = ptrlut16colors[backbuffer[base]];
            color1 = ptrlut16colors[backbuffer[base + 2]];

            tmp = color0 << 4 | color1;

            if (tmp != *(ptrvrambuffer))
            {
                *(vram) = tmp;
                *(ptrvrambuffer) = tmp;
            }
        }

        vram += 0x1FB0;
        ptrvrambuffer += 0x1FB0;

        for (x = 0; x < SCREENWIDTH / 4; x++, base += 4, vram++, ptrvrambuffer++)
        {
            unsigned char color0, color1;
            byte tmp;

            color0 = ptrlut16colors[backbuffer[base]];
            color1 = ptrlut16colors[backbuffer[base + 2]];

            tmp = color0 << 4 | color1;

            if (tmp != *(ptrvrambuffer))
            {
                *(vram) = tmp;
                *(ptrvrambuffer) = tmp;
            }
        }

        vram -= 0x2000;
        ptrvrambuffer -= 0x2000;
    }
}
#endif

#ifdef MODE_V2
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

#ifdef MODE_PCP
void PCP_DrawBackbuffer(void)
{
    int x;
    unsigned char *vram = (unsigned char *)0xB8000;
    byte *ptrvrambuffer = vrambuffer;
    unsigned int base = 0;

    for (base = 0; base < SCREENHEIGHT * 320;)
    {
        for (x = 0; x < SCREENWIDTH / 4; x++, base += 4, vram++, ptrvrambuffer++)
        {
            unsigned short color;
            unsigned short finalcolor;
            byte tmp;

            color = ptrlut16colors[backbuffer[base]];
            finalcolor = color & 0xC0C0;

            color = ptrlut16colors[backbuffer[base + 1]];
            finalcolor |= color & 0x3030;

            color = ptrlut16colors[backbuffer[base + 2]];
            finalcolor |= color & 0x0C0C;

            color = ptrlut16colors[backbuffer[base + 3]];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer))
            {
                *(vram) = tmp;
                *(ptrvrambuffer) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x4000))
            {
                *(vram + 0x4000) = tmp;
                *(ptrvrambuffer + 0x4000) = tmp;
            }

            color = ptrlut16colors[backbuffer[base + 320]];
            finalcolor = color & 0xC0C0;

            color = ptrlut16colors[backbuffer[base + 321]];
            finalcolor |= color & 0x3030;

            color = ptrlut16colors[backbuffer[base + 322]];
            finalcolor |= color & 0x0C0C;

            color = ptrlut16colors[backbuffer[base + 323]];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x2000))
            {
                *(vram + 0x2000) = tmp;
                *(ptrvrambuffer + 0x2000) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x6000))
            {
                *(vram + 0x6000) = tmp;
                *(ptrvrambuffer + 0x6000) = tmp;
            }
        }
        base += 320;
    }
}
#endif

#ifdef MODE_CGA
void CGA_DrawBackbuffer(void)
{
    int x;
    unsigned char *vram = (unsigned char *)0xB8000;
    unsigned short *ptrvrambuffer = vrambuffer;
    unsigned int base = 0;

    for (base = 0; base < SCREENHEIGHT * 320; base += 320)
    {
        for (x = 0; x < SCREENWIDTH / 4; x++, base += 4, vram++, ptrvrambuffer++)
        {
            unsigned short color;
            unsigned short tmpColor;
            byte tmp;

            BYTE1_USHORT(tmpColor) = (ptrlut4colors[backbuffer[base]]);
            BYTE0_USHORT(tmpColor) = (ptrlut4colors[backbuffer[base + 1]]);
            tmpColor &= 0xC030;

            BYTE1_USHORT(color) = (ptrlut4colors[backbuffer[base + 2]]);
            BYTE0_USHORT(color) = (ptrlut4colors[backbuffer[base + 3]]);
            tmpColor |= color & 0x0C03;

            if (tmpColor != *(ptrvrambuffer))
            {
                *(ptrvrambuffer) = tmpColor;
                *(vram) = BYTE0_USHORT(tmpColor) | BYTE1_USHORT(tmpColor);
            }

            BYTE1_USHORT(tmpColor) = (ptrlut4colors[backbuffer[base + 320]]);
            BYTE0_USHORT(tmpColor) = (ptrlut4colors[backbuffer[base + 321]]);
            tmpColor &= 0xC030;

            BYTE1_USHORT(color) = (ptrlut4colors[backbuffer[base + 322]]);
            BYTE0_USHORT(color) = (ptrlut4colors[backbuffer[base + 323]]);
            tmpColor |= color & 0x0C03;

            if (tmpColor != *(ptrvrambuffer + 0x2000))
            {
                *(ptrvrambuffer + 0x2000) = tmpColor;
                *(vram + 0x2000) = BYTE0_USHORT(tmpColor) | BYTE1_USHORT(tmpColor);
            }
        }
    }
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

#ifdef MODE_T8025
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
#ifdef MODE_Y
    outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0xc);

    // Next plane
    if (destscreen == (byte *)0xA8000)
        destscreen = (byte *)0xA0000;
    else
        destscreen += 0x4000;
#endif
#ifdef MODE_VBE2_DIRECT
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
#if defined(MODE_13H) || defined(MODE_VBE2)

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
#ifdef MODE_HERC
    HERC_DrawBackbuffer();
#endif
#ifdef MODE_CGA
    CGA_DrawBackbuffer();
#endif
#ifdef MODE_CGA_BW
    CGA_BW_DrawBackbuffer();
#endif
#ifdef MODE_CGA16
    if (CGAfix)
        CGA16_DrawBackbuffer_Snow();
    else
        CGA16_DrawBackbuffer();
#endif
#ifdef MODE_EGA16
    EGA16_DrawBackbuffer();
#endif
#ifdef MODE_CGA136
    if (CGAfix)
        CGA136_DrawBackbuffer_Snow();
    else
        CGA136_DrawBackbuffer();
#endif
#ifdef MODE_EGA80
    EGA80_DrawBackbuffer();
#endif
#ifdef MODE_EGA10
    EGA10_DrawBackbuffer();
#endif
#ifdef MODE_EGAW1
    EGAW1_DrawBackbuffer();
#endif
#ifdef MODE_VGA16
    VGA16_DrawBackbuffer();
#endif
#ifdef MODE_VGA136
    VGA136_DrawBackbuffer();
#endif
#ifdef MODE_EGA136
    EGA136_DrawBackbuffer();
#endif
#ifdef MODE_EGA
    EGA_DrawBackbuffer();
#endif
#ifdef MODE_EGA640
    EGA640_DrawBackbuffer();
#endif
#ifdef MODE_ATI640
    ATI640_DrawBackbuffer();
#endif
#ifdef MODE_PCP
    PCP_DrawBackbuffer();
#endif
#ifdef MODE_CVB
    CVBS_DrawBackbuffer();
#endif
#ifdef MODE_V2
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
#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
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
        _outbyte(PEL_WRITE_ADR, 0);
        OutString(PEL_DATA, test_palette, 768);

        // Read palette from VGA card
        // and compare results
        _outbyte(PEL_READ_ADR, 0);
        for (x = 0; x < 768; x++)
        {
            byte read_data = _inbyte(PEL_DATA);

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
#ifdef MODE_T8025
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
#ifdef MODE_Y
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
#ifdef MODE_13H
    regs.w.ax = 0x13;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xA0000;
#endif
#ifdef MODE_CGA
    // Set video mode 4
    regs.w.ax = 0x04;
    int386(0x10, (union REGS *)&regs, &regs);

    if (!CGApalette1)
    {
        // Set palette and intensity (CGA)
        regs.w.ax = 0x0B00;
        regs.w.bx = 0x0100;
        int386(0x10, (union REGS *)&regs, &regs);
        regs.w.ax = 0x0B00;
        regs.w.bx = 0x0000;
        int386(0x10, (union REGS *)&regs, &regs);

        // Fix EGA/VGA wrong colors
        regs.w.ax = 0x1000;
        regs.w.bx = 0x0000;
        int386(0x10, (union REGS *)&regs, &regs);

        regs.w.ax = 0x1000;
        regs.w.bx = 0x0201;
        int386(0x10, (union REGS *)&regs, &regs);

        regs.w.ax = 0x1000;
        regs.w.bx = 0x0402;
        int386(0x10, (union REGS *)&regs, &regs);

        regs.w.ax = 0x1000;
        regs.w.bx = 0x0603;
        int386(0x10, (union REGS *)&regs, &regs);
    }

    pcscreen = destscreen = (byte *)0xB8000;

    SetDWords(vrambuffer, 0, 8192);
    SetDWords(pcscreen, 0, 4096);
#endif
#if defined(MODE_EGA16) || defined(MODE_EGA136)
    unsigned char *vram = (unsigned char *)0xB8000;
    int i;

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

    /* EGA hires mode is 640x350 with a 9x14 character cell.  The pixel aspect
        ratio is 1:1.37, so if we make the blocks 3 scans tall you get a square
        pixel at 160x100, but some of the scan lines are not used (50) */

    outp(0x3D4, 0x09);
    outp(0x3D5, 0x02);

    SetDWords(vrambuffer, 0, 4096);

    for (i = 0; i < 1280; i += 2)
    {
        vram[i] = 0x00;
    }

    for (i = 1280; i < 16000 + 1280; i += 2)
    {
#ifdef MODE_EGA16
        vram[i] = 0xDE;
#endif
#ifdef MODE_EGA136
        vram[i] = 0xB1;
#endif
    }

    for (i = 0; i < 16000; i += 2)
    {
#ifdef MODE_EGA16
        vrambuffer[i] = 0xDE;
#endif
#ifdef MODE_EGA136
        vrambuffer[i] = 0xB1;
#endif
    }

    for (i = 16000 + 1280; i < 16000 + 1280 + 1280; i += 2)
    {
        vram[i] = 0x00;
    }
#endif
#if defined(MODE_CGA16) || defined(MODE_CGA136) || defined(MODE_VGA16) || defined(MODE_VGA136)
    unsigned char *vram = (unsigned char *)0xB8000;
    int i;

    // Set 80x25 color mode
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
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

    /* set mode control register for 80x25 text mode and disable video output */
    outp(0x3D8, 1);

    /*
        These settings put the 6845 into "graphics" mode without actually
        switching the CGA controller into graphics mode.  The register
        values are directly copied from CGA graphics mode register
        settings.  The 6845 does not directly display graphics, the
        6845 only generates addresses and sync signals, the CGA
        attribute controller either displays character ROM data or color
        pixel data, this is external to the 6845 and keeps the CGA card
        in text mode.
        ref: HELPPC
    */

    /* set vert total lines to 127 */
    outp(0x3D4, 0x04);
    outp(0x3D5, 0x7F);
    /* set vert displayed char rows to 100 */
    outp(0x3D4, 0x06);
    outp(0x3D5, 0x64);
    /* set vert sync position to 112 */
    outp(0x3D4, 0x07);
    outp(0x3D5, 0x70);
    /* set char scan line count to 1 */
    outp(0x3D4, 0x09);
    outp(0x3D5, 0x01);

    /* re-enable the video output in 80x25 text mode */
    outp(0x3D8, 9);

    /* init screen */
#endif

#if defined(MODE_CGA16)
    SetDWords(vrambuffer, 0, 4096);

    for (i = 0; i < 16000; i += 2)
    {
        vram[i] = 0xDE;
        vrambuffer[i] = 0xDE;
    }
#endif

#if defined(MODE_CGA136)
    SetDWords(vrambuffer, 0, 4096);

    for (i = 0; i < 16000; i += 2)
    {
        vram[i] = 0xB1;
        vrambuffer[i] = 0xB1;
    }
#endif

#if defined(MODE_VGA16)
    for (i = 0; i < 32000; i += 2)
    {
        vram[i] = 0xDE;
    }
#endif

#if defined(MODE_VGA136)
    for (i = 0; i < 32000; i += 2)
    {
        vram[i] = 0xB1;
    }
#endif

#ifdef MODE_CGA_BW
    regs.w.ax = 0x06;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xB8000;

    SetDWords(vrambuffer, 0, 8192);
    SetDWords(pcscreen, 0, 4096);
#endif
#ifdef MODE_PCP
    regs.w.ax = 0x04;
    int386(0x10, (union REGS *)&regs, &regs);
    outp(0x3DD, 0x10);
    pcscreen = destscreen = (byte *)0xB8000;

    SetDWords(vrambuffer, 0, 8192);
    SetDWords(pcscreen, 0, 8192);
#endif
#ifdef MODE_EGA
    regs.w.ax = 0x0D;
    int386(0x10, (union REGS *)&regs, &regs);
    outp(0x3C4, 0x2);
    pcscreen = destscreen = (byte *)0xA0000;

    SetDWords(vrambuffer[0][0], 0, 2048);
    SetDWords(vrambuffer[1][0], 0, 2048);
    SetDWords(vrambuffer[2][0], 0, 2048);
    SetDWords(vrambuffer[3][0], 0, 2048);
    SetDWords(vrambuffer[0][1], 0, 2048);
    SetDWords(vrambuffer[1][1], 0, 2048);
    SetDWords(vrambuffer[2][1], 0, 2048);
    SetDWords(vrambuffer[3][1], 0, 2048);
    SetDWords(vrambuffer[0][2], 0, 2048);
    SetDWords(vrambuffer[1][2], 0, 2048);
    SetDWords(vrambuffer[2][2], 0, 2048);
    SetDWords(vrambuffer[3][2], 0, 2048);
#endif
#ifdef MODE_EGA10
    {
        unsigned int pos1 = 0;
        unsigned int pos2 = 0;
        unsigned int pos3 = 0;
        unsigned int pos4 = 0;
        unsigned int counter = 0;
        byte *basevram;

        regs.w.ax = 0x0E;
        int386(0x10, (union REGS *)&regs, &regs);
        pcscreen = destscreen = (byte *)0xA0000;

        basevram = (byte *)0xA3E80; // Init at ending of viewable screen

        // Step 2
        // Copy all possible combinations to the VRAM

        outp(0x3C4, 0x02);
        for (pos1 = 0; pos1 < 10; pos1++)
        {
            for (pos2 = 0; pos2 < 10; pos2++)
            {
                for (pos3 = 0; pos3 < 10; pos3++)
                {
                    for (pos4 = 0; pos4 < 10; pos4++)
                    {
                        for (counter = 0; counter < 4; counter++)
                        {
                            byte tmppos1 = pos1;
                            byte tmppos2 = pos2;
                            byte tmppos3 = pos3;
                            byte tmppos4 = pos4;

                            byte bitstatuspos1;
                            byte bitstatuspos2;
                            byte bitstatuspos3;
                            byte bitstatuspos4;

                            byte final;

                            tmppos1 = EGA10ColorRemap(pos1);
                            tmppos2 = EGA10ColorRemap(pos2);
                            tmppos3 = EGA10ColorRemap(pos3);
                            tmppos4 = EGA10ColorRemap(pos4);

                            outp(0x3C5, 1 << counter); // Change plane

                            bitstatuspos1 = (tmppos1 >> counter) & 1;
                            bitstatuspos2 = (tmppos2 >> counter) & 1;
                            bitstatuspos3 = (tmppos3 >> counter) & 1;
                            bitstatuspos4 = (tmppos4 >> counter) & 1;

                            final = bitstatuspos1 | bitstatuspos1 << 1 | 
                                    bitstatuspos2 << 2 | bitstatuspos2 << 3 | 
                                    bitstatuspos3 << 4 | bitstatuspos3 << 5 |
                                    bitstatuspos4 << 6 | bitstatuspos4 << 7;
                            *basevram = final;
                        }
                        basevram++;
                    }
                }
            }
        }

        // Step 3

        // Write Mode 1
        outp(0x3CE, 0x05);
        outp(0x3CF, 0x01);

        // Write to all 4 planes
        outp(0x3C4, 0x02);
        outp(0x3C5, 0x0F);

        // Set Bit Mask to use the latch registers
        outp(0x3CE, 0x08);
        outp(0x3CF, 0xFF);
    }
#endif
#ifdef MODE_EGAW1
    {
        unsigned int pos1 = 0;
        unsigned int pos2 = 0;
        unsigned int counter = 0;
        byte *basevram;

        regs.w.ax = 0x0E;
        int386(0x10, (union REGS *)&regs, &regs);
        pcscreen = destscreen = (byte *)0xA0000;

        basevram = (byte *)0xA3E80; // Init at ending of viewable screen

        // Step 1 ??

        // Enable 128kb addressing
        // outp(0x3CE, 0x06);
        // outp(0x3CF, 0x01); // 0x03 ??

        // Step 2
        // Copy all possible combinations to the VRAM

        outp(0x3C4, 0x02);
        for (pos1 = 0; pos1 < 16; pos1++)
        {
            for (pos2 = 0; pos2 < 16; pos2++)
            {
                for (counter = 0; counter < 4; counter++)
                {
                    byte bitstatuspos1;
                    byte bitstatuspos2;
                    byte final;

                    outp(0x3C5, 1 << counter); // Change plane

                    bitstatuspos1 = (pos1 >> counter) & 1;
                    bitstatuspos2 = (pos2 >> counter) & 1;

                    final = bitstatuspos1 | bitstatuspos1 << 1 | bitstatuspos1 << 2 | bitstatuspos1 << 3 | bitstatuspos2 << 4 | bitstatuspos2 << 5 | bitstatuspos2 << 6 | bitstatuspos2 << 7;
                    *basevram = final;
                }

                basevram++;
            }
        }

        // Step 3

        // Write Mode 1
        outp(0x3CE, 0x05);
        outp(0x3CF, 0x01);

        // Write to all 4 planes
        outp(0x3C4, 0x02);
        outp(0x3C5, 0x0F);

        // Set Bit Mask to use the latch registers
        outp(0x3CE, 0x08);
        outp(0x3CF, 0xFF);
    }
#endif
#ifdef MODE_EGA80
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
#ifdef MODE_EGA640
    regs.w.ax = 0x0E;
    int386(0x10, (union REGS *)&regs, &regs);
    outp(0x3C4, 0x2);
    pcscreen = destscreen = (byte *)0xA0000;

    SetDWords(vrambufferR1, 0, 4096);
    SetDWords(vrambufferG1, 0, 4096);
    SetDWords(vrambufferB1, 0, 4096);
    SetDWords(vrambufferI1, 0, 4096);
    SetDWords(vrambufferR2, 0, 4096);
    SetDWords(vrambufferG2, 0, 4096);
    SetDWords(vrambufferB2, 0, 4096);
    SetDWords(vrambufferI2, 0, 4096);
    SetDWords(vrambufferR3, 0, 4096);
    SetDWords(vrambufferG3, 0, 4096);
    SetDWords(vrambufferB3, 0, 4096);
    SetDWords(vrambufferI3, 0, 4096);
#endif
#ifdef MODE_ATI640

    static int parms[16] = {0x70, 0x50, 0x58, 0x0a,
                            0x40, 0x06, 0x32, 0x38,
                            0x02, 0x03, 0x06, 0x07,
                            0x00, 0x00, 0x00, 0x00};
    int i;

    /* Set the Graphics Solution to 640 x 200 with 16 colors in
       Color Mode */
    outp(0x3D8, 0x2);

    /* Set extended graphics registers */
    outp(0x3DD, 0x80);

    outp(0x03D8, 0x2);
    /* Program 6845 crt controlller */
    for (i = 0; i < 16; ++i)
    {
        outp(0x3D4, i);
        outp(0x3D5, parms[i]);
    }
    outp(0x3D8, 0x0A);
    outp(0x3D9, 0x30);

    outp(0x3dd, 0x80);

    pcscreen = destscreen = (byte *)0xB0000;

    SetDWords(vrambuffer, 0, 16384);
    SetDWords(pcscreen, 0, 16384);
#endif
#ifdef MODE_CVB
    regs.w.ax = 0x06;
    int386(0x10, (union REGS *)&regs, &regs);
    outp(0x3D8, 0x1A); // Enable color burst
    pcscreen = destscreen = (byte *)0xB8000;

    SetDWords(vrambuffer, 0, 4096);
    SetDWords(pcscreen, 0, 4096);
#endif
#ifdef MODE_HERC
    // byte Graph_720x348[12] = {0x03, 0x36, 0x2D, 0x2E, 0x07, 0x5B, 0x02, 0x57, 0x57, 0x02, 0x03, 0x0A};
    byte Graph_640x400[12] = {0x03, 0x34, 0x28, 0x2A, 0x47, 0x69, 0x00, 0x64, 0x65, 0x02, 0x03, 0x0A};
    // byte Graph_640x200[12] = {0x03, 0x6E, 0x28, 0x2E, 0x07, 0x67, 0x0A, 0x64, 0x65, 0x02, 0x01, 0x0A};
    int i;

    outp(0x03BF, Graph_640x400[0]);
    for (i = 0; i < 10; i++)
    {
        outp(0x03B4, i);
        outp(0x03B5, Graph_640x400[i + 1]);
    }
    outp(0x03B8, Graph_640x400[11]);
    pcscreen = destscreen = (byte *)0xB0000;

    SetDWords(vrambuffer, 0, 8192);
    SetDWords(pcscreen, 0, 8192);
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

#if defined(MODE_Y) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT) || defined(MODE_V2)
    I_TestFastSetPalette();
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
    byte Text_80x25[12] = {0x00, 0x61, 0x50, 0x52, 0x0F, 0x19, 0x06, 0x19, 0x19, 0x02, 0x0D, 0x08};
    int i;

    outp(0x03BF, Text_80x25[0]);
    for (i = 0; i < 10; i++)
    {
        outp(0x03B4, i);
        outp(0x03B5, Text_80x25[i + 1]);
    }
    outp(0x03B8, Text_80x25[11]);
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

    keyboardque[kbdhead & (KBDQUESIZE - 1)] = _inbyte(0x60);
    kbdhead++;

    // acknowledge the interrupt

    _outbyte(0x20, 0x20);
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
