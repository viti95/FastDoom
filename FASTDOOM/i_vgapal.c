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
#include "i_vgapal.h"

#include "doomstat.h"

#if defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)

// Test VGA REP OUTSB capability
void VGA_TestFastSetPalette(void)
{
    if (!VGADACfix)
    {
        byte test_palette[768];
        unsigned short x;

        // Initialize test palette
        for (x = 0; x < 768; x++)
        {
            test_palette[x] = x & 63;
        }

        // Write test palette using REP STOSB
        FastPaletteOut(test_palette);
        
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
