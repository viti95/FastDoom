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

#if defined(MODE_CVB)

const byte colors[48] = { // standard IBM CGA
    0x00, 0x00, 0x00,
    0x00, 0x18, 0x06,
    0x09, 0x0a, 0x2f,
    0x04, 0x26, 0x37,
    0x20, 0x03, 0x15,
    0x1c, 0x1c, 0x1c,
    0x2c, 0x0e, 0x3f,
    0x26, 0x2a, 0x3f,
    0x12, 0x12, 0x00,
    0x0f, 0x2e, 0x00,
    0x1c, 0x1c, 0x1c,
    0x18, 0x3b, 0x21,
    0x37, 0x15, 0x04,
    0x35, 0x31, 0x07,
    0x3f, 0x20, 0x3a,
    0x3f, 0x3f, 0x3f};

/*const byte colors[48] = {  // ATi Small Wonder
    0x00, 0x00, 0x00,
    0x22, 0x04, 0x00,
    0x06, 0x15, 0x00,
    0x26, 0x1f, 0x00,
    0x03, 0x0f, 0x23,
    0x16, 0x17, 0x15,
    0x00, 0x2b, 0x00,
    0x1a, 0x38, 0x00,
    0x18, 0x01, 0x36,
    0x3f, 0x07, 0x31,
    0x14, 0x15, 0x13,
    0x3f, 0x22, 0x0e,
    0x0a, 0x13, 0x3f,
    0x39, 0x1e, 0x3f,
    0x07, 0x32, 0x3f,
    0x3f, 0x3f, 0x3f};*/

byte lut16colors[14 * 256];
byte *ptrlut16colors;
byte vrambuffer[16384];

void CGA_CVBS_ProcessPalette(byte *palette)
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

void CGA_CVBS_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 256;
}

void CGA_CVBS_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB8000;
    unsigned short base = 0;
    byte *ptrvrambuffer = vrambuffer;

    for (base = 0; base < SCREENHEIGHT * 320;)
    {
        unsigned char x;

        for (x = 0; x < SCREENWIDTH / 4; x++, base += 4, vram++, ptrvrambuffer++)
        {
            unsigned char color0, color1;
            byte tmp;

            color0 = ptrlut16colors[backbuffer[base]];
            color1 = ptrlut16colors[backbuffer[base + 2]];

            tmp = color0 << 4 | color1;

            if (tmp != *(ptrvrambuffer))
            {
                *(vram) = tmp;
                *(ptrvrambuffer) = tmp;
            }
        }

        vram += 0x1FB0;
        ptrvrambuffer += 0x1FB0;

        for (x = 0; x < SCREENWIDTH / 4; x++, base += 4, vram++, ptrvrambuffer++)
        {
            unsigned char color0, color1;
            byte tmp;

            color0 = ptrlut16colors[backbuffer[base]];
            color1 = ptrlut16colors[backbuffer[base + 2]];

            tmp = color0 << 4 | color1;

            if (tmp != *(ptrvrambuffer))
            {
                *(vram) = tmp;
                *(ptrvrambuffer) = tmp;
            }
        }

        vram -= 0x2000;
        ptrvrambuffer -= 0x2000;
    }
}

void CGA_CVBS_InitGraphics(void)
{
    union REGS regs;

    regs.w.ax = 0x06;
    int386(0x10, (union REGS *)&regs, &regs);
    outp(0x3D8, 0x1A); // Enable color burst
    pcscreen = destscreen = (byte *)0xB8000;

    SetDWords(vrambuffer, 0, 4096);
    SetDWords(pcscreen, 0, 4096);
}

#endif
