/*
 * Copyright (C) 2026 FastDoom contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *   IBM Micro Channel XGA video support, 640x480 with 256 colors,
 *   rendered 1:1 (no scaling) into a backbuffered frame.
 *
 *   The XGA is a self contained MCA coprocessor card. The host CPU
 *   reaches it two ways:
 *
 *   - A bank of I/O registers at 21x0-21x0F (x is the card instance,
 *     0-7, read from the POS record). The low ports 21x0-21x9 are the
 *     "DCR" display controller registers (operating mode, aperture
 *     control, interrupt enable/status, VM control, aperture index,
 *     memory access mode) and are written directly.
 *
 *   - An index/data pair at 21xA (index) / 21xB (data) that reaches
 *     the CRTC (10-2D), the display pixel map (40-44), the display
 *     control / clock / border registers (50-55, 70) and the palette
 *     (60-66).
 *
 *   In extended graphics mode the card maps a 64K window of its VRAM
 *   into system address space at A0000 (aperture control = 1). The
 *   window is paged by the aperture index register (21x8), 64K pages.
 *   The 640x480x8 framebuffer is 307200 bytes, so it spans pages 0-4.
 *   I_FinishUpdate walks the dirty scanlines, switches the aperture
 *   page as needed and memcpys the changed segments straight through
 *   A0000, differential against a shadow copy like the PGC driver.
 *
 *   The card is found by scanning the MCA POS records (INT 15h C4h)
 *   for the XGA product id (0x8FD8-0x8FDB). The 256 color palette is
 *   six bits per channel: I_ProcessPalette pre-converts the gamma
 *   table values and I_SetPalette streams them (R G B, auto
 *   incrementing index) into the palette data register.
 *
 *   The register sequence for the mode follows the IBM XGA Software
 *   Programmer's Guide (11.1.1) and matches the working driver from
 *   id Software's XGA DOOM, except the 64K A0000 aperture is used for
 *   the upload (instead of id's coprocessor virtual memory aperture)
 *   and the display runs 1:1 at 640x480 (no 2x scaling).
 */

#include <string.h>
#include <dos.h>
#include <stdlib.h>

#include "doomtype.h"
#include "i_ibm.h"
#include "v_video.h"
#include "i_system.h"
#include "doomstat.h"
#include "m_menu.h"
#include "i_gamma.h"
#include "i_debug.h"
#include "i_xga.h"

#if defined(MODE_XGA)

// The host has no framebuffer of its own: the renderer draws into the
// backbuffer and the card VRAM is filled by I_FinishUpdate. pcscreen is
// only referenced by code paths that assume a live host framebuffer, so
// declare it here (r_defs.h) rather than pull the whole renderer header
// chain into a video driver.
extern byte *pcscreen;

//
// XGA I/O and memory layout
//

// 64K system aperture, mapped to A0000 by aperture control = 1.
#define XGA_VRAM_ADDR		0xA0000
#define XGA_PAGE_SHIFT		16
#define XGA_PAGE_SIZE		(1u << XGA_PAGE_SHIFT)

// Framebuffer size and the number of 64K pages it spans.
#define XGA_FB_SIZE		(SCREENWIDTH * SCREENHEIGHT)
#define XGA_NUM_PAGES		((XGA_FB_SIZE + XGA_PAGE_SIZE - 1) >> XGA_PAGE_SHIFT)

// DCR (direct) register offsets within 21x0-21x9.
#define XGA_DCR_OPER_MODE	0	// 21x0: operating mode
#define XGA_DCR_APER_CTRL	1	// 21x1: aperture control
#define XGA_DCR_INT_ENA		4	// 21x4: interrupt enable
#define XGA_DCR_INT_STAT	5	// 21x5: interrupt status
#define XGA_DCR_VM_CTRL		6	// 21x6: coprocessor VM control
#define XGA_DCR_APER_IDX	8	// 21x8: aperture index (64K page)
#define XGA_DCR_MEM_ACCESS	9	// 21x9: memory access mode

