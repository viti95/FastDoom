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

#if defined(MODE_13H) || defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF)

byte processedpalette[14 * 768];

void I_ProcessPalette(byte *palette)
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

void I_SetPalette(int numpalette)
{
    int pos = Mul768(numpalette);

    if (VGADACfix)
    {
        int i;
        byte *ptrprocessedpalette = processedpalette + pos;

        I_WaitSingleVBL();

        outp(PEL_WRITE_ADR, 0);

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
        FastPaletteOut(((unsigned char *)processedpalette) + pos);
    }
}

#endif
