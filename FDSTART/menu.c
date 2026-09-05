/*
 * MENU.C - FastDoom text mode launcher, the menus
 *
 * The generic scrolling list picker, the group menu, the main menu
 * and the program launching.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>

#include "menu.h"
#include "screen.h"
#include "util.h"
#include "options.h"
#include "readme.h"
#include "warp.h"
#include "bench.h"
#include "texts.h"
#include "keys.h"

/* group_menu() return values: -1 to go back, -2 to quit, >= 0 the
   selected item. */
#define GMENU_BACK  (-1)
#define GMENU_QUIT  (-2)

/* One shot message shown in the main menu after a subscreen
   returns (for example, after the options were saved). flash_buf
   holds messages built with sprintf (like the default executable
   name). */
static char flash_buf[80];
static const char *flash_msg = NULL;

/*
 * Shows the list of the group's items with a cursor. Returns the
 * index of the selected item, GMENU_BACK if the user went back or
 * GMENU_QUIT if the user pressed Q.
 */
int group_menu(const group_t *g)
{
    char header[64];
    char saved[64];
    int i;
    int c;
    int sel = 0;
    int last_sel = -1;

    sprintf(header, TEXT_LAUNCHER " (%s)", g->title);
    /* The default executable: the one stored in FDSTART.CFG, or
       FDOOM.EXE if none is selected; it is marked with
       (default) in the list. */
    if (!load_launch_exe(saved, (int)sizeof(saved))) {
        strcpy(saved, "FDOOM.EXE");
    }

    for (;;) {
        /* Redraw only when the selection changed, so keys that
           do nothing don't flicker the screen. */
        if (sel != last_sel) {
            last_sel = sel;
            draw_menu_top(header);
            for (i = 0; i < g->count; i++) {
                printf("%s", i == sel ? " ->" : "   ");
                printf(" %2d. %12s - %s", i + 1, g->items[i].exe, g->items[i].desc);
                if (!file_exists(g->items[i].exe)) {
                    printf(" (missing)");
                }
                if (strcmp(g->items[i].exe, saved) == 0) {
                    printf(" (default)");
                }
                printf("\n");
            }
            print_bottom_row(MENU_FIRST_ROW + g->count,
                             "  " ARROW_UP "/" ARROW_DOWN " move, Enter/"
                             ARROW_RIGHT " run, D set default, Esc/"
                             ARROW_LEFT " back, Q quit.");
        }
        c = getch();
        if (c == -1 || c == 0x1B) {
            return GMENU_BACK;
        }
        if (c == 0 || c == 0xE0 || c == 0xE1) {
            /* Extended key (0, 0xE0 or 0xE1 prefix): the second
               byte is the scan code. */
            c = getch();
            if (c == KEY_UP) {
                sel = move_sel(sel, -1, g->count);
            } else if (c == KEY_DOWN) {
                sel = move_sel(sel, 1, g->count);
            } else if (c == KEY_LEFT) {
                return GMENU_BACK;
            } else if (c == KEY_RIGHT) {
                return sel;
            }
            continue;
        }
        c = toupper(c);
        if (c == 'Q') {
            return GMENU_QUIT;
        }
        if (c == 'D') {
            /* Set the highlighted executable as the default
               launch target (the "Launch" main menu entry),
               then go back to the main menu, showing the
               confirmation there without waiting for a key. */
            save_launch_exe(g->items[sel].exe);
            sprintf(flash_buf, "  Default executable set to %s.",
                    g->items[sel].exe);
            flash_msg = flash_buf;
            return GMENU_BACK;
        }
        if (c == '\r') {
            return sel;
        }
        i = digit_index(c);
        if (i >= 1 && i <= g->count) {
            return i - 1;
        }
    }
}

/* Rows that fit between the header box and the footer (same layout
   as the options menu). */
#define LIST_ROWS 17

/*
 * Shows a scrolling list of lines with a cursor. Returns the
 * selected line, PICK_BACK if the user went back or PICK_QUIT if
 * the user pressed Q.
 *
 * If marks is not NULL the list is multi-select: each line shows
 * a [X]/[ ] box, Space toggles the box of the current line and
 * Enter or Right confirms the whole selection (the marks array is
 * filled by the caller with 0/1 and updated in place; the
 * returned line is just the cursor position at the confirmation).
 */
