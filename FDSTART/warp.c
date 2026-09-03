/*
 * WARP.C - FastDoom text mode launcher, the single level launcher
 *
 * Lets the user pick the IWAD (-iwad), a single level (-warp) and
 * the skill (-skill), and runs the saved game executable with them.
 *
 * The IWADs offered are the ones FastDoom knows about (the same
 * list as the iwads[] table in d_main.c); the file is looked up in
 * the IWAD sub-directory first, then in the current directory.
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
#include <conio.h>

#include "warp.h"
#include "menu.h"
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
    const char *levels;
} wiwad_t;

#define NIWADS 8

static const wiwad_t wiwads[NIWADS] = {
    { "doom1.wad",    "LEVELS\\doom1.txt" },
    { "doom.wad",     "LEVELS\\doom.txt" },
    { "doomu.wad",    "LEVELS\\doomu.txt" },
    { "doom2.wad",    "LEVELS\\doom2.txt" },
    { "plutonia.wad", "LEVELS\\plutonia.txt" },
    { "tnt.wad",      "LEVELS\\tnt.txt" },
    { "freedm1.wad",  "LEVELS\\freedm1.txt" },
    { "freedm2.wad",  "LEVELS\\freedm2.txt" }
};

/* Where the IWAD files are looked up, in order. */
#define IWAD_DIR "IWAD\\"

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
static char iwad_line_buf[NIWADS][32];
static char *iwad_lines[NIWADS];

/*
 * Returns 1 if the IWAD file exists in the current directory or in
 * the IWAD sub-directory.
 */
static int iwad_exists(const char *name)
{
    char path[32];

    if (file_exists(name)) {
        return 1;
    }
    strcpy(path, IWAD_DIR);
    strcat(path, name);
    return file_exists(path);
}

/*
 * Finds the IWAD file, copying the name to pass to -iwad (the plain
 * file name if it is in the current directory, or the sub-directory
 * path if it is in IWAD). Returns 1 on success, 0 if not found.
 */
static int find_iwad(const char *name, char *path)
{
    char full[32];

    if (file_exists(name)) {
        strcpy(path, name);
        return 1;
    }
    strcpy(full, IWAD_DIR);
    strcat(full, name);
    if (file_exists(full)) {
        strcpy(path, full);
        return 1;
    }
    return 0;
}

/* Maps each shown line to its entry in the wiwads table. */
static int iwad_index[NIWADS];

/*
 * Builds the IWAD selection lines from the IWADs that exist (in
 * the current directory or in the IWAD sub-directory). Returns
 * the number of available IWADs.
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
        strcpy(iwad_line_buf[n], wiwads[i].name);
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
static char level_line_buf[MAX_LEVELS][MAX_LEVEL_NAME + 8];
static char *level_lines[MAX_LEVELS];

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
 * Builds the level selection lines from the parsed list.
 */
static void build_level_lines(void)
{
    int i;

    for (i = 0; i < nlevels; i++) {
        level_lines[i] = level_line_buf[i];
        if (level_commercial) {
            sprintf(level_line_buf[i], "MAP%02d  %s",
                    level_map[i], level_name[i]);
        } else {
            sprintf(level_line_buf[i], "E%dM%d  %s",
                    level_ep[i], level_map[i], level_name[i]);
        }
    }
}

/*
 * Shows a scrolling list of lines with a cursor. Returns the
 * selected line, PICK_BACK if the user went back or PICK_QUIT if
 * the user pressed Q.
 */
static int pick_list(const char *title, char *lines[], int n)
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
                printf(" %s\n", lines[i]);
            }
            print_bottom_row(MENU_FIRST_ROW + (i - top),
                             "  Up/Down to move, Enter/Right to choose, Esc/Left to go back, Q to quit.");
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
                    if (sel >= top + LIST_ROWS) {
                        sel = top + LIST_ROWS - 1;
                    }
                    dirty = 1;
                    break;
                case KEY_HOME:
                    top = 0;
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
        if (c == '\r') {
            return sel;
        }
    }
}

/*
 * Appends a formatted string to the command line, only if the
 * result stays within MAX_CMD_LEN (DOS command lines are short).
 */
static void append_cmd(char *cmd, const char *fmt, ...)
{
    char part[80];
    va_list ap;

    va_start(ap, fmt);
    (void)vsprintf(part, fmt, ap);
    va_end(ap);
    if (strlen(cmd) + strlen(part) + 1 <= MAX_CMD_LEN) {
        strcat(cmd, part);
    }
}

/*
 * The single level launcher: pick the IWAD, the level and the
 * skill, then run the saved executable (or FDOOM.EXE if none is
 * saved) with -iwad, -warp and -skill, plus the saved command
 * line options. Never returns once the game is run.
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
    int run = 0;

    for (;;) {
        niwads = build_iwad_lines();
        if (niwads == 0) {
            message("No IWAD found. Put a .WAD file in the IWAD directory.");
            return 0;
        }
        iwad = pick_list("FastDoom launcher (Single level, IWAD)",
                         iwad_lines, niwads);
        if (iwad == PICK_QUIT) {
            return 1;
        }
        if (iwad == PICK_BACK) {
            return 0;
        }
        iwad = iwad_index[iwad];
        if (!find_iwad(wiwads[iwad].name, iwad_path)) {
            message("IWAD not found.");
            continue;
        }
        nlevels = load_levels(wiwads[iwad].levels);
        if (nlevels <= 0) {
            message("Level list not found.");
            continue;
        }
        for (;;) {
            build_level_lines();
            lvl = pick_list("FastDoom launcher (Single level, level)",
                            level_lines, nlevels);
            if (lvl == PICK_QUIT) {
                return 1;
            }
            if (lvl == PICK_BACK) {
                break; /* Back to the IWAD selection */
            }
            skill = pick_list("FastDoom launcher (Single level, skill)",
                              skill_lines, NSKILLS);
            if (skill == PICK_QUIT) {
                return 1;
            }
            if (skill == PICK_BACK) {
                continue; /* Back to the level selection */
            }
            run = 1;
            break;
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
    build_command(exe, cmd);
    append_cmd(cmd, " -iwad %s", iwad_path);
    if (level_commercial) {
        /* Commercial IWADs: -warp takes the map number only. */
        append_cmd(cmd, " -warp %d", level_map[lvl]);
    } else {
        /* The others: -warp takes the episode and the map. */
        append_cmd(cmd, " -warp %d %d", level_ep[lvl], level_map[lvl]);
    }
    append_cmd(cmd, " -skill %d", skill + 1);
    clear_screen();
    (void)system(cmd);
    exit(0);
}
