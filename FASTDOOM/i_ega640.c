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

#define CRTC_INDEX 0x3D4

#if defined(MODE_EGA640)

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

byte lutcolors[14 * 256];
byte *ptrlutcolors;

byte vrambufferR[16000];
byte vrambufferG[16000];
byte vrambufferB[16000];
byte vrambufferI[16000];

byte page = 0;

void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++, palette += 3)
    {
        unsigned char color;
        int r, g, b;
        int r2, g2, b2;

        int l;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        l = (r * 300) / 1000 + (g * 600) / 1000 + (b * 100) / 1000;

        r2 = r + (200 * (l + r)) / 1000;
        if (r2 > 255) r2 = 255;

        g2 = g + (200 * (l + g)) / 1000;
        if (g2 > 255) g2 = 255;

        b2 = b + (200 * (l + b)) / 1000;
        if (b2 > 255) b2 = 255;

        color = GetClosestColor(colors, 16, r2, g2, b2);
        color = color << 4;

        r2 = r - (200 * (l - r)) / 1000;
        if (r2 < 0) r2 = 0;

        g2 = g - (200 * (l - g)) / 1000;
        if (g2 < 0) g2 = 0;

        b2 = b - (200 * (l - b)) / 1000;
        if (b2 < 0) b2 = 0;

        color |= GetClosestColor(colors, 16, r2, g2, b2);

        lutcolors[i] = color;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlutcolors = lutcolors + numpalette * 256;
}

void I_FinishUpdate(void)
{
    unsigned short i;
    byte *backbufferptr;

    // Red
    outp(0x3C5, 1 << (3 & 0x03));

    for (i = 0, backbufferptr = backbuffer; i < 2 * SCREENWIDTH * SCREENHEIGHT / 8; i++, backbufferptr += 4)
    {
        unsigned char color;
        unsigned char tmpColor;

        color = ptrlutcolors[*(backbufferptr)];
        tmpColor = (color & 0x80);
        tmpColor |= (color & 0x08) << 3;

        color = ptrlutcolors[*(backbufferptr + 1)];
        tmpColor |= (color & 0x80) >> 2;
        tmpColor |= (color & 0x08) << 1;

        color = ptrlutcolors[*(backbufferptr + 2)];
        tmpColor |= (color & 0x80) >> 4;
        tmpColor |= (color & 0x08) >> 1;

        color = ptrlutcolors[*(backbufferptr + 3)];
        tmpColor |= (color & 0x80) >> 6;
        tmpColor |= (color & 0x08) >> 3;

        if (tmpColor != vrambufferR[i])
        {
            destscreen[i] = tmpColor;
            vrambufferR[i] = tmpColor;
        }
    }

    // Green
    outp(0x3C5, 1 << (2 & 0x03));

    for (i = 0, backbufferptr = backbuffer; i < 2 * SCREENWIDTH * SCREENHEIGHT / 8; i++, backbufferptr += 4)
    {
        unsigned char color;
        unsigned char tmpColor;

        color = ptrlutcolors[*(backbufferptr)];
        tmpColor = (color & 0x40) << 1;
        tmpColor |= (color & 0x04) << 4;

        color = ptrlutcolors[*(backbufferptr + 1)];
        tmpColor |= (color & 0x40) >> 1;
        tmpColor |= (color & 0x04) << 2;

        color = ptrlutcolors[*(backbufferptr + 2)];
        tmpColor |= (color & 0x40) >> 3;
        tmpColor |= (color & 0x04);

        color = ptrlutcolors[*(backbufferptr + 3)];
        tmpColor |= (color & 0x40) >> 5;
        tmpColor |= (color & 0x04) >> 2;

        if (tmpColor != vrambufferG[i])
        {
            destscreen[i] = tmpColor;
            vrambufferG[i] = tmpColor;
        }
    }

    // Blue
    outp(0x3C5, 1 << (1 & 0x03));

    for (i = 0, backbufferptr = backbuffer; i < 2 * SCREENWIDTH * SCREENHEIGHT / 8; i++, backbufferptr += 4)
    {
        unsigned char color;
        unsigned char tmpColor;

        color = ptrlutcolors[*(backbufferptr)];
        tmpColor = (color & 0x20) << 2;
        tmpColor |= (color & 0x02) << 5;

        color = ptrlutcolors[*(backbufferptr + 1)];
        tmpColor |= (color & 0x20);
        tmpColor |= (color & 0x02) << 3;

        color = ptrlutcolors[*(backbufferptr + 2)];
        tmpColor |= (color & 0x20) >> 2;
        tmpColor |= (color & 0x02) << 1;

        color = ptrlutcolors[*(backbufferptr + 3)];
        tmpColor |= (color & 0x20) >> 4;
        tmpColor |= (color & 0x02) >> 1;

        if (tmpColor != vrambufferB[i])
        {
            destscreen[i] = tmpColor;
            vrambufferB[i] = tmpColor;
        }
    }

    // Intensity
    outp(0x3C5, 1 << (0 & 0x03));

    for (i = 0, backbufferptr = backbuffer; i < 2 * SCREENWIDTH * SCREENHEIGHT / 8; i++, backbufferptr += 4)
    {
        unsigned char color;
        unsigned char tmpColor;

        color = ptrlutcolors[*(backbufferptr)];
        tmpColor = (color & 0x10) << 3;
        tmpColor |= (color & 0x01) << 6;

        color = ptrlutcolors[*(backbufferptr + 1)];
        tmpColor |= (color & 0x10) << 1;
        tmpColor |= (color & 0x01) << 4;

        color = ptrlutcolors[*(backbufferptr + 2)];
        tmpColor |= (color & 0x10) >> 1;
        tmpColor |= (color & 0x01) << 2;

        color = ptrlutcolors[*(backbufferptr + 3)];
        tmpColor |= (color & 0x10) >> 3;
        tmpColor |= (color & 0x01);

        if (tmpColor != vrambufferI[i])
        {
            destscreen[i] = tmpColor;
            vrambufferI[i] = tmpColor;
        }
    }
}

void EGA_640_InitGraphics(void)
{
    union REGS regs;

    regs.w.ax = 0x0E;
    int386(0x10, (union REGS *)&regs, &regs);
    outp(0x3C4, 0x2);
    pcscreen = destscreen = (byte *)0xA0000;

    SetDWords(vrambufferR, 0, 4096);
    SetDWords(vrambufferG, 0, 4096);
    SetDWords(vrambufferB, 0, 4096);
    SetDWords(vrambufferI, 0, 4096);
}
#endif