int pick_list(const char *title, char *lines[], int n, int *marks)
{
    int top = 0;
    int sel = 0;
    int dirty = 1;
    int i;
    int c;

    for (;;) {
        /* Redraw only when the selection or the page changed, so
           keys that do nothing don't flicker the screen. */
        if (dirty) {
            dirty = 0;
            draw_menu_top(title);
            for (i = top; i < n && (i - top) < LIST_ROWS; i++) {
                printf("%s", i == sel ? " ->" : "   ");
                if (marks != NULL) {
                    printf(" %s %s\n", marks[i] ? "[X]" : "[ ]", lines[i]);
                } else {
                    printf(" %s\n", lines[i]);
                }
            }
            if (marks != NULL) {
                print_bottom_row(MENU_FIRST_ROW + (i - top),
                                 "  " ARROW_UP "/" ARROW_DOWN " to move, Space to select, "
                                 "Enter/" ARROW_RIGHT " to choose, Esc/"
                                 ARROW_LEFT " to go back, Q to quit.");
            } else {
                print_bottom_row(MENU_FIRST_ROW + (i - top),
                                 "  " ARROW_UP "/" ARROW_DOWN " to move, "
                                 "Enter/" ARROW_RIGHT " to choose, Esc/"
                                 ARROW_LEFT " to go back, Q to quit.");
            }
        }
        c = getch();
        if (c == -1 || c == 0x1B) {
            return PICK_BACK;
        }
        if (c == 0 || c == 0xE0 || c == 0xE1) {
            /* Extended key (0, 0xE0 or 0xE1 prefix): the second
               byte is the scan code. */
            c = getch();
            switch (c) {
                case KEY_UP:
                    if (sel > 0) {
                        sel--;
                        if (sel < top) {
                            top = sel;
                        }
                        dirty = 1;
                    }
                    break;
                case KEY_DOWN:
                    if (sel < n - 1) {
                        sel++;
                        if (sel >= top + LIST_ROWS) {
                            top = sel - LIST_ROWS + 1;
                        }
                        dirty = 1;
                    }
                    break;
                case KEY_LEFT:
                    return PICK_BACK;
                case KEY_RIGHT:
                    return sel;
                case KEY_PGUP:
                    top -= LIST_ROWS;
                    if (top < 0) {
                        top = 0;
                    }
                    if (sel < top) {
                        sel = top;
                    }
                    if (sel >= top + LIST_ROWS) {
                        sel = top + LIST_ROWS - 1;
                    }
                    dirty = 1;
                    break;
                case KEY_PGDN:
                    top += LIST_ROWS;
                    if (top > n - LIST_ROWS) {
                        top = n - LIST_ROWS;
                    }
                    if (top < 0) {
                        top = 0;
                    }
                    if (sel < top) {
                        sel = top;
                    }
                    if (sel >= top + LIST_ROWS) {
                        sel = top + LIST_ROWS - 1;
                    }
                    dirty = 1;
                    break;
                case KEY_HOME:
                    top = 0;
                    if (sel >= LIST_ROWS) {
                        sel = LIST_ROWS - 1;
                    }
                    dirty = 1;
                    break;
                case KEY_END:
                    top = n - LIST_ROWS;
                    if (top < 0) {
                        top = 0;
                    }
                    if (sel < top) {
                        sel = top;
                    } else if (sel >= top + LIST_ROWS) {
                        sel = top + LIST_ROWS - 1;
                    }
                    dirty = 1;
                    break;
                default:
                    break;
            }
            continue;
        }
        c = toupper(c);
        if (c == 'Q') {
            return PICK_QUIT;
        }
        if (marks != NULL && c == ' ') {
            marks[sel] = !marks[sel];
            dirty = 1;
            continue;
        }
        if (c == '\r') {
            return sel;
        }
    }
}

/*
 * Runs an executable: if it exists, clears the screen and runs it
 * with the saved options. When the program exits, returns to the
 * main menu.
 */
void run_exe(const char *exe)
{
    char cmd[MAX_CMD_LEN + 1];

    if (!file_exists(exe)) {
        message("Executable not found.");
    } else {
        /* Remember the game executables as the launch target
           (FDSETUP is not saved). */
        if (is_game_exe(exe)) {
            save_launch_exe(exe);
        }
        /* Clear the screen so the launched program
           starts on a clean one, then run it. When
           it exits, go back to the main menu. */
        build_command(exe, cmd);
        clear_screen();
        (void)system(cmd);
    }
}

/*
 * Enters the given group and launches the chosen program. When the
 * program exits, returns to the main menu.
 * Returns 1 if the launcher should quit, 0 otherwise.
 */
int run_group(int g)
{
    int sel = group_menu(&groups[g]);

    if (sel == GMENU_QUIT) {
        return 1;
    }
    if (sel >= 0) {
        run_exe(groups[g].items[sel].exe);
    }
    return 0;
}

/* Main menu entry indexes after the groups. */
#define MM_SETUP   NGROUPS
#define MM_BENCH   (NGROUPS + 1)
#define MM_README  (NGROUPS + 2)
#define MM_OPTIONS (NGROUPS + 3)
#define MM_WARP    (NGROUPS + 4)
#define MM_LAUNCH  (NGROUPS + 5)
#define MM_QUIT    (NGROUPS + 6)

/*
 * Enters the options menu, setting the flash message if the user
 * saved with S. Returns 1 if the launcher should quit, 0 otherwise.
 */
static int enter_options(void)
{
    int r = options_menu();

    if (r == OM_SAVED) {
        flash_msg = "  Options saved to FDSTART.CFG.";
    }
    return (r == OM_QUIT);
}

/*
 * Launches the last saved executable (see run_exe()).
 */
