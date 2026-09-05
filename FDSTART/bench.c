/*
 * BENCH.C - FastDoom text mode launcher, the benchmark launcher
 *
 * Lets the user pick the IWAD (-iwad), a benchmark file
 * (BENCH\*.BNC), the demo and the executable, and runs it with
 * -benchmark file (plus -advanced for the frametimes loop). When
 * the game exits, goes back to the main menu.
 *
 * The IWADs are the ones FastDoom knows about (the same wiwads[]
 * list as the single level launcher). The benchmark files are the
 * *.BNC files in the BENCH directory (the same ones the old
 * FDBENCH used). The executables are the ones listed in the
 * launcher groups (groups.c).
 *
 * C89 port of the old FDBENCH.EXE (itself a C89 port of the
 * original FDBENCH.BAS); the command line is lowercased before
 * running, like the LCASE$ in the BASIC original.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <errno.h>

#include "bench.h"
#include "menu.h"
#include "groups.h"
#include "warp.h"
#include "texts.h"
#include "screen.h"
#include "util.h"
#include "options.h"
#include "keys.h"

/* The benchmark files (static, so the buffers live in BSS). 8.3
   file names fit in NAME_LEN, and 128 entries are plenty for the
   .BNC files; the tables are kept small, the large model BSS is
   limited to 64k. */
#define MAX_ITEMS  128
#define NAME_LEN   16

static int nbncs;
static char bnc_names[MAX_ITEMS][NAME_LEN];
static char *bnc_lines[MAX_ITEMS];

/* The IWAD selection lines, rebuilt on every pass: the ones from
   the wiwads list that exist in the current directory, as in the
   single level launcher. */
static int nwads;
static char wad_line_buf[NIWADS][64];
static char *wad_lines[NIWADS];
/* Maps each shown line to its entry in the wiwads table. */
static int wad_index[NIWADS];

/*
 * Builds the IWAD selection lines from the IWADs that exist in
 * the current directory. Returns the number of available IWADs.
 */
static int build_wad_lines(void)
{
    int n = 0;
    int i;

    for (i = 0; i < NIWADS; i++) {
        if (!file_exists(wiwads[i].name)) {
            continue;
        }
        wad_lines[n] = wad_line_buf[n];
        sprintf(wad_line_buf[n], "%s (%s)",
                wiwads[i].display, wiwads[i].name);
        wad_index[n] = i;
        n++;
    }
    return n;
}

/* The executables, one entry per item in the groups list
 * (groups.c), with the group item description shown after the
 * name. */
/* 48 is plenty for the items in the groups list and the longest
 * "name - description" line fits in 56 characters. */
#define MAX_EXES 48

static int nexes;
static const char *exe_names[MAX_EXES];
static char exe_line_buf[MAX_EXES][56];
static char *exe_lines[MAX_EXES];

/* The demo names, as in the old FDBENCH. */
static char *demo_lines[4] = {
    "demo1", "demo2", "demo3", "demo4"
};

/* The benchmark mode selection lines. */
static char *mode_lines[2] = {
    "Normal benchmark",
    "Advanced benchmark (frametimes)"
};

/*
 * Finds the benchmark files (BENCH\*.BNC) and stores their names
 * in the table. Returns the number of files found.
 */
static int find_bench_files(char names[][NAME_LEN])
{
    struct find_t data;
    unsigned handle;
    int n = 0;

    handle = _dos_findfirst("BENCH\\*.BNC", 0, &data);
    while (handle == 0) {
        if (!(data.attrib & _A_SUBDIR) && n < MAX_ITEMS) {
            strncpy(names[n], data.name, NAME_LEN - 1);
            names[n][NAME_LEN - 1] = '\0';
            n++;
        }
        handle = _dos_findnext(&data);
    }
    _dos_findclose(&data);
    return n;
}

/*
 * Scans the benchmark files and builds the selection lines for
 * them and for the executables (the launcher groups, in the group
 * menu order).
 */
static void scan_bench_files(void)
{
    int g;
    int i;

    nbncs = find_bench_files(bnc_names);
    for (i = 0; i < nbncs; i++) {
        bnc_lines[i] = bnc_names[i];
    }

    nexes = 0;
    for (g = 0; g < NGROUPS; g++) {
        for (i = 0; i < groups[g].count && nexes < MAX_EXES; i++) {
            exe_names[nexes] = groups[g].items[i].exe;
            exe_lines[nexes] = exe_line_buf[nexes];
            sprintf(exe_line_buf[nexes], "%s - %s",
                    groups[g].items[i].exe, groups[g].items[i].desc);
            nexes++;
        }
    }
}

