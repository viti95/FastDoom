/*
 * WARP.H - FastDoom text mode launcher, the single level launcher
 */

#ifndef WARP_H
#define WARP_H

/*
 * The IWADs FastDoom knows about (the same list as the iwads[]
 * table in d_main.c, defined in warp.c): the file name (passed
 * to -iwad if found) and the level list file in the LEVELS
 * directory. The order matches iwads[].
 */
typedef struct {
    const char *name;
    const char *display;
    const char *levels;
} wiwad_t;

#define NIWADS 8

extern const wiwad_t wiwads[NIWADS];

/*
 * Single level launcher: the user picks the IWAD (-iwad), the
 * level (-warp) and the skill (-skill), and the saved executable
 * is run with them. Returns 1 if the launcher should quit, 0 to
 * go back to the main menu.
 */
int warp_menu(void);

#endif /* WARP_H */
