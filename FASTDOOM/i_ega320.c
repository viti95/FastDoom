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

#if defined(MODE_EGA)

const byte colors[48] = {
    0x00, 0x00, 0x00,  // 0
    0x00, 0x00, 0x2A,  // 1
    0x00, 0x2A, 0x00,  // 2
    0x00, 0x2A, 0x2A,  // 3
    0x2A, 0x00, 0x00,  // 4
    0x2A, 0x00, 0x2A,  // 5
    0x2A, 0x15, 0x00,  // 6
    0x2A, 0x2A, 0x2A,  // 7
    0x15, 0x15, 0x15,  // 8
    0x15, 0x15, 0x3F,  // 9
    0x15, 0x3F, 0x15,  // 10
    0x15, 0x3F, 0x3F,  // 11
    0x3F, 0x15, 0x15,  // 12
    0x3F, 0x15, 0x3F,  // 13
    0x3F, 0x3F, 0x15,  // 14
    0x3F, 0x3F, 0x3F}; // 15

unsigned int lut16colors[14 * 256];
unsigned int *ptrlut16colors;

void I_ProcessPalette(byte *palette)
{
    int x;
    byte *ptr = gammatable[usegamma];

    for (x = 0; x < 14 * 256; x++, palette+=3)
    {
        int r1, g1, b1;

        unsigned int bestcolor;
        unsigned int r,g,b,i;

        r1 = (int)ptr[*(palette)];
        g1 = (int)ptr[*(palette + 1)];
        b1 = (int)ptr[*(palette + 2)];

        bestcolor = GetClosestColor(colors, 16, r1, g1, b1);
        r = bestcolor & 1;
        g = (bestcolor & 2) >> 1;
        b = (bestcolor & 4) >> 2;
        i = (bestcolor & 8) >> 3;

        lut16colors[x] = i | (b << 8) | (g << 16) | (r << 24);
    }
}

void I_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 256;
}

void EGA_InitGraphics(void)
{
    union REGS regs;

    regs.w.ax = 0x0D;
    int386(0x10, (union REGS *)&regs, &regs);
    outp(0x3C4, 0x2);
    pcscreen = destscreen = (byte *)0xA0000;
}

#endif
