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

#if defined(MODE_CGA_BW)

byte lutcolors[14 * 512];
byte *ptrlutcolors;
unsigned short vrambuffer[16384];

void CGA_BW_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 512; i += 2, palette += 3)
    {
        int r, g, b;
        int sum;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        sum = r + g + b;

        lutcolors[i] = sum > 32 ? 0xFF : 0x00;
        lutcolors[i + 1] = sum > 64 ? 0xFF : 0x00;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlutcolors = lutcolors + numpalette * 512;
}

void I_FinishUpdate(void)
{
    unsigned char *vram = (unsigned char *)0xB8000;
    unsigned short *ptrvrambuffer = vrambuffer;
    byte *ptrbackbuffer = backbuffer;

    do
    {
        unsigned char x = 80;

        do
        {
            unsigned short *ptr;
            unsigned short finalcolor;

            // Process two pixels at the same time (16-bit)
            ptr = ptrlutcolors + *(ptrbackbuffer)*2;
            finalcolor = *ptr & 0x8040;
            ptr = ptrlutcolors + *(ptrbackbuffer + 1) * 2;
            finalcolor |= *ptr & 0x2010;
            ptr = ptrlutcolors + *(ptrbackbuffer + 2) * 2;
            finalcolor |= *ptr & 0x0804;
            ptr = ptrlutcolors + *(ptrbackbuffer + 3) * 2;
            finalcolor |= *ptr & 0x0201;

            if (finalcolor != *ptrvrambuffer)
            {
                *ptrvrambuffer = finalcolor;
                *vram = BYTE0_USHORT(finalcolor) | BYTE1_USHORT(finalcolor);
            }

            ptr = ptrlutcolors + *(ptrbackbuffer + 320) * 2;
            finalcolor = *ptr & 0x8040;
            ptr = ptrlutcolors + *(ptrbackbuffer + 321) * 2;
            finalcolor |= *ptr & 0x2010;
            ptr = ptrlutcolors + *(ptrbackbuffer + 322) * 2;
            finalcolor |= *ptr & 0x0804;
            ptr = ptrlutcolors + *(ptrbackbuffer + 323) * 2;
            finalcolor |= *ptr & 0x0201;

            if (finalcolor != *(ptrvrambuffer + 0x2000))
            {
                *(ptrvrambuffer + 0x2000) = finalcolor;
                *(vram + 0x2000) = BYTE0_USHORT(finalcolor) | BYTE1_USHORT(finalcolor);
            }

            ptrbackbuffer += 4;
            ptrvrambuffer++;
            vram++;
            x--;

        } while (x > 0);

        ptrbackbuffer += 320;
    } while (vram < 0xB9F40);
}

void CGA_BW_InitGraphics(void)
{
    union REGS regs;
    
    regs.w.ax = 0x06;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xB8000;

    SetDWords(vrambuffer, 0, 8192);
    SetDWords(pcscreen, 0, 4096);
}

#endif
