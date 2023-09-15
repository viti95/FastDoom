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
#include "i_herc.h"

#if defined(MODE_HERC)

unsigned short lutcolors[12 * 256];
unsigned short *ptrlutcolors;

void I_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 12 * 256; i++, palette += 3)
    {
        int r, g, b;
        int sum;

        unsigned short value = 0x0000;

        r = (int)ptr[*palette];
        g = (int)ptr[*(palette + 1)];
        b = (int)ptr[*(palette + 2)];

        sum = r + g + b;

        value = sum > 19 ? 0x0200 : 0x0000;
        value |= sum > 59 ? 0x0100 : 0x0000;
        value |= sum > 79 ? 0x0002 : 0x0000;
        value |= sum > 39 ? 0x0001 : 0x0000;

        lutcolors[i] = value;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlutcolors = lutcolors + numpalette * 256;
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
