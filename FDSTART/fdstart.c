/*
 * FDSTART.C - FastDoom text mode launcher
 *
 * C89 port. Lets the user pick any FastDoom executable, grouped by
 * video card type (VGA, VESA/VBE 2.0, EGA, Hercules, CGA, text/MDA,
 * other cards), or run FDSETUP and FDBENCH.
 *
 * Pure text mode: only stdio, conio and BIOS video services are
 * used, no direct video memory access, so it works on
 * MDA/Hercules cards as well as on more advanced ones.
 *
 * Build as a 16-bit DOS executable with Open Watcom (see makefile).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <i86.h>

/*
 * A single launchable program: a short description and the
 * executable file name.
 */
typedef struct {
    const char *desc;
    const char *exe;
} item_t;

/*
 * A menu group: a title and the list of items it contains.
 */
typedef struct {
    const char *title;
    const item_t *items;
    int count;
} group_t;

/* VGA */
static const item_t vga_items[] = {
    { "Mode Y, 320x200, 256 colors (default)", "FDOOM.EXE" },
    { "Mode X, 320x240, 256 colors",           "FDOOMX.EXE" },
    { "Mode Y half height, 320x100",           "FDOOMH.EXE" },
    { "Mode 13h, 320x200, 256 colors",         "FDOOM13H.EXE" }
};

/* VESA / VBE 2.0, backbuffered (real mode) */
static const item_t vesa_r_items[] = {
    { "VBE 2.0, real mode (most compatible)",  "FDOOMVBR.EXE" },
    { "VESA 320x240 backbuffered",             "FDM240R.EXE" },
    { "VESA 400x300 backbuffered",             "FDM300R.EXE" },
    { "VESA 512x384 backbuffered",             "FDM384R.EXE" },
    { "VESA 640x400 backbuffered",             "FDM400R.EXE" },
    { "VESA 640x480 backbuffered",             "FDM480R.EXE" },
    { "VESA 800x600 backbuffered",             "FDM600R.EXE" },
    { "VESA 1024x768 backbuffered",            "FDM768R.EXE" },
    { "VESA 1280x800 backbuffered",            "FDM800R.EXE" },
    { "VESA 1280x1024 backbuffered",           "FDM1024R.EXE" },
    { "VESA 1600x1200 backbuffered",           "FDM1200R.EXE" }
};

/* VESA / VBE 2.0, direct rendering (LFB) */
static const item_t vesa_d_items[] = {
    { "VBE 2.0 LFB, direct rendering",         "FDOOMVBD.EXE" },
    { "VESA 320x240 direct rendering",         "FDM240D.EXE" },
    { "VESA 400x300 direct rendering",         "FDM300D.EXE" },
    { "VESA 512x384 direct rendering",         "FDM384D.EXE" },
    { "VESA 640x400 direct rendering",         "FDM400D.EXE" },
    { "VESA 640x480 direct rendering",         "FDM480D.EXE" },
    { "VESA 800x600 direct rendering",         "FDM600D.EXE" },
    { "VESA 1024x768 direct rendering",        "FDM768D.EXE" },
    { "VESA 1280x800 direct rendering",        "FDM800D.EXE" },
    { "VESA 1280x1024 direct rendering",       "FDM1024D.EXE" },
    { "VESA 1600x1200 direct rendering",       "FDM1200D.EXE" }
};

/* EGA */
static const item_t ega_items[] = {
    { "EGA 320x200, 16 colors",                "FDOOMEGA.EXE" }
};

/* Hercules */
static const item_t herc_items[] = {
    { "Hercules 640x400 monochrome",           "FDOOMHGC.EXE" },
    { "Hercules InColor 320x200, 16 colors",   "FDOOMINC.EXE" }
};

/* CGA */
static const item_t cga_items[] = {
    { "CGA 320x200, 4 colors",                 "FDOOMCGA.EXE" },
    { "CGA 640x200 monochrome",                "FDOOMBWC.EXE" },
    { "CGA 160x100, 16 colors",                "FDOOMC16.EXE" },
    { "CGA composite 160x200, 16 colors",      "FDOOMCVB.EXE" },
    { "CGA 512 color composite, 80x100",       "FDOOM512.EXE" },
    { "CGA ANSI from Hell 320x100, 16 colors", "FDOOMCAH.EXE" }
};

/* Text mode / MDA */
static const item_t text_items[] = {
    { "MDA 80x25 text, 80x50 internal",        "FDOOMMDA.EXE" },
    { "MDA Color 80x25 text (rev 0 cards)",    "FDOOMCDA.EXE" },
    { "40x25 text, 16 colors",                 "FDOOMT1.EXE" },
    { "40x25 text, 16 colors (40x50 virtual)", "FDOOMT12.EXE" },
    { "80x25 text, 16 colors (80x50 virtual)", "FDOOMT25.EXE" },
    { "80x43 text, 16 colors (EGA cards)",     "FDOOMT43.EXE" },
    { "80x50 text, 16 colors",                 "FDOOMT50.EXE" },
    { "VT100 serial terminal, 80x25",          "FDMVT100.EXE" }
};

/* Specials */
static const item_t other_items[] = {
    { "Plantronics ColorPlus 320x200, 16 colors", "FDOOMPCP.EXE" },
    { "Sigma Color 400 320x200, 16 colors",       "FDOOM400.EXE" }
};

