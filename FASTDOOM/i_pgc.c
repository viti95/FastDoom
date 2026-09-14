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

// 4 bit quantized palettes, one packed entry per colour (14 palettes).
// Packed as a 16 bit value: r4 in bits 8-11, g4 in 4-7, b4 in 0-3.
static unsigned short pgc_palette[14 * 256];

// Context for the stall watchdog and the frame logging
static unsigned int pgc_frames = 0;
static int pgc_curline = -1;

// Total bytes sent to the command ring, for the frame log.
static unsigned long pgc_bytes_total = 0;

//
// PGC_LogInt
// Print a plain decimal integer. I_Printf would render it as a
// fixed point value ("123.0000"), which is confusing in the log.
//
static void PGC_LogInt(long v)
{
    char buf[16];
    char *p = buf + 15;
    unsigned long u;

    *p = 0;
    u = (v < 0 ? (unsigned long)(-v) : (unsigned long)v);
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
// PGC_LogHex8
// Print an 8 bit value in hex (2 digits), for ring pointers.
//
static void PGC_LogHex8(long v)
{
    char buf[3];
    int i;

    for (i = 1; i >= 0; i--)
    {
        int n = (int)((v >> (i * 4)) & 0x0F);
        buf[i] = (char)(n < 10 ? '0' + n : 'a' + n - 10);
    }
    buf[2] = 0;
    I_Puts(buf);
}

//
// PGC_LogHex32
// Print a 32 bit value in hex (8 digits), for memory addresses.
//
static void PGC_LogHex32(long v)
{
    char buf[9];
    int i;

    for (i = 7; i >= 0; i--)
    {
        int n = (int)((v >> (i * 4)) & 0x0F);
        buf[7 - i] = (char)(n < 10 ? '0' + n : 'a' + n - 10);
    }
    buf[8] = 0;
    I_Puts(buf);
}

//
// Burst write buffer. Writing the command ring one byte at a time
// costs the host four accesses to the PGC shared RAM per byte (two
// pointer reads, one data write, one pointer write). The PGC is
// picky about the ISA bus it finds (the reference notes report it
// failing on a 486 whose ISA bus was too fast), and every host
// access is a chance to collide with the 8088 in the middle of one
// of its own bus cycles. Bytes are therefore accumulated here and
// written to the ring in bursts: one space check and one pointer
// update per burst instead of per byte, cutting the host's shared
// RAM traffic by about a factor of four.
//
// Protocol safe: the 8088 only ever reads ring bytes with an index
// below IN_WR. The burst stores its bytes at indices at or above
// the current IN_WR, then advances IN_WR in a single atomic write;
// x86 stores are globally ordered, so the 8088 either sees the old
// IN_WR (burst not visible) or the new one (all bytes in place).
//
#define PGC_BURST_SIZE 32

static byte pgc_burst[PGC_BURST_SIZE];
static int pgc_burst_len = 0;

//
// PGC_BurstFlush
// Write the pending burst to the command ring: wait until there is
// room for the whole burst, store the bytes, then advance the write
// pointer in a single write. If the PGC stops consuming for too
// long it is considered stuck: this is logged with as much context
// as possible and the game aborts, so the log shows exactly where
// the card wedged.
//
static void PGC_BurstFlush(void)
{
    int wr;
    int len;
    int i;

    if (pgc_fatal)
        return;

    len = pgc_burst_len;
    pgc_burst_len = 0;
    if (len == 0)
        return;

    wr = PGC_IN_WR;

    // The ring has room for len more bytes unless IN_WR + len would
    // wrap around onto IN_RD.
    while (((wr + len) & 0xFF) == PGC_IN_RD)
    {

    }

    for (i = 0; i < len; i++)
        pgc_base[(wr + i) & 0xFF] = pgc_burst[i];
    PGC_IN_WR = (byte)((wr + len) & 0xFF);
}

//
// PGC_WriteByte
// Append one byte to the burst buffer; a full burst is written to
// the ring immediately.
//
static void PGC_WriteByte(byte b)
{
    if (pgc_fatal)
        return;

    pgc_burst[pgc_burst_len++] = b;
    if (pgc_burst_len == PGC_BURST_SIZE)
        PGC_BurstFlush();
    pgc_bytes_total++;
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

    // Make sure all pending command bytes reached the card before
    // looking for a response.
    PGC_BurstFlush();

    n = PGC_ReadOutput(buf, 64);
    if (n == 0)
        return;

    // Printable bytes are shown as characters so the 8088's ASCII
    // messages can be read directly from the log; the rest as hex.
    I_Printf("PGC: output buffer: ");
    for (i = 0; i < n; i++)
    {
        if (buf[i] >= 0x20 && buf[i] < 0x7F)
        {
            char c[2];

            c[0] = (char)buf[i];
            c[1] = 0;
            I_Puts(c);
        }
        else
            PGC_LogHex8((int)buf[i]);
    }
    I_Printf("\n");
}

//
// PGC_WaitOutput
// Wait until the PGC puts something in the output ring buffer.
//
static void PGC_WaitOutput(void)
{
    // Make sure all pending command bytes reached the card before
    // waiting for the response to the last one.
    PGC_BurstFlush();

    while (PGC_OUT_RD == PGC_OUT_WR)
    {
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

    // Make sure all pending command bytes reached the card first.
    PGC_BurstFlush();

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
        if (buf[i] >= 0x20 && buf[i] < 0x7F)
        {
            char c[2];

            c[0] = (char)buf[i];
            c[1] = 0;
            I_Puts(c);
        }
        else
            PGC_LogHex8((int)buf[i]);
    }
    I_Printf("\n");
}

//
// PGC_WriteLine
// Upload one screen scanline as a Hex mode IMAGEW command:
//   D9 row(2) col1(2) col2(2) data...
// Row 0 is the bottom line of the screen, so screen rows are
// flipped (verified on the real card: sending them unflipped shows
// the picture upside down).
// The data is RLE compressed. The encoding follows the reference
// implementation (PGCBMP) byte for byte:
//
//   [n][x]  n < 0x80   : n+1 copies of pixel x. Emitted only when
//                        3+ pixels are equal, at most 128 per block.
//                        A run of 1-2 pixels is a literal instead.
//   [n][...] n >= 0x80 : n-0x7F literal pixels follow (1..128,
//                        header 0xFF = 128 literals).
//
// Getting either header wrong desynchronizes the firmware RLE
// decoder, which then eats following data as garbage and can
// eventually compute an out of range row address, killing the
// PGC processor. This function has been verified host-side against
// the PGCBMP algorithm and a spec decoder (100k random lines,
// byte identical output, exact 640 pixel round trip).
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
        // Literal block: header = 0x7F + count, count 1..128.
        // Stop before the next 3-pixel run starts, like PGCBMP.
        int count = 1;

        while (count < 128 && x + count < SCREENWIDTH)
        {
            int p = x + count;

            if (p + 2 < SCREENWIDTH &&
                line[p] == line[p + 1] && line[p + 1] == line[p + 2])
                break;
            count++;
        }

        PGC_WriteByte((byte)(0x7F + count));
        while (count--)
            PGC_WriteByte(line[x++]);
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

    PGC_FlushOutput();
    PGC_FlushErrors();

    // The host has no framebuffer of its own, point pcscreen at
    // the backbuffer so any code touching it does not crash.
    pcscreen = backbuffer;

    // The demo screens never clear the backbuffer themselves and
    // nothing on the PGC path does either, so start from a known
    // clean slate to keep the first frame deterministic.
    memset(backbuffer, 0, SCREENWIDTH * SCREENHEIGHT);

    // Log both buffer addresses: the PGC shared RAM sits at C6000
    // and on this class of machine the DPMI linear mapping (or the
    // 640k A20 wraparound) can alias other memory onto it. If
    // either buffer below has a 640k wrap class near 0x6000-0x6FFF
    // (i.e. addr & 0x3FFFF in that range), part of it is the PGC
    // RAM itself and host writes to it hit the card.
    I_Printf("PGC: backbuffer at 0x");
    PGC_LogHex32((long)&backbuffer[0]);
    I_Printf("\n");

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
    PGC_BurstFlush();
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
}

