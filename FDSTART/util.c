/*
 * UTIL.C - FastDoom text mode launcher, small helpers
 */

#include <stdio.h>
#include <ctype.h>

#include "util.h"

/*
 * Returns 1 if the file exists, 0 otherwise.
 */
int file_exists(const char *name)
{
    FILE *f = fopen(name, "r");

    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}

/*
 * Moves the cursor by dir (-1 or 1) entries in a list of count
 * entries, wrapping around at both ends.
 */
int move_sel(int sel, int dir, int count)
{
    sel += dir;
    if (sel < 0) {
        sel = count - 1;
    } else if (sel >= count) {
        sel = 0;
    }
    return sel;
}

/*
 * Converts a digit character to a one based menu index (0 means 10).
 * Returns -1 if the character is not a digit.
 */
int digit_index(int c)
{
    if (!isdigit(c)) {
        return -1;
    }
    c -= '0';
    return (c == 0) ? 10 : c;
}
