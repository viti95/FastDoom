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
    char ch;

    if (!term_enabled || !TERM_IsActive())
        return;

    // On first frame, send full screen
    if (term_first_frame)
    {
        TERM_Clear();
        term_first_frame = false;

        for (y = 0; y < 25; y++)
        {
            for (x = 0; x < 80; x++)
            {
                idx = (unsigned int)y * 80u + (unsigned int)x;
                cell = backbuffer[idx];
                ch = (char)(cell & 0xFF);

                if (ch >= 0x20 && ch <= 0x7E)
                {
                    TERM_SendByte((byte)ch);
                }
                else if (ch == '\r' || ch == '\n')
                {
                    TERM_SendByte((byte)ch);
                }
                else
                {
                    TERM_SendByte(' ');
                }

                term_prev[idx] = cell;
            }
            TERM_SendByte('\r');
            TERM_SendByte('\n');
        }
        TERM_SetCursor(0, 24);
        return;
    }

    // Incremental update: walk the buffer and emit changed cells
    for (y = 0; y < 25; y++)
    {
        for (x = 0; x < 80; x++)
        {
            idx = (unsigned int)y * 80u + (unsigned int)x;
            cell = backbuffer[idx];

            if (cell != term_prev[idx])
            {
                // Move cursor to the changed cell
                TERM_SetCursor(x, y);

                ch = (char)(cell & 0xFF);

                if (ch >= 0x20 && ch <= 0x7E)
                {
                    TERM_SendByte((byte)ch);
                }
                else
                {
                    TERM_SendByte(' ');
                }

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
