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
//  IBM DOS VGA graphics and key/mouse/joystick.
//

#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <graph.h>
#include "d_main.h"
#include "doomstat.h"
#include "doomdef.h"
#include "r_local.h"
#include "sounds.h"
#include "i_system.h"
#include "i_sound.h"
#include "g_game.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "dpmiapi.h"
//
// Macros
//

#define DPMI_INT 0x31
//#define NOKBD
//#define NOTIMER

//
// Code
//

void I_StartupNet(void);
void I_ShutdownNet(void);
void I_ReadExternDriver(void);

typedef struct
{
    unsigned        edi, esi, ebp, reserved, ebx, edx, ecx, eax;
    unsigned short  flags, es, ds, fs, gs, ip, cs, sp, ss;
} dpmiregs_t;

extern dpmiregs_t dpmiregs;

void I_ReadMouse(void);
void I_InitDiskFlash(void);

extern int usemouse, usejoystick;

//
// Constants
//

#define SC_INDEX                0x3C4
#define SC_RESET                0
#define SC_CLOCK                1
#define SC_MAPMASK              2
#define SC_CHARMAP              3
#define SC_MEMMODE              4

#define CRTC_INDEX              0x3D4
#define CRTC_H_TOTAL            0
#define CRTC_H_DISPEND          1
#define CRTC_H_BLANK            2
#define CRTC_H_ENDBLANK         3
#define CRTC_H_RETRACE          4
#define CRTC_H_ENDRETRACE       5
#define CRTC_V_TOTAL            6
#define CRTC_OVERFLOW           7
#define CRTC_ROWSCAN            8
#define CRTC_MAXSCANLINE        9
#define CRTC_CURSORSTART        10
#define CRTC_CURSOREND          11
#define CRTC_STARTHIGH          12
#define CRTC_STARTLOW           13
#define CRTC_CURSORHIGH         14
#define CRTC_CURSORLOW          15
#define CRTC_V_RETRACE          16
#define CRTC_V_ENDRETRACE       17
#define CRTC_V_DISPEND          18
#define CRTC_OFFSET             19
#define CRTC_UNDERLINE          20
#define CRTC_V_BLANK            21
#define CRTC_V_ENDBLANK         22
#define CRTC_MODE               23
#define CRTC_LINECOMPARE        24


#define GC_INDEX                0x3CE
#define GC_SETRESET             0
#define GC_ENABLESETRESET       1
#define GC_COLORCOMPARE         2
#define GC_DATAROTATE           3
#define GC_READMAP              4
#define GC_MODE                 5
#define GC_MISCELLANEOUS        6
#define GC_COLORDONTCARE        7
#define GC_BITMASK              8

#define ATR_INDEX               0x3c0
#define ATR_MODE                16
#define ATR_OVERSCAN            17
#define ATR_COLORPLANEENABLE    18
#define ATR_PELPAN              19
#define ATR_COLORSELECT         20

#define STATUS_REGISTER_1       0x3da

#define PEL_WRITE_ADR           0x3c8
#define PEL_READ_ADR            0x3c7
#define PEL_DATA                0x3c9
#define PEL_MASK                0x3c6

boolean grmode;

//
// joystick vars
//

boolean joystickpresent;
extern unsigned joystickx, joysticky;
boolean I_ReadJoystick(void); // returns false if not connected

#define VBLCOUNTER 34000 // hardware tics to a frame


#define TIMERINT 8
#define KEYBOARDINT 9

#define CRTCOFF (_inbyte(STATUS_REGISTER_1)&1)
#define CLI     _disable()
#define STI     _enable()

#define _outbyte(x,y) (outp(x,y))
#define _outhword(x,y) (outpw(x,y))

#define _inbyte(x) (inp(x))
#define _inhword(x) (inpw(x))

#define MOUSEB1 1
#define MOUSEB2 2
#define MOUSEB3 4

boolean mousepresent;

int ticcount;

// REGS stuff used for int calls
union REGS regs;
struct SREGS segregs;

boolean novideo; // if true, stay in text mode for debugging

#define KBDQUESIZE 32
byte keyboardque[KBDQUESIZE];
int kbdtail, kbdhead;

#define KEY_LSHIFT      0xfe