// Index/data register numbers (reached through 21xA / 21xB).
#define XGA_IDX_HTOTAL_LO	0x10
#define XGA_IDX_HDISP_LO	0x12
#define XGA_IDX_HBLANK_LO	0x14
#define XGA_IDX_HSYNC_LO	0x18
#define XGA_IDX_HSYNC_POS_A	0x1C
#define XGA_IDX_HSYNC_POS_B	0x1E
#define XGA_IDX_VTOTAL_LO	0x20
#define XGA_IDX_VDISP_LO	0x22
#define XGA_IDX_VBLANK_LO	0x24
#define XGA_IDX_VSYNC_LO	0x28
#define XGA_IDX_VSYNC_END	0x2A
#define XGA_IDX_VLINE_LO	0x2C
#define XGA_IDX_SPRITE_CTRL	0x36
#define XGA_IDX_DPM_OFF_LO	0x40
#define XGA_IDX_DPM_PITCH_LO	0x43
#define XGA_IDX_DPM_PITCH_HI	0x44
#define XGA_IDX_DISPCNTL_1	0x50
#define XGA_IDX_DISPCNTL_2	0x51
#define XGA_IDX_CLK_SEL		0x54
#define XGA_IDX_BORDER		0x55
#define XGA_IDX_PAL_IDX_HI	0x61
#define XGA_IDX_PAL_IDX_LO	0x60
#define XGA_IDX_PAL_MASK	0x64
#define XGA_IDX_PAL_DATA	0x65
#define XGA_IDX_PAL_SEQ		0x66
#define XGA_IDX_XTRN_CLK	0x70

// Operating mode values (21x0).
#define XGA_MODE_VGA		0x01
#define XGA_MODE_EXT_GRAPH	0x04

// Aperture control values (21x1): 64K window location.
#define XGA_APER_NONE		0x00
#define XGA_APER_A0000		0x01

// Memory access mode (21x9): 8 bit pixels, Intel (LSB) order.
#define XGA_ACCESS_8BIT		0x03

// XGA product id range in the MCA POS records.
#define XGA_POS_ID_MIN		0x8FD8
#define XGA_POS_ID_MAX		0x8FDB

// Unchanged gap of this many pixels between two changed areas on a
// line is resent with the surrounding segment instead of ending it:
// a fresh 64K page switch or a longer run is more expensive than
// retransmitting up to one chunk of pixels.
#define XGA_MERGE_GAP		256

//
// Palette scaling
//
// The XGA palette data register is eight bits wide per channel and the
// card uses the top six bits, so a six bit gamma value (0-63) is
// expanded to the full eight bit range by replicating its top two bits
// into the low two. This is effectively what id Software's XGA DOOM
// does (its gamma table is already eight bit, 0-255). Writing the six
// bit value raw would cap the brightest color at 63/255 and look very
// dark.
#define XGA_PAL8(v)	((byte)((((v) & 0x3F) << 2) | (((v) & 0x3F) >> 4)))

//
// State
//

static int xga_active = 0;
static int xga_io_base = 0;
static int xga_instance = 0;

// Current 64K aperture page (avoid redundant 21x8 writes).
static int xga_cur_page = -1;

// Shadow copy of the backbuffer: what the card framebuffer currently
// holds. I_FinishUpdate diffs the backbuffer against this and only
// uploads what changed.
static byte xga_shadow[SCREENWIDTH * SCREENHEIGHT];

// XGA palettes: 14 palettes, 256 entries of three eight bit channels
// (six bit gamma values expanded to the full range, see XGA_PAL8).
static byte xga_palette[14 * 256 * 3];

// Force a full screen upload on the very first frame so any content
// the card already showed is overwritten, matching the black shadow.
static byte xga_firstframe = 1;

//
// Log helpers
//
// I_Printf renders %d/%i as a fixed point value ("123.0000"), which is
// confusing for register dumps, so plain integers and hex values go
// through these helpers and I_Puts.

static void XGA_LogInt(long v)
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

static void XGA_LogHex(long v, int digits)
{
    char buf[17];
    int i;

    for (i = digits - 1; i >= 0; i--)
    {
        int n = (int)((v >> (i * 4)) & 0x0F);

        buf[digits - 1 - i] = (char)(n < 10 ? '0' + n : 'a' + n - 10);
    }
    buf[digits] = 0;
    I_Puts(buf);
}

//
// Register access
//

// Write a DCR register (21x0-21x9) directly.
static void XGA_WriteDCR(int reg, int val)
{
    outp((unsigned short)(xga_io_base + reg), (unsigned char)val);
}

// Write an index/data register (21xA index, 21xB data).
static void XGA_WriteReg(int idx, int val)
{
    outp((unsigned short)(xga_io_base + 0x0A), (unsigned char)idx);
    outp((unsigned short)(xga_io_base + 0x0B), (unsigned char)val);
}

//
// Detection
//

