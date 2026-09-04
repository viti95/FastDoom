/*
 * OPTIONS.C - FastDoom text mode launcher, command line options
 *
 * The options that can be passed to the game executables (taken
 * from the FastDoom source, d_main.c and friends), the
 * FDSTART.CFG file where the selection is stored, and the options
 * menu.
 *
 * There are two kinds of options: the simple flags (like -fast)
 * and the ones that take a value (like -limitram 8192). The value
 * options ask for their value with a small prompt when they are
 * enabled.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <i86.h>

#include "options.h"
#include "texts.h"
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
    { "-forceSound",    "Skip sound hardware checks" },
    /*
     * The rest are the options that take no value (flags) and the
     * ones that take a value (def_value != NULL; the value is
     * asked with a prompt when the option is enabled).
     */
    /* Bit depth and frame buffer (VBE 2.0 modes). */
    { "-8bpp",         "Force 8 bits per pixel" },
    { "-15bpp",        "Force 15 bits per pixel" },
    { "-16bpp",        "Force 16 bits per pixel" },
    { "-24bpp",        "Force 24 bits per pixel" },
    { "-32bpp",        "Force 32 bits per pixel" },
    { "-noLFB",        "Do not use the linear frame buffer" },
    { "-scale",        "Scale the output (width height)", "" },
    /* Video mode specific. */
    { "-cga",          "Use CGA graphics (text mode)" },
    { "-palette1",     "Use the CGA palette 1" },
    { "-snow",         "Enable the CGA snow effect" },
    { "-hercmap",      "Draw the automap in Hercules mode" },
    { "-fixDAC",       "Fix the VGA DAC colors" },
    { "-pagefix",      "Fix the text mode page flicker" },
    { "-term",         "Enable the VT100 serial terminal" },
    /* Sound and input. */
    { "-nomouse",      "Disable the mouse" },
    { "-nomusic",      "Disable music" },
    { "-nosfx",        "Disable sound effects" },
    { "-nosound",      "Disable all sound" },
    /* Back to the default renderings and behaviors. */
    { "-defSpan",      "Default floor and ceiling rendering" },
    { "-defWall",      "Default wall rendering" },
    { "-defSprite",    "Default sprite rendering" },
    { "-defPSprite",   "Default weapon sprite rendering" },
    { "-defSky",       "Default sky rendering" },
    { "-defInv",       "Default invisible sprite rendering" },
    { "-far",          "Do not draw very distant sprites" },
    { "-stereo",       "Stereo sound" },
    { "-melt",         "Enable screen melt transitions" },
    { "-fastbus",      "Assume a fast (non ISA) bus" },
    { "-novsync",      "Do not wait for vertical sync" },
    { "-capped",       "Cap the framerate" },
    { "-nofps",        "Do not show the FPS counter" },
    /* Memory. */
    { "-limitram",     "Limit the memory heap (KB)", "" },
    { "-freeram",      "Leave this much RAM free (KB)", "" },
    /* Game mode and configuration. */
    { "-complevel",    "Compatibility level (2, 3 or 4)", "" },
    { "-gamemode",     "Game mode", "" },
    { "-gamemission",  "Game mission (doom, doom2, tnt, plutonia)", "" },
    { "-config",       "Configuration file", "" },
    { "-size",         "Screen size (3-12)", "" },
    { "-sbk",          "SoundBlaster OPL config file (SBK)", "" },
    /* Demos, benchmark and saved games. */
    { "-episode",      "Start episode (1-4)", "" },
    { "-record",       "Record a demo (name)", "" },
    { "-playdemo",     "Play a demo (name)", "" },
    { "-timedemo",     "Time a demo (name)", "" },
    { "-maxdemo",      "Maximum demo size (KB)", "" },
    { "-benchmark",    "Benchmark (single <demo> or file <demo> <file>)", "" },
    { "-loadgame",     "Load a saved game (1-9)", "" },
    { "-advanced",     "Run the advanced benchmark loop" }
};

/*
 * The config file: the first line is "EXE=<executable>", the saved
 * launch target (optional); the remaining lines are option names
 * (one per line) passed to the game executables, the value ones
 * optionally followed by their value ("-limitram 8192").
 */
