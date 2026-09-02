/*
 * SCREEN.H - FastDoom text mode launcher, screen helpers
 */

#ifndef SCREEN_H
#define SCREEN_H

/* Row where the first menu entry is printed (zero based): the empty
   top row, the 3 header rows and a blank row. */
#define MENU_FIRST_ROW 5

void clear_screen(void);
void message(const char *text);
void print_header(const char *text);
void draw_menu_top(const char *title);
void print_bottom_row(int row, const char *text);

#endif /* SCREEN_H */