#define KEY_INS         (0x80+0x52)
#define KEY_DEL         (0x80+0x53)
#define KEY_PGUP        (0x80+0x49)
#define KEY_PGDN        (0x80+0x51)
#define KEY_HOME        (0x80+0x47)
#define KEY_END         (0x80+0x4f)

#define SC_RSHIFT       0x36
#define SC_LSHIFT       0x2a
void DPMIInt(int i);
void I_WaitVBL(int vbls);
void I_StartupCyberMan(void);
void I_StartupSound(void);
void I_ShutdownSound(void);
void I_ShutdownTimer(void);

byte scantokey[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '-',    '=',    KEY_BACKSPACE, 9, // 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13 ,    KEY_RCTRL,'a',  's',      // 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
	39 ,    '`',    KEY_LSHIFT,92,  'z',    'x',    'c',    'v',      // 2
	'b',    'n',    'm',    ',',    '.',    '/',    KEY_RSHIFT,'*',
	KEY_RALT,' ',   0  ,    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,   // 3
	KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,0  ,    0  , KEY_HOME,
	KEY_UPARROW,KEY_PGUP,'-',KEY_LEFTARROW,'5',KEY_RIGHTARROW,'+',KEY_END, //4
	KEY_DOWNARROW,KEY_PGDN,KEY_INS,KEY_DEL,0,0,             0,              KEY_F11,
	KEY_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7
};

typedef struct
{
    int irq;
    ticcmd_t cmd;
} extapi_t;

extapi_t *extcontrol;

ticcmd_t emptycmd;

ticcmd_t* I_BaseTiccmd(void)
{
    if (!extcontrol)
    {
        return &emptycmd;
    }
    DPMIInt(extcontrol->irq);
    return &extcontrol->cmd;
}

//
// I_GetTime
// Returns time in 1/35th second tics.
//
int I_GetTime(void)
{
	return ticcount;
}

//
// I_ColorBorder
//
void I_ColorBorder(void)
{
    int i;

    I_WaitVBL(1);
    _outbyte(PEL_WRITE_ADR, 0);
    for (i = 0; i < 3; i++)
    {
        _outbyte(PEL_DATA, 63);
    }
}

//
// I_UnColorBorder
//
void I_UnColorBorder(void)
{
    int i;

    I_WaitVBL(1);
    _outbyte(PEL_WRITE_ADR, 0);
    for (i = 0; i < 3; i++)
    {
        _outbyte(PEL_DATA, 0);
    }
}

//
// User input
//

//
// I_WaitVBL
//
void I_WaitVBL(int vbls)
{
    int stat;

    if (novideo)
    {
        return;
    }
    while (vbls--)
    {
        do
        {
            stat = inp(STATUS_REGISTER_1);
            if (stat & 8)
            {
                break;
            }
        } while (1);
        do
        {
            stat = inp(STATUS_REGISTER_1);
            if ((stat & 8) == 0)
            {
                break;
            }
        } while (1);
    }
}

//
// I_SetPalette
// Palette source must use 8 bit RGB elements.
//
void I_SetPalette(byte *palette)
{
	int i;

	if(novideo)
	{
		return;
	}
	I_WaitVBL(1);
	_outbyte(PEL_WRITE_ADR, 0);
	for(i = 0; i < 768; i++)
	{
		_outbyte(PEL_DATA, (gammatable[usegamma][*palette++])>>2);
	}
}

//
// Graphics mode
//

byte *pcscreen, *currentscreen, *destscreen, *destview;


//
// I_UpdateBox
//
void I_UpdateBox(int x, int y, int w, int h)
{
    int i, j, k, count;
    int sp_x1, sp_x2;
    int poffset;
    int offset;
    int pstep;
    int step;
    byte *dest, *source;

    if (x < 0 || y < 0 || w <= 0 || h <= 0
     || x + w > SCREENWIDTH || y + h > SCREENHEIGHT)
    {
        I_Error("Bad I_UpdateBox (%i, %i, %i, %i)", x, y, w, h);
    }

    sp_x1 = x / 8;
    sp_x2 = (x + w) / 8;
    count = sp_x2 - sp_x1 + 1;
    offset = y * SCREENWIDTH + sp_x1 * 8;
    step = SCREENWIDTH - count * 8;
    poffset = offset / 4;
    pstep = step / 4;

    outp(SC_INDEX, SC_MAPMASK);

    for (i = 0; i < 4; i++)
    {
        outp(SC_INDEX + 1, 1 << i);
        source = &screens[0][offset + i];
        dest = destscreen + poffset;

        for (j = 0; j < h; j++)
        {
            k = count;
            while (k--)
            {
                *(unsigned short *)dest = (unsigned short)(((*(source + 4)) << 8) + (*source));
                dest += 2;
                source += 8;
            }

            source += step;
            dest += pstep;
        }
    }
}

