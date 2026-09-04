/*
 * WARP.C - FastDoom text mode launcher, the single level launcher
 *
 * Lets the user pick the IWAD (-iwad), a level (-warp, optional)
 * and the skill (-skill), and runs the saved game executable with
 * them.
 *
 * The IWADs offered are the ones FastDoom knows about (the same
 * list as the iwads[] table in d_main.c); the file is looked up in
 * the current directory. Optionally the user can also pick one or
 * more PWADs (-file, they can be multiple, separated by a single
 * space) from the WADS sub-directory.
 *
 * The level lists are read from the LEVELS\<iwad>.txt files that
 * ship with FastDoom (the same ones the game reads for the level
 * names). The line prefix gives the level number: "E1M1:" style
 * for non commercial IWADs (episode and map) and "MAP01:" or
 * "level N:" style for the commercial ones (map only).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <dos.h>
#include <errno.h>
#include <conio.h>

#include "warp.h"
#include "menu.h"
#include "texts.h"
#include "screen.h"
#include "util.h"
#include "options.h"
#include "keys.h"

/* pick_list() return values: -1 to go back, -2 to quit, >= 0 the
   selected line. */
#define PICK_BACK  (-1)
#define PICK_QUIT  (-2)

/* Rows that fit between the header box and the footer (same layout
   as the options menu). */
#define LIST_ROWS 17

/*
 * The IWADs: the file name (passed to -iwad if found) and the
 * level list file in the LEVELS directory. The order matches the
 * iwads[] table in d_main.c.
 */
typedef struct {
    const char *name;
    const char *display;
    const char *levels;
} wiwad_t;

#define NIWADS 8

static const wiwad_t wiwads[NIWADS] = {
    { "doom1.wad",    "DOOM Shareware",                       "LEVELS\\doom1.txt" },
    { "doom.wad",     "DOOM",                                 "LEVELS\\doom.txt" },
    { "doomu.wad",    "The Ultimate DOOM",                    "LEVELS\\doomu.txt" },
    { "doom2.wad",    "DOOM II",                              "LEVELS\\doom2.txt" },
    { "plutonia.wad", "Final Doom - The Plutonia Experiment", "LEVELS\\plutonia.txt" },
    { "tnt.wad",      "Final Doom - TNT: Evilution",          "LEVELS\\tnt.txt" },
    { "freedm1.wad",  "Freedoom: Phase 1",                    "LEVELS\\freedm1.txt" },
    { "freedm2.wad",  "Freedoom: Phase 2",                    "LEVELS\\freedm2.txt" }
};

/* Where the PWAD files are looked up. */
#define WAD_DIR "WADS\\"

/* The PWAD list (static, so the buffers live in BSS). */
#define MAX_PWADS     255
#define PWAD_NAME_LEN 32

static int npwads;
static char pwad_names[MAX_PWADS][PWAD_NAME_LEN];
static char pwad_line_buf[MAX_PWADS][PWAD_NAME_LEN + 8];
static char *pwad_lines[MAX_PWADS];
/* The PWADs selected in the multi-select list (indices into
   pwad_names) and how many of them. */
static int pwad_sel[MAX_PWADS];
static int npwad_sel;
static int pwad_mark[MAX_PWADS];

/*
 * Scans the WADS directory for PWADs (*.WAD, files only) and
 * stores their names. Returns the number of PWADs found, 0 if the
 * directory does not exist or is empty.
 */
static int scan_pwads(void)
{
    struct find_t data;
    unsigned handle;
    int n = 0;

    errno = 0;
    handle = _dos_findfirst(WAD_DIR "*.WAD", 0, &data);
    while (!errno) {
        if (!(data.attrib & _A_SUBDIR) && n < MAX_PWADS) {
            strncpy(pwad_names[n], data.name, PWAD_NAME_LEN - 1);
            pwad_names[n][PWAD_NAME_LEN - 1] = '\0';
            n++;
        }
        errno = 0;
        _dos_findnext(&data);
    }
    return n;
}

