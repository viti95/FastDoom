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

#if defined(MODE_ATI640)

const byte colors[48] = { // Color      R G B I     G R I B
    0x00, 0x00, 0x00,     // Black      0 0 0 0     0 0 0 0
    0x00, 0x2A, 0x00,     // Green      0 1 0 0     1 0 0 0
    0x2A, 0x00, 0x00,     // Red        1 0 0 0     0 1 0 0
    0x2A, 0x15, 0x00,     // Brown      1 1 0 0     1 1 0 0
    0x15, 0x15, 0x15,     // Gray       0 0 0 1     0 0 1 0
    0x15, 0x3F, 0x15,     // L green    0 1 0 1     1 0 1 0
    0x3F, 0x15, 0x15,     // L red      1 0 0 1     0 1 1 0
    0x3F, 0x3F, 0x15,     // Yellow     1 1 0 1     1 1 1 0
    0x00, 0x00, 0x2A,     // Blue       0 0 1 0     0 0 0 1
    0x00, 0x2A, 0x2A,     // Cyan       0 1 1 0     1 0 0 1
    0x2A, 0x00, 0x2A,     // Magenta    1 0 1 0     0 1 0 1
    0x2A, 0x2A, 0x2A,     // L Gray     1 1 1 0     1 1 0 1
    0x15, 0x15, 0x3F,     // L blue     0 0 1 1     0 0 1 1
    0x15, 0x3F, 0x3F,     // L cyan     0 1 1 1     1 0 1 1
    0x3F, 0x15, 0x3F,     // L magenta  1 0 1 1     0 1 1 1
    0x3F, 0x3F, 0x3F};    // White      1 1 1 1     1 1 1 1

unsigned short lutcolors[14 * 512];
unsigned short *ptrlutcolors;
byte vrambuffer[65536];

void ATI640_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 512; i += 2, palette += 3)
    {
        unsigned char color;
        unsigned short value;
        unsigned short value2;
        int r, g, b;
        int r2, g2, b2;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        r2 = (r * 1) / 3 + r;
        g2 = (g * 1) / 3 + g;
        b2 = (b * 1) / 3 + b;

        color = GetClosestColor(colors, 16, r2, g2, b2);
        value = color & 12;
        value = value | value >> 2 | value << 2 | value << 4;
        value <<= 8;

        value2 = color & 3;
        value2 = value2 | value2 << 2 | value2 << 4 | value2 << 6;
        value2;

        lutcolors[i] = value | value2;

        r2 = (r * 2) / 3 + r;
        g2 = (g * 2) / 3 + g;
        b2 = (b * 2) / 3 + b;

        color = GetClosestColor(colors, 16, r2, g2, b2);
        value = color & 12;
        value = value | value >> 2 | value << 2 | value << 4;
        value <<= 8;

        value2 = color & 3;
        value2 = value2 | value2 << 2 | value2 << 4 | value2 << 6;
        value2;

        lutcolors[i + 1] = value | value2;
    }
}

void ATI640_SetPalette(int numpalette)
{
    ptrlutcolors = lutcolors + numpalette * 512;
}

void ATI640_DrawBackbuffer(void)
{
    int x;
    unsigned char *vram = (unsigned char *)0xB0000;
    byte *ptrvrambuffer = vrambuffer;
    unsigned int base = 0;

    for (base = 0; base < SCREENHEIGHT * 320; base += 960)
    {
        for (x = 0; x < 160; x++, base += 2, vram++, ptrvrambuffer++)
        {
            unsigned short color;
            unsigned short finalcolor;
            byte tmp;

            color = ptrlutcolors[backbuffer[base] * 2];
            finalcolor = color & 0xC0C0;

            color = ptrlutcolors[backbuffer[base] * 2 + 1];
            finalcolor |= color & 0x3030;

            color = ptrlutcolors[backbuffer[base + 1] * 2];
            finalcolor |= color & 0x0C0C;

            color = ptrlutcolors[backbuffer[base + 1] * 2 + 1];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer))
            {
                *(vram) = tmp;
                *(ptrvrambuffer) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x8000))
            {
                *(vram + 0x8000) = tmp;
                *(ptrvrambuffer + 0x8000) = tmp;
            }

            color = ptrlutcolors[backbuffer[base + 320] * 2];
            finalcolor = color & 0xC0C0;

            color = ptrlutcolors[backbuffer[base + 320] * 2 + 1];
            finalcolor |= color & 0x3030;

            color = ptrlutcolors[backbuffer[base + 321] * 2];
            finalcolor |= color & 0x0C0C;

            color = ptrlutcolors[backbuffer[base + 321] * 2 + 1];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x2000))
            {
                *(vram + 0x2000) = tmp;
                *(ptrvrambuffer + 0x2000) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0xA000))
            {
                *(vram + 0xA000) = tmp;
                *(ptrvrambuffer + 0xA000) = tmp;
            }

            color = ptrlutcolors[backbuffer[base + 640] * 2];
            finalcolor = color & 0xC0C0;

            color = ptrlutcolors[backbuffer[base + 640] * 2 + 1];
            finalcolor |= color & 0x3030;

            color = ptrlutcolors[backbuffer[base + 641] * 2];
            finalcolor |= color & 0x0C0C;

            color = ptrlutcolors[backbuffer[base + 641] * 2 + 1];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x4000))
            {
                *(vram + 0x4000) = tmp;
                *(ptrvrambuffer + 0x4000) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0xC000))
            {
                *(vram + 0xC000) = tmp;
                *(ptrvrambuffer + 0xC000) = tmp;
            }

            color = ptrlutcolors[backbuffer[base + 960] * 2];
            finalcolor = color & 0xC0C0;

            color = ptrlutcolors[backbuffer[base + 960] * 2 + 1];
            finalcolor |= color & 0x3030;

            color = ptrlutcolors[backbuffer[base + 961] * 2];
            finalcolor |= color & 0x0C0C;

            color = ptrlutcolors[backbuffer[base + 961] * 2 + 1];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x6000))
            {
                *(vram + 0x6000) = tmp;
                *(ptrvrambuffer + 0x6000) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0xE000))
            {
                *(vram + 0xE000) = tmp;
                *(ptrvrambuffer + 0xE000) = tmp;
            }
        }
    }
}

void ATI640_InitGraphics(void)
{
    static int parms[16] = {0x70, 0x50, 0x58, 0x0a,
                            0x40, 0x06, 0x32, 0x38,
                            0x02, 0x03, 0x06, 0x07,
                            0x00, 0x00, 0x00, 0x00};
    int i;

    /* Set the Graphics Solution to 640 x 200 with 16 colors in
       Color Mode */
    outp(0x3D8, 0x2);

    /* Set extended graphics registers */
    outp(0x3DD, 0x80);

    outp(0x03D8, 0x2);
    /* Program 6845 crt controlller */
    for (i = 0; i < 16; ++i)
    {
        outp(0x3D4, i);
        outp(0x3D5, parms[i]);
    }
    outp(0x3D8, 0x0A);
    outp(0x3D9, 0x30);

    outp(0x3dd, 0x80);

    pcscreen = destscreen = (byte *)0xB0000;

    SetDWords(vrambuffer, 0, 16384);
    SetDWords(pcscreen, 0, 16384);
}

#endif