/*
 * Converts a string to lowercase in place.
 */
static void to_lower(char *s)
{
    while (*s) {
        *s = (char)tolower((unsigned char)*s);
        s++;
    }
}

/*
 * The benchmark launcher: pick the IWAD, the benchmark file, the
 * demo, the executable and the mode (normal or advanced), then run
 * it with -iwad and -benchmark file (and -advanced, when the
 * advanced mode was picked). When the game exits, goes back to the
 * main menu.
 *
 * Returns 1 if the launcher should quit, 0 to go back to the main
 * menu.
 */
int bench_menu(void)
{
    char cmd[256];
    int pick;
    int wad = -1;
    int bnc = -1;
    int demo = -1;
    int exe = -1;
    int advanced = 0;
    int run = 0;

    scan_bench_files();
    if (nbncs == 0) {
        message("No benchmark file found. Put .BNC files in the BENCH directory.");
        return 0;
    }

    for (;;) {
        nwads = build_wad_lines();
        if (nwads == 0) {
            message("No IWAD found. Put a .WAD file in the FastDoom directory.");
            return 0;
        }
        pick = pick_list(TEXT_TITLE_BENCH_IWAD, wad_lines, nwads, NULL);
        if (pick == PICK_QUIT) {
            return 1;
        }
        if (pick == PICK_BACK) {
            return 0;
        }
        wad = pick;
        for (;;) {
            pick = pick_list(TEXT_TITLE_BENCH_FILE, bnc_lines, nbncs, NULL);
            if (pick == PICK_QUIT) {
                return 1;
            }
            if (pick == PICK_BACK) {
                break; /* Back to the IWAD selection */
            }
            bnc = pick;
            for (;;) {
                pick = pick_list(TEXT_TITLE_BENCH_DEMO, demo_lines, 4, NULL);
                if (pick == PICK_QUIT) {
                    return 1;
                }
                if (pick == PICK_BACK) {
                    break; /* Back to the benchmark file selection */
                }
                demo = pick;
                for (;;) {
                    pick = pick_list(TEXT_TITLE_BENCH_EXE, exe_lines, nexes, NULL);
                    if (pick == PICK_QUIT) {
                        return 1;
                    }
                    if (pick == PICK_BACK) {
                        break; /* Back to the demo selection */
                    }
                    exe = pick;
                    pick = pick_list(TEXT_TITLE_BENCH_MODE, mode_lines, 2, NULL);
                    if (pick == PICK_QUIT) {
                        return 1;
                    }
                    if (pick == PICK_BACK) {
                        break; /* Back to the executable selection */
                    }
                    advanced = pick;
                    run = 1;
                    break;
                }
                if (run) {
                    break;
                }
            }
            if (run) {
                break;
            }
        }
        if (run) {
            break;
        }
    }

    /* Build the command line: the executable with the -iwad and
       -benchmark file arguments (plus -advanced, when the
       advanced mode was picked). */
    if (advanced) {
        sprintf(cmd, "%s -iwad %s -benchmark file %s BENCH\\%s -advanced",
                exe_names[exe], wiwads[wad_index[wad]].name, demo_lines[demo],
                bnc_names[bnc]);
    } else {
        sprintf(cmd, "%s -iwad %s -benchmark file %s BENCH\\%s",
                exe_names[exe], wiwads[wad_index[wad]].name, demo_lines[demo],
                bnc_names[bnc]);
    }

    /* Lowercase the whole line, like LCASE$ in the BASIC original */
    to_lower(cmd);

    if ((int)strlen(cmd) > MAX_CMD_LEN) {
        /* The command line would not fit: abort instead of running
           the game with a broken command line. */
        char msg[128];

        sprintf(msg, "Command line too long (%d of %d characters used).",
                (int)strlen(cmd), MAX_CMD_LEN);
        message(msg);
        return 0;
    }

    if (!file_exists(exe_names[exe])) {
        message("Executable not found.");
        return 0;
    }

    clear_screen();
    (void)system(cmd);
    return 0;
}
