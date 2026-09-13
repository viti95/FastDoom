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
//   Note on logging: I_Printf from i_debug.c prints every numeric
//   argument as a fixed point value ("123.0000"), so plain integers
//   are logged through PGC_LogInt instead.
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

// How many times the host may spin waiting for the PGC to consume
// a byte from the command ring before declaring the card stuck.
#define PGC_RING_TIMEOUT 1000000

static volatile byte *pgc_base = (volatile byte *)PGC_BASE;

#define PGC_IN_WR (pgc_base[0x300])
#define PGC_IN_RD (pgc_base[0x301])
#define PGC_OUT_WR (pgc_base[0x302])
#define PGC_OUT_RD (pgc_base[0x303])
#define PGC_ERR_WR (pgc_base[0x304])
#define PGC_ERR_RD (pgc_base[0x305])
#define PGC_TEST_BYTE (pgc_base[0x3DB])
#define PGC_FW_VER_HI (pgc_base[0x3F8])
#define PGC_FW_VER_LO (pgc_base[0x3F9])

// Hex command bytes
#define PGC_CMD_IMAGEW 0xD9
#define PGC_CMD_LUT 0xEE
#define PGC_CMD_LUTRD 0x50
#define PGC_CMD_CLEARS 0x0F

static byte pgc_present = 0;
static byte pgc_fatal = 0;
static byte pgc_verified = 0;

// 4 bit quantized palettes, one packed entry per colour (14 palettes).
// Packed as a 16 bit value: r4 in bits 8-11, g4 in 4-7, b4 in 0-3.
static unsigned short pgc_palette[14 * 256];

// Copy of the last frame uploaded to the PGC, used to detect
// which scanlines need to be retransmitted.
static byte *pgc_lastframe;

// Context for the stall watchdog and the frame logging
static unsigned int pgc_frames = 0;
static int pgc_curline = -1;

//
// PGC_LogInt
// Print a plain decimal integer. I_Printf would render it as a
// fixed point value ("123.0000"), which is confusing in the log.
//
static void PGC_LogInt(int v)
{
    char buf[16];
    char *p = buf + 15;
    unsigned int u;

    *p = 0;
    u = (unsigned int)(v < 0 ? -v : v);
    do
    {
        *--p = (char)('0' + (u % 10));
        u /= 10;
    } while (u);
    if (v < 0)
        *--p = '-';
    I_Puts(p);
}

