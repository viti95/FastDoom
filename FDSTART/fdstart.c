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
    { "CGA 512 color composite 80x100",       "FDOOM512.EXE" },
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

static const group_t groups[] = {
    { "Text mode / MDA",      text_items,  (int)sizeof(text_items) / sizeof(text_items[0]) },
    { "Hercules",             herc_items,  (int)sizeof(herc_items) / sizeof(herc_items[0]) },
    { "CGA",                  cga_items,   (int)sizeof(cga_items) / sizeof(cga_items[0]) },
    { "EGA",                  ega_items,   (int)sizeof(ega_items) / sizeof(ega_items[0]) },
    { "VGA",                  vga_items,   (int)sizeof(vga_items) / sizeof(vga_items[0]) },
    { "VESA VBE 2.0, backbuffered", vesa_r_items, (int)sizeof(vesa_r_items) / sizeof(vesa_r_items[0]) },
    { "VESA VBE 2.0, direct rendering", vesa_d_items, (int)sizeof(vesa_d_items) / sizeof(vesa_d_items[0]) },
    { "Specials",             other_items, (int)sizeof(other_items) / sizeof(other_items[0]) }
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

/* Scan codes of the arrow keys */
#define KEY_UP    72
#define KEY_DOWN  80
#define KEY_LEFT  75
#define KEY_RIGHT 77

/*
 * Prints the menu header box around the given text. The number of
 * dashes is computed from the text length, so the box always fits
 * the title exactly ("- " + text + " -").
 */
static void print_header(const char *text)
{
    int i;
    int width = (int)strlen(text) + 4;

    (void)printf("  ");
    for (i = 0; i < width; i++) {
        (void)putchar('-');
    }
    (void)printf("\n  - %s -\n  ", text);
    for (i = 0; i < width; i++) {
        (void)putchar('-');
    }
    (void)printf("\n\n");
}

/*
 * Shows the list of the group's items with a cursor. Returns the
 * index of the selected item, or -1 if the user went back.
 */
static int group_menu(const group_t *g)
{
    int i;
    int c;
    int sel = 0;

    for (;;) {
        char header[64];

        clear_screen();
        sprintf(header, "FastDoom launcher (%s)", g->title);
        print_header(header);
        for (i = 0; i < g->count; i++) {
            if (i == sel) {
                printf(" >");
            } else {
                printf("  ");
            }
            if (file_exists(g->items[i].exe)) {
                printf(" %d. %-20s %s\n", i + 1, g->items[i].exe, g->items[i].desc);
            } else {
                printf(" %d. %-20s %s (missing)\n", i + 1, g->items[i].exe, g->items[i].desc);
            }
        }
        printf("\n  Up/Down to move, Enter/Right to run, Esc/Left to go back, Q to quit.\n");
        c = getch();
        if (c == -1 || c == 0x1B) {
            return -1;
        }
        if (c == 0) {
            c = getch();
            if (c == KEY_UP) {
                sel = (sel > 0) ? sel - 1 : g->count - 1;
            } else if (c == KEY_DOWN) {
                sel = (sel < g->count - 1) ? sel + 1 : 0;
            } else if (c == KEY_LEFT) {
                return -1;
            } else if (c == KEY_RIGHT) {
                return sel;
            }
            continue;
        }
        c = toupper(c);
        if (c == 'Q') {
            exit(0);
        }
        if (c == '\r') {
            return sel;
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
 * Runs an executable: if it exists, clears the screen and runs it,
 * then quits the launcher when it exits. Never returns in that case.
 */
static void run_exe(const char *exe)
{
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

/*
 * Enters the given group, launches the chosen program and loops
 * until the user goes back. Never returns if a program was run.
 */
static void run_group(int g)
{
    int sel = group_menu(&groups[g]);

    if (sel >= 0) {
        run_exe(groups[g].items[sel].exe);
    }
}

/* Main menu entry indexes after the groups. */
#define MM_SETUP  NGROUPS
#define MM_BENCH  (NGROUPS + 1)
#define MM_QUIT   (NGROUPS + 2)

/*
 * Executes the given main menu entry. Returns 1 if the launcher
 * should quit, 0 otherwise.
 */
static int choose_entry(int i)
{
    if (i < NGROUPS) {
        run_group(i);
    } else if (i == MM_SETUP) {
        run_exe("FDSETUP.EXE");
    } else if (i == MM_BENCH) {
        run_exe("FDBENCH.EXE");
    } else {
        return 1;
    }
    return 0;
}

/*
 * Main menu: lists the groups with a cursor, enters the selected
 * one, and loops until the user quits.
 */
static void main_menu(void)
{
    int i;
    int c;
    int sel = 0;

    for (;;) {
        clear_screen();
        print_header("FastDoom launcher");
        for (i = 0; i < MM_QUIT; i++) {
            if (i == sel) {
                printf(" >");
            } else {
                printf("  ");
            }
            if (i < NGROUPS) {
                printf(" %d. %s\n", i + 1, groups[i].title);
            } else if (i == MM_SETUP) {
                printf(" S. FDSETUP (Setup controls and sound cards)\n");
            } else {
                printf(" B. FDBENCH (Benchmark utility)\n");
            }
        }
        if (sel == MM_QUIT) {
            printf(" > Q. Quit\n\n");
        } else {
            printf("   Q. Quit\n\n");
        }
        printf("  Up/Down to move, Enter/Right to choose, Esc/Left/Q to quit.\n");
        c = getch();
        if (c == -1 || c == 0x1B) {
            break;
        }
        if (c == 0) {
            c = getch();
            if (c == KEY_UP) {
                sel = (sel > 0) ? sel - 1 : MM_QUIT;
            } else if (c == KEY_DOWN) {
                sel = (sel < MM_QUIT) ? sel + 1 : 0;
            } else if (c == KEY_LEFT) {
                break;
            } else if (c == KEY_RIGHT) {
                if (choose_entry(sel)) {
                    break;
                }
            }
            continue;
        }
        c = toupper(c);
        if (c == 'Q') {
            break;
        }
        if (c == 'S') {
            run_exe("FDSETUP.EXE");
        }
        if (c == 'B') {
            run_exe("FDBENCH.EXE");
        }
        if (c == '\r') {
            if (choose_entry(sel)) {
                break;
            }
        }
        if (isdigit(c)) {
            i = c - '0';
            if (i == 0) {
                i = 10;
            }
            if (i >= 1 && i <= NGROUPS) {
                run_group(i - 1);
            }
        }
    }
}

int main(void)
{
    main_menu();
    clear_screen();
    printf("RIP AND TEAR\n");
    return 0;
}
