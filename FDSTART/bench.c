/*
 * BENCH.C - FastDoom text mode launcher, the benchmark launcher
 *
 * Lets the user pick the IWAD (-iwad), a benchmark file
 * (BENCH\*.BNC), the demo and the executable, and runs it with
 * -benchmark file (plus -advanced for the frametimes loop). When
 * the game exits, goes back to the main menu.
 *
 * The IWADs are the .WAD files in the current directory, skipping
 * the video mode WADs (MODE* and FONT*). The benchmark files are
 * the *.BNC files in the BENCH directory (the same ones the old
 * FDBENCH used). The executables are the FDOOM*.EXE and FDM*.EXE
 * files in the current directory.
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
#include "texts.h"
#include "screen.h"
#include "util.h"
#include "options.h"
#include "keys.h"

/* The IWADs, benchmark files and executables (static, so the
   buffers live in BSS). 8.3 file names fit in NAME_LEN, and 128
   entries are plenty for the WADs, the .BNC files and the
   executables; the tables are kept small, the large model BSS is
   limited to 64k. */
#define MAX_ITEMS  128
#define NAME_LEN   16

static int nwads;
static char wad_names[MAX_ITEMS][NAME_LEN];
static char wad_line_buf[MAX_ITEMS][NAME_LEN + 8];
static char *wad_lines[MAX_ITEMS];

static int nbncs;
static char bnc_names[MAX_ITEMS][NAME_LEN];
static char bnc_line_buf[MAX_ITEMS][NAME_LEN + 8];
static char *bnc_lines[MAX_ITEMS];

static int nexes;
static char exe_names[MAX_ITEMS][NAME_LEN];
static char exe_line_buf[MAX_ITEMS][NAME_LEN + 8];
static char *exe_lines[MAX_ITEMS];

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
 * Finds files matching the pattern and stores their names in the
 * table. If skipModeFont is true, names containing "MODE" or
 * "FONT" are excluded (the video mode WADs, skipped in the IWAD
 * list). Returns the number of files found.
 */
static int find_files(const char *pattern, int skipModeFont,
                      char names[][NAME_LEN])
{
    struct find_t data;
    unsigned handle;
    int n = 0;

    handle = _dos_findfirst(pattern, 0, &data);
    while (handle == 0) {
        if (!(data.attrib & _A_SUBDIR) && n < MAX_ITEMS) {
            if (!skipModeFont ||
                (strstr(data.name, "MODE") == NULL &&
                 strstr(data.name, "FONT") == NULL)) {
                strncpy(names[n], data.name, NAME_LEN - 1);
                names[n][NAME_LEN - 1] = '\0';
                n++;
            }
        }
        handle = _dos_findnext(&data);
    }
    _dos_findclose(&data);
    return n;
}

/*
 * Scans the IWADs, the benchmark files and the executables, and
 * builds the selection lines for each of them.
 */
static void scan_bench_files(void)
{
    int i;

    nwads = find_files("*.WAD", 1, wad_names);
    for (i = 0; i < nwads; i++) {
        wad_lines[i] = wad_line_buf[i];
        strcpy(wad_line_buf[i], wad_names[i]);
    }

    nbncs = find_files("BENCH\\*.BNC", 0, bnc_names);
    for (i = 0; i < nbncs; i++) {
        bnc_lines[i] = bnc_line_buf[i];
        strcpy(bnc_line_buf[i], bnc_names[i]);
    }

    nexes = find_files("FDOOM*.EXE", 0, exe_names);
    i = find_files("FDM*.EXE", 0, exe_names + nexes);
    if (nexes + i > MAX_ITEMS) {
        i = MAX_ITEMS - nexes;
    }
    nexes += i;
    for (i = 0; i < nexes; i++) {
        exe_lines[i] = exe_line_buf[i];
        strcpy(exe_line_buf[i], exe_names[i]);
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
    if (nexes == 0) {
        message("No executable found. Put a FastDoom .EXE in the current directory.");
        return 0;
    }

    for (;;) {
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
                exe_names[exe], wad_names[wad], demo_lines[demo],
                bnc_names[bnc]);
    } else {
        sprintf(cmd, "%s -iwad %s -benchmark file %s BENCH\\%s",
                exe_names[exe], wad_names[wad], demo_lines[demo],
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

    clear_screen();
    (void)system(cmd);
    return 0;
}
