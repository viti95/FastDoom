//
// Borland conio.h shim for Open Watcom
// Implements textcolor/textbackground/clrscr/gotoxy via BIOS INT 10h
//
#ifndef BORLAND_SHIM_H
#define BORLAND_SHIM_H

#include <i86.h>

static unsigned char _bshim_attr = 0x07; // default: light grey on black

static void textcolor(int fg)
{
    _bshim_attr = (_bshim_attr & 0xF0) | (fg & 0x0F);
}

static void textbackground(int bg)
{
    _bshim_attr = (_bshim_attr & 0x0F) | ((bg & 0x07) << 4);
}

static void gotoxy(int x, int y)
{
    union REGS r;
    r.h.ah = 0x02;
    r.h.bh = 0x00;
    r.h.dh = (unsigned char)(y - 1);
    r.h.dl = (unsigned char)(x - 1);
    int86(0x10, &r, &r);
}

static void clrscr(void)
{
    union REGS r;
    r.h.ah = 0x06;
    r.h.al = 0x00;
    r.h.bh = _bshim_attr;
    r.h.ch = 0x00;
    r.h.cl = 0x00;
    r.h.dh = 24;
    r.h.dl = 79;
    int86(0x10, &r, &r);
    gotoxy(1, 1);
}

#endif
