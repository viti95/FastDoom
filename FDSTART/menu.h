/*
 * MENU.H - FastDoom text mode launcher, the menus
 */

#ifndef MENU_H
#define MENU_H

#include "groups.h"

int group_menu(const group_t *g);
void run_exe(const char *exe);
int run_group(int g);
void main_menu(void);

#endif /* MENU_H */