#define OPTS_FILE "FDSTART.CFG"

/* The prefix of the launch target line. */
#define EXE_KEY "EXE="

/* Max length of the saved executable name. */
#define LAUNCH_MAX 64

/* The enabled options (one byte per entry of opts), the value of
   the value options and the saved launch target. */
static char opt_enabled[NUMOPTS];
static char opt_value[NUMOPTS][OPT_VALUE_MAX + 1];
static char launch_exe[LAUNCH_MAX];

/*
 * Loads the config file into opt_enabled, opt_value and
 * launch_exe. Lines that match an option name enable it (optionally
 * setting its value), the "EXE=" line sets the launch target,
 * everything else is ignored.
 */
void load_options(void)
{
    FILE *f;
    char line[80];
    int i;

    memset(opt_enabled, 0, NUMOPTS);
    memset(opt_value, 0, NUMOPTS * (OPT_VALUE_MAX + 1));
    launch_exe[0] = '\0';
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
        if (strncmp(line, EXE_KEY, strlen(EXE_KEY)) == 0) {
            char *name = line + strlen(EXE_KEY);

            len = (int)strlen(name);
            if (len >= LAUNCH_MAX) {
                len = LAUNCH_MAX - 1;
            }
            memcpy(launch_exe, name, (size_t)len);
            launch_exe[len] = '\0';
        } else {
            for (i = 0; i < NUMOPTS; i++) {
                int alen = (int)strlen(opts[i].arg);
                char *rest;

                if (strncmp(line, opts[i].arg, (size_t)alen) != 0) {
                    continue;
                }
                rest = line + alen;
                if (*rest == '\0') {
                    /* Just the option name (no value). */
                    opt_enabled[i] = 1;
                    break;
                }
                if (*rest == ' ' && opts[i].def_value != NULL) {
                    /* The option name followed by its value. */
                    int vlen;

                    while (rest[1] == ' ') {
                        rest++;
                    }
                    vlen = (int)strlen(rest + 1);
                    if (vlen > OPT_VALUE_MAX) {
                        vlen = OPT_VALUE_MAX;
                    }
                    memcpy(opt_value[i], rest + 1, (size_t)vlen);
                    opt_value[i][vlen] = '\0';
                    opt_enabled[i] = 1;
                    break;
                }
                /* The line starts with this option name but is a
                   longer one ("-pentium" vs "-pentiumMMX"): try
                   the next option. */
            }
        }
    }
    fclose(f);
}

/*
 * Saves the config file: the launch target line (if any) followed
 * by the enabled options, one per line, the value ones with their
 * value.
 */
void save_options(void)
{
    FILE *f;
    int i;

    f = fopen(OPTS_FILE, "w");
    if (f == NULL) {
        return;
    }
    if (launch_exe[0] != '\0') {
        fprintf(f, "%s%s\n", EXE_KEY, launch_exe);
    }
    for (i = 0; i < NUMOPTS; i++) {
        if (opt_enabled[i]) {
            fprintf(f, "%s", opts[i].arg);
            if (opts[i].def_value != NULL) {
                fprintf(f, " %s", opt_value[i]);
            }
            fprintf(f, "\n");
        }
    }
    fclose(f);
}

/*
 * Saves the name of an executable as the launch target, rewriting
 * the config file (the current options are kept).
 */
void save_launch_exe(const char *exe)
{
    int len = (int)strlen(exe);

    load_options();
    if (len >= LAUNCH_MAX) {
        len = LAUNCH_MAX - 1;
    }
    memcpy(launch_exe, exe, (size_t)len);
    launch_exe[len] = '\0';
    save_options();
}

/*
 * Loads the saved executable name into exe (size bytes). Returns
 * 1 on success, 0 if there is no saved executable.
 */
