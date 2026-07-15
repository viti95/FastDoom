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

/* Tracked cursor position on the VT100 (0-based).  -1 = unknown. */
static int term_cursor_row;
static int term_cursor_col;

/*
 * TERM_UpdateFromBuffer
 *
 * Optimised incremental screen mirror for VT100.  Strategy:
 *   1. Build a dirty-row bitmap (skip unchanged rows entirely).
 *   2. Within each dirty row, merge consecutive changed cells
 *      into a single CUP + run of N characters (run-length).
 *   3. Between non-consecutive runs on the SAME row, use CUF
 *      (\x1B[nC) instead of CUP to save 2-4 bytes per jump.
 *   4. Track the virtual cursor position so CUF is safe.
 *
 * Byte budget comparison per changed cell:
 *   Before: CUP (6-7 bytes) + char = 7-8 bytes
 *   After  (consecutive run): CUP / 1st only + 1 byte per char
 *   After  (same-row CUF):    3-4 bytes + char = 4-5 bytes
 */
void TERM_UpdateFromBuffer(void)
{
    int x, y, rx;
    int run_start;
    int dirty_rows;
    unsigned int idx;
    byte rx_ch;
    static boolean row_dirty[TERM_ROWS];

    if (!term_enabled || !TERM_IsActive())
        return;

    /* ---- Full-screen transfer (first frame only) ----
     * VT100 has 24 rows. Sending \r\n on row 24 would scroll
     * the display and lose line 1.  So we send \r\n only after
     * rows 0-22, then stream row 23 as the final line.
     * Cursor ends at (row=23, col=79) after 80 chars. */
    if (term_first_frame)
    {
        TERM_Clear();
        term_first_frame = false;

        for (y = 0; y < TERM_ROWS; y++)
        {
            for (x = 0; x < 80; x++)
            {
                idx = (unsigned int)y * 80u + (unsigned int)x;
                TERM_SendChar((byte)backbuffer[idx]);
                term_prev[idx] = backbuffer[idx];
            }

            /* Only \r\n for rows 0..22.
             * Row 23 (the last VT100 row) does NOT get \r\n
             * to avoid scrolling the display and losing line 1. */
            if (y < TERM_ROWS - 1)
            {
                TERM_SendByte('\r');
                TERM_SendByte('\n');
            }
        }

        /* Cursor position after full-screen is uncertain
         * (terminal may auto-wrap past column 80).
         * Mark unknown so the first incremental run uses CUP
         * to reposition correctly. */
        term_cursor_row = TERM_ROWS - 1;
        term_cursor_col = -1;
        return;
    }

    /* ---- Phase 1: build dirty-row bitmap ---- */
    dirty_rows = 0;
    for (y = 0; y < TERM_ROWS; y++)
    {
        row_dirty[y] = false;
        for (x = 0; x < 80; x++)
        {
            idx = (unsigned int)y * 80u + (unsigned int)x;
            if (backbuffer[idx] != term_prev[idx])
            {
                row_dirty[y] = true;
                dirty_rows++;
                break;
            }
        }
    }

    if (dirty_rows == 0)
        return;       /* nothing changed -- zero bytes sent */

    /* ---- Phase 2: emit changed runs ---- */
    for (y = 0; y < TERM_ROWS; y++)
    {
        if (!row_dirty[y])
            continue;

        x = 0;
        while (x < 80)
        {
            idx = (unsigned int)y * 80u + (unsigned int)x;

            /* Skip unchanged cells */
            if (backbuffer[idx] == term_prev[idx])
            {
                x++;
                continue;
            }

            /* Found start of a changed run */
            run_start = x;

            /* Extend run through consecutive changed cells */
            while (x < 80)
            {
                idx = (unsigned int)y * 80u + (unsigned int)x;
                if (backbuffer[idx] == term_prev[idx])
                    break;
                x++;
            }
            /* run covers columns [run_start .. x-1] */

            /* Position cursor to start of run.
             * Prefer CUF if already on the same row. */
            if (term_cursor_row == y && term_cursor_col == run_start)
            {
                /* Cursor already positioned -- zero bytes! */
            }
            else if (term_cursor_row == y && term_cursor_col >= 0)
            {
                int gap = run_start - term_cursor_col;
                if (gap > 0 && gap < 80)
                {
                    /* CUF (Cursor Forward): \x1B[nC (3-4 bytes) */
                    TERM_SendByte(TERM_ESC);
                    TERM_SendByte('[');
                    TERM_FormatUnsigned(gap);
                    TERM_SendByte('C');
                    term_cursor_col = run_start;
                }
                else
                {
                    TERM_SetCursor(run_start, y);
                    term_cursor_row = y;
                    term_cursor_col = run_start;
                }
            }
            else
            {
                TERM_SetCursor(run_start, y);
                term_cursor_row = y;
                term_cursor_col = run_start;
            }

            /* Stream character data for the entire run */
            for (rx = run_start; rx < x; rx++)
            {
                idx = (unsigned int)y * 80u + (unsigned int)rx;
                rx_ch = (byte)backbuffer[idx];
                TERM_SendChar(rx_ch);
                term_prev[idx] = backbuffer[idx];
            }

            /* Cursor is now at column x (0-based), same row. */
            term_cursor_col = x;
        }
    }
}

/*
 * TERM_FormatUnsigned (local helper for CUF gap encoding).
 * Same logic as in i_term.c but inlined here to avoid
 * cross-module dependency on a static function.
 */
static void TERM_FormatUnsigned(unsigned int val)
{
    if (val == 0)
    {
        TERM_SendByte('0');
        return;
    }
    if (val < 10)
    {
        TERM_SendByte((byte)(val + '0'));
        return;
    }
    if (val < 100)
    {
        TERM_SendByte((byte)(val / 10 + '0'));
        TERM_SendByte((byte)(val % 10 + '0'));
        return;
    }
    /* val < 1000 */
    TERM_SendByte((byte)(val / 100 + '0'));
    val %= 100;
    TERM_SendByte((byte)(val / 10 + '0'));
    TERM_SendByte((byte)(val % 10 + '0'));
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
