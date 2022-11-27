#include <string.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "vga.h"
#include "vga_13h.h"
#include "doomtype.h"
#include "i_ibm.h"
#include "v_video.h"
#include "tables.h"
#include "math.h"
#include "i_system.h"
#include "doomstat.h"
#include "m_menu.h"

#define SBARHEIGHT 32

#if defined(MODE_13H)

union REGS regs;

void VGA_13H_DrawBackbuffer(void)
{
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
}

void VGA_13H_InitGraphics(void)
{
    regs.w.ax = 0x13;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xA0000;
}

#endif
