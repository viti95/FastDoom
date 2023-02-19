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

#if defined(MODE_EGA80)

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

byte lut16colors[14 * 256];
byte *ptrlut16colors;
byte vrambuffer[16384];

void I_ProcessPalette(byte *palette)
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

        bestcolor = GetClosestColor(colors, 16, r1, g1, b1);

        lut16colors[i] = bestcolor;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 256;
}

void I_FinishUpdate(void)
{
    unsigned char *vram = (unsigned char *)0xA0000;
    byte *ptrvrambuffer = vrambuffer;
    byte *ptrbackbuffer = backbuffer;

    do
    {
        byte tmp;

        tmp = ptrlut16colors[*ptrbackbuffer];
        if (tmp != *(ptrvrambuffer))
        {
            *(vram) = tmp;
            *(ptrvrambuffer) = tmp;
        }

        tmp = ptrlut16colors[*(ptrbackbuffer + 4)];
        if (tmp != *(ptrvrambuffer + 1))
        {
            *(vram + 1) = tmp;
            *(ptrvrambuffer + 1) = tmp;
        }

        tmp = ptrlut16colors[*(ptrbackbuffer + 8)];
        if (tmp != *(ptrvrambuffer + 2))
        {
            *(vram + 2) = tmp;
            *(ptrvrambuffer + 2) = tmp;
        }

        tmp = ptrlut16colors[*(ptrbackbuffer + 12)];
        if (tmp != *(ptrvrambuffer + 3))
        {
            *(vram + 3) = tmp;
            *(ptrvrambuffer + 3) = tmp;
        }

        vram += 4;
        ptrvrambuffer += 4;
        ptrbackbuffer += 16;
    } while (vram < (unsigned char *)0xA3E80);
}

void EGA_80_InitGraphics(void)
{
    union REGS regs;

    regs.w.ax = 0x0E;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xA0000;

    // Write Mode 2
    outp(0x3CE, 0x05);
    outp(0x3CF, 0x02);

    // Write to all 4 planes
    outp(0x3C4, 0x02);
    outp(0x3C5, 0x0F);

    // Set Bit Mask to omit the latch registers
    outp(0x3CE, 0x08);
    outp(0x3CF, 0xFF);

    SetDWords(vrambuffer, 0, 4096);
}

#endif
