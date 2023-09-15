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

#if defined(MODE_EGA)

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

unsigned char lut16colors[12 * 256 + 255];
unsigned char *ptrlut16colors;

void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    ptrlut16colors = (byte *)(((int)lut16colors + 255) & ~0xff);

    for (i = 0; i < 12 * 256; i++, palette+=3)
    {
        int r1, g1, b1;

        unsigned char bestcolor;

        r1 = (int)ptr[*(palette)];
        g1 = (int)ptr[*(palette + 1)];
        b1 = (int)ptr[*(palette + 2)];

        bestcolor = GetClosestColor(colors, 16, r1, g1, b1);
        bestcolor |= bestcolor << 4;
        ptrlut16colors[i] = bestcolor;
    }
}

void EGA_InitGraphics(void)
{
    union REGS regs;

    unsigned int pos1 = 0;
    unsigned int pos2 = 0;
    unsigned int pos3 = 0;
    unsigned int counter = 0;
    byte *basevram;

    regs.w.ax = 0x0E;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = destscreen = (byte *)0xA0000;

    basevram = (byte *)0xA3E80; // Init at ending of viewable screen

    // Step 1
    // Copy all possible combinations to the VRAM

    outp(0x3C4, 0x02);
    for (pos1 = 0; pos1 < 16; pos1++)
    {
        for (pos2 = 0; pos2 < 16; pos2++)
        {
            for (pos3 = 0; pos3 < 16; pos3++)
            {
                for (counter = 0; counter < 4; counter++)
                {
                    byte bitstatuspos1;
                    byte bitstatuspos2;
                    byte bitstatuspos3;

                    byte final;

                    outp(0x3C5, 1 << counter); // Change plane

                    bitstatuspos1 = (pos1 >> counter) & 1;
                    bitstatuspos2 = (pos2 >> counter) & 1;
                    bitstatuspos3 = (pos3 >> counter) & 1;

                    final = bitstatuspos1 << 6 | bitstatuspos1 << 7 |
                            bitstatuspos2 << 4 | bitstatuspos2 << 5 |
                            bitstatuspos3 << 2 | bitstatuspos3 << 3;
                    *basevram = final;
                }
                basevram++;
            }
        }
    }

    // Step 2

    // Write Mode 2
    outp(0x3CE, 0x05);
    outp(0x3CF, 0x02);

    // Write to all 4 planes
    outp(0x3C4, 0x02);
    outp(0x3C5, 0x0F);

    // Set Bit Mask to use the latch registers
    outp(0x3CE, 0x08);
    outp(0x3CF, 0x03);

    // Set logical operation to OR
    outp(0x3CE, 0x03);
    outp(0x3CF, 0x10);
}

#endif