//
// XGA_Detect
// Scan the MCA POS records for an XGA coprocessor and work out its I/O
// base from the instance field. Returns 1 on success.
//
static int XGA_Detect(void)
{
    union REGS regs;
    unsigned int pos_base;
    int slot;

    // POS base address: INT 15h AH=C4 AL=00. CF set means no MCA/POS.
    regs.h.ah = 0xC4;
    regs.h.al = 0x00;
    int386(0x15, &regs, &regs);
    if (regs.x.cflag)
    {
        I_Printf("XGA: no POS support (not a Micro Channel system)\n");
        return 0;
    }
    pos_base = (unsigned int)regs.w.dx;
    I_Printf("XGA: POS base at 0x");
    XGA_LogHex(pos_base, 4);
    I_Puts("\n");

    for (slot = 0; slot <= 9; slot++)
    {
        unsigned int pos_id;
        int pos_reg2;

        // Put the slot in the NBSL (new board setup list) so its POS
        // record is selectable. Slot 0 (the system board) is toggled
        // through port 94h, the rest through INT 15h C4h.
        if (slot == 0)
        {
            outp(0x094, 0xDF);
        }
        else
        {
            regs.w.ax = 0xC401;
            regs.w.bx = (unsigned short)slot;
            int386(0x15, &regs, &regs);
        }

        pos_id = inpw((unsigned short)pos_base);
        pos_reg2 = inp((unsigned short)(pos_base + 2));

        // Restore the NBSL.
        if (slot == 0)
        {
            outp(0x094, 0xFF);
        }
        else
        {
            regs.w.ax = 0xC402;
            regs.w.bx = (unsigned short)slot;
            int386(0x15, &regs, &regs);
        }

        if (pos_id >= XGA_POS_ID_MIN && pos_id <= XGA_POS_ID_MAX)
        {
            xga_instance = (pos_reg2 >> 1) & 7;
            xga_io_base = 0x2100 + xga_instance * 0x10;
            I_Printf("XGA: found in MCA slot ");
            XGA_LogInt(slot);
            I_Printf(", instance ");
            XGA_LogInt(xga_instance);
            I_Printf(", I/O base 0x");
            XGA_LogHex(xga_io_base, 4);
            I_Puts("\n");
            return 1;
        }
    }

    I_Printf("XGA: no XGA adapter found in MCA slots 0-9\n");
    return 0;
}

//
// Mode setup
//

