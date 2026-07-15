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
#include "i_term.h"

#if defined(MODE_MDA)

unsigned short backbuffer[80 * 25];
unsigned short *textdestscreen = backbuffer;

/* Terminal output for MDA mode. */
boolean term_enabled;
int     term_port;
int     term_baud;

/* Previous backbuffer content for detecting changed cells. */
static unsigned short term_prev[80 * 25];
static boolean        term_first_frame;

void TERM_UpdateFromBuffer(void)
{
    int x, y;
    unsigned int idx;
    unsigned short cell;
    byte ch;

    if (!term_enabled || !TERM_IsActive())
        return;

    /* On first frame, send full screen to VT100 (80x24).
       MDA has 25 rows but VT100 only shows 24, so we
       skip row 24 (index 24) to prevent unwanted scrolling.
       Use \r\n line endings for full-screen transfer. */
    if (term_first_frame)
    {
        TERM_Clear();
        term_first_frame = false;

        for (y = 0; y < TERM_ROWS; y++)
        {
            for (x = 0; x < 80; x++)
            {
                idx = (unsigned int)y * 80u + (unsigned int)x;
                cell = backbuffer[idx];
                ch = (byte)(cell & 0xFF);

                /* Translate CP437 char to VT100-safe ASCII */
                TERM_SendChar(ch);

                /* Strip MDA blink/intensity bits for VT100.
                   MDA attribute bits: 0-3=colour, 4=intensity,
                   5-6=reserved, 7=blink.  VT100 has no SGR
                   support for these, so ignore all attributes. */
                (void)(cell >> 8);

                term_prev[idx] = cell;
            }
            TERM_SendByte('\r');
            TERM_SendByte('\n');
        }
        TERM_SetCursor(0, TERM_ROWS - 1);
        return;
    }

    /* Incremental update: walk the buffer and emit changed cells.
       Only update rows 0-23 (VT100 display); skip row 24. */
    for (y = 0; y < TERM_ROWS; y++)
    {
        for (x = 0; x < 80; x++)
        {
            idx = (unsigned int)y * 80u + (unsigned int)x;
            cell = backbuffer[idx];

            if (cell != term_prev[idx])
            {
                /* Move cursor to the changed cell (VT100 CUP) */
                TERM_SetCursor(x, y);

                ch = (byte)(cell & 0xFF);

                /* Translate CP437 char to VT100-safe ASCII */
                TERM_SendChar(ch);

                term_prev[idx] = cell;
            }
        }
    }
}

void MDA_InitGraphics(void)
{
    union REGS regs;

    // Set 80x25 monochrome mode
    regs.h.ah = 0x00;
    regs.h.al = 0x07;
    int386(0x10, &regs, &regs);

    // Disable cursor
    regs.h.ah = 0x01;
    regs.h.ch = 0x3F;
    int386(0x10, &regs, &regs);

    // Disable MDA blink
    I_DisableMDABlink();

    // Initialise serial terminal output if requested
    if (term_enabled)
    {
        if (TERM_Init(term_port, term_baud) == 0)
        {
            term_first_frame = true;
            printf("Serial terminal: active (port 0x%03X, %d baud)\n",
                   term_port, term_baud);
        }
        else
        {
            printf("Serial terminal: failed to initialise\n");
            term_enabled = false;
        }
    }
}

void I_ProcessPalette(byte *palette)
{
    // Do nothing
}

void I_SetPalette(int numpalette)
{
    // Do nothing
}

void I_FinishUpdate(void)
{
    CopyDWords(backbuffer, 0xB0000, 1000);

    // Mirror text output to serial terminal
    if (term_enabled)
    {
        TERM_UpdateFromBuffer();
        TERM_Flush();
    }
}

void MDA_ShutdownTerminal(void)
{
    if (term_enabled)
    {
        TERM_Shutdown();
        term_enabled = false;
    }
}

#endif
