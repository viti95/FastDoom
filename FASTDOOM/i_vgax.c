#include <string.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "doomtype.h"
#include "i_ibm.h"
#include "v_video.h"
#include "tables.h"
#include "math.h"
#include "i_system.h"
#include "doomstat.h"
#include "m_menu.h"
#include "i_vga.h"

#if defined(MODE_X)

#define MISC_OUTPUT 0x03c2 /* VGA misc. output register */
#define SC_INDEX 0x03c4    /* VGA sequence controller */
#define SC_DATA 0x03c5
#define PALETTE_INDEX 0x03c8 /* VGA digital-to-analog converter */
#define PALETTE_DATA 0x03c9
#define CRTC_INDEX 0x03d4 /* VGA CRT controller */

#define MAP_MASK 0x02 /* Sequence controller registers */
#define MEMORY_MODE 0x04

#define H_TOTAL 0x00 /* CRT controller registers */
#define H_DISPLAY_END 0x01
#define H_BLANK_START 0x02
#define H_BLANK_END 0x03
#define H_RETRACE_START 0x04
#define H_RETRACE_END 0x05
#define V_TOTAL 0x06
#define OVERFLOW 0x07
#define MAX_SCAN_LINE 0x09
#define V_RETRACE_START 0x10
#define V_RETRACE_END 0x11
#define V_DISPLAY_END 0x12
#define OFFSET 0x13
#define UNDERLINE_LOCATION 0x14
#define V_BLANK_START 0x15
#define V_BLANK_END 0x16
#define MODE_CONTROL 0x17

void VGA_X_InitGraphics(void)
{
    union REGS regs;

    /* set mode 13 */
    regs.w.ax = 0x13;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = (byte *)0xA0000;
    currentscreen = (unsigned short *)0xA0000;
    destscreen = (byte *)0xA0000;

    /* turn off chain-4 mode */
    outp(SC_INDEX, MEMORY_MODE);
    outp(SC_INDEX + 1, 0x06);

    /* set map mask to all 4 planes for screen clearing */
    outp(SC_INDEX, MAP_MASK);
    outp(SC_INDEX + 1, 0xFF);

    /* clear all 256K of memory */
    SetDWords(pcscreen, 0, 0x4000);

    /* turn off long mode */
    outp(CRTC_INDEX, UNDERLINE_LOCATION);
    outp(CRTC_INDEX + 1, 0x00);

    /* turn on byte mode */
    outp(CRTC_INDEX, MODE_CONTROL);
    outp(CRTC_INDEX + 1, 0xE3);

    outp(MISC_OUTPUT, 0xE3);

    /* turn off write protect */
    outp(CRTC_INDEX, V_RETRACE_END);
    outp(CRTC_INDEX + 1, 0x2C);

    outp(CRTC_INDEX, V_TOTAL);
    outp(CRTC_INDEX + 1, 0x0D);

    outp(CRTC_INDEX, OVERFLOW);
    outp(CRTC_INDEX + 1, 0x3E);

    outp(CRTC_INDEX, V_RETRACE_START);
    outp(CRTC_INDEX + 1, 0xEA);

    outp(CRTC_INDEX, V_RETRACE_END);
    outp(CRTC_INDEX + 1, 0xAC);

    outp(CRTC_INDEX, V_DISPLAY_END);
    outp(CRTC_INDEX + 1, 0xDF);

    outp(CRTC_INDEX, V_BLANK_START);
    outp(CRTC_INDEX + 1, 0xE7);

    outp(CRTC_INDEX, V_BLANK_END);
    outp(CRTC_INDEX + 1, 0x06);
}

void I_FinishUpdate(void)
{
    outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0xc);

    // Next plane
    if (destscreen == (byte *)0xA9600)
        destscreen = (byte *)0xA0000;
    else
        destscreen += 0x4B00;
}

#endif
