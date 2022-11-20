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

union REGS regs;

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

byte lutcolors[14 * 512];
byte *ptrlutcolors;

byte vrambufferR1[16384];
byte vrambufferG1[16384];
byte vrambufferB1[16384];
byte vrambufferI1[16384];
byte vrambufferR2[16384];
byte vrambufferG2[16384];
byte vrambufferB2[16384];
byte vrambufferI2[16384];
byte vrambufferR3[16384];
byte vrambufferG3[16384];
byte vrambufferB3[16384];
byte vrambufferI3[16384];

byte page = 0;

void EGA_640_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 512; i += 2, palette += 3)
    {
        unsigned char color;
        int r, g, b;
        int r2, g2, b2;

        int l;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        l = (r * 300) / 1000 + (g * 600) / 1000 + (b * 100) / 1000;

        r2 = r + (333 * (l + r)) / 1000;
        if (r2 > 255) r2 = 255;

        g2 = g + (333 * (l + g)) / 1000;
        if (g2 > 255) g2 = 255;

        b2 = b + (333 * (l + b)) / 1000;
        if (b2 > 255) b2 = 255;

        color = GetClosestColor(colors, 16, r2, g2, b2);
        color |= color << 4;

        lutcolors[i] = color;

        r2 = r - (333 * (l - r)) / 1000;
        if (r2 < 0) r2 = 0;

        g2 = g - (333 * (l - g)) / 1000;
        if (g2 < 0) g2 = 0;

        b2 = b - (333 * (l - b)) / 1000;
        if (b2 < 0) b2 = 0;

        color = GetClosestColor(colors, 16, r2, g2, b2);
        color |= color << 4;

        lutcolors[i + 1] = color;
    }
}

void EGA_640_SetPalette(int numpalette)
{
    ptrlutcolors = lutcolors + numpalette * 512;
}