static void launch_saved_exe(void)
{
    char exe[64];

    /* No default executable selected in FDSTART.CFG: FDOOM.EXE is
       the default one. */
    if (!load_launch_exe(exe, (int)sizeof(exe))) {
        strcpy(exe, "FDOOM.EXE");
    }
    if (!file_exists(exe)) {
        message("Executable not found.");
    } else {
        run_exe(exe);
    }
}

/*
 * Executes the given main menu entry. Returns 1 if the launcher
 * should quit, 0 otherwise.
 */
static int choose_entry(int i)
{
    if (i < NGROUPS) {
        if (run_group(i)) {
            return 1;
        }
    } else if (i == MM_SETUP) {
        run_exe("FDSETUP.EXE");
    } else if (i == MM_BENCH) {
        if (bench_menu()) {
            return 1;
        }
    } else if (i == MM_README) {
        if (show_readme()) {
            return 1;
        }
    } else if (i == MM_OPTIONS) {
        return enter_options();
    } else if (i == MM_WARP) {
        if (warp_menu()) {
            return 1;
        }
    } else if (i == MM_LAUNCH) {
        launch_saved_exe();
    } else {
        return 1;
    }
    return 0;
}

/*
 * Main menu: lists the groups with a cursor, enters the selected
 * one, and loops until the user quits.
 */
void main_menu(void)
{
    int i;
    int c;
    int sel = 0;
    int last_sel = -1;

    for (;;) {
        /* Redraw only when the selection changed, so keys that
           do nothing don't flicker the screen. */
        if (sel != last_sel) {
            /* The default executable shown in the Launch entry:
               the one stored in FDSTART.CFG, or FDOOM.EXE if none
               is selected. */
            char launch_exe[64];

            if (!load_launch_exe(launch_exe, (int)sizeof(launch_exe))) {
                strcpy(launch_exe, "FDOOM.EXE");
            }
            last_sel = sel;
            draw_menu_top(TEXT_TITLE_MAIN);
            for (i = 0; i < MM_QUIT; i++) {
                printf("%s", i == sel ? " ->" : "   ");
                if (i < NGROUPS) {
                    printf(" %2d. %s\n", i + 1, groups[i].title);
                } else if (i == MM_SETUP) {
                    printf("  S. Setup (controls and sound cards)\n");
                } else if (i == MM_BENCH) {
                    printf("  B. Benchmark\n");
                } else if (i == MM_README) {
                    printf("  R. Readme\n");
                } else if (i == MM_OPTIONS) {
                    printf("  O. Options (command line parameters)\n");
                } else if (i == MM_WARP) {
                    printf("  W. Launch single level\n");
                } else {
                    printf("  L. Launch FastDoom (%s)\n", launch_exe);
                }
            }
            if (sel == MM_QUIT) {
                printf(" ->  Q. Quit\n\n");
            } else {
                printf("     Q. Quit\n\n");
            }
            if (flash_msg != NULL) {
                /* One shot message line just below the last entry,
                   shown without waiting for a key. */
                printf("%s\n", flash_msg);
                flash_msg = NULL;
                print_bottom_row(MENU_FIRST_ROW + MM_QUIT + 3,
                                 "  " ARROW_UP "/" ARROW_DOWN " to move, "
                                 "Enter/" ARROW_RIGHT " to choose, Esc/Q to quit.");
            } else {
                print_bottom_row(MENU_FIRST_ROW + MM_QUIT + 2,
                                 "  " ARROW_UP "/" ARROW_DOWN " to move, "
                                 "Enter/" ARROW_RIGHT " to choose, Esc/Q to quit.");
            }
        }
        c = getch();
        if (c == -1 || c == 0x1B) {
            break;
        }
        if (c == 0 || c == 0xE0 || c == 0xE1) {
            /* Extended key (0, 0xE0 or 0xE1 prefix): the second
               byte is the scan code. */
            c = getch();
            if (c == KEY_UP) {
                sel = move_sel(sel, -1, MM_QUIT + 1);
            } else if (c == KEY_DOWN) {
                sel = move_sel(sel, 1, MM_QUIT + 1);
            } else if (c == KEY_RIGHT) {
                if (choose_entry(sel)) {
                    break;
                }
                last_sel = -1; /* Subscreen was shown: force a redraw */
            }
            continue;
        }
        c = toupper(c);
        if (c == 'Q') {
            break;
        }
        if (c == 'L') {
            launch_saved_exe();
            last_sel = -1;
        }
        if (c == 'S') {
            run_exe("FDSETUP.EXE");
            last_sel = -1;
        }
        if (c == 'B') {
            if (bench_menu()) {
                break;
            }
            last_sel = -1;
        }
        if (c == 'R') {
            if (show_readme()) {
                break;
            }
            last_sel = -1;
        }
        if (c == 'O') {
            if (enter_options()) {
                break;
            }
            last_sel = -1;
        }
        if (c == 'W') {
            if (warp_menu()) {
                break;
            }
            last_sel = -1;
        }
        if (c == '\r') {
            if (choose_entry(sel)) {
                break;
            }
            last_sel = -1;
        }
        i = digit_index(c);
        if (i >= 1 && i <= NGROUPS) {
            if (choose_entry(i - 1)) {
                break;
            }
            last_sel = -1;
        }
    }
}