//
// PGC_WriteByte
// Write one byte into the PGC command ring buffer. If the buffer
// is full, wait until the PGC has consumed some bytes. If the PGC
// stops consuming for too long, it is considered stuck: this is
// logged with as much context as possible and the game aborts, so
// the log shows exactly where the card wedged.
//
static void PGC_WriteByte(byte b)
{
    int wait = 0;

    if (pgc_fatal)
        return;

    while (((PGC_IN_WR + 1) & 0xFF) == PGC_IN_RD)
    {
        if (++wait >= PGC_RING_TIMEOUT)
        {
            pgc_fatal = 1;
            I_Printf("PGC: ring buffer stalled, the card stopped consuming commands\n");
            I_Printf("PGC: IN_WR=");
            PGC_LogInt((int)PGC_IN_WR);
            I_Printf(" IN_RD=");
            PGC_LogInt((int)PGC_IN_RD);
            I_Printf(" frame=");
            PGC_LogInt((int)pgc_frames);
            I_Printf(" line=");
            PGC_LogInt(pgc_curline);
            I_Printf("\n");
            I_Error(10); // does not return
        }
    }

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
// PGC_ReadOutput
// Read up to max pending bytes from the PGC output ring buffer.
//
static int PGC_ReadOutput(byte *buf, int max)
{
    int n = 0;

    while (PGC_OUT_RD != PGC_OUT_WR && n < max)
    {
        buf[n] = pgc_base[0x100 + PGC_OUT_RD];
        PGC_OUT_RD = (byte)((PGC_OUT_RD + 1) & 0xFF);
        n++;
    }
    return n;
}

//
// PGC_FlushOutput
// Drain the PGC output ring buffer and log whatever it contained.
//
static void PGC_FlushOutput(void)
{
    byte buf[64];
    int n;
    int i;

    n = PGC_ReadOutput(buf, 64);
    if (n == 0)
        return;

    I_Printf("PGC: output buffer: ");
    for (i = 0; i < n; i++)
    {
        PGC_LogInt((int)buf[i]);
        I_Printf(" ");
    }
    I_Printf("\n");
}

//
// PGC_WaitOutput
// Wait until the PGC puts something in the output ring buffer.
//
static void PGC_WaitOutput(void)
{
    int wait = 0;

    while (PGC_OUT_RD == PGC_OUT_WR)
    {
        if (++wait >= PGC_RING_TIMEOUT)
        {
            I_Printf("PGC: timeout waiting for output data\n");
            return;
        }
    }
}

//
// PGC_FlushErrors
// Read any pending bytes from the PGC error ring buffer and log them.
//
static void PGC_FlushErrors(void)
{
    byte buf[64];
    int n;
    int i;

    n = 0;
    while (PGC_ERR_RD != PGC_ERR_WR && n < 64)
    {
        buf[n] = pgc_base[0x200 + PGC_ERR_RD];
        PGC_ERR_RD = (byte)((PGC_ERR_RD + 1) & 0xFF);
        n++;
    }
    if (n == 0)
        return;

    I_Printf("PGC: error buffer: ");
    for (i = 0; i < n; i++)
    {
        PGC_LogInt((int)buf[i]);
        I_Printf(" ");
    }
    I_Printf("\n");
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
            // run of identical pixels starts.
            //
            // PGCBMP (the reference implementation) limits literal
            // runs to 127 bytes and never emits a 128 byte literal
            // run (header 0xFF). The PGC firmware may mishandle that
            // case, so do not emit it either.
            int count = 1;

            while (count < 127 &&
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

    I_Printf("PGC: presence test at C6000:00\n");

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
// PGC_CheckLut
// Read one palette entry back from the PGC with a LUTRD command and
// log both the values that were sent and the values that came back.
// If the card reports different values (or a different channel
// order), the log will show it.
//
static void PGC_CheckLut(int palette, int ink)
{
    unsigned short v = pgc_palette[palette * 256 + ink];
    byte out[16];
    int n;
    int i;

    PGC_WriteByte(PGC_CMD_LUTRD);
    PGC_WriteByte((byte)ink);
    PGC_WaitOutput();
    n = PGC_ReadOutput(out, 16);

    I_Printf("PGC: LUT check ink=");
    PGC_LogInt(ink);
    I_Printf(" sent r=");
    PGC_LogInt((int)((v >> 8) & 0x0F));
    I_Printf(" g=");
    PGC_LogInt((int)((v >> 4) & 0x0F));
    I_Printf(" b=");
    PGC_LogInt((int)(v & 0x0F));
    I_Printf(" readback=");
    PGC_LogInt(n);
    I_Printf(" byte(s):");
    for (i = 0; i < n; i++)
    {
        I_Printf(" ");
        PGC_LogInt((int)out[i]);
    }
    I_Printf("\n");
}

//
// PGC_VerifyPalette
// Pick the palette entries with the strongest red, green and blue
// components and read them back from the card to verify that the
// LUT values were actually stored as sent.
//
static void PGC_VerifyPalette(int palette)
{
    int inkr = 0;
    int inkg = 0;
    int inkb = 0;
    int ink;

    for (ink = 0; ink < 256; ink++)
    {
        int v = (int)pgc_palette[palette * 256 + ink];
        int vr = (int)((pgc_palette[palette * 256 + inkr] >> 8) & 0x0F);
        int vg = (int)((pgc_palette[palette * 256 + inkg] >> 4) & 0x0F);
        int vb = (int)(pgc_palette[palette * 256 + inkb] & 0x0F);

        if (((v >> 8) & 0x0F) > vr)
            inkr = ink;
        if (((v >> 4) & 0x0F) > vg)
            inkg = ink;
        if ((v & 0x0F) > vb)
            inkb = ink;
    }

    I_Printf("PGC: verifying palette with LUT readback\n");
    PGC_CheckLut(palette, inkr);
    if (inkg != inkr)
        PGC_CheckLut(palette, inkg);
    if (inkb != inkr && inkb != inkg)
        PGC_CheckLut(palette, inkb);
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

    I_Printf("PGC: detected, firmware version ");
    PGC_LogInt((int)PGC_FW_VER_HI);
    I_Printf(".");
    PGC_LogInt((int)PGC_FW_VER_LO);
    I_Printf("\n");

    I_Printf("PGC: switching to native 640x480 display (DI 0)\n");
    PGC_WriteAscii("DI 0\n");

    I_Printf("PGC: switching to hex command mode (CX)\n");
    PGC_WriteAscii("CX\n");
    PGC_FlushOutput();
    PGC_FlushErrors();

    I_Printf("PGC: clearing framebuffer to black\n");
    PGC_WriteByte(PGC_CMD_CLEARS);
    PGC_WriteByte(0);

    // Track the contents of the PGC framebuffer to only upload
    // the scanlines that actually changed.
    if (pgc_lastframe == 0)
    {
        pgc_lastframe = (byte *)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
        I_Printf("PGC: allocated shadow framebuffer of ");
        PGC_LogInt(SCREENWIDTH * SCREENHEIGHT);
        I_Printf(" bytes\n");
    }
    memset(pgc_lastframe, 0, SCREENWIDTH * SCREENHEIGHT);

    // The host has no framebuffer of its own, point pcscreen at
    // the backbuffer so any code touching it does not crash.
    pcscreen = backbuffer;

    I_Printf("PGC: init done, waiting for the first frame\n");
}

//
// PGC_ShutdownGraphics
// Switch back to the emulated CGA display so the host video output
// is usable again after the game exits.
//
void PGC_ShutdownGraphics(void)
{
    if (!pgc_present || pgc_fatal)
        return;

    I_Printf("PGC: switching back to CGA emulation display (DI 1)\n");
    PGC_WriteAscii("CA\n");
    PGC_WriteAscii("DI 1\n");
}

//
// I_ProcessPalette
// Takes full 8 bit values (the whole PLAYPAL lump, 14 palettes).
// Quantizes them to the 4 bits per channel the PGC palette holds.
//
// Note: gammatable holds 6 bit values (0-63, see I_SetGamma), so the
// high 4 of the 6 bits give the 4 bit channel value.
//
void I_ProcessPalette(byte *palette)
{
    int i;
    byte *ptr = gammatable;

    for (i = 0; i < 14 * 256; i++, palette += 3)
    {
        int r = (int)ptr[*(palette)] >> 2;
        int g = (int)ptr[*(palette + 1)] >> 2;
        int b = (int)ptr[*(palette + 2)] >> 2;

        pgc_palette[i] = (unsigned short)((r << 8) | (g << 4) | b);
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
    unsigned short *pal;

    if (!pgc_present || pgc_fatal)
        return;

    if (numpalette < 0 || numpalette > 13)
    {
        I_Printf("PGC: invalid palette number ");
        PGC_LogInt(numpalette);
        I_Printf("\n");
        return;
    }

    I_Printf("PGC: loading palette ");
    PGC_LogInt(numpalette);
    I_Printf(" into the card (256 LUTs)\n");

    pal = &pgc_palette[numpalette * 256];
    for (i = 0; i < 256; i++)
    {
        PGC_WriteByte(PGC_CMD_LUT);
        PGC_WriteByte((byte)i);
        PGC_WriteByte((byte)((pal[i] >> 8) & 0x0F));
        PGC_WriteByte((byte)((pal[i] >> 4) & 0x0F));
        PGC_WriteByte((byte)(pal[i] & 0x0F));
        if (pgc_fatal)
            return;
    }

    PGC_FlushErrors();

    // One time only: read a few entries back to verify that the
    // card stored the LUT values the way we sent them.
    if (!pgc_verified)
    {
        pgc_verified = 1;
        PGC_VerifyPalette(numpalette);
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
    int dirty = 0;

    if (!pgc_present || pgc_fatal)
        return;

    if (updatestate == I_NOUPDATE)
        return;

    pgc_frames++;

    for (y = 0; y < SCREENHEIGHT; y++)
    {
        byte *line = backbuffer + (unsigned int)y * SCREENWIDTH;
        byte *last = pgc_lastframe + (unsigned int)y * SCREENWIDTH;

        if (memcmp(line, last, SCREENWIDTH) == 0)
            continue;

        dirty++;
        pgc_curline = y;

        // Log every uploaded line. If the PGC 8088 dies while
        // processing a line and stops arbitrating the ISA bus,
        // the whole machine hangs on the very next shared memory
        // access, so the last line in the log is the last line
        // that made it through.
        I_Printf("PGC: f");
        PGC_LogInt((int)pgc_frames);
        I_Printf(" l");
        PGC_LogInt(y);
        I_Printf("\n");

        PGC_WriteLine(line, y);
        memcpy(last, line, SCREENWIDTH);
        if (pgc_fatal)
            return;
    }

    pgc_curline = -1;
    I_Printf("PGC: frame ");
    PGC_LogInt((int)pgc_frames);
    I_Printf(" done, ");
    PGC_LogInt(dirty);
    I_Printf(" line(s) uploaded\n");

    // Keep the PGC output and error rings drained so the card
    // can never block on a full ring buffer.
    PGC_FlushOutput();
    PGC_FlushErrors();
    updatestate = I_NOUPDATE;
}

#endif