//
// I_UpdateNoBlit
//
int olddb[2][4];
void I_UpdateNoBlit(void)
{
    int realdr[4];
    int x, y, w, h;
    // Set current screen
    currentscreen = destscreen;

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
    memcpy(olddb[0], olddb[1], 16);
    memcpy(olddb[1], dirtybox, 16);

    // Update screen
    if (realdr[BOXBOTTOM] <= realdr[BOXTOP])
    {
        x = realdr[BOXLEFT];
        y = realdr[BOXBOTTOM];
        w = realdr[BOXRIGHT] - realdr[BOXLEFT] + 1;
        h = realdr[BOXTOP] - realdr[BOXBOTTOM] + 1;
        I_UpdateBox(x, y, w, h);
    }
    // Clear box
    M_ClearBox(dirtybox);
}

//
// I_FinishUpdate
//
void I_FinishUpdate(void)
{
    static int lasttic;
    int tics;
    int i;
    if (devparm)
    {
        i = ticcount;
        tics = i - lasttic;
        lasttic = i;
        if (tics > 20) tics = 20;
        outpw(SC_INDEX, 0x102);
        for (i = 0; i < tics; i++)
        {
            destscreen[(SCREENHEIGHT - 1)*SCREENWIDTH / 4 + i] = 0xff;
        }
        for (; i < 20; i++)
        {
            destscreen[(SCREENHEIGHT - 1)*SCREENWIDTH / 4 + i] = 0x0;
        }
    }
    outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0xc);

    //Next plane
    destscreen += 0x4000;
    if (destscreen == (byte*)0xac000)
    {
        destscreen = (byte*)0xa0000;
    }
}

//
// I_InitGraphics
//
void I_InitGraphics(void)
{
    if (novideo)
    {
        return;
    }
    grmode = true;
    regs.w.ax = 0x13;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = currentscreen = (byte *)0xa0000;
    destscreen = (byte *)0xa4000;

    outp(SC_INDEX, SC_MEMMODE);
    outp(SC_INDEX + 1, (inp(SC_INDEX + 1)&~8) | 4);
    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1)&~0x13);
    outp(GC_INDEX, GC_MISCELLANEOUS);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1)&~2);
    outpw(SC_INDEX, 0xf02);
    memset(pcscreen, 0, 0x10000);
    outp(CRTC_INDEX, CRTC_UNDERLINE);
    outp(CRTC_INDEX + 1, inp(CRTC_INDEX + 1)&~0x40);
    outp(CRTC_INDEX, CRTC_MODE);
    outp(CRTC_INDEX + 1, inp(CRTC_INDEX + 1) | 0x40);
    outp(GC_INDEX, GC_READMAP);

    I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
    I_InitDiskFlash();
}

//
// I_ShutdownGraphics
//
void I_ShutdownGraphics(void)
{
    if (*(byte *)0x449 == 0x13) // don't reset mode if it didn't get set
    {
        regs.w.ax = 3;
        int386(0x10, &regs, &regs); // back to text mode
    }
}

