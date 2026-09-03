/*
 * GROUPS.C - FastDoom text mode launcher, the executable groups
 *
 * The list of launchable FastDoom programs, grouped by video card
 * type.
 */

#include "groups.h"

/* VGA */
static const item_t vga_items[] = {
    { "Mode Y 320x200 256 colors (default)",   "FDOOM.EXE" },
    { "Mode X 320x240 256 colors",             "FDOOMX.EXE" },
    { "Mode Y half height 320x100 256 colors", "FDOOMH.EXE" },
    { "Mode 13h 320x200 256 colors",           "FDOOM13H.EXE" }
};

/* VESA / VBE 2.0, backbuffered (real mode) */
static const item_t vesa_r_items[] = {
    { "VESA 320x200 backbuffered",            "FDOOMVBR.EXE" },
    { "VESA 320x240 backbuffered",            "FDM240R.EXE" },
    { "VESA 400x300 backbuffered",            "FDM300R.EXE" },
    { "VESA 512x384 backbuffered",            "FDM384R.EXE" },
    { "VESA 640x400 backbuffered",            "FDM400R.EXE" },
    { "VESA 640x480 backbuffered",            "FDM480R.EXE" },
    { "VESA 800x600 backbuffered",            "FDM600R.EXE" },
    { "VESA 1024x768 backbuffered",           "FDM768R.EXE" },
    { "VESA 1280x800 backbuffered",           "FDM800R.EXE" },
    { "VESA 1280x1024 backbuffered",          "FDM1024R.EXE" },
    { "VESA 1600x1200 backbuffered",          "FDM1200R.EXE" }
};

/* VESA / VBE 2.0, direct rendering (LFB) */
static const item_t vesa_d_items[] = {
    { "VESA 320x200 direct rendering",        "FDOOMVBD.EXE" },
    { "VESA 320x240 direct rendering",        "FDM240D.EXE" },
    { "VESA 400x300 direct rendering",        "FDM300D.EXE" },
    { "VESA 512x384 direct rendering",        "FDM384D.EXE" },
    { "VESA 640x400 direct rendering",        "FDM400D.EXE" },
    { "VESA 640x480 direct rendering",        "FDM480D.EXE" },
    { "VESA 800x600 direct rendering",        "FDM600D.EXE" },
    { "VESA 1024x768 direct rendering",       "FDM768D.EXE" },
    { "VESA 1280x800 direct rendering",       "FDM800D.EXE" },
    { "VESA 1280x1024 direct rendering",      "FDM1024D.EXE" },
    { "VESA 1600x1200 direct rendering",      "FDM1200D.EXE" }
};

/* EGA */
static const item_t ega_items[] = {
    { "EGA 320x200 16 colors",                "FDOOMEGA.EXE" }
};

/* Hercules */
static const item_t herc_items[] = {
    { "Hercules 640x400 monochrome",          "FDOOMHGC.EXE" },
    { "Hercules InColor 320x200 16 colors",   "FDOOMINC.EXE" }
};

/* CGA */
static const item_t cga_items[] = {
    { "CGA 320x200 4 colors",                 "FDOOMCGA.EXE" },
    { "CGA 640x200 monochrome",               "FDOOMBWC.EXE" },
    { "CGA 160x100 16 colors",                "FDOOMC16.EXE" },
    { "CGA composite 160x200 16 colors",      "FDOOMCVB.EXE" },
    { "CGA composite 80x100 512 colors",       "FDOOM512.EXE" },
    { "CGA ANSI from Hell 320x100 16 colors", "FDOOMCAH.EXE" }
};

/* Text mode / MDA */
static const item_t text_items[] = {
    { "MDA 80x25 text monochrome",              "FDOOMMDA.EXE" },
    { "MDA 80x25 text color (IBM rev 0 cards)", "FDOOMCDA.EXE" },
    { "40x25 text 16 colors",                   "FDOOMT1.EXE" },
    { "40x25 text 16 colors (40x50 virtual)",   "FDOOMT12.EXE" },
    { "80x25 text 16 colors (80x50 virtual)",   "FDOOMT25.EXE" },
    { "80x43 text 16 colors (EGA cards)",       "FDOOMT43.EXE" },
    { "80x50 text 16 colors",                   "FDOOMT50.EXE" },
    { "VT100 serial terminal 80x24",            "FDMVT100.EXE" }
};

/* Specials */
static const item_t other_items[] = {
    { "Plantronics ColorPlus 320x200 16 colors", "FDOOMPCP.EXE" },
    { "Sigma Color 400 320x200 16 colors",       "FDOOM400.EXE" }
};

const group_t groups[NGROUPS] = {
    { "Text mode / MDA",      text_items,  (int)sizeof(text_items) / sizeof(text_items[0]) },
    { "Hercules",             herc_items,  (int)sizeof(herc_items) / sizeof(herc_items[0]) },
    { "CGA",                  cga_items,   (int)sizeof(cga_items) / sizeof(cga_items[0]) },
    { "EGA",                  ega_items,   (int)sizeof(ega_items) / sizeof(ega_items[0]) },
    { "VGA",                  vga_items,   (int)sizeof(vga_items) / sizeof(vga_items[0]) },
    { "VESA VBE 2.0, backbuffered", vesa_r_items, (int)sizeof(vesa_r_items) / sizeof(vesa_r_items[0]) },
    { "VESA VBE 2.0, direct rendering", vesa_d_items, (int)sizeof(vesa_d_items) / sizeof(vesa_d_items[0]) },
    { "Specials",             other_items, (int)sizeof(other_items) / sizeof(other_items[0]) }
};
