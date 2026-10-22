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
//   PGC with run length compressed IMAGEW commands in
//   I_FinishUpdate. I_FinishUpdate only scans the screen areas
//   the renderer marked dirty in updatestate (view, status bar,
//   messages, full screen), like I_FinishUpdateDifferential386,
//   and a shadow copy of the backbuffer lets it skip unchanged
//   lines entirely and upload only the changed segments of the
//   rest, as partial line IMAGEW commands. Each command is pushed
//   to the ring in bulk (PGC_WriteBuf): the staged command bytes
//   are written with a single IN_WR advance per chunk of at most
//   255 bytes, so the slow 0xC6000 page is touched once per byte
//   instead of four times.
//

#include <string.h>

#include "doomtype.h"
#include "i_ibm.h"
#include "v_video.h"
#include "i_system.h"
#include "doomstat.h"
#include "m_menu.h"
#include "i_gamma.h"
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

// Hex command bytes
#define PGC_CMD_IMAGEW 0xD9
#define PGC_CMD_LUT 0xEE
#define PGC_CMD_CLEARS 0x0F

static byte pgc_present = 0;

// 4 bit quantized palettes, one packed entry per colour (14 palettes).
// Packed as a 16 bit value: r4 in bits 8-11, g4 in 4-7, b4 in 0-3.
static unsigned short pgc_palette[14 * 256];

// Shadow copy of the backbuffer: what the PGC framebuffer currently
// holds. I_FinishUpdate diffs the backbuffer against this and only
// uploads what changed.
static byte pgc_shadow[SCREENWIDTH * SCREENHEIGHT];

// Unchanged gap of this many words (pixels / 2) between two changed
// areas on a line is resent with the surrounding segment instead of
// ending it: a new IMAGEW costs 7 header bytes plus at least 2 RLE
// bytes, so retransmitting up to 8 gap pixels is cheaper.
#define PGC_MERGE_GAP 4

//
// PGC_WriteByte
// Write one byte directly into the host to PGC command ring at
// 0xC6000, using the ring buffer fully: the 256 byte ring can hold
// up to 255 bytes (it is full when IN_WR is one ahead of IN_RD),
// so the host is allowed to run up to 255 bytes ahead of the 8088.
// If the ring is full, wait until the PGC consumes a byte.
//
// Protocol safe: the 8088 only ever reads ring bytes with an index
// below IN_WR. The byte is stored at the current IN_WR, then IN_WR
// is advanced in a single atomic write; x86 stores are globally
// ordered, so the 8088 either sees the old IN_WR (byte not visible)
// or the new one (byte in place).
//
static void PGC_WriteByte(byte b)
{
    byte wr;
    byte rd;

    wr = PGC_IN_WR;
    rd = PGC_IN_RD;

    // Wait until there is room: full when IN_WR + 1 wraps onto
    // IN_RD.
    while ((byte)(wr + 1) == rd)
        rd = PGC_IN_RD;

    pgc_base[wr] = b;
    PGC_IN_WR = (byte)(wr + 1);
}

