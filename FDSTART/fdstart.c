/*
 * FDSTART.C - FastDoom text mode launcher
 *
 * C89 port. Lets the user pick any FastDoom executable, grouped by
 * video card type (VGA, VESA/VBE 2.0, EGA, Hercules, CGA, text/MDA,
 * other cards), or run FDSETUP or the benchmark launcher.
 *
 * Pure text mode: only stdio, conio and BIOS video services are
 * used, no direct video memory access, so it works on
 * MDA/Hercules cards as well as on more advanced ones.
 *
 * This file only holds the entry point; the rest of the launcher
 * is split over several modules, each with its own header:
 *
 *   screen.h/.c   screen clearing and drawing helpers
 *   util.h/.c     small helpers (file exists, cursor movement, ...)
 *   keys.h        keyboard scan codes
 *   groups.h/.c   the groups of launchable executables
 *   menu.h/.c     the group and main menus, program launching
 *   options.h/.c  command line options, FDSTART.CFG, options menu
 *   readme.h/.c   the README.TXT reader
 *   warp.h/.c     the single level launcher (-iwad, -warp, -skill)
 *   bench.h/.c    the benchmark launcher (-benchmark file/single)
 *
 * Build as a 16-bit DOS executable with Open Watcom (see makefile).
 */

#include <stdio.h>

#include "menu.h"
#include "screen.h"

int main(void)
{
    main_menu();
    clear_screen();
    printf("\n RIP AND TEAR\n\n");
    return 0;
}
