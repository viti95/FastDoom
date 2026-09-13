//
// Copyright (C) 2026 FastDoom contributors
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//   IBM Professional Graphics Controller (PGC) video support.
//
//   The PGC is a self contained graphics board with its own 8088
//   processor, 320k of video RAM and a 2k of RAM shared with the
//   host PC, mapped at 0xC6000. The host cannot address the PGC
//   framebuffer directly; instead it sends commands into a 256 byte
//   ring buffer and the PGC executes them asynchronously.
//
//   The game is rendered into the main memory backbuffer (see
//   USE_BACKBUFFER) and the changed scanlines are uploaded to the
//   PGC with run length compressed IMAGEW commands in I_FinishUpdate.
//

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
#include "i_gamma.h"
#include "i_debug.h"
#include "z_zone.h"
#include "i_pgc.h"

#if defined(MODE_PGC)

//
// PGC shared memory layout (2k at 0xC6000, 32-bit flat address 0xC6000)
//
//   0xC6000-0xC60FF  host to PGC command ring buffer
//   0xC6100-0xC61FF  PGC to host output ring buffer
//   0xC6200-0xC62FF  PGC to host error ring buffer
//   0xC6300          write pointer in the command buffer (host)
//   0xC6301          read pointer in the command buffer (PGC)
//   0xC6302          write pointer in the output buffer (PGC)
//   0xC6303          read pointer in the output buffer (host)
//   0xC6304          write pointer in the error buffer (PGC)
//   0xC6305          read pointer in the error buffer (host)
//   0xC63DB          presence test byte
//   0xC63F8-0xC63F9  PGC firmware version
//

#define PGC_BASE 0xC6000

static volatile byte *pgc_base = (volatile byte *)PGC_BASE;

#define PGC_IN_WR (pgc_base[0x300])
#define PGC_IN_RD (pgc_base[0x301])
#define PGC_ERR_WR (pgc_base[0x304])
#define PGC_ERR_RD (pgc_base[0x305])
#define PGC_TEST_BYTE (pgc_base[0x3DB])
#define PGC_FW_VER_HI (pgc_base[0x3F8])
#define PGC_FW_VER_LO (pgc_base[0x3F9])

// Hex command bytes
#define PGC_CMD_IMAGEW 0xD9
#define PGC_CMD_LUT 0xEE
#define PGC_CMD_CLEARS 0x0F

static byte pgc_present = 0;

// 4 bit quantized palettes, one packed entry per colour (14 palettes)
static byte pgc_palette[14 * 256];

// Copy of the last frame uploaded to the PGC, used to detect
// which scanlines need to be retransmitted.
static byte *pgc_lastframe;

//
// PGC_WriteByte
// Write one byte into the PGC command ring buffer. If the buffer
// is full, wait until the PGC has consumed some bytes.
//
static void PGC_WriteByte(byte b)
{
    while (((PGC_IN_WR + 1) & 0xFF) == PGC_IN_RD)
        ;

    pgc_base[PGC_IN_WR] = b;
    PGC_IN_WR = (byte)((PGC_IN_WR + 1) & 0xFF);
}

//
// PGC_WriteWord
// Write a 16 bit value, least significant byte first.
//
static void PGC_WriteWord(unsigned short v)
{
    PGC_WriteByte((byte)(v & 0xFF));
    PGC_WriteByte((byte)(v >> 8));
}

//
// PGC_WriteAscii
// Send an ASCII command, newline terminated. Only to be used before
// (or to switch back to) Hex mode, Hex commands are much faster.
//
static void PGC_WriteAscii(const char *s)
{
    while (*s)
        PGC_WriteByte((byte)*s++);
}

//
// PGC_FlushErrors
// Read any pending bytes from the PGC error ring buffer and log them.
//
static void PGC_FlushErrors(void)
{
    int c;

    while (PGC_ERR_RD != PGC_ERR_WR)
    {
        c = (int)pgc_base[0x200 + PGC_ERR_RD];
        PGC_ERR_RD = (byte)((PGC_ERR_RD + 1) & 0xFF);
        I_Printf("PGC: error byte %02X\n", c);
    }
}

//
// PGC_WriteLine
// Upload one scanline with a Hex mode IMAGEW command:
//   D9 row(2) col1(2) col2(2) data...
// Row 0 is the bottom line of the screen, so screen rows have to be
// flipped. The pixel data is run length compressed on the fly:
//   [count][xx] with count < 0x80: xx repeated count+1 times
//   [count][xxxx] with count >= 0x80: count-0x7F literal bytes
//
static void PGC_WriteLine(byte *line, int screenrow)
{
    int x = 0;

    PGC_WriteByte(PGC_CMD_IMAGEW);
    PGC_WriteWord((unsigned short)(SCREENHEIGHT - 1 - screenrow));
    PGC_WriteWord(0);
    PGC_WriteWord((unsigned short)(SCREENWIDTH - 1));

    while (x < SCREENWIDTH)
    {
        byte c = line[x];
        int end = x + 1;

        // Find the end of the run of identical pixels
        while (end < SCREENWIDTH && line[end] == c)
            end++;

        if (end - x > 1)
        {
            // Run of identical pixels, up to 128 copies per entry
            int run = end - x;

            while (run > 128)
            {
                PGC_WriteByte(0x7F);
                PGC_WriteByte(c);
                x += 128;
                run -= 128;
            }
            PGC_WriteByte((byte)(run - 1));
            PGC_WriteByte(c);
            x += run;
        }
        else
        {
            // Literal run of single pixels, stop before the next
            // run of identical pixels starts. Up to 128 bytes.
            int count = 1;

            while (count < 128 &&
                   x + count < SCREENWIDTH - 1 &&
                   line[x + count] != line[x + count + 1])
                count++;

            PGC_WriteByte((byte)(0x7F + count - 1));
            while (count--)
                PGC_WriteByte(line[x++]);
        }
    }
}

