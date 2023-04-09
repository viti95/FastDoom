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

byte lutcolors[14 * 512];
byte *ptrlutcolors;

void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 512; i += 2, palette += 3)
    {
        int r, g, b;
        int sum;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        sum = r + g + b;

        lutcolors[i] = sum > 19 ? 0xFF : 0x00;
        lutcolors[i + 1] = sum > 59 ? 0xFF : 0x00;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlutcolors = lutcolors + numpalette * 512;
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