void EGA_640_DrawBackbuffer(void)
{
    unsigned short i;
    byte *backbufferptr;

    byte *vrambufferR;
    byte *vrambufferG;
    byte *vrambufferB;
    byte *vrambufferI;

    if (destscreen == 0xA0000)
    {
        vrambufferR = vrambufferR1;
        vrambufferG = vrambufferG1;
        vrambufferB = vrambufferB1;
        vrambufferI = vrambufferI1;
    }
    else if (destscreen == 0xA4000)
    {
        vrambufferR = vrambufferR2;
        vrambufferG = vrambufferG2;
        vrambufferB = vrambufferB2;
        vrambufferI = vrambufferI2;
    }
    else
    {
        vrambufferR = vrambufferR3;
        vrambufferG = vrambufferG3;
        vrambufferB = vrambufferB3;
        vrambufferI = vrambufferI3;
    }

    // Red
    outp(0x3C5, 1 << (3 & 0x03));

    for (i = 0, backbufferptr = backbuffer; i < 2 * SCREENWIDTH * SCREENHEIGHT / 8; i++, backbufferptr += 4)
    {
        unsigned char color;
        unsigned char tmpColor;

        color = ptrlutcolors[*(backbufferptr)*2];
        tmpColor = color & 0x80;

        color = ptrlutcolors[*(backbufferptr)*2 + 1];
        tmpColor |= (color & 0x80) >> 1;

        color = ptrlutcolors[*(backbufferptr + 1) * 2];
        tmpColor |= (color & 0x80) >> 2;

        color = ptrlutcolors[*(backbufferptr + 1) * 2 + 1];
        tmpColor |= (color & 0x80) >> 3;

        color = ptrlutcolors[*(backbufferptr + 2) * 2];
        tmpColor |= color & 0x08;

        color = ptrlutcolors[*(backbufferptr + 2) * 2 + 1];
        tmpColor |= (color & 0x08) >> 1;

        color = ptrlutcolors[*(backbufferptr + 3) * 2];
        tmpColor |= (color & 0x08) >> 2;

        color = ptrlutcolors[*(backbufferptr + 3) * 2 + 1];
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

        color = ptrlutcolors[*(backbufferptr)*2];
        tmpColor = (color & 0x40) << 1;

        color = ptrlutcolors[*(backbufferptr)*2 + 1];
        tmpColor |= color & 0x40;

        color = ptrlutcolors[*(backbufferptr + 1) * 2];
        tmpColor |= (color & 0x40) >> 1;

        color = ptrlutcolors[*(backbufferptr + 1) * 2 + 1];
        tmpColor |= (color & 0x40) >> 2;

        color = ptrlutcolors[*(backbufferptr + 2) * 2];
        tmpColor |= (color & 0x04) << 1;

        color = ptrlutcolors[*(backbufferptr + 2) * 2 + 1];
        tmpColor |= color & 0x04;

        color = ptrlutcolors[*(backbufferptr + 3) * 2];
        tmpColor |= (color & 0x04) >> 1;

        color = ptrlutcolors[*(backbufferptr + 3) * 2 + 1];
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

        color = ptrlutcolors[*(backbufferptr)*2];
        tmpColor = (color & 0x20) << 2;

        color = ptrlutcolors[*(backbufferptr)*2 + 1];
        tmpColor |= (color & 0x20) << 1;

        color = ptrlutcolors[*(backbufferptr + 1) * 2];
        tmpColor |= color & 0x20;

        color = ptrlutcolors[*(backbufferptr + 1) * 2 + 1];
        tmpColor |= (color & 0x20) >> 1;

        color = ptrlutcolors[*(backbufferptr + 2) * 2];
        tmpColor |= (color & 0x02) << 2;

        color = ptrlutcolors[*(backbufferptr + 2) * 2 + 1];
        tmpColor |= (color & 0x02) << 1;

        color = ptrlutcolors[*(backbufferptr + 3) * 2];
        tmpColor |= color & 0x02;

        color = ptrlutcolors[*(backbufferptr + 3) * 2 + 1];
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

        color = ptrlutcolors[*(backbufferptr)*2];
        tmpColor = (color & 0x10) << 3;

        color = ptrlutcolors[*(backbufferptr)*2 + 1];
        tmpColor |= (color & 0x10) << 2;

        color = ptrlutcolors[*(backbufferptr + 1) * 2];
        tmpColor |= (color & 0x10) << 1;

        color = ptrlutcolors[*(backbufferptr + 1) * 2 + 1];
        tmpColor |= color & 0x10;

        color = ptrlutcolors[*(backbufferptr + 2) * 2];
        tmpColor |= (color & 0x01) << 3;

        color = ptrlutcolors[*(backbufferptr + 2) * 2 + 1];
        tmpColor |= (color & 0x01) << 2;

        color = ptrlutcolors[*(backbufferptr + 3) * 2];
        tmpColor |= (color & 0x01) << 1;

        color = ptrlutcolors[*(backbufferptr + 3) * 2 + 1];
        tmpColor |= color & 0x01;

        if (tmpColor != vrambufferI[i])
        {
            destscreen[i] = tmpColor;
            vrambufferI[i] = tmpColor;
        }
    }

    // Change video page
    outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0xc);

    // Next plane
    if (destscreen == 0xA8000)
        destscreen = 0xA0000;
    else
        destscreen += 0x4000;
}

void EGA_640_InitGraphics(void)
{
    regs.w.ax = 0x0E;
    int386(0x10, (union REGS *)&regs, &regs);
    outp(0x3C4, 0x2);
    pcscreen = destscreen = (byte *)0xA0000;

    SetDWords(vrambufferR1, 0, 4096);
    SetDWords(vrambufferG1, 0, 4096);
    SetDWords(vrambufferB1, 0, 4096);
    SetDWords(vrambufferI1, 0, 4096);
    SetDWords(vrambufferR2, 0, 4096);
    SetDWords(vrambufferG2, 0, 4096);
    SetDWords(vrambufferB2, 0, 4096);
    SetDWords(vrambufferI2, 0, 4096);
    SetDWords(vrambufferR3, 0, 4096);
    SetDWords(vrambufferG3, 0, 4096);
    SetDWords(vrambufferB3, 0, 4096);
    SetDWords(vrambufferI3, 0, 4096);
}

#endif
