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

unsigned short lut16colors[14 * 256];
unsigned short *ptrlut16colors;
byte vrambuffer_p2[32768];
byte vrambuffer_p3[32768];

void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++,palette+=3)
    {
        int r1, g1, b1;

        int bestcolor;

        unsigned short value;
        unsigned short value2;

        r1 = (int)ptr[*(palette)];
        g1 = (int)ptr[*(palette+1)];
        b1 = (int)ptr[*(palette+2)];

        bestcolor = GetClosestColor(colors, 16, r1, g1, b1);

        value = bestcolor & 12;
        value = value | value >> 2 | value << 2 | value << 4;
        value <<= 8;

        value2 = bestcolor & 3;
        value2 = value2 | value2 << 2 | value2 << 4 | value2 << 6;
        value2;

        lut16colors[i] = value | value2;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 256;
}

void I_FinishUpdate(void)
{
    int x;
    unsigned char *vram = (unsigned char *)0xB8000;
    byte *ptrvrambuffer_p2 = vrambuffer_p2;
    byte *ptrvrambuffer_p3 = vrambuffer_p3;
    unsigned int base = 0;

    for (base = 0; base < SCREENHEIGHT * 320;)
    {
        for (x = 0; x < SCREENWIDTH / 4; x++, base += 4, vram++, ptrvrambuffer_p2++, ptrvrambuffer_p3++)
        {
            unsigned short color;
            unsigned short finalcolor;
            byte tmp;

            color = ptrlut16colors[backbuffer[base]];
            finalcolor = color & 0xC0C0;

            color = ptrlut16colors[backbuffer[base + 1]];
            finalcolor |= color & 0x3030;

            color = ptrlut16colors[backbuffer[base + 2]];
            finalcolor |= color & 0x0C0C;

            color = ptrlut16colors[backbuffer[base + 3]];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer_p2))
            {
                outp(0x2DE, 2);

                *(vram) = tmp;
                *(ptrvrambuffer_p2) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer_p3))
            {
                outp(0x2DE, 3);

                *(vram) = tmp;
                *(ptrvrambuffer_p3) = tmp;
            }

            color = ptrlut16colors[backbuffer[base + 320]];
            finalcolor = color & 0xC0C0;

            color = ptrlut16colors[backbuffer[base + 321]];
            finalcolor |= color & 0x3030;

            color = ptrlut16colors[backbuffer[base + 322]];
            finalcolor |= color & 0x0C0C;

            color = ptrlut16colors[backbuffer[base + 323]];
            finalcolor |= color & 0x0303;

            tmp = BYTE0_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer_p2 + 0x2000))
            {
                outp(0x2DE, 2);

                *(vram + 0x2000) = tmp;
                *(ptrvrambuffer_p2 + 0x2000) = tmp;
            }

            tmp = BYTE1_USHORT(finalcolor);

            if (tmp != *(ptrvrambuffer_p3 + 0x2000))
            {
                outp(0x2DE, 3);

                *(vram + 0x2000) = tmp;
                *(ptrvrambuffer_p3 + 0x2000) = tmp;
            }
        }
        base += 320;
    }
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

    SetDWords(vrambuffer_p2, 0, 8192);
    SetDWords(vrambuffer_p3, 0, 8192);
}

#endif
