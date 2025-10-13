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
#include "i_vga13h.h"


#if defined(MODE_13H)

void (*finishfunc)(void);

extern byte vrambuffer[SCREENWIDTH*SCREENHEIGHT];

void I_CleanupVRAMbuffer(void)
{
    SetDWords(vrambuffer, 0, SCREENWIDTH * SCREENHEIGHT / 4);
}

void I_UpdateFinishFunc(void)
{
    if (busSpeed)
    {
        I_CleanupVRAMbuffer();

        if (I_GetCPUModel() == 386)
        {
            // Avoid crashes of using 486 code on 386 processors (BSWAP)
            finishfunc = I_FinishUpdateDifferential386;
            return;
        }

        switch(selectedCPU) {
            case INTEL_486:
                finishfunc = I_FinishUpdateDifferential486;
                break;
            default:
                finishfunc = I_FinishUpdateDifferential386;
                break;
        }
    }
    else
    {
        finishfunc = I_FinishUpdateDirect;
    }
}

void I_FinishUpdateDifferential386(void)
{
    if (updatestate & I_FULLSCRN)
    {
        I_CopyLine386(0, SCREENHEIGHT * SCREENWIDTH);
        updatestate = I_NOUPDATE; // clear out all draw types
    }
    if (updatestate & I_FULLVIEW)
    {
        if (updatestate & I_MESSAGES && screenblocks > 7)
        {
            int i;
            for (i = 0; i < endscreen; i += SCREENWIDTH)
            {
                I_CopyLine386(i, SCREENWIDTH + i);
            }
            updatestate &= ~(I_FULLVIEW | I_MESSAGES);
        }
        else
        {
            int i;
            for (i = startscreen; i < endscreen; i += SCREENWIDTH)
            {
                I_CopyLine386(i, SCREENWIDTH + i);
            }
            updatestate &= ~I_FULLVIEW;
        }
    }
    if (updatestate & I_STATBAR)
    {
        I_CopyLine386(SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), SCREENWIDTH * SBARHEIGHT + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT));
        updatestate &= ~I_STATBAR;
    }
    if (updatestate & I_MESSAGES)
    {
        I_CopyLine386(0, SCREENWIDTH * 28);
        updatestate &= ~I_MESSAGES;
    }
}

void I_FinishUpdateDifferential486(void)
{
    if (updatestate & I_FULLSCRN)
    {
        I_CopyLine486(0, SCREENHEIGHT * SCREENWIDTH);
        updatestate = I_NOUPDATE; // clear out all draw types
    }
    if (updatestate & I_FULLVIEW)
    {
        if (updatestate & I_MESSAGES && screenblocks > 7)
        {
            int i;
            for (i = 0; i < endscreen; i += SCREENWIDTH)
            {
                I_CopyLine486(i, SCREENWIDTH + i);
            }
            updatestate &= ~(I_FULLVIEW | I_MESSAGES);
        }
        else
        {
            int i;
            for (i = startscreen; i < endscreen; i += SCREENWIDTH)
            {
                I_CopyLine486(i, SCREENWIDTH + i);
            }
            updatestate &= ~I_FULLVIEW;
        }
    }
    if (updatestate & I_STATBAR)
    {
        I_CopyLine486(SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), SCREENWIDTH * SBARHEIGHT + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT));
        updatestate &= ~I_STATBAR;
    }
    if (updatestate & I_MESSAGES)
    {
        I_CopyLine486(0, SCREENWIDTH * 28);
        updatestate &= ~I_MESSAGES;
    }
}

void I_FinishUpdateDirect(void)
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
    union REGS regs;

    regs.w.ax = 0x13;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xA0000;
}

#endif
