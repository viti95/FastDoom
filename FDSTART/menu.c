/*
 * MENU.C - FastDoom text mode launcher, the menus
 *
 * The group menu, the main menu and the program launching.
 */

#include <stdio.h>
#include <ctype.h>
#include <conio.h>

#include "menu.h"
#include "screen.h"
#include "util.h"
#include "options.h"
#include "readme.h"
#include "warp.h"
#include "texts.h"
#include "keys.h"

/* group_menu() return values: -1 to go back, -2 to quit, >= 0 the
   selected item. */
#define GMENU_BACK  (-1)
#define GMENU_QUIT  (-2)

/*
 * Shows the list of the group's items with a cursor. Returns the
 * index of the selected item, GMENU_BACK if the user went back or
 * GMENU_QUIT if the user pressed Q.
 */
int group_menu(const group_t *g)
{
    char header[64];
    int i;
    int c;
    int sel = 0;
    int last_sel = -1;

    sprintf(header, TEXT_LAUNCHER " (%s)", g->title);

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
                printf("\n");
            }
            print_bottom_row(MENU_FIRST_ROW + g->count,
                             "  Up/Down to move, Enter/Right to run, Esc/Left to go back, Q to quit.");
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
        if (c == '\r') {
            return sel;
        }
        i = digit_index(c);
        if (i >= 1 && i <= g->count) {
            return i - 1;
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
           (FDSETUP and FDBENCH are not saved). */
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

/* One shot message shown in the main menu after a subscreen
   returns (for example, after the options were saved). */
static const char *flash_msg = NULL;

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

    if (!load_launch_exe(exe, (int)sizeof(exe))) {
        message("No executable saved. Run one from a group first.");
    } else if (!file_exists(exe)) {
        message("Saved executable not found.");
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
        run_exe("FDBENCH.EXE");
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
            last_sel = sel;
            draw_menu_top(TEXT_TITLE_MAIN);
            for (i = 0; i < MM_QUIT; i++) {
                printf("%s", i == sel ? " ->" : "   ");
                if (i < NGROUPS) {
                    printf(" %2d. %s\n", i + 1, groups[i].title);
                } else if (i == MM_SETUP) {
                    printf("  S. FDSETUP (Setup controls and sound cards)\n");
                } else if (i == MM_BENCH) {
                    printf("  B. FDBENCH (Benchmark utility)\n");
                } else if (i == MM_README) {
                    printf("  R. Readme\n");
                } else if (i == MM_OPTIONS) {
                    printf("  O. Options (command line parameters)\n");
                } else if (i == MM_WARP) {
                    printf("  W. Single level (warp to a level)\n");
                } else {
                    printf("  L. Launch (last saved executable)\n");
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
                                 "  Up/Down to move, Enter/Right to choose, Esc/Left/Q to quit.");
            } else {
                print_bottom_row(MENU_FIRST_ROW + MM_QUIT + 2,
                                 "  Up/Down to move, Enter/Right to choose, Esc/Left/Q to quit.");
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
            } else if (c == KEY_LEFT) {
                break;
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
            run_exe("FDBENCH.EXE");
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