//
// I_ReadScreen
// Reads the screen currently displayed into a linear buffer.
//
void I_ReadScreen(byte *scr)
{
	int i;
	int j;

	outp(GC_INDEX, GC_READMAP);

	for (i = 0; i < 4; i++)
	{
		outp(GC_INDEX+1, i);
		for (j = 0; j < SCREENWIDTH*SCREENHEIGHT/4; j++)
		{
			scr[i+j*4] = currentscreen[j];
		}
	}
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


#define SC_UPARROW      0x48
#define SC_DOWNARROW    0x50
#define SC_LEFTARROW    0x4b
#define SC_RIGHTARROW   0x4d

void I_StartTic(void)
{
    int k;
    event_t ev;

    I_ReadMouse();

    //
    // keyboard events
    //
    while (kbdtail < kbdhead)
    {
        k = keyboardque[kbdtail&(KBDQUESIZE - 1)];
        kbdtail++;

        // extended keyboard shift key bullshit
        if ((k & 0x7f) == SC_LSHIFT || (k & 0x7f) == SC_RSHIFT)
        {
            if (keyboardque[(kbdtail - 2)&(KBDQUESIZE - 1)] == 0xe0)
            {
                continue;
            }
            k &= 0x80;
            k |= SC_RSHIFT;
        }

        if (k == 0xe0)
        {
            continue;   // special / pause keys
        }
        if (keyboardque[(kbdtail - 2)&(KBDQUESIZE - 1)] == 0xe1)
        {
            continue;   // pause key bullshit
        }
        if (k == 0xc5 && keyboardque[(kbdtail - 2)&(KBDQUESIZE - 1)] == 0x9d)
        {
            ev.type = ev_keydown;
            ev.data1 = KEY_PAUSE;
            D_PostEvent(&ev);
            continue;
        }

        if (k & 0x80)
            ev.type = ev_keyup;
        else
            ev.type = ev_keydown;
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

void I_ReadKeys(void)
{
    int k;

    while (1)
    {
        while (kbdtail < kbdhead)
        {
            k = keyboardque[kbdtail&(KBDQUESIZE - 1)];
            kbdtail++;
            printf("0x%x\n", k);
            if (k == 1)
                I_Quit();
        }
    }
}

void I_ColorBlack(int r, int g, int b)
{
    _outbyte(PEL_WRITE_ADR, 0);
    _outbyte(PEL_DATA, r);
    _outbyte(PEL_DATA, g);
    _outbyte(PEL_DATA, b);
}

//
// Timer interrupt
//


//
// I_TimerISR
//
int I_TimerISR(void)
{
    ticcount++;
    return 0;
}

//
// Keyboard
//

void (__interrupt __far *oldkeyboardisr) () = NULL;

int lastpress;

//
// I_KeyboardISR
//

void __interrupt I_KeyboardISR(void)
{
// Get the scan code

    keyboardque[kbdhead&(KBDQUESIZE - 1)] = lastpress = _inbyte(0x60);
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
	_dos_setvect (0x8000 | KEYBOARDINT, I_KeyboardISR);

    //I_ReadKeys ();
}


void I_ShutdownKeyboard(void)
{
	if (oldkeyboardisr)
		_dos_setvect (KEYBOARDINT, oldkeyboardisr);
	*(short *)0x41c = *(short *)0x41a;      // clear bios key buffer
}


//
// Mouse
//

int I_ResetMouse(void)
{
	regs.w.ax = 0; // reset
	int386 (0x33, &regs, &regs);
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

    I_StartupCyberMan();
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
    if (!mousepresent)
    {
        return;
    }

    ev.type = ev_mouse;

    memset(&dpmiregs, 0, sizeof(dpmiregs));
    dpmiregs.eax = 3;   // read buttons / position
    DPMIInt(0x33);
    ev.data1 = dpmiregs.ebx;

    dpmiregs.eax = 11;  // read counters
    DPMIInt(0x33);
    ev.data2 = (short)dpmiregs.ecx;
    ev.data3 = -(short)dpmiregs.edx;

    D_PostEvent(&ev);
}

//
// Joystick
//

int joyxl, joyxh, joyyl, joyyh;

boolean WaitJoyButton(void)
{
    int oldbuttons, buttons;

    oldbuttons = 0;
    do
    {
        I_WaitVBL(1);
        buttons = ((inp(0x201) >> 4) & 1) ^ 1;
        if (buttons != oldbuttons)
        {
            oldbuttons = buttons;
            continue;
        }

        if ((lastpress & 0x7f) == 1)
        {
            joystickpresent = false;
            return false;
        }
    } while (!buttons);

    do
    {
        I_WaitVBL(1);
        buttons = ((inp(0x201) >> 4) & 1) ^ 1;
        if (buttons != oldbuttons)
        {
            oldbuttons = buttons;
            continue;
        }

        if ((lastpress & 0x7f) == 1)
        {
            joystickpresent = false;
            return false;
        }
    } while (buttons);

    return true;
}


//
// I_StartupJoystick
//
int basejoyx, basejoyy;

void I_StartupJoystick(void)
{
    int buttons;
    int count;
    int centerx, centery;

    joystickpresent = 0;
    if (M_CheckParm("-nojoy") || !usejoystick)
    {
        return;
    }

    if (!I_ReadJoystick())
    {
        joystickpresent = false;
        printf("joystick not found\n");
        return;
    }
    printf("joystick found\n");
    joystickpresent = true;

    printf("CENTER the joystick and press button 1:");
    if (!WaitJoyButton())
    {
        return;
    }
    I_ReadJoystick();
    centerx = joystickx;
    centery = joysticky;

    printf("\nPush the joystick to the UPPER LEFT corner and press button 1:");
    if (!WaitJoyButton())
    {
        return;
    }
    I_ReadJoystick();
    joyxl = (centerx + joystickx) / 2;
    joyyl = (centerx + joysticky) / 2;

    printf("\nPush the joystick to the LOWER RIGHT corner and press button 1:");
    if (!WaitJoyButton())
    {
        return;
    }
    I_ReadJoystick();
    joyxh = (centerx + joystickx) / 2;
    joyyh = (centery + joysticky) / 2;
    printf("\n");
}

//
// I_StartFrame
//
void I_StartFrame(void)
{
    event_t ev;

    //
    // joystick events
    //
    if (!joystickpresent)
    {
        return;
    }

    I_ReadJoystick();
    ev.type = ev_joystick;
    ev.data1 = ((inp(0x201) >> 4) & 15) ^ 15;

    if (joystickx < joyxl)
    {
        ev.data2 = -1;
    }
    else if (joystickx > joyxh)
    {
        ev.data2 = 1;
    }
    else
    {
        ev.data2 = 0;
    }
    if (joysticky < joyyl)
    {
        ev.data3 = -1;
    }
    else if (joysticky > joyyh)
    {
        ev.data3 = 1;
    }
    else
    {
        ev.data3 = 0;
    }

    D_PostEvent(&ev);
}

//
// DPMI stuff
//

#define REALSTACKSIZE 1024

dpmiregs_t dpmiregs;

unsigned realstackseg;

void I_DivException(void);
int I_SetDivException(void);

//
// DPMIFarCall
//
void DPMIFarCall(void)
{
    segread(&segregs);
    regs.w.ax = 0x301;
    regs.w.bx = 0;
    regs.w.cx = 0;
    regs.x.edi = (unsigned)&dpmiregs;
    segregs.es = segregs.ds;
    int386x(DPMI_INT, &regs, &regs, &segregs);
}


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
    realstackseg = (int)I_AllocLow (1024) >> 4;

//
// lock the entire program down
//

    _dpmi_lockregion (&__begtext, &___Argc - &__begtext);


//
// catch divide by 0 exception
//
#if 0
    segread(&segregs);
    regs.w.ax = 0x0203; // DPMI set processor exception handler vector
    regs.w.bx = 0;  // int 0
    regs.w.cx = segregs.cs;
    regs.x.edx = (int)&I_DivException;
    printf("%x : %x\n", regs.w.cx, regs.x.edx);
    int386(DPMI_INT, &regs, &regs);
#endif

#if 0
    n = I_SetDivException();
    printf("return: %i\n", n);
    n = 100;
    d = 0;
    printf("100 / 0 = %i\n", n / d);

    exit(1);
#endif
}

//
// Timer interrupt
//

void (__interrupt __far *oldtimerisr) ();

//
// IO_TimerISR
//

//void __interrupt IO_TimerISR(void)

void __interrupt __far IO_TimerISR(void)
{
    ticcount++;
    _outbyte(0x20, 0x20);                            // Ack the interrupt
}

//
// IO_SetTimer0
// Sets system timer 0 to the specified speed
//
void IO_SetTimer0(int speed)
{
    if (speed > 0 && speed < 150)
    {
        I_Error("INT_SetTimer0: %i is a bad value", speed);
    }

    _outbyte(0x43, 0x36);                            // Change timer 0
    _outbyte(0x40, speed);
    _outbyte(0x40, speed >> 8);
}



//
// IO_StartupTimer
//
void IO_StartupTimer(void)
{
    oldtimerisr = _dos_getvect(TIMERINT);

    _dos_setvect(0x8000 | TIMERINT, IO_TimerISR);
    IO_SetTimer0(VBLCOUNTER);
}

void IO_ShutdownTimer(void)
{
    if (oldtimerisr)
    {
        IO_SetTimer0(0);              // back to 18.4 ips
        _dos_setvect(TIMERINT, oldtimerisr);
    }
}

//
// I_Init
// hook interrupts and set graphics mode
//
void I_Init(void)
{
    int p;
    novideo = M_CheckParm("novideo");
    p = M_CheckParm("-control");
    if (p)
    {
        extcontrol = (extapi_t*)atoi(myargv[p + 1]);
        printf("Using external control API\n");
    }
    printf("I_StartupDPMI\n");
    I_StartupDPMI();
    printf("I_StartupMouse\n");
    I_StartupMouse();
    printf("I_StartupJoystick\n");
    I_StartupJoystick();
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
void I_Error (char *error, ...)
{
    va_list argptr;

    D_QuitNetGame();
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
    else
    {
        D_QuitNetGame();
    }
    M_SaveDefaults();
    scr = (byte*)W_CacheLumpName("ENDOOM", PU_CACHE);
    I_ShutdownGraphics();
    I_ShutdownSound();
    I_ShutdownTimer();
    I_ShutdownMouse();
    I_ShutdownKeyboard();
    memcpy((void *)0xb8000, scr, 80 * 25 * 2);
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

    memset(meminfo, 0, sizeof(meminfo));
    segread(&segregs);
    segregs.es = segregs.ds;
    regs.w.ax = 0x500; // get memory info
    regs.x.edi = (int)&meminfo;
    int386x(0x31, &regs, &regs, &segregs);

    heap = meminfo[0];
    printf("DPMI memory: 0x%x", heap);

    do
    {
        heap -= 0x20000; // leave 128k alone
        if (heap > 0x800000)
        {
            heap = 0x800000;
        }
        ptr = malloc(heap);
    } while (!ptr);

    printf(", 0x%x allocated for zone\n", heap);
    if (heap < 0x180000)
    {
        printf("\n");
        printf("Insufficient memory!  You need to have at least 3.7 megabytes of total\n");
        printf("free memory available for DOOM to execute.  Reconfigure your CONFIG.SYS\n");
        printf("or AUTOEXEC.BAT to load fewer device drivers or TSR's.  We recommend\n");
        printf("creating a custom boot menu item in your CONFIG.SYS for optimum DOOMing.\n");
        printf("Please consult your DOS manual (\"Making more memory available\") for\n");
        printf("information on how to free up more memory for DOOM.\n\n");
        printf("DOOM aborted.\n");
        exit(1);
    }
#if 0
    regs.w.ax = 0x501; // allocate linear block
    regs.w.bx = heap >> 16;
    regs.w.cx = heap & 0xffff;
    int386(0x31, &regs, &regs);
    if (regs.w.cflag)
    {
        I_Error("Couldn't allocate DPMI memory!");
    }
    block = (regs.w.si << 16) + regs.w.di;
#endif

    *size = heap;
    return ptr;
}

//
// Disk icon flashing
//

void I_InitDiskFlash(void)
{
    void *pic;
    byte *temp;

    if (M_CheckParm("-cdrom"))
    {
        pic = W_CacheLumpName("STCDDISK", PU_CACHE);
    }
    else
    {
        pic = W_CacheLumpName("STDISK", PU_CACHE);
    }
    temp = destscreen;
    destscreen = (byte *)0xac000;
    V_DrawPatchDirect(SCREENWIDTH - 16, SCREENHEIGHT - 16, 0, pic);
    destscreen = temp;
}

// draw disk icon
void I_BeginRead(void)
{
    byte *src, *dest;
    int y;

    if (!grmode)
    {
        return;
    }

    // write through all planes
    outp(SC_INDEX, SC_MAPMASK);
    outp(SC_INDEX + 1, 15);
    // set write mode 1
    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1) | 1);

    // copy to backup
    src = currentscreen + 184 * 80 + 304 / 4;
    dest = (byte *)0xac000 + 184 * 80 + 288 / 4;
    for (y = 0; y<16; y++)
    {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        src += 80;
        dest += 80;
    }

    // copy disk over
    dest = currentscreen + 184 * 80 + 304 / 4;
    src = (byte *)0xac000 + 184 * 80 + 304 / 4;
    for (y = 0; y<16; y++)
    {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        src += 80;
        dest += 80;
    }


    // set write mode 0
    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1)&~1);
}

