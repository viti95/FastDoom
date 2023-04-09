#include <string.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "doomtype.h"
#include "doomstat.h"
#include "i_ibm.h"
#include "v_video.h"
#include "tables.h"
#include "math.h"
#include "i_system.h"

#if defined(MODE_CGA_BW)

unsigned short lutcolors[14 * 256];
unsigned short *ptrlutcolors;

void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++, palette += 3)
    {
        int r, g, b;
        int sum;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        sum = r + g + b;

        lutcolors[i] = sum > 19 ? 0x8000 : 0x0000;
        lutcolors[i] |= sum > 59 ? 0x4000 : 0x0000;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlutcolors = lutcolors + numpalette * 256;
}

void CGA_BW_InitGraphics(void)
{
    union REGS regs;
    
    regs.w.ax = 0x06;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xB8000;

    SetDWords(pcscreen, 0, 4096);
}

#endif