//
// XGA_SetMode
// Program the card for 640x480x256 extended graphics, 1:1, with the
// 64K A0000 aperture enabled for the framebuffer upload. The sequence
// is the 11.1.1 mode table from the IBM XGA Software Programmer's
// Guide (matching id's XGA DOOM driver), with aperture control set to
// A0000 and the display control 2 left at 03h (no 2x scaling).
//
static void XGA_SetMode(void)
{
    // Silence the card and blank the palette so no garbage flashes.
    XGA_WriteDCR(XGA_DCR_INT_ENA, 0x00);
    XGA_WriteDCR(XGA_DCR_INT_STAT, 0xFF);
    XGA_WriteReg(XGA_IDX_PAL_MASK, 0x00);

    // Extended graphics mode, 64K aperture at A0000, page 0, no
    // coprocessor VM, 8 bit pixels.
    XGA_WriteDCR(XGA_DCR_OPER_MODE, XGA_MODE_EXT_GRAPH);
    XGA_WriteDCR(XGA_DCR_APER_CTRL, XGA_APER_A0000);
    XGA_WriteDCR(XGA_DCR_APER_IDX, 0x00);
    XGA_WriteDCR(XGA_DCR_VM_CTRL, 0x00);
    XGA_WriteDCR(XGA_DCR_MEM_ACCESS, XGA_ACCESS_8BIT);
    xga_cur_page = 0;

    // Reset the CRTC (display control 1: prepare for reset, then reset).
    XGA_WriteReg(XGA_IDX_DISPCNTL_1, 0x01);
    XGA_WriteReg(XGA_IDX_DISPCNTL_1, 0x00);

    // Horizontal timing. The XGA display registers count 8 pixel
    // groups, so hdisp 4Fh + 1 = 80 groups = 640 pixels displayed and
    // htotal 63h gives the full 800 pixel line. These are the working
    // values from the 11.1.1 mode table.
    XGA_WriteReg(XGA_IDX_HTOTAL_LO, 0x63);
    XGA_WriteReg(XGA_IDX_HTOTAL_LO + 1, 0x00);
    XGA_WriteReg(XGA_IDX_HDISP_LO, 0x4F);
    XGA_WriteReg(XGA_IDX_HDISP_LO + 1, 0x00);
    XGA_WriteReg(XGA_IDX_HBLANK_LO, 0x4F);
    XGA_WriteReg(XGA_IDX_HBLANK_LO + 1, 0x00);
    XGA_WriteReg(XGA_IDX_HBLANK_LO + 2, 0x63);
    XGA_WriteReg(XGA_IDX_HBLANK_LO + 3, 0x00);
    XGA_WriteReg(XGA_IDX_HSYNC_LO, 0x55);
    XGA_WriteReg(XGA_IDX_HSYNC_LO + 1, 0x00);
    XGA_WriteReg(XGA_IDX_HSYNC_LO + 2, 0x61);
    XGA_WriteReg(XGA_IDX_HSYNC_LO + 3, 0x00);
    XGA_WriteReg(XGA_IDX_HSYNC_POS_A, 0x00);
    XGA_WriteReg(XGA_IDX_HSYNC_POS_B, 0x00);

    // Vertical timing: 480 displayed of a 524 line frame.
    XGA_WriteReg(XGA_IDX_VTOTAL_LO, 0x0C);
    XGA_WriteReg(XGA_IDX_VTOTAL_LO + 1, 0x02);
    XGA_WriteReg(XGA_IDX_VDISP_LO, 0xDF);
    XGA_WriteReg(XGA_IDX_VDISP_LO + 1, 0x01);
    XGA_WriteReg(XGA_IDX_VBLANK_LO, 0xDF);
    XGA_WriteReg(XGA_IDX_VBLANK_LO + 1, 0x01);
    XGA_WriteReg(XGA_IDX_VBLANK_LO + 2, 0x0C);
    XGA_WriteReg(XGA_IDX_VBLANK_LO + 3, 0x02);
    XGA_WriteReg(XGA_IDX_VSYNC_LO, 0xEA);
    XGA_WriteReg(XGA_IDX_VSYNC_LO + 1, 0x01);
    XGA_WriteReg(XGA_IDX_VSYNC_END, 0xEC);
    XGA_WriteReg(XGA_IDX_VLINE_LO, 0xFF);
    XGA_WriteReg(XGA_IDX_VLINE_LO + 1, 0xFF);

    // Sprite off.
    XGA_WriteReg(XGA_IDX_SPRITE_CTRL, 0x00);

    // Display pixel map: start at VRAM 0, line pitch 640 bytes
    // (the pitch register counts 8 byte groups: 50h=80 -> 640).
    XGA_WriteReg(XGA_IDX_DPM_OFF_LO, 0x00);
    XGA_WriteReg(XGA_IDX_DPM_OFF_LO + 1, 0x00);
    XGA_WriteReg(XGA_IDX_DPM_OFF_LO + 2, 0x00);
    XGA_WriteReg(XGA_IDX_DPM_PITCH_LO, 0x50);
    XGA_WriteReg(XGA_IDX_DPM_PITCH_HI, 0x00);

    // 640x480 clock, 8 bit pixels, no scaling (display on).
    XGA_WriteReg(XGA_IDX_CLK_SEL, 0x00);
    XGA_WriteReg(XGA_IDX_DISPCNTL_2, 0x03);
    XGA_WriteReg(XGA_IDX_XTRN_CLK, 0x00);

    // Start the CRTC (display control 1: normal operation).
    XGA_WriteReg(XGA_IDX_DISPCNTL_1, 0xC7);
}

//
// XGA_ClearVRAM
// Fill the whole framebuffer (and the rest of the pages it spans) with
// pixel 0, paging the 64K window through the aperture index.
//
static void XGA_ClearVRAM(void)
{
    byte *vram = (byte *)XGA_VRAM_ADDR;
    int page;

    for (page = 0; page < XGA_NUM_PAGES; page++)
    {
        XGA_WriteDCR(XGA_DCR_APER_IDX, page);
        xga_cur_page = page;
        memset(vram, 0, XGA_PAGE_SIZE);
    }
}

//
// Sanity check the aperture: write a pattern through the last page and
// read it back. Catches a card that is not actually routing A0000 to
// VRAM (e.g. wrong instance or the aperture left disabled).
//
static int XGA_TestAperture(void)
{
    byte *vram = (byte *)XGA_VRAM_ADDR;
    int last_page = XGA_NUM_PAGES - 1;
    int offset = (XGA_FB_SIZE - 1) & (XGA_PAGE_SIZE - 1);
    int i;
    byte pattern = 0xA5;

    XGA_WriteDCR(XGA_DCR_APER_IDX, last_page);
    xga_cur_page = last_page;

    for (i = 0; i < 16; i++)
        vram[offset - 8 + i] = (byte)(pattern ^ (i & 1));

    for (i = 0; i < 16; i++)
    {
        if (vram[offset - 8 + i] != (byte)(pattern ^ (i & 1)))
        {
            I_Printf("XGA: aperture readback failed at page ");
            XGA_LogInt(last_page);
            I_Puts("\n");
            return 0;
        }
    }
    return 1;
}

