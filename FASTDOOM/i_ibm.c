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
#include "ns_cd.h"

#include "am_map.h"

#include "std_func.h"

#include "options.h"

#include "math.h"


#if defined(MODE_CGA_AFH)
#include "i_cgaafh.h"
#endif

#if defined(MODE_CGA16)
#include "i_cga16.h"
#endif

#if defined(MODE_13H)
#include "i_vga.h"
#include "i_vga13h.h"
#endif

#if defined(MODE_PCP)
#include "i_pcp.h"
#endif

#if defined(MODE_SIGMA)
#include "i_sigma.h"
#endif

#if defined(MODE_CGA)
#include "i_cga4.h"
#endif

#if defined(MODE_CVB)
#include "i_cgacvb.h"
#endif

#if defined(MODE_HERC)
#include "i_herc.h"
#endif

#if defined(MODE_INCOLOR)
#include "i_incolor.h"
#endif

#if defined(MODE_CGA_BW)
#include "i_cgabw.h"
#endif

#if defined(MODE_EGA)
#include "i_ega320.h"
#endif

#if defined(MODE_VBE2)
#include "i_vga.h"
#endif

#if defined(MODE_Y)
#include "i_vgay.h"
#include "i_vga.h"
#endif

#if defined(MODE_Y_HALF)
#include "i_vgayh.h"
#include "i_vga.h"
#endif

#if defined(MODE_X)
#include "i_vgax.h"
#include "i_vga.h"
#endif

#if defined(MODE_VBE2_DIRECT)
#include "i_vga.h"
#endif

#if defined(MODE_CGA512)
#include "i_cga512.h"
#endif

#if defined(TEXT_MODE)
#include "i_text.h"
#endif

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
#include "i_vesa.h"
#endif

#if defined(MODE_MDA)
#include "i_mda.h"
#endif

//
// Macros
//

#define DPMI_INT 0x31

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

unsigned int ticcount;
unsigned int mscount;
unsigned int fps;

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

//
// Graphics mode
//

#if defined(USE_BACKBUFFER)
int updatestate;
#endif
byte *pcscreen, *destscreen, *destview;
unsigned short *currentscreen;

//
// I_UpdateBox
//
#if defined(MODE_VBE2_DIRECT)
void I_UpdateBox(int x, int y, int w, int h)
{
    byte *dest;
    byte *source;
    int i;
    int offset = MulScreenWidth(y) + x;

    dest = destscreen + offset;
    source = screen0 + offset;

    if (w & 1)
    {
        for (i = y; i < y + h; i++)
        {
            CopyBytes(source, dest, w);
            dest += SCREENWIDTH;
            source += SCREENWIDTH;
        }
    }
    else
    {
        w /= 2;

        for (i = y; i < y + h; i++)
        {
            CopyWords(source, dest, w);
            dest += SCREENWIDTH;
            source += SCREENWIDTH;
        }
    }
}

void I_UpdateBoxTransparent(int x, int y, int w, int h)
{
    byte *dest;
    byte *source;
    int i;
    int offset = MulScreenWidth(y) + x;

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

        dest += SCREENWIDTH;
        source += SCREENWIDTH;
    }
}
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF)
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
    offset = MulScreenWidth(y) + sp_x1 * 8;
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
    offset = MulScreenWidth(y) + sp_x1 * 8;
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
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
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

void I_CalculateFPS(void)
{
    static unsigned int fps_counter, fps_starttime, fps_nextcalculation;
    unsigned int opt1, opt2;

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
        opt1 = 35 * 10 * (fps_counter - 1);
        opt2 = ticcount - fps_starttime;
        fps = opt1 / opt2;
        fps_nextcalculation = ticcount + 12;
        fps_counter = 0; // flush old data
    }
}

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

void I_FinishHerculesHalfMode(void)
{
    byte Text_80x25[12] = {0x00, 0x61, 0x50, 0x52, 0x0F, 0x19, 0x06, 0x19, 0x19, 0x02, 0x0D, 0x08};
    int i;

    outp(0x03BF, Text_80x25[0]);
    for (i = 0; i < 10; i++)
    {
        outp(0x03B4, i);
        outp(0x03B5, Text_80x25[i + 1]);
    }
    outp(0x03B8, Text_80x25[11]);

    SetDWords((byte *)0xB0000, 0, 8192);
}
#endif

