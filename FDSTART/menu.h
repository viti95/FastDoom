/*
 * MENU.H - FastDoom text mode launcher, the menus
 */

#ifndef MENU_H
#define MENU_H

#include "groups.h"

/* pick_list() return values: -1 to go back, -2 to quit, >= 0 the
   selected line. */
#define PICK_BACK  (-1)
#define PICK_QUIT  (-2)

int group_menu(const group_t *g);
void run_exe(const char *exe);
int run_group(int g);
int pick_list(const char *title, char *lines[], int n, int *marks);
void main_menu(void);

#endif /* MENU_H */