/* The skill choices, as in the game skill menu (1..5). */
#define NSKILLS 5

static char *skill_lines[NSKILLS] = {
    "1. I'm too young to die",
    "2. Hey, not too rough",
    "3. Hurt me plenty",
    "4. Ultra-violence",
    "5. NIGHTMARE!"
};

/* The IWAD selection lines, rebuilt on every pass. */
static char iwad_line_buf[NIWADS][64];
static char *iwad_lines[NIWADS];

/*
 * Returns 1 if the IWAD file exists in the current directory.
 */
static int iwad_exists(const char *name)
{
    return file_exists(name);
}

/* Maps each shown line to its entry in the wiwads table. */
static int iwad_index[NIWADS];

/*
 * Builds the IWAD selection lines from the IWADs that exist in the
 * current directory. Returns the number of available IWADs.
 */
static int build_iwad_lines(void)
{
    int n = 0;
    int i;

    for (i = 0; i < NIWADS; i++) {
        if (!iwad_exists(wiwads[i].name)) {
            continue;
        }
        iwad_lines[n] = iwad_line_buf[n];
        sprintf(iwad_line_buf[n], "%s (%s)",
                wiwads[i].display, wiwads[i].name);
        iwad_index[n] = i;
        n++;
    }
    return n;
}

/* The parsed level list (static, so the buffers live in BSS). */
#define MAX_LEVELS     64
#define MAX_LEVEL_NAME 40

static int nlevels;
static int level_commercial;
static int level_ep[MAX_LEVELS];
static int level_map[MAX_LEVELS];
static char level_name[MAX_LEVELS][MAX_LEVEL_NAME];
static char level_line_buf[MAX_LEVELS + 1][MAX_LEVEL_NAME + 8];
static char *level_lines[MAX_LEVELS + 1];

/*
 * Loads the level list file. Each line has the form "E1M1: name",
 * "MAP01: name" or "level 1: name"; the prefix gives the episode
 * and map (map only for the commercial IWADs). Returns the number
 * of levels found, 0 if the file is missing or empty.
 */