//
// PGC_WriteBuf
// Write a whole buffer into the host to PGC command ring. Unlike
// PGC_WriteByte, IN_WR and IN_RD are only read once per chunk and
// IN_WR is advanced once per chunk, which cuts the number of
// accesses to the slow 0xC6000 page from 4x to about 1x the byte
// count.
//
// Protocol safe for the same reason as PGC_WriteByte: the 8088
// only ever reads ring bytes with an index below IN_WR, and x86
// stores are globally ordered, so the 8088 only sees the advanced
// IN_WR after every chunk byte is in place. If the chunk crosses
// the end of the 256 byte ring it is split; the ring being full
// waits for the PGC to consume bytes, as in PGC_WriteByte.
//
static void PGC_WriteBuf(byte *buf, int len)
{
    while (len > 0)
    {
        int wr = PGC_IN_WR;
        int rd = PGC_IN_RD;
        int avail = (rd - wr - 1) & 0xFF;
        int n;
        int i;

        // avail is 0 when the ring is full (IN_WR one ahead of
        // IN_RD), 255 when empty. Wait for the PGC to consume.
        if (avail == 0)
            continue;

        n = (len < avail) ? len : avail;

        // The chunk may not cross the end of the 0xC6000-0xC60FF
        // ring buffer; the remainder goes in the next iteration.
        if (wr + n > 256)
            n = 256 - wr;

        for (i = 0; i < n; i++)
            pgc_base[wr + i] = buf[i];

        PGC_IN_WR = (byte)(wr + n);
        buf += n;
        len -= n;
    }
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
// PGC_FlushOutput
// Drain the PGC output ring buffer so the card can never block on
// a full ring.
//
static void PGC_FlushOutput(void)
{
    while (PGC_OUT_RD != PGC_OUT_WR)
        PGC_OUT_RD = (byte)((PGC_OUT_RD + 1) & 0xFF);
}

//
// PGC_FlushErrors
// Drain the PGC error ring buffer so the card can never block on a
// full ring.
//
static void PGC_FlushErrors(void)
{
    while (PGC_ERR_RD != PGC_ERR_WR)
        PGC_ERR_RD = (byte)((PGC_ERR_RD + 1) & 0xFF);
}

//
// PGC_WriteRange
// Upload one pixel range of a screen scanline as a Hex mode IMAGEW
// command:
//   D9 row(2) col1(2) col2(2) data...
// Row 0 is the bottom line of the screen, so screen rows are
// flipped (verified on the real card: sending them unflipped shows
// the picture upside down). col1/col2 select a sub range of the
// line; the card plots exactly those columns.
// The data is RLE compressed. The encoding follows the reference
// implementation (PGCBMP) byte for byte:
//
//   [n][x]  n < 0x80   : n+1 copies of pixel x. Emitted only when
//                        3+ pixels are equal, at most 128 per block
//                        (a longer run becomes several blocks).
//                        A run of 1-2 pixels is a literal instead.
//   [n][...] n >= 0x80 : n-0x7F literal pixels follow (1..128,
//                        header 0xFF = 128 literals). A literal
//                        block stops just before a 3-pixel run.
//
// Getting either header wrong desynchronizes the firmware RLE
// decoder, which then eats following data as garbage and can
// eventually compute an out of range row address, killing the
// PGC processor.
//
// Staging area for one IMAGEW command. Worst case is a full 640
// pixel line without runs: 5 literal blocks of 128 pixels (645
// bytes) plus the 7 byte header.
static byte pgc_rangebuf[704];

static void PGC_WriteRange(byte *line, int screenrow, int col1, int col2)
{
    int x = col1;
    int end = col2 + 1;
    int p = 0;
    int row = SCREENHEIGHT - 1 - screenrow;

    // Header: D9 row(2) col1(2) col2(2), little endian.
    pgc_rangebuf[p++] = PGC_CMD_IMAGEW;
    pgc_rangebuf[p++] = (byte)(row & 0xFF);
    pgc_rangebuf[p++] = (byte)(row >> 8);
    pgc_rangebuf[p++] = (byte)(col1 & 0xFF);
    pgc_rangebuf[p++] = (byte)(col1 >> 8);
    pgc_rangebuf[p++] = (byte)(col2 & 0xFF);
    pgc_rangebuf[p++] = (byte)(col2 >> 8);

    while (x < end)
    {
        if (x + 2 < end &&
            line[x] == line[x + 1] && line[x + 1] == line[x + 2])
        {
            // Repeat block: n < 0x80, n+1 copies of one pixel,
            // max 128 per block.
            int count = 1;

            while (count < 128 && x + count < end &&
                   line[x] == line[x + count])
                count++;

            pgc_rangebuf[p++] = (byte)(count - 1);
            pgc_rangebuf[p++] = line[x];
            x += count;
        }
        else
        {
            // Literal block: header = 0x7F + count, count 1..128.
            // Stop before the next 3-pixel run starts, like PGCBMP.
            int count = 1;

            while (count < 128 && x + count < end)
            {
                int q = x + count;

                if (q + 2 < end &&
                    line[q] == line[q + 1] && line[q + 1] == line[q + 2])
                    break;
                count++;
            }

            pgc_rangebuf[p++] = (byte)(0x7F + count);
            while (count--)
                pgc_rangebuf[p++] = line[x++];
        }
    }

    PGC_WriteBuf(pgc_rangebuf, p);
}

//
// PGC_UploadLine
// Diff one backbuffer line against its shadow copy and upload the
// changed parts. x always stays word aligned, so the comparison is
// a 16 bit compare; the PGC side does not care about pixel parity.
// Changed words are grouped into segments: an unchanged gap of up
// to PGC_MERGE_GAP words inside a segment is resent with its
// surroundings, a longer gap splits the segment.
//
static void PGC_UploadLine(byte *src, byte *shadow, int screenrow)
{
    int x = 0;

    while (x < SCREENWIDTH)
    {
        int segstart;
        int segend;

        // Skip unchanged pixels.
        while (x < SCREENWIDTH &&
               *(unsigned short *)(src + x) == *(unsigned short *)(shadow + x))
            x += 2;
        if (x >= SCREENWIDTH)
            return;

        segstart = x & ~1;

        // Grow the segment over changed words, absorbing small gaps.
        for (;;)
        {
            int gap;

            while (x < SCREENWIDTH &&
                   *(unsigned short *)(src + x) != *(unsigned short *)(shadow + x))
                x += 2;
            segend = x;
            if (x >= SCREENWIDTH)
                break;

            // Measure the unchanged gap, but only up to the merge
            // limit: beyond that the segment is closed here and the
            // outer skip loop walks the rest of the gap.
            gap = 0;
            while (gap < PGC_MERGE_GAP &&
                   x + gap * 2 < SCREENWIDTH &&
                   *(unsigned short *)(src + x + gap * 2) ==
                   *(unsigned short *)(shadow + x + gap * 2))
                gap++;
            if (gap == PGC_MERGE_GAP)
                break;

            // A changed word lies within the merge limit: absorb the
            // gap and keep growing, or the gap ran to the end of the
            // line and the segment is done.
            x += gap * 2;
            if (x >= SCREENWIDTH)
                break;
        }

        PGC_WriteRange(src, screenrow, segstart, segend - 1);
        memcpy(shadow + segstart, src + segstart, segend - segstart);
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
        I_Error(10);

    pgc_present = 1;

    // Switch to the native 640x480 display and hex command mode.
    PGC_WriteAscii("DI 0\n");
    PGC_WriteAscii("CX\n");
    PGC_FlushOutput();
    PGC_FlushErrors();

    // Clear the framebuffer to black.
    PGC_WriteByte(PGC_CMD_CLEARS);
    PGC_WriteByte(0);

    PGC_FlushOutput();
    PGC_FlushErrors();

    // The host has no framebuffer of its own, point pcscreen at
    // the backbuffer so any code touching it does not crash.
    pcscreen = backbuffer;

    // The demo screens never clear the backbuffer themselves and
    // nothing on the PGC path does either, so start from a known
    // clean slate to keep the first frame deterministic. The shadow
    // copy must agree with what the card holds (black) from the
    // start, or the first frame reuploads the whole screen.
    memset(backbuffer, 0, SCREENWIDTH * SCREENHEIGHT);
    memcpy(pgc_shadow, backbuffer, SCREENWIDTH * SCREENHEIGHT);
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

    if (!pgc_present)
        return;

    if (numpalette < 0 || numpalette > 13)
        return;

    pal = &pgc_palette[numpalette * 256];
    for (i = 0; i < 256; i++)
    {
        PGC_WriteByte(PGC_CMD_LUT);
        PGC_WriteByte((byte)i);
        PGC_WriteByte((byte)((pal[i] >> 8) & 0x0F));
        PGC_WriteByte((byte)((pal[i] >> 4) & 0x0F));
        PGC_WriteByte((byte)(pal[i] & 0x0F));
    }

    PGC_FlushErrors();
}

//
// PGC_UploadRegion
// Differential upload of the backbuffer lines in [first, last)
// (line numbers) to the PGC.
//
static void PGC_UploadRegion(int first, int last)
{
    int y;

    for (y = first; y < last; y++)
    {
        PGC_UploadLine(backbuffer + (unsigned int)y * SCREENWIDTH,
                       pgc_shadow + (unsigned int)y * SCREENWIDTH, y);
    }
}

//
// I_FinishUpdate
// Upload the scanlines of the backbuffer that changed since the
// last frame to the PGC framebuffer. Only the areas the renderer
// marked dirty in updatestate are scanned, exactly like
// I_FinishUpdateDifferential386: a flagged area is diffed against
// the shadow line by line, so flagging costs no card traffic for
// the pixels that did not actually change.
//
void I_FinishUpdate(void)
{
    if (!pgc_present)
        return;

    if (updatestate == I_NOUPDATE)
        return;

    if (updatestate & I_FULLSCRN)
    {
        PGC_UploadRegion(0, SCREENHEIGHT);
        updatestate = I_NOUPDATE; // clear out all draw types
    }
    if (updatestate & I_FULLVIEW)
    {
        if (updatestate & I_MESSAGES && screenblocks > 7)
        {
            // The view starts at the top of the screen and the
            // messages overlap it: upload both in one sweep.
            PGC_UploadRegion(0, endscreen / SCREENWIDTH);
            updatestate &= ~(I_FULLVIEW | I_MESSAGES);
        }
        else
        {
            PGC_UploadRegion(startscreen / SCREENWIDTH, endscreen / SCREENWIDTH);
            updatestate &= ~I_FULLVIEW;
        }
    }
    if (updatestate & I_STATBAR)
    {
        PGC_UploadRegion(SCREENHEIGHT - SBARHEIGHT, SCREENHEIGHT);
        updatestate &= ~I_STATBAR;
    }
    if (updatestate & I_MESSAGES)
    {
        // 14 message lines times the 2x vertical scaling.
        PGC_UploadRegion(0, 28);
        updatestate &= ~I_MESSAGES;
    }

    // Keep the PGC output and error rings drained so the card
    // can never block on a full ring buffer.
    PGC_FlushOutput();
    PGC_FlushErrors();
    updatestate = I_NOUPDATE;
}

#endif