// erase disk icon
void I_EndRead(void)
{
    byte *src, *dest;
    int y;

    if (!grmode)
    {
        return;
    }

    // write through all planes
    outp(SC_INDEX, SC_MAPMASK);
    outp(SC_INDEX + 1, 15);
    // set write mode 1
    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1) | 1);


    // copy disk over
    dest = currentscreen + 184 * 80 + 304 / 4;
    src = (byte *)0xac000 + 184 * 80 + 288 / 4;
    for (y = 0; y<16; y++)
    {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        src += 80;
        dest += 80;
    }

    // set write mode 0
    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1)&~1);
}



//
// I_AllocLow
//
byte *I_AllocLow (int length)
{
    byte *mem;

    // DPMI call 100h allocates DOS memory
    segread(&segregs);
    regs.w.ax = 0x0100; // DPMI allocate DOS memory
    regs.w.bx = (length + 15) / 16;
    int386(DPMI_INT, &regs, &regs);
    //segment = regs.w.ax;
    //selector = regs.w.dx;
    if (regs.w.cflag != 0)
    {
        I_Error("I_AllocLow: DOS alloc of %i failed, %i free",
                length, regs.w.bx * 16);
    }

    mem = (void *)((regs.x.eax & 0xFFFF) << 4);

    memset(mem, 0, length);
    return mem;
}

