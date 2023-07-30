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

unsigned char lutcolors[14 * 256 + 255];
unsigned char *ptrlutcolors;

void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    ptrlutcolors = (byte *)(((int)lutcolors + 255) & ~0xff);

    for (i = 0; i < 14 * 256; i++, palette += 3)
    {
        int r, g, b;
        int sum;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        sum = r + g + b;

        ptrlutcolors[i] = sum > 19 ? 0xAA : 0x00;
        ptrlutcolors[i] |= sum > 59 ? 0x55 : 0x00;
    }
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
