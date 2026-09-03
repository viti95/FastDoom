#ifndef __OPTIONS__
#define __OPTIONS__

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_CGA) || defined(MODE_CVB) || defined(MODE_CGA_BW) || defined(MODE_PCP) || defined(MODE_EGA)
#define SUPPORTS_HERCULES_AUTOMAP
#endif

#if defined(MODE_13H) || defined(MODE_CGA_BW) || defined(MODE_CGA16) || defined(MODE_CGA) || defined(MODE_CVB) || defined(MODE_HERC) || defined(MODE_INCOLOR) || defined(MODE_PCP) || defined(MODE_VBE2) || defined(MODE_EGA) || defined(MODE_CGA_AFH) || defined(MODE_CGA512) || defined(MODE_SIGMA)
#define USE_BACKBUFFER
#endif

#if defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T8025) || defined(MODE_T8043) || defined(MODE_T8050) || defined(MODE_COLOR_MDA)
#define TEXT_MODE
#endif

//
// Human-readable name of the video mode this executable was built with,
// shown in the startup banner. SCREENWIDTH/SCREENHEIGHT must be defined
// (see doomdef.h).
//
#define FD_XSTR2(s) #s
#define FD_XSTR(s) FD_XSTR2(s)
#define FD_RES FD_XSTR(SCREENWIDTH) "x" FD_XSTR(SCREENHEIGHT)

//
// Wording follows the executable descriptions in FDSTART/groups.c
// (except for the VBE builds, whose resolution varies and is taken
// from SCREENWIDTH/SCREENHEIGHT).
//
#if defined(MODE_Y)
#define FD_MODE_NAME "VGA 320x200 256 colors"
#elif defined(MODE_X)
#define FD_MODE_NAME "VGA mode X 320x240 256 colors"
#elif defined(MODE_Y_HALF)
#define FD_MODE_NAME "VGA mode Y 320x100 256 colors"
#elif defined(MODE_13H)
#define FD_MODE_NAME "VGA mode 13h 320x200 256 colors"
#elif defined(MODE_VBE2)
#define FD_MODE_NAME "VESA " FD_RES " backbuffered"
#elif defined(MODE_VBE2_DIRECT)
#define FD_MODE_NAME "VESA " FD_RES " direct rendering"
#elif defined(MODE_EGA)
#define FD_MODE_NAME "EGA 320x200 16 colors"
#elif defined(MODE_HERC)
#define FD_MODE_NAME "Hercules 640x400 monochrome (320x200)"
#elif defined(MODE_INCOLOR)
#define FD_MODE_NAME "Hercules InColor 320x200 16 colors"
#elif defined(MODE_CGA)
#define FD_MODE_NAME "CGA 320x200 4 colors"
#elif defined(MODE_CGA_BW)
#define FD_MODE_NAME "CGA 640x200 monochrome"
#elif defined(MODE_CGA16)
#define FD_MODE_NAME "CGA 160x100 16 colors"
#elif defined(MODE_CVB)
#define FD_MODE_NAME "CGA composite 160x200 16 colors"
#elif defined(MODE_CGA512)
#define FD_MODE_NAME "CGA composite 80x100 512 colors"
#elif defined(MODE_CGA_AFH)
#define FD_MODE_NAME "CGA ANSI from Hell 320x100 16 colors"
#elif defined(MODE_MDA)
#define FD_MODE_NAME "MDA 80x25 text monochrome"
#elif defined(MODE_COLOR_MDA)
#define FD_MODE_NAME "MDA 80x25 text color (IBM rev 0 cards)"
#elif defined(MODE_T4025)
#define FD_MODE_NAME "40x25 text 16 colors"
#elif defined(MODE_T4050)
#define FD_MODE_NAME "40x25 text 16 colors (40x50 virtual)"
#elif defined(MODE_T8025)
#define FD_MODE_NAME "80x25 text 16 colors (80x50 virtual)"
#elif defined(MODE_T8043)
#define FD_MODE_NAME "80x43 text 16 colors (EGA cards)"
#elif defined(MODE_T8050)
#define FD_MODE_NAME "80x50 text 16 colors"
#elif defined(MODE_VT100)
#define FD_MODE_NAME "VT100 serial terminal 80x24"
#elif defined(MODE_PCP)
#define FD_MODE_NAME "Plantronics ColorPlus 320x200 16 colors"
#elif defined(MODE_SIGMA)
#define FD_MODE_NAME "Sigma Color 400 320x200 16 colors"
#else
#define FD_MODE_NAME "Unknown"
#endif

#endif
