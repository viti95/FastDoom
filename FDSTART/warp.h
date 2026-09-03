/*
 * WARP.H - FastDoom text mode launcher, the single level launcher
 */

#ifndef WARP_H
#define WARP_H

/*
 * Single level launcher: the user picks the IWAD (-iwad), the
 * level (-warp) and the skill (-skill), and the saved executable
 * is run with them. Returns 1 if the launcher should quit, 0 to
 * go back to the main menu.
 */
int warp_menu(void);

#endif /* WARP_H */
