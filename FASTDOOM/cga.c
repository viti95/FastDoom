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

union REGS regs;

byte lut4colors[14 * 256];
byte *ptrlut4colors;

const byte colors[12] = {
    0x00, 0x00, 0x00,
    0x00, 0x2A, 0x00,
    0x2A, 0x00, 0x00,
    0x2A, 0x15, 0x00};

unsigned short vrambuffer[16384];

void CGA_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++)
    {
        int distance;

        int r1, g1, b1;

        int bestcolor;

        r1 = (int)ptr[*palette++];
        g1 = (int)ptr[*palette++];
        b1 = (int)ptr[*palette++];

        bestcolor = GetClosestColor(colors, 4, r1, g1, b1);

        lut4colors[i] = bestcolor | bestcolor << 2 | bestcolor << 4 | bestcolor << 6;
    }
}

void CGA_SetPalette(int numpalette)
{
    ptrlut4colors = lut4colors + numpalette * 256;
}

void CGA_DrawBackbuffer(void)
{
    int x;
    unsigned char *vram = (unsigned char *)0xB8000;
    unsigned short *ptrvrambuffer = vrambuffer;
    unsigned int base = 0;

    for (base = 0; base < SCREENHEIGHT * 320; base += 320)
    {
        for (x = 0; x < SCREENWIDTH / 4; x++, base += 4, vram++, ptrvrambuffer++)
        {
            unsigned short color;
            unsigned short tmpColor;
            byte tmp;

            BYTE1_USHORT(tmpColor) = (ptrlut4colors[backbuffer[base]]);
            BYTE0_USHORT(tmpColor) = (ptrlut4colors[backbuffer[base + 1]]);
            tmpColor &= 0xC030;

            BYTE1_USHORT(color) = (ptrlut4colors[backbuffer[base + 2]]);
            BYTE0_USHORT(color) = (ptrlut4colors[backbuffer[base + 3]]);
            tmpColor |= color & 0x0C03;

            if (tmpColor != *(ptrvrambuffer))
            {
                *(ptrvrambuffer) = tmpColor;
                *(vram) = BYTE0_USHORT(tmpColor) | BYTE1_USHORT(tmpColor);
            }

            BYTE1_USHORT(tmpColor) = (ptrlut4colors[backbuffer[base + 320]]);
            BYTE0_USHORT(tmpColor) = (ptrlut4colors[backbuffer[base + 321]]);
            tmpColor &= 0xC030;

            BYTE1_USHORT(color) = (ptrlut4colors[backbuffer[base + 322]]);
            BYTE0_USHORT(color) = (ptrlut4colors[backbuffer[base + 323]]);
            tmpColor |= color & 0x0C03;

            if (tmpColor != *(ptrvrambuffer + 0x2000))
            {
                *(ptrvrambuffer + 0x2000) = tmpColor;
                *(vram + 0x2000) = BYTE0_USHORT(tmpColor) | BYTE1_USHORT(tmpColor);
            }
        }
    }
}

void CGA_InitGraphics(void)
{
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

    SetDWords(vrambuffer, 0, 8192);
    SetDWords(pcscreen, 0, 4096);
}

#endif
