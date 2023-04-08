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
#include "doomstat.h"

#if defined(MODE_CGA16)

byte lut16colors[14 * 256];
byte *ptrlut16colors;
byte vrambuffer[16384];

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

void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++,palette+=3)
    {
        int r1, g1, b1;

        r1 = (int)ptr[*(palette)];
        g1 = (int)ptr[*(palette+1)];
        b1 = (int)ptr[*(palette+2)];

        lut16colors[i] = GetClosestColor(colors, 16, r1, g1, b1);
    }
}

void I_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 256;
}

void CGA_16_DrawBackbuffer_Snow(void)
{
    unsigned char *vram = (unsigned char *)0xB8001;
    unsigned char line = 80;
    byte *ptrbackbuffer = backbuffer;
    byte *ptrvrambuffer = vrambuffer;

    do
    {
        unsigned char tmp = ptrlut16colors[*ptrbackbuffer] << 4 | ptrlut16colors[*(ptrbackbuffer + 2)];

        if (tmp != *ptrvrambuffer)
        {
            I_WaitCGA();
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
    } while (vram < (unsigned char *)0xBBE80);
}

void CGA_16_DrawBackbuffer(void)
{
    unsigned char *vram = (unsigned char *)0xB8001;
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
    } while (vram < (unsigned char *)0xBBE80);
}

void I_FinishUpdate(void)
{
    if(snowfix)
        CGA_16_DrawBackbuffer_Snow();
    else
        CGA_16_DrawBackbuffer();
}

void CGA_16_InitGraphics(void)
{
    union REGS regs;
    unsigned char *vram = (unsigned char *)0xB8000;
    int i;

    // Set 80x25 color mode
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
    int386(0x10, &regs, &regs);

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

    /* set mode control register for 80x25 text mode and disable video output */
    outp(0x3D8, 1);

    /*
        These settings put the 6845 into "graphics" mode without actually
        switching the CGA controller into graphics mode.  The register
        values are directly copied from CGA graphics mode register
        settings.  The 6845 does not directly display graphics, the
        6845 only generates addresses and sync signals, the CGA
        attribute controller either displays character ROM data or color
        pixel data, this is external to the 6845 and keeps the CGA card
        in text mode.
        ref: HELPPC
    */

    /* set vert total lines to 127 */
    outp(0x3D4, 0x04);
    outp(0x3D5, 0x7F);
    /* set vert displayed char rows to 100 */
    outp(0x3D4, 0x06);
    outp(0x3D5, 0x64);
    /* set vert sync position to 112 */
    outp(0x3D4, 0x07);
    outp(0x3D5, 0x70);
    /* set char scan line count to 1 */
    outp(0x3D4, 0x09);
    outp(0x3D5, 0x01);

    /* re-enable the video output in 80x25 text mode */
    outp(0x3D8, 9);

    /* init buffers */

    SetDWords(vrambuffer, 0, 4096);

    for (i = 0; i < 16000; i += 2)
    {
        vram[i] = 0xDE;
        vrambuffer[i] = 0xDE;
    }
}

#endif