static int load_levels(const char *file)
{
    FILE *f;
    char line[80];
    int n = 0;

    f = fopen(file, "r");
    if (f == NULL) {
        return 0;
    }
    level_commercial = 0;
    while (n < MAX_LEVELS && fgets(line, sizeof(line), f) != NULL) {
        char *colon;
        char *name;
        int len = (int)strlen(line);
        int ep = 0;
        int map = 0;
        int commercial = 0;

        while (len > 0 &&
               (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        colon = strchr(line, ':');
        if (colon == NULL) {
            continue;
        }
        *colon = '\0';
        name = colon + 1;
        while (*name == ' ') {
            name++;
        }
        if (line[0] == 'E' && isdigit((unsigned char)line[1]) &&
            line[2] == 'M' && isdigit((unsigned char)line[3]) &&
            line[4] == '\0') {
            ep = line[1] - '0';
            map = line[3] - '0';
        } else if (strncmp(line, "MAP", 3) == 0) {
            map = atoi(line + 3);
            commercial = 1;
        } else if (strncmp(line, "level", 5) == 0) {
            map = atoi(line + 5);
            commercial = 1;
        } else {
            continue;
        }
        if (map < 1) {
            continue;
        }
        level_ep[n] = ep;
        level_map[n] = map;
        level_commercial = commercial;
        strncpy(level_name[n], name, MAX_LEVEL_NAME - 1);
        level_name[n][MAX_LEVEL_NAME - 1] = '\0';
        n++;
    }
    fclose(f);
    return n;
}

/*
 * Builds the level selection lines from the parsed list. Line 0 is
 * the "No level" option: it runs the game without -warp (and
 * without -skill), with just the IWAD and the PWADs.
 */
static void build_level_lines(void)
{
    int i;

    level_lines[0] = level_line_buf[0];
    strcpy(level_line_buf[0], "No level");
    for (i = 0; i < nlevels; i++) {
        level_lines[i + 1] = level_line_buf[i + 1];
        if (level_commercial) {
            sprintf(level_line_buf[i + 1], "MAP%02d  %s",
                    level_map[i], level_name[i]);
        } else {
            sprintf(level_line_buf[i + 1], "E%dM%d  %s",
                    level_ep[i], level_map[i], level_name[i]);
        }
    }
}

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
static int pick_list(const char *title, char *lines[], int n, int *marks)
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
 * Appends a formatted string to the command line. The caller must
 * make sure the result stays within MAX_CMD_LEN (DOS command lines
 * are short); see launcher_cmd_len().
 */
static void append_cmd(char *cmd, const char *fmt, ...)
{
    char part[80];
    va_list ap;

    va_start(ap, fmt);
    (void)vsprintf(part, fmt, ap);
    va_end(ap);
    strcat(cmd, part);
}

/*
 * Returns the total length of the command line the launcher would
 * build: the executable with the enabled options (without the
 * MAX_CMD_LEN limit, as in command_length()) plus the -iwad, -file
 * (if any), -warp and -skill (only when a level was selected).
 */
static int launcher_cmd_len(const char *exe, const char *iwad,
                            int lvl, int skill)
{
    char num[16];
    int len = command_length(exe);
    int i;

    len += 7 + (int)strlen(iwad); /* " -iwad " */
    for (i = 0; i < npwad_sel; i++) {
        /* " -file " for the first PWAD, " " for the rest. */
        len += (i == 0 ? 7 : 1) + (int)strlen(WAD_DIR)
             + (int)strlen(pwad_names[pwad_sel[i]]);
    }
    if (lvl >= 0) {
        if (level_commercial) {
            /* Commercial IWADs: -warp takes the map number only. */
            sprintf(num, "%d", level_map[lvl]);
        } else {
            /* The others: -warp takes the episode and the map. */
            sprintf(num, "%d %d", level_ep[lvl], level_map[lvl]);
        }
        len += 7 + (int)strlen(num); /* " -warp " */
        len += 8 + 1; /* " -skill " and a one digit value */
    }
    return len;
}

/*
 * The single level launcher: pick the IWAD, the PWADs (one or
 * more), a level (optional) and the skill, then run the saved
 * executable (or FDOOM.EXE if none is saved) with -iwad, -file
 * (if any) and, when a level is selected, -warp and -skill, plus
 * the saved command line options. When the game exits, goes back
 * to the main menu.
 *
 * Returns 1 if the launcher should quit, 0 to go back to the main
 * menu.
 */
int warp_menu(void)
{
    char exe[64];
    char cmd[MAX_CMD_LEN + 1];
    char iwad_path[32];
    int niwads;
    int iwad;
    int lvl;
    int skill;
    int i;
    int run = 0;
    int pwad_pick;
    int lvl_pick;
    int skill_pick;
    int to_iwad;

    for (;;) {
        niwads = build_iwad_lines();
        if (niwads == 0) {
            message("No IWAD found. Put a .WAD file in the FastDoom directory.");
            return 0;
        }
        iwad = pick_list(TEXT_TITLE_SINGLE_IWAD,
                         iwad_lines, niwads, NULL);
        if (iwad == PICK_QUIT) {
            return 1;
        }
        if (iwad == PICK_BACK) {
            return 0;
        }
        iwad = iwad_index[iwad];
        if (!iwad_exists(wiwads[iwad].name)) {
            message("IWAD not found.");
            continue;
        }
        strcpy(iwad_path, wiwads[iwad].name);
        for (;;) {
            /* The PWAD selection is shown only when there are
               PWADs in the WADS directory; several can be picked
               at once (-file takes them all, separated by a
               single space) or none. Going back from it (or from
               the level selection when there are no PWADs) goes
               to the IWAD selection; going back from the level
               selection comes back to it. */
            npwad_sel = 0;
            npwads = scan_pwads();
            if (npwads > 0) {
                for (i = 0; i < npwads; i++) {
                    pwad_lines[i] = pwad_line_buf[i];
                    strcpy(pwad_line_buf[i], pwad_names[i]);
                    pwad_mark[i] = 0;
                }
                pwad_pick = pick_list(TEXT_TITLE_SINGLE_PWAD,
                                      pwad_lines, npwads, pwad_mark);
                if (pwad_pick == PICK_QUIT) {
                    return 1;
                }
                if (pwad_pick == PICK_BACK) {
                    break; /* Back to the IWAD selection */
                }
                for (i = 0; i < npwads; i++) {
                    if (pwad_mark[i]) {
                        pwad_sel[npwad_sel++] = i;
                    }
                }
            }
            nlevels = load_levels(wiwads[iwad].levels);
            if (nlevels <= 0) {
                message("Level list not found.");
                break; /* Back to the IWAD selection */
            }
            to_iwad = 0;
            for (;;) {
                build_level_lines();
                lvl_pick = pick_list(TEXT_TITLE_SINGLE_LEVEL,
                                     level_lines, nlevels + 1, NULL);
                if (lvl_pick == PICK_QUIT) {
                    return 1;
                }
                if (lvl_pick == PICK_BACK) {
                    /* Back to the PWAD selection if there are
                       PWADs, to the IWAD selection otherwise. */
                    if (npwads > 0) {
                        break;
                    }
                    to_iwad = 1;
                    break;
                }
                if (lvl_pick == 0) {
                    /* "No level": run with just the IWAD and the
                       PWADs, without -warp or -skill. */
                    lvl = -1;
                    run = 1;
                    break;
                }
                lvl = lvl_pick - 1;
                skill_pick = pick_list(TEXT_TITLE_SINGLE_SKILL,
                                       skill_lines, NSKILLS, NULL);
                if (skill_pick == PICK_QUIT) {
                    return 1;
                }
                if (skill_pick == PICK_BACK) {
                    continue; /* Back to the level selection */
                }
                skill = skill_pick;
                run = 1;
                break;
            }
            if (run || to_iwad) {
                break;
            }
        }
        if (run) {
            break;
        }
    }

    if (!load_launch_exe(exe, (int)sizeof(exe))) {
        strcpy(exe, "FDOOM.EXE");
    }
    if (!file_exists(exe)) {
        message("Executable not found.");
        return 0;
    }
    if (is_game_exe(exe)) {
        save_launch_exe(exe);
    }
    if (launcher_cmd_len(exe, iwad_path, lvl, skill) > MAX_CMD_LEN) {
        /* The command line would not fit: abort instead of dropping
           arguments the game needs to run the level the user picked. */
        char msg[128];
        sprintf(msg,
                "Command line too long (%d of %d characters used). "
                "Disable some options.",
                launcher_cmd_len(exe, iwad_path, lvl, skill),
                MAX_CMD_LEN);
        message(msg);
        return 0;
    }
    build_command(exe, cmd);
    append_cmd(cmd, " -iwad %s", iwad_path);
    for (i = 0; i < npwad_sel; i++) {
        /* -file takes several WADs separated by a single space. */
        if (i == 0) {
            append_cmd(cmd, " -file %s%s", WAD_DIR,
                       pwad_names[pwad_sel[i]]);
        } else {
            append_cmd(cmd, " %s%s", WAD_DIR,
                       pwad_names[pwad_sel[i]]);
        }
    }
    if (lvl >= 0) {
        if (level_commercial) {
            /* Commercial IWADs: -warp takes the map number only. */
            append_cmd(cmd, " -warp %d", level_map[lvl]);
        } else {
            /* The others: -warp takes the episode and the map. */
            append_cmd(cmd, " -warp %d %d", level_ep[lvl], level_map[lvl]);
        }
        append_cmd(cmd, " -skill %d", skill + 1);
    }
    clear_screen();
    (void)system(cmd);
    return 0;
}
