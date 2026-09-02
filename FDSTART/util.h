/*
 * UTIL.H - FastDoom text mode launcher, small helpers
 */

#ifndef UTIL_H
#define UTIL_H

int file_exists(const char *name);
int move_sel(int sel, int dir, int count);
int digit_index(int c);

#endif /* UTIL_H */
