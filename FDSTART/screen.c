/*
 * SCREEN.C - FastDoom text mode launcher, screen helpers
 *
 * Screen clearing (BIOS video services), the menu header box and
 * the fixed bottom key description line.
 */

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <i86.h>

#include "screen.h"

/*
 * Clears the screen and moves the cursor to the top left corner,
 * using BIOS video services (INT 10h). Works on MDA/Hercules and
 * all more advanced cards, without direct video memory access.
 */
void clear_screen(void)
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
void message(const char *text)
{
    printf("\n  %s", text);
    printf("\n\n  Press any key to continue...");
    (void)getch();
}

/*
 * Prints n dash characters.
 */
static void print_dashes(int n)
{
    int i;

    for (i = 0; i < n; i++) {
        (void)putchar('-');
    }
}

/*
 * Prints the menu header box around the given text. The number of
 * dashes is computed from the text length, so the box always fits
 * the title exactly ("- " + text + " -").
 */
void print_header(const char *text)
{
    int width = (int)strlen(text) + 4;

    (void)printf("  ");
    print_dashes(width);
    (void)printf("\n  - %s -\n  ", text);
    print_dashes(width);
    (void)printf("\n\n");
}

/*
 * Clears the screen, leaves the top row empty and prints the menu
 * header box.
 */
void draw_menu_top(const char *title)
{
    clear_screen();
    (void)printf("\n");
    print_header(title);
}

/* Bottom row (zero based) where the key description line is shown. */
#define BOTTOM_ROW 23

/*
 * Prints the key description line fixed to the bottom of the
 * screen. Row is the row the cursor is currently at (zero based).
 */
void print_bottom_row(int row, const char *text)
{
    int i;

    for (i = row; i < BOTTOM_ROW; i++) {
        (void)printf("\n");
    }
    (void)printf("%s\n", text);
}