//
// PGC_DiagLine
// Log a compact description of how a dirty backbuffer line differs
// from its shadow copy: how many bytes changed and the first few
// old->new values. Used to figure out what is actually changing on
// a screen that should be static.
//
static void PGC_DiagLine(byte *line, int frame, int y, int firstdiff)
{
    int shown = 0;
    int i;

    I_Printf("PGC: DIAG f");
    PGC_LogInt(frame);
    I_Printf(" l");
    PGC_LogInt(y);
    I_Printf(" p=");
    PGC_LogInt(firstdiff);
}

//
// I_FinishUpdate
// Upload the scanlines of the backbuffer that changed since the
// last frame to the PGC framebuffer.
//
void I_FinishUpdate(void)
{
    int y;

    if (!pgc_present || pgc_fatal)
        return;

    if (updatestate == I_NOUPDATE)
        return;

    pgc_frames++;

    for (y = 0; y < SCREENHEIGHT; y++)
    {
        byte *line = backbuffer + (unsigned int)y * SCREENWIDTH;
        int i;

        pgc_curline = y;

        PGC_WriteLine(line, y);
        if (pgc_fatal)
            return;
    }

    // Flush the burst before logging so the ring pointers below
    // are the real ones.
    PGC_BurstFlush();
    pgc_curline = -1;
    I_Printf("PGC: frame ");
    PGC_LogInt((int)pgc_frames);
    I_Printf(" done, ");
    I_Printf("total ");
    PGC_LogInt((long)pgc_bytes_total);
    I_Printf(" bytes");
    I_Printf("\n");

    // Keep the PGC output and error rings drained so the card
    // can never block on a full ring buffer.
    PGC_FlushOutput();
    PGC_FlushErrors();
    updatestate = I_NOUPDATE;
}

#endif