//
// Palette
//

//
// I_ProcessPalette
// Takes the whole PLAYPAL lump (14 palettes, 256 entries, RGB). Each
// entry is the six bit gamma table value expanded to the full eight bit
// range the XGA palette register uses (top six bits of the byte).
//
void I_ProcessPalette(byte *palette)
{
    int i;
    byte *ptr = gammatable;

    for (i = 0; i < 14 * 256; i++, palette += 3)
    {
        xga_palette[i * 3] = XGA_PAL8(ptr[*(palette)]);
        xga_palette[i * 3 + 1] = XGA_PAL8(ptr[*(palette + 1)]);
        xga_palette[i * 3 + 2] = XGA_PAL8(ptr[*(palette + 2)]);
    }
}

//
// I_SetPalette
// Stream the 256 entries of the requested palette into the card. The
// palette sequence register selects R G B order, the index is set to 0
// and the index auto increments as each three byte group is loaded, so
// the load is a run of 768 writes to the palette data register.
//
void I_SetPalette(int numpalette)
{
    const byte *pal;
    int i;

    if (!xga_active)
        return;

    if (numpalette < 0 || numpalette > 13)
        return;

    pal = &xga_palette[numpalette * 768];

    XGA_WriteReg(XGA_IDX_PAL_SEQ, 0x00);	// R G B order
    XGA_WriteReg(XGA_IDX_PAL_IDX_HI, 0x00);
    XGA_WriteReg(XGA_IDX_PAL_IDX_LO, 0x00);
    for (i = 0; i < 768; i++)
        XGA_WriteReg(XGA_IDX_PAL_DATA, pal[i]);

    // Unmask the palette now that it holds the requested colors.
    XGA_WriteReg(XGA_IDX_PAL_MASK, 0xFF);
}

//
// Upload
//

//
// XGA_SetPage
// Switch the 64K aperture to the given 64K page unless it is already
// selected.
//
static void XGA_SetPage(int page)
{
    if (page != xga_cur_page)
    {
        XGA_WriteDCR(XGA_DCR_APER_IDX, page);
        xga_cur_page = page;
    }
}

//
// XGA_UploadRange
// Copy one pixel run of a screen scanline (backbuffer to VRAM) through
// the 64K aperture, switching pages if the run sits in a different one.
//
static void XGA_UploadRange(int row, int col1, int col2)
{
    byte *vram = (byte *)XGA_VRAM_ADDR;
    unsigned int vram_off;
    int page;
    unsigned int off_in_page;
    int len;

    vram_off = (unsigned int)row * SCREENWIDTH + col1;
    page = (int)(vram_off >> XGA_PAGE_SHIFT);
    off_in_page = vram_off & (XGA_PAGE_SIZE - 1);
    len = col2 - col1 + 1;

    XGA_SetPage(page);
    memcpy(vram + off_in_page, backbuffer + vram_off, len);
}

//
// XGA_UploadLine
// Diff one backbuffer line against its shadow copy and upload the
// changed parts. x always stays word aligned, so the comparison is a
// 16 bit compare. Changed words are grouped into segments: an unchanged
// gap of up to XGA_MERGE_GAP pixels inside a segment is resent with its
// surroundings, a longer gap splits the segment.
//
static void XGA_UploadLine(int row)
{
    byte *src = backbuffer + (unsigned int)row * SCREENWIDTH;
    byte *shadow = xga_shadow + (unsigned int)row * SCREENWIDTH;
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
            while (gap < XGA_MERGE_GAP &&
                   x + gap < SCREENWIDTH &&
                   src[x + gap] == shadow[x + gap])
                gap++;
            if (gap == XGA_MERGE_GAP)
                break;

            // A changed pixel lies within the merge limit: absorb the
            // gap and keep growing, or the gap ran to the end of the
            // line and the segment is done.
            x += gap;
            if (x >= SCREENWIDTH)
                break;
        }

        XGA_UploadRange(row, segstart, segend - 1);
        memcpy(shadow + segstart, src + segstart, segend - segstart);
    }
}

