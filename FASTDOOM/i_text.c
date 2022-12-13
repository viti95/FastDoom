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
#include "m_menu.h"

#if defined(TEXT_MODE)

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

byte lut16colors[14 * 256];
byte *ptrlut16colors;

unsigned short *textdestscreen = (unsigned short *)0xB8000;
byte textpage = 0;

void TEXT_40x25_InitGraphics(void)
{
    union REGS regs;

    // Set 40x25 color mode
    regs.h.ah = 0x00;
    regs.h.al = 0x01;
    int386(0x10, &regs, &regs);

    // Disable cursor
    regs.h.ah = 0x01;
    regs.h.ch = 0x3F;
    int386(0x10, &regs, &regs);

    // CGA Disable blink
    if (CGAcard)
    {
        I_DisableCGABlink();
    }
    else
    {
        // Disable blinking
        regs.h.ah = 0x10;
        regs.h.al = 0x03;
        regs.h.bl = 0x00;
        regs.h.bh = 0x00;
        int386(0x10, &regs, &regs);
    }
}

void TEXT_80x25_InitGraphics(void)
{
    union REGS regs;

    // Set 80x25 color mode
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
    int386(0x10, &regs, &regs);

    // Disable cursor
    regs.h.ah = 0x01;
    regs.h.ch = 0x3F;
    int386(0x10, &regs, &regs);

    // CGA Disable blink
    if (CGAcard)
    {
        I_DisableCGABlink();
    }
    else
    {
        // Disable blinking
        regs.h.ah = 0x10;
        regs.h.al = 0x03;
        regs.h.bl = 0x00;
        regs.h.bh = 0x00;
        int386(0x10, &regs, &regs);
    }
}

void TEXT_80x25_Double_InitGraphics(void)
{
    union REGS regs;

    // Set 80x25 color mode
    regs.h.ah = 0x00;
    regs.h.al = 0x03;
    int386(0x10, &regs, &regs);

    // Change font size to 8x8
    regs.h.ah = 0x11;
    regs.h.al = 0x12;
    regs.h.bh = 0;
    regs.h.bl = 0;
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
}

void I_ProcessPalette(byte *palette)
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

void I_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 256;
}

#if defined(MODE_T4025) || defined(MODE_T4050)

// 40x25 ChangeVideoPage
void I_FinishUpdate(void)
{
    union REGS regs;

    // Change video page
    regs.h.ah = 0x05;
    regs.h.al = textpage;
    regs.h.bh = 0x00;
    regs.h.bl = 0x00;
    int386(0x10, &regs, &regs);

    if (textpage == 2)
    {
        textpage = 0;
        textdestscreen = (unsigned short *)0xB8000;
    }
    else
    {
        textpage++;
        textdestscreen += 1024;
    }
}

#endif

#if defined(MODE_T8025)

// 80x25 ChangeVideoPage
void I_FinishUpdate(void)
{
    union REGS regs;

    // Change video page
    regs.h.ah = 0x05;
    regs.h.al = textpage;
    regs.h.bh = 0x00;
    regs.h.bl = 0x00;
    int386(0x10, &regs, &regs);

    if (textpage == 2)
    {
        textpage = 0;
        textdestscreen = (unsigned short *)0xB8000;
    }
    else
    {
        textpage++;
        textdestscreen += 2048;
    }
}

#endif

#if defined(MODE_T8043) || defined(MODE_T8086)

// 80x25 EGA Double ChangeVideoPage
void I_FinishUpdate(void)
{
    union REGS regs;

    // Change video page
    regs.h.ah = 0x05;
    regs.h.al = textpage;
    regs.h.bh = 0x00;
    regs.h.bl = 0x00;
    int386(0x10, &regs, &regs);

    if (textpage == 2)
    {
        textpage = 0;
        textdestscreen = (unsigned short *)0xB8000;
    }
    else
    {
        textpage++;
        textdestscreen += 3568;
    }
}

#endif

#if defined(MODE_T8050) || defined(MODE_T80100)

// 80x25 Double ChangeVideoPage
void I_FinishUpdate(void)
{
    union REGS regs;

    // Change video page
    regs.h.ah = 0x05;
    regs.h.al = textpage;
    regs.h.bh = 0x00;
    regs.h.bl = 0x00;
    int386(0x10, &regs, &regs);

    if (textpage == 2)
    {
        textpage = 0;
        textdestscreen = (unsigned short *)0xB8000;
    }
    else
    {
        textpage++;
        if (videoPageFix)
            textdestscreen += 4000;
        else
            textdestscreen += 4128;
    }
}

#endif

#endif
