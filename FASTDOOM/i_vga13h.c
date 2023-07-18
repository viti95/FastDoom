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

#define SBARHEIGHT 32

#if defined(MODE_13H)

byte vrambuffer[320*200];

void I_CopyLine(unsigned int position, unsigned int count)
{
    unsigned int i = 0;

    for (i = 0; i < count; i++)
    {
        byte value = backbuffer[position];

        if (value != vrambuffer[position])
        {
            vrambuffer[position] = value;
            pcscreen[position] = value;
        }

        position++;
    }
}

void I_FinishUpdate(void)
{
    if (updatestate & I_FULLSCRN)
    {
        I_CopyLine(0, SCREENHEIGHT * SCREENWIDTH);
        updatestate = I_NOUPDATE; // clear out all draw types
    }
    if (updatestate & I_FULLVIEW)
    {
        if (updatestate & I_MESSAGES && screenblocks > 7)
        {
            int i;
            for (i = 0; i < endscreen; i += SCREENWIDTH)
            {
                I_CopyLine(i, SCREENWIDTH);
            }
            updatestate &= ~(I_FULLVIEW | I_MESSAGES);
        }
        else
        {
            int i;
            for (i = startscreen; i < endscreen; i += SCREENWIDTH)
            {
                I_CopyLine(i, SCREENWIDTH);
            }
            updatestate &= ~I_FULLVIEW;
        }
    }
    if (updatestate & I_STATBAR)
    {
        I_CopyLine(SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), SCREENWIDTH * SBARHEIGHT);
        updatestate &= ~I_STATBAR;
    }
    if (updatestate & I_MESSAGES)
    {
        I_CopyLine(0, SCREENWIDTH * 28);
        updatestate &= ~I_MESSAGES;
    }
}

void VGA_13H_InitGraphics(void)
{
    union REGS regs;

    regs.w.ax = 0x13;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xA0000;
}

#endif