//
// XGA_UploadRegion
// Differential upload of the backbuffer lines in [first, last) (line
// numbers).
//
static void XGA_UploadRegion(int first, int last)
{
    int y;

    for (y = first; y < last; y++)
        XGA_UploadLine(y);
}

//
// I_InitGraphics
//
void XGA_InitGraphics(void)
{
    if (!XGA_Detect())
    {
        I_Error(10);
    }

    XGA_SetMode();

    if (!XGA_TestAperture())
    {
        I_Error(10);
    }

    I_Printf("XGA: 640x480x256 extended graphics, 64K aperture at 0x");
    XGA_LogHex(XGA_VRAM_ADDR, 6);
    I_Puts("\n");

    // The host has no framebuffer of its own, point pcscreen at the
    // backbuffer so any code touching it does not crash.
    pcscreen = backbuffer;

    XGA_ClearVRAM();
    memset(backbuffer, 0, XGA_FB_SIZE);
    memcpy(xga_shadow, backbuffer, XGA_FB_SIZE);

    xga_active = 1;
    xga_firstframe = 1;
}

//
// I_ShutdownGraphics
//
// Restore the card to VGA mode so the host display is usable again
// after the game exits: release the 64K aperture, silence interrupts,
// turn the extended display off and switch the operating mode back to
// VGA (the caller then sets a text mode).
//
void XGA_ShutdownGraphics(void)
{
    if (!xga_active)
        return;

    // Release the aperture and blank before dropping out of extended
    // graphics so the host VGA memory hole is not aliased to VRAM.
    XGA_WriteDCR(XGA_DCR_APER_CTRL, XGA_APER_NONE);
    XGA_WriteDCR(XGA_DCR_INT_ENA, 0x00);
    XGA_WriteDCR(XGA_DCR_INT_STAT, 0xFF);

    XGA_WriteReg(XGA_IDX_PAL_MASK, 0xFF);
    XGA_WriteReg(XGA_IDX_DISPCNTL_1, 0x15);
    XGA_WriteReg(XGA_IDX_DISPCNTL_1, 0x14);
    XGA_WriteReg(XGA_IDX_DISPCNTL_2, 0x00);
    XGA_WriteReg(XGA_IDX_CLK_SEL, 0x04);
    XGA_WriteReg(XGA_IDX_XTRN_CLK, 0x00);
    XGA_WriteReg(XGA_IDX_VSYNC_END, 0x20);

    XGA_WriteDCR(XGA_DCR_OPER_MODE, XGA_MODE_VGA);

    xga_active = 0;
}

//
// I_FinishUpdate
//
// Upload the scanlines of the backbuffer that changed since the last
// frame to the video RAM. Only the areas the renderer marked dirty in
// updatestate are scanned, exactly like the PGC driver: a flagged area
// is diffed against the shadow line by line, so flagging costs no card
// traffic for the pixels that did not actually change. The very first
// frame always uploads the whole screen, to clear whatever the card
// showed before.
//
void I_FinishUpdate(void)
{
    if (!xga_active)
        return;

    if (xga_firstframe)
    {
        XGA_UploadRegion(0, SCREENHEIGHT);
        xga_firstframe = 0;
        updatestate = I_NOUPDATE;
        return;
    }

    if (updatestate == I_NOUPDATE)
        return;

    if (updatestate & I_FULLSCRN)
    {
        XGA_UploadRegion(0, SCREENHEIGHT);
        updatestate = I_NOUPDATE;
    }
    if (updatestate & I_FULLVIEW)
    {
        if (updatestate & I_MESSAGES && screenblocks > 7)
        {
            // The view starts at the top of the screen and the messages
            // overlap it: upload both in one sweep.
            XGA_UploadRegion(0, endscreen / SCREENWIDTH);
            updatestate &= ~(I_FULLVIEW | I_MESSAGES);
        }
        else
        {
            XGA_UploadRegion(startscreen / SCREENWIDTH, endscreen / SCREENWIDTH);
            updatestate &= ~I_FULLVIEW;
        }
    }
    if (updatestate & I_STATBAR)
    {
        XGA_UploadRegion(SCREENHEIGHT - SBARHEIGHT, SCREENHEIGHT);
        updatestate &= ~I_STATBAR;
    }
    if (updatestate & I_MESSAGES)
    {
        XGA_UploadRegion(0, 28);
        updatestate &= ~I_MESSAGES;
    }

    updatestate = I_NOUPDATE;
}

#endif