/* Utilities */
static const item_t util_items[] = {
    { "Setup controls and sound cards",  "FDSETUP.EXE" },
    { "Benchmark launcher",              "FDBENCH.EXE" }
};

static const group_t groups[] = {
    { "Text mode / MDA",      text_items,  (int)sizeof(text_items) / sizeof(text_items[0]) },
    { "Hercules",             herc_items,  (int)sizeof(herc_items) / sizeof(herc_items[0]) },
    { "CGA",                  cga_items,   (int)sizeof(cga_items) / sizeof(cga_items[0]) },
    { "EGA",                  ega_items,   (int)sizeof(ega_items) / sizeof(ega_items[0]) },
    { "VGA",                  vga_items,   (int)sizeof(vga_items) / sizeof(vga_items[0]) },
    { "VESA (VBE 2.0, backbuffered)", vesa_r_items, (int)sizeof(vesa_r_items) / sizeof(vesa_r_items[0]) },
    { "VESA (VBE 2.0, direct rendering)", vesa_d_items, (int)sizeof(vesa_d_items) / sizeof(vesa_d_items[0]) },
    { "Specials",             other_items, (int)sizeof(other_items) / sizeof(other_items[0]) },
    { "Utilities",            util_items,  (int)sizeof(util_items) / sizeof(util_items[0]) }
};

#define NGROUPS ((int)sizeof(groups) / sizeof(groups[0]))

/*
 * Returns 1 if the file exists, 0 otherwise.
 */
static int file_exists(const char *name)
{
    FILE *f = fopen(name, "r");

    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}



/*
 * Clears the screen and moves the cursor to the top left corner,
 * using BIOS video services (INT 10h). Works on MDA/Hercules and
 * all more advanced cards, without direct video memory access.
 */
static void clear_screen(void)
{
    union REGS regs;

    /* AH = 06h: scroll window up (0 lines = full clear) */
    regs.h.ah = 0x06;
    regs.h.al = 0x00; /* Clear whole screen */
    regs.h.bh = 0x07; /* Text attribute: white on black */
    regs.h.ch = 0x00; /* Start row */
    regs.h.cl = 0x00; /* Start column */
    regs.h.dh = 24;   /* End row (24 for a 25 row screen) */
    regs.h.dl = 79;   /* End column (79 for an 80 column screen) */

    int86(0x10, &regs, &regs);

    /* Move the cursor to the top left corner (0, 0) */
    regs.h.ah = 0x02;
    regs.h.bh = 0x00; /* Page 0 */
    regs.h.dh = 0x00; /* Row 0 */
    regs.h.dl = 0x00; /* Column 0 */

    int86(0x10, &regs, &regs);
}

/*
 * Shows a one line message and waits for any key.
 */
static void message(const char *text)
{
    printf("\n  %s", text);
    printf("\n\n  Press any key to continue...");
    (void)getch();
}

/*
 * Shows a numbered list of the group's items. Returns the index
 * of the selected item, or -1 if the user went back.
 */
static int group_menu(const group_t *g)
{
    int i;
    int c;

    for (;;) {
        clear_screen();
        printf("  FastDoom launcher - %s\n", g->title);
        printf("  -------------------------------\n");
        for (i = 0; i < g->count; i++) {
            if (file_exists(g->items[i].exe)) {
                printf("  %d. %-20s %s\n", i + 1, g->items[i].exe, g->items[i].desc);
            } else {
                printf("  %d. %-20s %s (missing)\n", i + 1, g->items[i].exe, g->items[i].desc);
            }
        }
        printf("\n  Enter number to run, Esc to go back, Q to quit.\n");
        c = getch();
        if (c == -1 || c == 0x1B) {
            return -1;
        }
        c = toupper(c);
        if (c == 'Q') {
            exit(0);
        }
        if (isdigit(c)) {
            i = c - '0';
            if (i == 0) {
                i = 10;
            }
            if (i >= 1 && i <= g->count) {
                return i - 1;
            }
        }
    }
}

/*
 * Main menu: lists the groups, enters the selected one, launches
 * the chosen program and loops until the user quits.
 */
static void main_menu(void)
{
    int i;
    int c;

    for (;;) {
        clear_screen();
        printf("  FastDoom text mode launcher\n");
        printf("  -------------------------------\n");
        for (i = 0; i < NGROUPS; i++) {
            printf("  %d. %s\n", i + 1, groups[i].title);
        }
        printf("  Q. Quit\n\n");
        printf("  Enter number to choose, Q to quit.\n");
        c = getch();
        if (c == -1 || c == 0x1B) {
            break;
        }
        c = toupper(c);
        if (c == 'Q') {
            break;
        }
        if (isdigit(c)) {
            i = c - '0';
            if (i == 0) {
                i = 10;
            }
            if (i >= 1 && i <= NGROUPS) {
                int sel = group_menu(&groups[i - 1]);

                if (sel >= 0) {
                    const char *exe = groups[i - 1].items[sel].exe;

                    if (!file_exists(exe)) {
                        message("Executable not found.");
                    } else {
                        /* Clear the screen so the launched program
                           starts on a clean one, then run it. When
                           it exits, quit the launcher too. */
                        clear_screen();
                        (void)system(exe);
                        exit(0);
                    }
                }
            }
        }
    }
}

int main(void)
{
    main_menu();
    clear_screen();
    printf("  Goodbye.\n");
    return 0;
}
