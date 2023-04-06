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

byte lut16colors[14 * 256 * 4];
byte *ptrlut16colors;

unsigned short lastlatch;
unsigned short vrambuffer[16000];

void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256 * 4; i += 4, palette += 3)
    {
        int distance;

        int r1, g1, b1;
        int r2, g2, b2;

        int l;

        int color;

        r1 = (int)ptr[*(palette)];
        g1 = (int)ptr[*(palette + 1)];
        b1 = (int)ptr[*(palette + 2)];

        l = (r1 * 300) / 1000 + (g1 * 600) / 1000 + (b1 * 100) / 1000;

        r2 = r1 + (0 * (l + r1)) / 1000;
        if (r2 > 255)
            r2 = 255;

        g2 = g1 + (0 * (l + g1)) / 1000;
        if (g2 > 255)
            g2 = 255;

        b2 = b1 + (0 * (l + b1)) / 1000;
        if (b2 > 255)
            b2 = 255;

        color = GetClosestColor(colors, 16, r2, g2, b2);
        lut16colors[i] = color;

        r2 = r1 + (500 * (l + r1)) / 1000;
        if (r2 > 255)
            r2 = 255;

        g2 = g1 + (500 * (l + g1)) / 1000;
        if (g2 > 255)
            g2 = 255;

        b2 = b1 + (500 * (l + b1)) / 1000;
        if (b2 > 255)
            b2 = 255;

        color = GetClosestColor(colors, 16, r2, g2, b2);
        lut16colors[i + 1] = color;

        r2 = r1 + (750 * (l + r1)) / 1000;
        if (r2 > 255)
            r2 = 255;

        g2 = g1 + (750 * (l + g1)) / 1000;
        if (g2 > 255)
            g2 = 255;

        b2 = b1 + (750 * (l + b1)) / 1000;
        if (b2 > 255)
            b2 = 255;

        color = GetClosestColor(colors, 16, r2, g2, b2);
        lut16colors[i + 2] = color;

        r2 = r1 + (250 * (l + r1)) / 1000;
        if (r2 > 255)
            r2 = 255;

        g2 = g1 + (250 * (l + g1)) / 1000;
        if (g2 > 255)
            g2 = 255;

        b2 = b1 + (250 * (l + b1)) / 1000;
        if (b2 > 255)
            b2 = 255;

        color = GetClosestColor(colors, 16, r2, g2, b2);
        lut16colors[i + 3] = color;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 1024;
}

void I_FinishUpdate(void)
{
    byte *vram = (byte *)0xA0000;
    byte *ptrbackbuffer = backbuffer;
    unsigned short *ptrvrambuffer = vrambuffer;

    int counter = 0;
    int evenodd = 0;

    do
    {
        unsigned short fullvalue;

        if (counter == 80)
        {
            counter = 0;
            if (evenodd == 0)
                evenodd = 1;
            else
                evenodd = 0;
        }

        counter++;

        if (evenodd == 0)
        {
            fullvalue = 16 * 16 * 16 * ptrlut16colors[*(ptrbackbuffer)*4] +
                        16 * 16 * ptrlut16colors[*(ptrbackbuffer + 1) * 4 + 1] +
                        16 * ptrlut16colors[*(ptrbackbuffer + 2) * 4] +
                        ptrlut16colors[*(ptrbackbuffer + 3) * 4 + 1];
        }
        else
        {
            fullvalue = 16 * 16 * 16 * ptrlut16colors[*(ptrbackbuffer)*4 + 2] +
                        16 * 16 * ptrlut16colors[*(ptrbackbuffer + 1) * 4 + 3] +
                        16 * ptrlut16colors[*(ptrbackbuffer + 2) * 4 + 2] +
                        ptrlut16colors[*(ptrbackbuffer + 3) * 4 + 3];
        }

        if (*(ptrvrambuffer) != fullvalue)
        {
            unsigned short vramlut;

            *(ptrvrambuffer) = fullvalue;
            vramlut = fullvalue >> 4;

            if (lastlatch != vramlut)
            {
                lastlatch = vramlut;
                ReadMem((byte *)0xA3E80 + vramlut);
            }

            *(vram) = fullvalue;
        }

        vram += 1;
        ptrbackbuffer += 4;
        ptrvrambuffer += 1;
    } while (vram < (byte *)0xA3E80);
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
