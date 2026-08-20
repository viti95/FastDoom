/*
 * VT100 video mode.
 *
 * The game is rendered into an 80x25 text backbuffer exactly like
 * MODE_MDA, but no PC video mode is initialised and nothing is
 * written to the PC screen: the only graphical output is the
 * serial terminal, handled entirely by i_term.c.  This file just
 * owns the text backbuffer and drives the terminal driver.
 */

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
#include "i_term.h"

#if defined(MODE_VT100)

unsigned short backbuffer[80 * 25];
unsigned short *textdestscreen = backbuffer;

void VT100_InitGraphics(void)
{
    /* No PC video mode is set up; the serial terminal is the
       only display, so failure to initialise it is fatal. */
    TERM_SetBackbuffer(backbuffer);
    if (TERM_Init(term_port, term_baud) != 0)
    {
        printf("VT100 mode: failed to initialise serial terminal\n");
        exit(1);
    }
    printf("VT100 terminal: active (port 0x%03X, %d baud)\n",
           term_port, term_baud);
}

void I_ProcessPalette(byte *palette)
{
    /* Do nothing */
}

void I_SetPalette(int numpalette)
{
    /* Do nothing */
}

void I_FinishUpdate(void)
{
    /* Mirror the text backbuffer to the serial terminal.
       The PC screen is left untouched. */
    TERM_UpdateFromBuffer(backbuffer);
    TERM_Flush();
}

void VT100_ShutdownTerminal(void)
{
    TERM_Shutdown();
}

#endif