int load_launch_exe(char *exe, int size)
{
    int len;

    load_options();
    len = (int)strlen(launch_exe);
    if (len == 0) {
        return 0;
    }
    if (len >= size) {
        len = size - 1;
    }
    memcpy(exe, launch_exe, (size_t)len);
    exe[len] = '\0';
    return 1;
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
 * followed by the enabled options (game executables only), the
 * value ones with their value.
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
        int add = 1 + (int)strlen(opts[i].arg);

        if (!opt_enabled[i]) {
            continue;
        }
        if (opts[i].def_value != NULL && opt_value[i][0] != '\0') {
            add += 1 + (int)strlen(opt_value[i]);
        }
        if (strlen(cmd) + add <= MAX_CMD_LEN) {
            strcat(cmd, " ");
            strcat(cmd, opts[i].arg);
            if (opts[i].def_value != NULL && opt_value[i][0] != '\0') {
                strcat(cmd, " ");
                strcat(cmd, opt_value[i]);
            }
        }
    }
}

/*
 * Returns the length of the command line build_command() would
 * produce for exe with the enabled options, without applying the
 * MAX_CMD_LEN limit, so the caller can check that it fits.
 */
int command_length(const char *exe)
{
    int i;
    int len = (int)strlen(exe);

    if (!is_game_exe(exe)) {
        return len;
    }
    load_options();
    for (i = 0; i < NUMOPTS; i++) {
        if (opt_enabled[i]) {
            len += 1 + (int)strlen(opts[i].arg);
            if (opts[i].def_value != NULL && opt_value[i][0] != '\0') {
                len += 1 + (int)strlen(opt_value[i]);
            }
        }
    }
    return len;
}

/*
 * Options menu: toggles the command line parameters that are
 * passed to the game executables. The selection is stored in
 * FDSTART.CFG on exit; S saves and closes the menu.
 *
 * Returns OM_BACK, OM_QUIT or OM_SAVED.
 */
/* Rows of options that fit between the header box and the footer:
   MENU_FIRST_ROW .. 21, row 22 is blank and row 23 the footer. */
#define OPTS_ROWS 17

/* The row where the value prompt is shown (the footer row, which
   it replaces while the prompt is active). */
#define VALUE_PROMPT_ROW 23

/*
 * Moves the cursor to the given row (BIOS video service, column 0),
 * without scrolling the screen.
 */
static void goto_row(int row)
{
    union REGS regs;

    regs.h.ah = 0x02; /* Move cursor */
    regs.h.bh = 0x00; /* Page 0 */
    regs.h.dh = (unsigned char)row;
    regs.h.dl = 0x00; /* Column 0 */

    int86(0x10, &regs, &regs);
}

/*
 * Asks for the value of an option that takes one. The saved value
 * is pre-filled (empty if none); printable characters are
 * appended, Backspace removes from the end, Enter accepts and Esc
 * cancels. The accepted value is stored in opt_value. Returns 1 on
 * accept, 0 on cancel.
 *
 * The prompt line has no trailing newline, so it must be flushed
 * explicitly after being printed: the console stdio only flushes
 * on newlines, and without the flush the prompt would stay in the
 * output buffer (and be shown later, at a random spot). The line
 * is padded with spaces up to the full width so that no stale
 * characters are left when the text gets shorter.
 */
static int value_prompt(int i)
{
    char buf[OPT_VALUE_MAX + 1];
    char line[81];
    int len;  /* length of buf (the value being edited) */
    int llen; /* length of the prompt line */
    int c;

    if (opt_value[i][0] != '\0') {
        strcpy(buf, opt_value[i]);
    } else {
        buf[0] = '\0';
    }
    len = (int)strlen(buf);

    for (;;) {
        /* The "_" placeholder is only shown with an empty value,
           like a cursor; once the user types something it goes
           away. */
        if (buf[0] == '\0') {
            sprintf(line, "  %s = _  (Enter ok, Esc cancel)",
                    opts[i].arg);
        } else {
            sprintf(line, "  %s = %s  (Enter ok, Esc cancel)",
                    opts[i].arg, buf);
        }
        llen = (int)strlen(line);
        while (llen < 80) {
            line[llen++] = ' ';
        }
        line[80] = '\0';
        goto_row(VALUE_PROMPT_ROW);
        printf("%s", line);
        fflush(stdout);
        c = getch();
        if (c == -1 || c == 0x1B) {
            return 0;
        }
        if (c == '\r') {
            strcpy(opt_value[i], buf);
            return 1;
        }
        if (c == 0 || c == 0xE0 || c == 0xE1) {
            /* Extended key (0, 0xE0 or 0xE1 prefix): discard the
               scan code. */
            (void)getch();
            continue;
        }
        if (c == 8 || c == 0x7F) {
            if (len > 0) {
                buf[--len] = '\0';
            }
            continue;
        }
        if (c >= 0x20 && c < 0x7F && len < OPT_VALUE_MAX) {
            buf[len++] = (char)c;
            buf[len] = '\0';
        }
    }
}