//
// PGC_Detect
// Probe the presence test byte in the shared memory area. A regular
// PC memory read/write cycle will not fail, so this only returns
// true if the PGC is actually there.
//
static boolean PGC_Detect(void)
{
    byte saved;

    saved = PGC_TEST_BYTE;
    PGC_TEST_BYTE = 0x5A;
    if (PGC_TEST_BYTE != 0x5A)
    {
        PGC_TEST_BYTE = saved;
        return false;
    }
    PGC_TEST_BYTE = 0xA5;
    if (PGC_TEST_BYTE != 0xA5)
    {
        PGC_TEST_BYTE = saved;
        return false;
    }
    PGC_TEST_BYTE = saved;
    return true;
}

//
// PGC_InitGraphics
//
void PGC_InitGraphics(void)
{
    if (!PGC_Detect())
    {
        I_Printf("PGC: no Professional Graphics Controller detected at C6000:00\n");
        I_Error(10);
    }
    pgc_present = 1;
    I_Printf("PGC: detected, firmware version %02X.%02X\n",
             (int)PGC_FW_VER_HI, (int)PGC_FW_VER_LO);

    // Switch to the native 640x480 display and to Hex command mode.
    // ASCII CA/CX commands work in either mode, so this is safe
    // regardless of the mode the card was left in.
    PGC_WriteAscii("DI 0\n");
    PGC_WriteAscii("CX\n");
    PGC_FlushErrors();

    // Clear the framebuffer to black
    PGC_WriteByte(PGC_CMD_CLEARS);
    PGC_WriteByte(0);

    // Track the contents of the PGC framebuffer to only upload
    // the scanlines that actually changed.
    if (pgc_lastframe == 0)
        pgc_lastframe = (byte *)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    memset(pgc_lastframe, 0, SCREENWIDTH * SCREENHEIGHT);

    // The host has no framebuffer of its own, point pcscreen at
    // the backbuffer so any code touching it does not crash.
    pcscreen = backbuffer;
}

//
// PGC_ShutdownGraphics
// Switch back to the emulated CGA display so the host video output
// is usable again after the game exits.
//
void PGC_ShutdownGraphics(void)
{
    if (!pgc_present)
        return;

    PGC_WriteAscii("CA\n");
    PGC_WriteAscii("DI 1\n");
}

//
// I_ProcessPalette
// Takes full 8 bit values (the whole PLAYPAL lump, 14 palettes).
// Quantizes them to the 4 bits per channel the PGC palette holds.
//
void I_ProcessPalette(byte *palette)
{
    int i;
    byte *ptr = gammatable;

    for (i = 0; i < 14 * 256; i++, palette += 3)
    {
        int r = (int)ptr[*(palette)];
        int g = (int)ptr[*(palette + 1)];
        int b = (int)ptr[*(palette + 2)];

        pgc_palette[i] = (byte)(((r >> 4) << 4) | ((g >> 4) << 2) | (b >> 4));
    }
}

//
// I_SetPalette
// Send the 256 LUT entries of the requested palette to the PGC
// as Hex mode LUT commands: EE ink r g b
//
void I_SetPalette(int numpalette)
{
    int i;
    byte *pal;

    if (!pgc_present || numpalette < 0 || numpalette > 13)
    {
        I_Printf("PGC: invalid palette number %i\n", numpalette);
        return;
    }

    pal = &pgc_palette[numpalette * 256];
    for (i = 0; i < 256; i++)
    {
        PGC_WriteByte(PGC_CMD_LUT);
        PGC_WriteByte((byte)i);
        PGC_WriteByte((byte)((pal[i] >> 4) & 0x0F));
        PGC_WriteByte((byte)((pal[i] >> 2) & 0x0F));
        PGC_WriteByte((byte)(pal[i] & 0x0F));
    }
}

//
// I_FinishUpdate
// Upload the scanlines of the backbuffer that changed since the
// last frame to the PGC framebuffer.
//
void I_FinishUpdate(void)
{
    int y;

    if (!pgc_present)
        return;

    if (updatestate == I_NOUPDATE)
        return;

    for (y = 0; y < SCREENHEIGHT; y++)
    {
        byte *line = backbuffer + (unsigned int)y * SCREENWIDTH;
        byte *last = pgc_lastframe + (unsigned int)y * SCREENWIDTH;

        if (memcmp(line, last, SCREENWIDTH) == 0)
            continue;

        PGC_WriteLine(line, y);
        memcpy(last, line, SCREENWIDTH);
    }

    updatestate = I_NOUPDATE;
}

#endif
