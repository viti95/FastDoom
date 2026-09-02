/*
 * README.C - FastDoom text mode launcher, the README.TXT reader
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>

#include "readme.h"
#include "screen.h"
#include "keys.h"

#define README_MAX_LINES  512
#define README_MAX_WIDTH  79
#define README_TEXT_ROWS  22

/*
 * Shows the README.TXT file in a simple text reader: the text from
 * the top row down, a key description line at the bottom and a
 * blank row in between. Scrolls with the cursor.
 *
 * The screen has 25 rows: rows 0..21 are the text, row 22 is blank
 * and row 23 is the key description line (the trailing newline of
 * it would scroll the screen if the footer were on the last row).
 *
 * Returns 1 if the user wants to quit the launcher, 0 to go back
 * to the menu.
 */
int show_readme(void)
{
    /* Static, so the (large) line buffer lives in BSS instead
       of on the stack. */
    static char lines[README_MAX_LINES][README_MAX_WIDTH + 1];
    char buf[README_MAX_WIDTH + 4];
    FILE *f;
    int nlines = 0;
    int top = 0;
    int last_top = -1;
    int i;
    int c;

    f = fopen("README.TXT", "r");
    if (f == NULL) {
        message("README.TXT not found.");
        return 0;
    }
    while (nlines < README_MAX_LINES &&
           fgets(buf, sizeof(buf), f) != NULL) {
        int len = (int)strlen(buf);

        while (len > 0 &&
               (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }
        if (len > README_MAX_WIDTH) {
            len = README_MAX_WIDTH; /* truncate long lines */
        }
        memcpy(lines[nlines], buf, (size_t)len + 1);
        nlines++;
    }
    fclose(f);

    for (;;) {
        /* Redraw only when the page changed, so keys that do
           nothing don't flicker the screen. */
        if (top != last_top) {
            last_top = top;
            clear_screen();
            for (i = top; i < nlines && (i - top) < README_TEXT_ROWS; i++) {
                (void)printf("%s\n", lines[i]);
            }
            print_bottom_row(i - top,
                             "  PgUp/PgDn or arrows to scroll, Home/End to jump, Esc to go back, Q to quit.");
        }
        c = getch();
        if (c == -1 || c == 0x1B) {
            return 0;
        }
        if (c == 0 || c == 0xE0 || c == 0xE1) {
            /* Extended key (0, 0xE0 or 0xE1 prefix): the second
               byte is the scan code. */
            c = getch();
            switch (c) {
                case KEY_UP:
                    if (top > 0) {
                        top--;
                    }
                    break;
                case KEY_DOWN:
                    if (top < nlines - 1) {
                        top++;
                    }
                    break;
                case KEY_PGUP:
                    top -= README_TEXT_ROWS;
                    if (top < 0) {
                        top = 0;
                    }
                    break;
                case KEY_PGDN:
                    top += README_TEXT_ROWS;
                    if (top > nlines - README_TEXT_ROWS) {
                        top = nlines - README_TEXT_ROWS;
                    }
                    if (top < 0) {
                        top = 0;
                    }
                    break;
                case KEY_HOME:
                    top = 0;
                    break;
                case KEY_END:
                    top = nlines - README_TEXT_ROWS;
                    if (top < 0) {
                        top = 0;
                    }
                    break;
                default:
                    break;
            }
            continue;
        }
        c = toupper(c);
        if (c == 'Q') {
            return 1;
        }
        if (c == ' ' || c == '\r') {
            top += README_TEXT_ROWS;
            if (top > nlines - README_TEXT_ROWS) {
                top = nlines - README_TEXT_ROWS;
            }
            if (top < 0) {
                top = 0;
            }
        }
    }
}