int options_menu(void)
{
    int top = 0;
    int sel = 0;
    int dirty = 1;
    int i;
    int c;
    /* One shot message shown under the list after the value prompt
       returns, so the user knows what happened (and with which
       value). notice_buf holds messages built with sprintf. */
    char notice_buf[80];
    const char *notice = NULL;

    load_options();

    for (;;) {
        if (dirty) {
            dirty = 0;
            draw_menu_top(TEXT_TITLE_OPTIONS);
            for (i = top; i < NUMOPTS && (i - top) < OPTS_ROWS; i++) {
                printf("%s", i == sel ? " ->" : "   ");
                printf(" %s %-16s %s",
                       opt_enabled[i] ? "[X]" : "[ ]",
                       opts[i].arg, opts[i].desc);
                if (opts[i].def_value != NULL) {
                    /* Value options: show the current value when
                       enabled, a hint otherwise. */
                    if (opt_enabled[i] && opt_value[i][0] != '\0') {
                        printf(" (%s)", opt_value[i]);
                    } else {
                        printf(" (value)");
                    }
                }
                printf("\n");
            }
            if (notice != NULL) {
                printf("  %s\n", notice);
            }
            print_bottom_row(MENU_FIRST_ROW + (i - top) + (notice != NULL),
                             "  Enter/Space toggle, S save, PgUp/PgDn scroll, Esc/Left back, Q quit.");
        }
        c = getch();
        notice = NULL; /* One shot: cleared once the user presses a key */
        if (c == -1 || c == 0x1B) {
            save_options();
            return OM_BACK;
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
                case KEY_LEFT:
                    save_options();
                    return OM_BACK;
                case KEY_PGUP:
                    top -= OPTS_ROWS;
                    if (top < 0) {
                        top = 0;
                    }
                    if (sel < top) {
                        sel = top;
                    }
                    if (sel >= top + OPTS_ROWS) {
                        sel = top + OPTS_ROWS - 1;
                    }
                    dirty = 1;
                    break;
                case KEY_PGDN:
                    top += OPTS_ROWS;
                    if (top > NUMOPTS - OPTS_ROWS) {
                        top = NUMOPTS - OPTS_ROWS;
                    }
                    if (sel < top) {
                        sel = top;
                    }
                    if (sel >= top + OPTS_ROWS) {
                        sel = top + OPTS_ROWS - 1;
                    }
                    dirty = 1;
                    break;
                case KEY_HOME:
                    top = 0;
                    if (sel >= OPTS_ROWS) {
                        sel = OPTS_ROWS - 1;
                    }
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
            return OM_QUIT;
        }
        if (c == 'S') {
            save_options();
            return OM_SAVED;
        }
        if (c == ' ' || c == '\r') {
            if (opt_enabled[sel]) {
                opt_enabled[sel] = 0;
            } else if (opts[sel].def_value != NULL) {
                /* The value options ask for their value and are
                   only enabled with a non empty one. */
                if (value_prompt(sel)) {
                    if (opt_value[sel][0] != '\0') {
                        opt_enabled[sel] = 1;
                        sprintf(notice_buf, "%s enabled, value: %s",
                                opts[sel].arg, opt_value[sel]);
                    } else {
                        sprintf(notice_buf,
                                "%s needs a non empty value: not enabled",
                                opts[sel].arg);
                    }
                } else {
                    sprintf(notice_buf, "%s not enabled (cancelled)",
                            opts[sel].arg);
                }
                notice = notice_buf;
            } else {
                opt_enabled[sel] = 1;
            }
            dirty = 1;
        }
    }
}
