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
#include "i_sigma.h"

#if defined(MODE_SIGMA)

const byte colors[48] = {
    0x00, 0x00, 0x00,
    0x00, 0x2A, 0x00,
    0x2A, 0x00, 0x00,
    0x2A, 0x15, 0x00,
    0x15, 0x15, 0x15,
    0x15, 0x3F, 0x15,
    0x3F, 0x15, 0x15,
    0x3F, 0x3F, 0x15,
    0x00, 0x00, 0x2A,
    0x00, 0x2A, 0x2A,
    0x2A, 0x00, 0x2A,
    0x2A, 0x2A, 0x2A,
    0x15, 0x15, 0x3F,
    0x15, 0x3F, 0x3F,
    0x3F, 0x15, 0x3F,
    0x3F, 0x3F, 0x3F};

unsigned short lut16colors[12 * 256];
unsigned short *ptrlut16colors;

void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 12 * 256; i++, palette+=3)
    {
        int r1, g1, b1;

        int bestcolor;

        unsigned short value;
        unsigned short value2;

        r1 = (int)ptr[*(palette)];
        g1 = (int)ptr[*(palette+1)];
        b1 = (int)ptr[*(palette+2)];

        bestcolor = GetClosestColor(colors, 16, r1, g1, b1);

        value = (bestcolor & 12) << 6;
        value2 = bestcolor & 3;

        lut16colors[i] = value | value2;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 256;
}

void Sigma_Init(void);

void Sigma_ClearVRAM(void)
{
    unsigned char *vram = (unsigned char *)0xB8000;
    unsigned int x;

    // Clear blue/intensity page
    outp(0x2DE, 3);

    for (x = 0; x < 16384; x++)
    {
        *vram = 0;
        vram++;
    }

    *vram = (unsigned char *)0xB8000;

    // Clear red/green page
    outp(0x2DE, 2);

    for (x = 0; x < 16384; x++)
    {
        *vram = 0;
        vram++;
    }
}

void Sigma_InitGraphics(void)
{
    // ASM magic
    Sigma_Init();

    Sigma_ClearVRAM();
}

#endif
