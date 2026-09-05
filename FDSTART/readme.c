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

#define README_MAX_WIDTH  79
#define README_TEXT_ROWS  22

/*
 * The visible window of lines (static, so the buffer lives in BSS
 * instead of on the stack). Only the window is kept in memory:
 * the rest of the file is read on demand from the disk with
 * fseek, so the reader works with files of any length without a
 * big BSS buffer.
 */
static char window[README_TEXT_ROWS][README_MAX_WIDTH + 1];

/*
 * Reads a line from the file as the reader shows it: with the end
 * of line stripped and truncated to README_MAX_WIDTH. Returns 1
 * if a line was read, 0 at the end of the file.
 */
static int read_line(FILE *f, char *line, int size)
{
    int len;

    if (fgets(line, size, f) == NULL) {
        line[0] = '\0';
        return 0;
    }
    len = (int)strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
    if (len > README_MAX_WIDTH) {
        line[README_MAX_WIDTH] = '\0'; /* truncate long lines */
    }
    return 1;
}

/*
 * Counts the lines of the file, chunking them exactly as the
 * reader does (same fgets size), so the count matches the windows
 * read with read_window().
 */
static int count_lines(FILE *f)
{
    char line[README_MAX_WIDTH + 4];
    int n = 0;

    (void)fseek(f, 0L, SEEK_SET);
    while (fgets(line, sizeof(line), f) != NULL) {
        n++;
    }
    return n;
}

/*
 * Reads the window of README_TEXT_ROWS lines starting at line top
 * into the window buffer. Returns the number of lines read.
 */
static int read_window(FILE *f, int top)
{
    char skip[README_MAX_WIDTH + 4];
    int n = 0;

    (void)fseek(f, 0L, SEEK_SET);
    while (n < top && fgets(skip, sizeof(skip), f) != NULL) {
        n++;
    }
    n = 0;
    while (n < README_TEXT_ROWS &&
           read_line(f, window[n], (int)sizeof(window[n]))) {
        n++;
    }
    return n;
}

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
    FILE *f;
    int nlines;
    int nwin = 0;
    int top = 0;
    int last_top = -1;
    int i;
    int c;

    f = fopen("README.TXT", "r");
    if (f == NULL) {
        message("README.TXT not found.");
        return 0;
    }
    nlines = count_lines(f);

    for (;;) {
        /* Redraw only when the page changed, so keys that do
           nothing don't flicker the screen. */
        if (top != last_top) {
            last_top = top;
            clear_screen();
            nwin = read_window(f, top);
            for (i = 0; i < nwin; i++) {
                (void)printf("%s\n", window[i]);
            }
            print_bottom_row(nwin,
                             "  PgUp/PgDn or arrows to scroll, Home/End to jump, Esc to go back, Q to quit.");
        }
        c = getch();
        if (c == -1 || c == 0x1B) {
            fclose(f);
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
            fclose(f);
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
