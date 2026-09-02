/*
 * OPTIONS.C - FastDoom text mode launcher, command line options
 *
 * The options that can be passed to the game executables (taken
 * from the FastDoom source, d_main.c), the FDSTART.CFG file where
 * the selection is stored, and the options menu.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>

#include "options.h"
#include "screen.h"
#include "keys.h"

const opt_t opts[NUMOPTS] = {
    { "-high",          "Force high detail graphics" },
    { "-low",           "Force low detail graphics" },
    { "-potato",        "Force potato detail graphics" },
    { "-respawn",       "Respawn killed enemies" },
    { "-fast",          "Make enemies faster" },
    { "-disabledemo",   "Disable attract mode demos" },
    { "-386sx",         "CPU: Intel 386SX" },
    { "-386dx",         "CPU: Intel 386DX" },
    { "-i486",          "CPU: Intel 486" },
    { "-umc486",        "CPU: UMC Green 486" },
    { "-pentium",       "CPU: Intel Pentium P5/P54C" },
    { "-pentiumP54CS",  "CPU: Intel Pentium P54CS" },
    { "-pentiumMMX",    "CPU: Intel Pentium MMX" },
    { "-pentiumII",     "CPU: Intel Pentium II" },
    { "-k5",            "CPU: AMD K5" },
    { "-k6",            "CPU: AMD K6" },
    { "-cy386",         "CPU: Cyrix 386 DLC" },
    { "-cy486",         "CPU: Cyrix 486" },
    { "-cy5x86",        "CPU: Cyrix 5x86" },
    { "-cy6x86",        "CPU: Cyrix 6x86" },
    { "-cy6x86mx",      "CPU: Cyrix 6x86 MX" },
    { "-winchip",       "CPU: IDT WinChip" },
    { "-mp6",           "CPU: Rise MP6" },
    { "-flatSpan",      "Simpler floor and ceiling rendering" },
    { "-flatterSpan",   "Even simpler floor and ceiling rendering" },
    { "-flatWall",      "Simpler wall rendering" },
    { "-flatterWall",   "Even simpler wall rendering" },
    { "-flatSprite",    "Simpler sprite rendering" },
    { "-flatterSprites","Even simpler sprite rendering" },
    { "-flatPSprite",   "Simpler weapon sprite rendering" },
    { "-flatterPSprites","Even simpler weapon sprite rendering" },
    { "-flatsky",       "Simpler sky rendering" },
    { "-flatInv",       "Draw invisible sprites flat" },
    { "-flatsaturn",    "Draw invisible sprites flat (Saturn)" },
    { "-saturn",        "Draw invisible sprites (Saturn)" },
    { "-translucent",   "Draw invisible sprites translucent" },
    { "-near",          "Draw very distant sprites" },
    { "-mono",          "Monaural sound" },
    { "-nomelt",        "Disable screen melt transitions" },
    { "-slowbus",       "Assume a slow (ISA) bus" },
    { "-vsync",         "Wait for vertical sync" },
    { "-uncapped",      "Uncap the framerate" },
    { "-preload",       "Preload all WAD lumps in RAM" },
    { "-xt",            "XT compatibility mode" },
    { "-csv",           "Write CSV timing output" },
    { "-fps",           "Show FPS counter" },
    { "-debugCard2",    "Show 2D card FPS counter" },
    { "-debugCard4",    "Show 4D card FPS counter" },
    { "-reverseStereo", "Swap stereo sound channels" },
    { "-forceSound",    "Skip sound hardware checks" }
};

/* File where the selected options are stored. */
#define OPTS_FILE "FDSTART.CFG"

/* The enabled options (one byte per entry of opts). */
static char opt_enabled[NUMOPTS];

/*
 * Loads the selected options from the config file. Lines that
 * match an option name enable it, everything else is ignored.
 */
