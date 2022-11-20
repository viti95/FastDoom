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

#if defined(MODE_EGA16)

byte lut16colors[14 * 256];
byte *ptrlut16colors;
byte vrambuffer[16384];
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

void EGA_16_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++)
    {
        int distance;

        int r1, g1, b1;

        int best_difference = MAXINT;

        r1 = (int)ptr[*palette++];
        g1 = (int)ptr[*palette++];
        b1 = (int)ptr[*palette++];

        for (j = 0; j < 16; j++)
        {
            int r2, g2, b2;
            int cR, cG, cB;
            int pos = j * 3;

            r2 = (int)colors[pos];
            cR = abs(r2 - r1);

            g2 = (int)colors[pos + 1];
            cG = abs(g2 - g1);

            b2 = (int)colors[pos + 2];
            cB = abs(b2 - b1);

            distance = cR + cG + cB;

            if (distance == 0)
            {
                lut16colors[i] = j;
                break;
            }

            distance = SQRT(distance);

            if (best_difference > distance)
            {
                best_difference = distance;
                lut16colors[i] = j;
            }
        }
    }
}

void EGA_16_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 256;
}

void EGA_16_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB8501;
    unsigned char line = 80;
    byte *ptrbackbuffer = backbuffer;
    byte *ptrvrambuffer = vrambuffer;

    do
    {
        unsigned char tmp = ptrlut16colors[*ptrbackbuffer] << 4 | ptrlut16colors[*(ptrbackbuffer + 2)];

        if (tmp != *ptrvrambuffer)
        {
            *vram = tmp;
            *ptrvrambuffer = tmp;
        }

        vram += 2;
        ptrvrambuffer += 2;
        ptrbackbuffer += 4;

        line--;
        if (line == 0)
        {
            line = 80;
            ptrbackbuffer += 320;
        }
    } while (vram < (unsigned char *)0xBC380);
}

void EGA_16_InitGraphics(void)
{
    unsigned char *vram = (unsigned char *)0xB8000;
    int i;

    // Disable cursor
    regs.h.ah = 0x01;
    regs.h.ch = 0x3F;
    int386(0x10, &regs, &regs);

    // Disable blinking
    regs.h.ah = 0x10;
    regs.h.al = 0x03;
    regs.h.bl = 0x00;
    regs.h.bh = 0x00;
    int386(0x10, &regs, &regs);

    /* EGA hires mode is 640x350 with a 9x14 character cell.  The pixel aspect
        ratio is 1:1.37, so if we make the blocks 3 scans tall you get a square
        pixel at 160x100, but some of the scan lines are not used (50) */

    outp(0x3D4, 0x09);
    outp(0x3D5, 0x02);

    /* init buffers */

    SetDWords(vrambuffer, 0, 4096);

    for (i = 0; i < 1280; i += 2)
    {
        vram[i] = 0x00;
    }

    for (i = 1280; i < 16000 + 1280; i += 2)
    {
        vram[i] = 0xDE;
    }

    for (i = 0; i < 16000; i += 2)
    {
        vrambuffer[i] = 0xDE;
    }

    for (i = 16000 + 1280; i < 16000 + 1280 + 1280; i += 2)
    {
        vram[i] = 0x00;
    }
}

#endif