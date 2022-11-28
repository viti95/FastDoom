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
#include "i_vga.h"

#include "doomstat.h"

#if defined(MODE_13H) || defined(MODE_V2) || defined(MODE_VBE2) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT)

byte processedpalette[14 * 768];

// Test VGA REP OUTSB capability
void VGA_TestFastSetPalette(void)
{
    if (!VGADACfix)
    {
        byte test_palette[768];
        unsigned short x;
        byte y;

        // Initialize test palette
        for (x = 0; x < 768; x++)
        {
            test_palette[x] = x & 63;
        }

        // Write test palette using REP STOSB
        outp(PEL_WRITE_ADR, 0);
        OutString(PEL_DATA, test_palette, 768);

        // Read palette from VGA card
        // and compare results
        outp(PEL_READ_ADR, 0);
        for (x = 0; x < 768; x++)
        {
            byte read_data = inp(PEL_DATA);

            if (read_data != test_palette[x])
            {
                VGADACfix = true;
                return;
            }
        }
    }
}

void VGA_ProcessPalette(byte *palette)
{
    int i;

    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 768; i += 4, palette += 4)
    {
        processedpalette[i] = ptr[*palette];
        processedpalette[i + 1] = ptr[*(palette + 1)];
        processedpalette[i + 2] = ptr[*(palette + 2)];
        processedpalette[i + 3] = ptr[*(palette + 3)];
    }
}

void VGA_SetPalette(int numpalette)
{
    int i;
    int pos = Mul768(numpalette);

    outp(PEL_WRITE_ADR, 0);

    if (VGADACfix)
    {
        byte *ptrprocessedpalette = processedpalette + pos;
        for (i = 0; i < 768; i += 4)
        {
            outp(PEL_DATA, *(ptrprocessedpalette));
            outp(PEL_DATA, *(ptrprocessedpalette + 1));
            outp(PEL_DATA, *(ptrprocessedpalette + 2));
            outp(PEL_DATA, *(ptrprocessedpalette + 3));
            ptrprocessedpalette += 4;
        }
    }
    else
    {
        OutString(PEL_DATA, ((unsigned char *)processedpalette) + pos, 768);
    }
}

#endif