//
// Networking
//

/* // FUCKED LINES
typedef struct
{
	char    priv[508];
} doomdata_t;
*/ // FUCKED LINES

#define DOOMCOM_ID 0x12345678l

/* // FUCKED LINES
typedef struct
{
	long    id;
	short   intnum;                 // DOOM executes an int to execute commands

// communication between DOOM and the driver
	short   command;                // CMD_SEND or CMD_GET
	short   remotenode;             // dest for send, set by get (-1 = no packet)
	short   datalength;             // bytes in doomdata to be sent

// info common to all nodes
	short   numnodes;               // console is allways node 0
	short   ticdup;                 // 1 = no duplication, 2-5 = dup for slow nets
	short   extratics;              // 1 = send a backup tic in every packet
	short   deathmatch;             // 1 = deathmatch
	short   savegame;               // -1 = new game, 0-5 = load savegame
	short   episode;                // 1-3
	short   map;                    // 1-9
	short   skill;                  // 1-5

// info specific to this node
	short   consoleplayer;
	short   numplayers;
	short   angleoffset;    // 1 = left, 0 = center, -1 = right
	short   drone;                  // 1 = drone

// packet data to be sent
	doomdata_t      data;
} doomcom_t;
*/ // FUCKED LINES

extern doomcom_t *doomcom;

//
// I_InitNetwork
//
void I_InitNetwork(void)
{
	int i;

	i = M_CheckParm ("-net");
    if (!i)
    {
        //
        // single player game
        //
        doomcom = malloc(sizeof(*doomcom));
        if (!doomcom)
        {
            I_Error("malloc() in I_InitNetwork() failed");
        }
        memset(doomcom, 0, sizeof(*doomcom));
        netgame = false;
        doomcom->id = DOOMCOM_ID;
        doomcom->numplayers = doomcom->numnodes = 1;
        doomcom->deathmatch = false;
        doomcom->consoleplayer = 0;
        doomcom->ticdup = 1;
        doomcom->extratics = 0;
        return;
    }

    netgame = true;
    doomcom = (doomcom_t *)atoi(myargv[i + 1]);
    //DEBUG
    doomcom->skill = startskill;
    doomcom->episode = startepisode;
    doomcom->map = startmap;
    doomcom->deathmatch = deathmatch;

}

void I_NetCmd(void)
{
    if (!netgame)
    {
        I_Error("I_NetCmd when not in netgame");
    }
    DPMIInt(doomcom->intnum);
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