void load_options(void)
{
    FILE *f;
    char line[40];
    int i;

    memset(opt_enabled, 0, NUMOPTS);
    f = fopen(OPTS_FILE, "r");
    if (f == NULL) {
        return;
    }
    while (fgets(line, sizeof(line), f) != NULL) {
        int len = (int)strlen(line);

        while (len > 0 &&
               (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        for (i = 0; i < NUMOPTS; i++) {
            if (strcmp(line, opts[i].arg) == 0) {
                opt_enabled[i] = 1;
            }
        }
    }
    fclose(f);
}

/*
 * Saves the selected options to the config file, one per line.
 */
void save_options(void)
{
    FILE *f;
    int i;

    f = fopen(OPTS_FILE, "w");
    if (f == NULL) {
        return;
    }
    for (i = 0; i < NUMOPTS; i++) {
        if (opt_enabled[i]) {
            fprintf(f, "%s\n", opts[i].arg);
        }
    }
    fclose(f);
}

/*
 * Returns 1 if the executable is a game one (FDOOM*.EXE or
 * FDM*.EXE), the only ones that take these options.
 */
int is_game_exe(const char *exe)
{
    return exe[0] == 'F' && exe[1] == 'D' &&
           (exe[2] == 'O' || exe[2] == 'M');
}

/*
 * Builds the command line to run an executable: the file name
 * followed by the enabled options (game executables only).
 */
void build_command(const char *exe, char *cmd)
{
    int i;

    cmd[0] = '\0';
    strcat(cmd, exe);
    if (!is_game_exe(exe)) {
        return;
    }
    load_options();
    for (i = 0; i < NUMOPTS; i++) {
        if (opt_enabled[i] &&
            strlen(cmd) + strlen(opts[i].arg) + 2 <= MAX_CMD_LEN) {
            strcat(cmd, " ");
            strcat(cmd, opts[i].arg);
        }
    }
}

/*
 * Options menu: toggles the command line parameters that are
 * passed to the game executables. The selection is stored in
 * FDSTART.CFG on exit (and with S at any time).
 *
 * Returns 1 if the launcher should quit, 0 to go back.
 */
#define OPTS_ROWS 22

int options_menu(void)
{
    int top = 0;
    int sel = 0;
    int dirty = 1;
    int i;
    int c;

    load_options();

    for (;;) {
        if (dirty) {
            dirty = 0;
            clear_screen();
            for (i = top; i < NUMOPTS && (i - top) < OPTS_ROWS; i++) {
                printf("%s", i == sel ? " ->" : "   ");
                printf(" %s %-15s %s\n",
                       opt_enabled[i] ? "[X]" : "[ ]",
                       opts[i].arg, opts[i].desc);
            }
            print_bottom_row(i - top,
                             "  Enter/Space toggle, S save, PgUp/PgDn scroll, Esc back, Q quit.");
        }
        c = getch();
        if (c == -1 || c == 0x1B) {
            save_options();
            return 0;
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
                    if (sel < NUMOPTS - 1) {
                        sel++;
                        if (sel >= top + OPTS_ROWS) {
                            top = sel - OPTS_ROWS + 1;
                        }
                        dirty = 1;
                    }
                    break;
                case KEY_PGUP:
                    top -= OPTS_ROWS;
                    if (top < 0) {
                        top = 0;
                    }
                    if (sel < top) {
                        sel = top;
                    }
                    dirty = 1;
                    break;
                case KEY_PGDN:
                    top += OPTS_ROWS;
                    if (top > NUMOPTS - OPTS_ROWS) {
                        top = NUMOPTS - OPTS_ROWS;
                    }
                    if (sel >= top + OPTS_ROWS) {
                        sel = top + OPTS_ROWS - 1;
                    }
                    dirty = 1;
                    break;
                case KEY_HOME:
                    top = 0;
                    dirty = 1;
                    break;
                case KEY_END:
                    top = NUMOPTS - OPTS_ROWS;
                    if (sel < top) {
                        sel = top;
                    } else if (sel >= top + OPTS_ROWS) {
                        sel = top + OPTS_ROWS - 1;
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
            save_options();
            return 1;
        }
        if (c == 'S') {
            save_options();
            message("Options saved to FDSTART.CFG.");
            dirty = 1;
        }
        if (c == ' ' || c == '\r') {
            opt_enabled[sel] = !opt_enabled[sel];
            dirty = 1;
        }
    }
}
