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
#include "i_vga.h"

#if defined(MODE_V2)

void I_FinishUpdate(void)
{
    byte *ptrdestscreen;
    int pos;

    outp(SC_INDEX + 1, 1 << 0);

    ptrdestscreen = destscreen + 15 * 80 + 15;
    pos = 319;

    do
    {
        unsigned char x;

        for (x = 0; x < 10; x++)
        {
            ptrdestscreen[0] = backbuffer[pos];
            ptrdestscreen[1] = backbuffer[pos + 1280];
            ptrdestscreen[2] = backbuffer[pos + 2560];
            ptrdestscreen[3] = backbuffer[pos + 3840];
            ptrdestscreen[4] = backbuffer[pos + 5120];

            pos += 1280 * 5;
            ptrdestscreen += 5;
        }

        pos -= (1280 * 50 + 1);
        ptrdestscreen += 30;
    } while (pos > -1);

    outp(SC_INDEX + 1, 1 << 1);

    ptrdestscreen = destscreen + 15 * 80 + 15;
    pos = 639;

    do
    {
        unsigned char x;

        for (x = 0; x < 10; x++)
        {
            ptrdestscreen[0] = backbuffer[pos];
            ptrdestscreen[1] = backbuffer[pos + 1280];
            ptrdestscreen[2] = backbuffer[pos + 2560];
            ptrdestscreen[3] = backbuffer[pos + 3840];
            ptrdestscreen[4] = backbuffer[pos + 5120];

            pos += 1280 * 5;
            ptrdestscreen += 5;
        }

        pos -= (1280 * 50 + 1);
        ptrdestscreen += 30;
    } while (pos > 319);

    outp(SC_INDEX + 1, 1 << 2);

    ptrdestscreen = destscreen + 15 * 80 + 15;
    pos = 959;

    do
    {
        unsigned char x;

        for (x = 0; x < 10; x++)
        {
            ptrdestscreen[0] = backbuffer[pos];
            ptrdestscreen[1] = backbuffer[pos + 1280];
            ptrdestscreen[2] = backbuffer[pos + 2560];
            ptrdestscreen[3] = backbuffer[pos + 3840];
            ptrdestscreen[4] = backbuffer[pos + 5120];

            pos += 1280 * 5;
            ptrdestscreen += 5;
        }

        pos -= (1280 * 50 + 1);
        ptrdestscreen += 30;
    } while (pos > 639);

    outp(SC_INDEX + 1, 1 << 3);

    ptrdestscreen = destscreen + 15 * 80 + 15;
    pos = 1279;

    do
    {
        unsigned char x;

        for (x = 0; x < 10; x++)
        {
            ptrdestscreen[0] = backbuffer[pos];
            ptrdestscreen[1] = backbuffer[pos + 1280];
            ptrdestscreen[2] = backbuffer[pos + 2560];
            ptrdestscreen[3] = backbuffer[pos + 3840];
            ptrdestscreen[4] = backbuffer[pos + 5120];

            pos += 1280 * 5;
            ptrdestscreen += 5;
        }

        pos -= (1280 * 50 + 1);
        ptrdestscreen += 30;
    } while (pos > 959);

    outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0xc);

    // Next plane
    if (destscreen == (byte *)0xA7000)
        destscreen = (byte *)0xA0000;
    else
        destscreen += 0x7000;
}

void VGA_VERT_InitGraphics(void)
{
    union REGS regs;

    regs.w.ax = 0x13;
    int386(0x10, (union REGS *)&regs, &regs);
    pcscreen = (byte *)0xA0000;
    currentscreen = (unsigned short *)0xA0000;
    destscreen = (byte *)0xA7000;

    //
    // switch to linear, non-chain4 mode
    //
    outp(SC_INDEX, SYNC_RESET);
    outp(SC_DATA, 1);

    outp(SC_INDEX, MEMORY_MODE);
    outp(SC_DATA, (inp(SC_DATA) & ~0x08) | 0x04);
    outp(GC_INDEX, GRAPHICS_MODE);
    outp(GC_DATA, (inp(GC_DATA) & ~0x10) | 0x00);
    outp(GC_INDEX, MISCELLANOUS);
    outp(GC_DATA, (inp(GC_DATA) & ~0x02) | 0x00);

    outpw(SC_INDEX, 0xf02);
    SetDWords(pcscreen, 0, 0x4000);

    outp(MISC_OUTPUT, 0xA3); // 350-scan-line scan rate

    outp(SC_INDEX, SYNC_RESET);
    outp(SC_DATA, 3);

    //
    // unprotect CRTC0 through CRTC0
    //
    outp(CRTC_INDEX, 0x11);
    outp(CRTC_DATA, (inp(CRTC_DATA) & ~0x80) | 0x00);

    //
    // stop scanning each line twice
    //
    outp(CRTC_INDEX, MAX_SCAN_LINE);
    outp(CRTC_DATA, (inp(CRTC_DATA) & ~0x1F) | 0x00);

    //
    // change the CRTC from doubleword to byte mode
    //
    outp(CRTC_INDEX, UNDERLINE);
    outp(CRTC_DATA, (inp(CRTC_DATA) & ~0x40) | 0x00);
    outp(CRTC_INDEX, MODE_CONTROL);
    outp(CRTC_DATA, (inp(CRTC_DATA) & ~0x00) | 0x40);

    //
    // set the vertical counts for 350-scan-line mode
    //
    outp(CRTC_INDEX, 0x06);
    outp(CRTC_INDEX + 1, 0xBF);
    outp(CRTC_INDEX, 0x07);
    outp(CRTC_INDEX + 1, 0x1F);
    outp(CRTC_INDEX, 0x10);
    outp(CRTC_INDEX + 1, 0x83);
    outp(CRTC_INDEX, 0x11);
    outp(CRTC_INDEX + 1, 0x85);
    outp(CRTC_INDEX, 0x12);
    outp(CRTC_INDEX + 1, 0x5D);
    outp(CRTC_INDEX, 0x15);
    outp(CRTC_INDEX + 1, 0x63);
    outp(CRTC_INDEX, 0x16);
    outp(CRTC_INDEX + 1, 0xBA);

    outp(SC_INDEX, MAP_MASK);
    outp(GC_INDEX, READ_MAP);
}

#endif
