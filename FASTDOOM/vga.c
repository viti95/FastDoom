#include <string.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "vga.h"
#include "doomtype.h"
#include "i_ibm.h"
#include "v_video.h"
#include "tables.h"
#include "math.h"
#include "i_system.h"

#include "doomstat.h"

#if defined(MODE_13H)

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

#endif
