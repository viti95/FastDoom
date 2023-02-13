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

#if defined(MODE_HERC)

byte lutcolors[14 * 1024];
byte *ptrlutcolors;
byte vrambuffer[32768];

void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 1024; i += 4, palette += 3)
    {
        int r, g, b;
        int sum;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        sum = r + g + b;

        lutcolors[i] = sum > 19 ? 0xFF : 0x00;
        lutcolors[i + 1] = sum > 59 ? 0xFF : 0x00;
        lutcolors[i + 2] = sum > 79 ? 0xFF : 0x00;
        lutcolors[i + 3] = sum > 39 ? 0xFF : 0x00;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlutcolors = lutcolors + numpalette * 1024;
}

void I_FinishUpdate(void)
{
    unsigned char *vram = (unsigned char *)0xB0000;
    byte *ptrvrambuffer = vrambuffer;
    byte *ptrbackbuffer = backbuffer;

    do
    {
        unsigned char x = 80;

        do
        {
            unsigned int *ptr;
            unsigned int finalcolor;
            byte tmp;

            // Process four pixels at the same time (32-bit)
            ptr = ptrlutcolors + *(ptrbackbuffer)*4;
            finalcolor = *ptr & 0x80408040;
            ptr = ptrlutcolors + *(ptrbackbuffer + 1) * 4;
            finalcolor |= *ptr & 0x20102010;
            ptr = ptrlutcolors + *(ptrbackbuffer + 2) * 4;
            finalcolor |= *ptr & 0x08040804;
            ptr = ptrlutcolors + *(ptrbackbuffer + 3) * 4;
            finalcolor |= *ptr & 0x02010201;

            tmp = BYTE0_UINT(finalcolor) | BYTE1_UINT(finalcolor);

            if (tmp != *ptrvrambuffer)
            {
                *vram = tmp;
                *ptrvrambuffer = tmp;
            }

            tmp = BYTE2_UINT(finalcolor) | BYTE3_UINT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x2000))
            {
                *(vram + 0x2000) = tmp;
                *(ptrvrambuffer + 0x2000) = tmp;
            }

            ptr = ptrlutcolors + *(ptrbackbuffer + 320) * 4;
            finalcolor = *ptr & 0x80408040;
            ptr = ptrlutcolors + *(ptrbackbuffer + 321) * 4;
            finalcolor |= *ptr & 0x20102010;
            ptr = ptrlutcolors + *(ptrbackbuffer + 322) * 4;
            finalcolor |= *ptr & 0x08040804;
            ptr = ptrlutcolors + *(ptrbackbuffer + 323) * 4;
            finalcolor |= *ptr & 0x02010201;

            tmp = BYTE0_UINT(finalcolor) | BYTE1_UINT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x4000))
            {
                *(vram + 0x4000) = tmp;
                *(ptrvrambuffer + 0x4000) = tmp;
            }

            tmp = BYTE2_UINT(finalcolor) | BYTE3_UINT(finalcolor);

            if (tmp != *(ptrvrambuffer + 0x6000))
            {
                *(vram + 0x6000) = tmp;
                *(ptrvrambuffer + 0x6000) = tmp;
            }

            ptrbackbuffer += 4;
            ptrvrambuffer++;
            vram++;
            x--;

        } while (x > 0);

        ptrbackbuffer += 320;
    } while (vram < 0xB1F40);
}

void HERC_InitGraphics(void)
{
    byte Graph_640x400[12] = {0x03, 0x34, 0x28, 0x2A, 0x47, 0x69, 0x00, 0x64, 0x65, 0x02, 0x03, 0x0A};
    int i;

    outp(0x03BF, Graph_640x400[0]);
    for (i = 0; i < 10; i++)
    {
        outp(0x03B4, i);
        outp(0x03B5, Graph_640x400[i + 1]);
    }
    outp(0x03B8, Graph_640x400[11]);
    pcscreen = destscreen = (byte *)0xB0000;

    SetDWords(vrambuffer, 0, 8192);
    SetDWords(pcscreen, 0, 8192);
}

void HERC_ShutdownGraphics(void)
{
    byte Text_80x25[12] = {0x00, 0x61, 0x50, 0x52, 0x0F, 0x19, 0x06, 0x19, 0x19, 0x02, 0x0D, 0x08};
    int i;

    outp(0x03BF, Text_80x25[0]);
    for (i = 0; i < 10; i++)
    {
        outp(0x03B4, i);
        outp(0x03B5, Text_80x25[i + 1]);
    }
    outp(0x03B8, Text_80x25[11]);
}

#endif
