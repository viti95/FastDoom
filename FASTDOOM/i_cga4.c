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

#if defined(MODE_CGA)

byte lut4colors[14 * 256];
byte *ptrlut4colors;

const byte colors[12] = {
    0x00, 0x00, 0x00,
    0x00, 0x2A, 0x00,
    0x2A, 0x00, 0x00,
    0x2A, 0x15, 0x00};

void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++, palette += 3)
    {
        unsigned int r1, g1, b1;
        unsigned int bestcolor;

        r1 = (int)ptr[*(palette)];
        g1 = (int)ptr[*(palette+1)];
        b1 = (int)ptr[*(palette+2)];

        bestcolor = GetClosestColor(colors, 4, r1, g1, b1);

        lut4colors[i] = bestcolor | bestcolor << 2 | bestcolor << 4 | bestcolor << 6;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlut4colors = lut4colors + numpalette * 256;
}

void CGA_InitGraphics(void)
{
    union REGS regs;

    // Set video mode 4
    regs.w.ax = 0x04;
    int386(0x10, (union REGS *)&regs, &regs);

    if (!CGApalette1)
    {
        // Set palette and intensity (CGA)
        regs.w.ax = 0x0B00;
        regs.w.bx = 0x0100;
        int386(0x10, (union REGS *)&regs, &regs);
        regs.w.ax = 0x0B00;
        regs.w.bx = 0x0000;
        int386(0x10, (union REGS *)&regs, &regs);

        // Fix EGA/VGA wrong colors
        regs.w.ax = 0x1000;
        regs.w.bx = 0x0000;
        int386(0x10, (union REGS *)&regs, &regs);

        regs.w.ax = 0x1000;
        regs.w.bx = 0x0201;
        int386(0x10, (union REGS *)&regs, &regs);

        regs.w.ax = 0x1000;
        regs.w.bx = 0x0402;
        int386(0x10, (union REGS *)&regs, &regs);

        regs.w.ax = 0x1000;
        regs.w.bx = 0x0603;
        int386(0x10, (union REGS *)&regs, &regs);
    }

    pcscreen = destscreen = (byte *)0xB8000;

    SetDWords(pcscreen, 0, 4096);
}

#endif