void I_InitGraphics(void)
{

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

#if defined(MODE_T4025) || defined(MODE_T4050)
    TEXT_40x25_InitGraphics();
#endif

#if defined(MODE_T8025)
    TEXT_80x25_InitGraphics();
#endif

#if defined(MODE_T8050) || defined(MODE_T8043)
    TEXT_80x25_Double_InitGraphics();
#endif

#if defined(MODE_MDA)
    MDA_InitGraphics();
#endif

#if defined(MODE_Y)
    VGA_Y_InitGraphics();
#endif

#if defined(MODE_Y_HALF)
    VGA_Y_Half_InitGraphics();
#endif

#if defined(MODE_X)
    VGA_X_InitGraphics();
#endif

#if defined(MODE_13H)
    VGA_13H_InitGraphics();
#endif

#if defined(MODE_CGA)
    CGA_InitGraphics();
#endif

#if defined(MODE_CGA512)
    CGA_512_InitGraphics();
#endif

#if defined(MODE_CGA_BW)
    CGA_BW_InitGraphics();
#endif

#if defined(MODE_PCP)
    PCP_InitGraphics();
#endif

#if defined(MODE_SIGMA)
    Sigma_InitGraphics();
#endif

#if defined(MODE_EGA)
    EGA_InitGraphics();
#endif

#if defined(MODE_CVB)
    CGA_CVBS_InitGraphics();
#endif

#if defined(MODE_HERC)
    HERC_InitGraphics();
#endif

#if defined(MODE_INCOLOR)
    InColor_InitGraphics();
#endif

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
    VBE2_InitGraphics();
#endif

#if defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
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

#if defined(MODE_INCOLOR)
    InColor_ShutdownGraphics();
#endif

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)
    VBE_Done();
#endif

#ifdef SUPPORTS_HERCULES_AUTOMAP
    if (HERCmap)
        I_FinishHerculesHalfMode();
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

        ev.data1 = scantokey[k];

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

void I_TimerMS(task *task)
{
    mscount++;
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

    keyboardque[kbdhead & (KBDQUESIZE - 1)] = InByte60h();
    kbdhead++;

    // acknowledge the interrupt

    OutByte20h(0x20);
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
    printf("I_StartupDPMI\n");
    I_StartupDPMI();
    printf("I_StartupMouse\n");
    I_StartupMouse();
    printf("I_StartupKeyboard\n");
    I_StartupKeyboard();
    printf("I_StartupSound\n");
    I_StartupSound();

#if defined(MODE_13H)
    I_UpdateFinishFunc();
#endif
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

    if (snd_MusicDevice == snd_CD)
        CD_Exit();

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
#if defined(MODE_HERC) || defined(MODE_MDA) || defined(MODE_INCOLOR)
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

    if (snd_MusicDevice == snd_CD)
        CD_Exit();

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
    printf("Available DPMI memory: %d Kb\n", heap >> 10);

    do
    {
        ptr = malloc(heap);
        if (!ptr) heap -= 0x400; // try again with 1Kb less
    } while (!ptr);

    printf("Zone memory: %d Kb\n", heap >> 10);

    *size = heap;
    return ptr;
}

void *I_DosMemAlloc(unsigned long size)
{
    union REGS Regs;

    // DPMI allocate DOS memory
    Regs.x.eax = 0x0100;

    // Number of paragraphs requested
    Regs.x.ebx = (size + 15) >> 4;

    int386(0x31, &Regs, &Regs);

    if (Regs.x.cflag != 0)
    {
        // Failed
        return ((unsigned long)0);
    }

    return ((void *)((Regs.x.eax & 0xFFFF) << 4));
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

int I_GetCPUModel(void)
{
    int result;
    union REGS r;
    r.x.eax = 0x0400; // DPMI get version
    int386(0x31, &r, &r);
    result = (r.x.ecx & 0xff) * 100 + 86; // Returns: 386,486,586,686
    return result;
}
