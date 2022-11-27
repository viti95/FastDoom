#include <string.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "vga.h"
#include "doomtype.h"
#include "i_ibm.h"
#include "v_video.h"
#include "tables.h"
#include "math.h"
#include "i_system.h"
#include "doomstat.h"
#include "m_menu.h"
#include "i_vesa.h"

#define SBARHEIGHT 32

#if defined(MODE_VBE2)

static struct VBE_VbeInfoBlock vbeinfo;
static struct VBE_ModeInfoBlock vbemode;
unsigned short vesavideomode = 0xFFFF;
int vesalinear = -1;
char *vesavideoptr;

void VBE2_DrawBackbuffer(void)
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

void VBE2_InitGraphics(void)
{
    int mode;

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
}

#endif
