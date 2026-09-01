/*
 * FDBENCH.C - FastDoom benchmark launcher
 *
 * C89 port of FDBENCH.BAS.
 * Lets the user pick an IWAD, a BENCH\*.BNC benchmark file, a demo,
 * a FastDoom executable, and optional arguments, then writes the
 * resulting command line to BENCH2.BAT.
 *
 * Build as a 16-bit DOS executable with Open Watcom (see makefile).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <errno.h>
#include <conio.h>

#define MAX_ITEMS 255
#define NAME_LEN  32
#define LINE_LEN  128

/*
 * File table helper:
 * finds files matching a pattern and stores their names in the table.
 * If skipModeFont is true, names containing "MODE" or "FONT" are
 * excluded (used for the IWAD list, to skip the video mode WADs).
 */
static int findFiles(const char *pattern, int skipModeFont, char table[][NAME_LEN])
{
    struct find_t data;
    unsigned handle;
    int count = 0;

    errno = 0;
    handle = _dos_findfirst(pattern, 0, &data);
    while (!errno)
    {
        if (!(data.attrib & _A_SUBDIR) && count < MAX_ITEMS)
        {
            if (!skipModeFont ||
                (strstr(data.name, "MODE") == NULL &&
                 strstr(data.name, "FONT") == NULL))
            {
                strncpy(table[count], data.name, NAME_LEN - 1);
                table[count][NAME_LEN - 1] = '\0';
                count++;
            }
        }
        errno = 0;
        _dos_findnext(&data);
    }
    if (handle)
        _dos_findclose(&data);
    return count;
}

/* Clears the screen and prints the program banner. */
static void cleanScreen(void)
{
    system("CLS");
    printf("\n");
    printf("   ##########################################################################\n");
    printf("   #                                                                        #\n");
    printf("   #                           FASTDOOM BENCHMARK                           #\n");
    printf("   #                                                                        #\n");
    printf("   ##########################################################################\n");
    printf("\n");
}

/*
 * Checks the keyboard buffer for a pending 'Q'/'q' keypress and exits
 * the program if one is found (call between interactive steps, where
 * the program is not waiting for line input).
 */
static void checkQuit(void)
{
    int ch;

    while (kbhit())
    {
        ch = getch();
        if (ch == 'q' || ch == 'Q')
        {
            printf("\n     Quitting.\n");
            exit(0);
        }
    }
}

/*
 * Reads a line from the keyboard (equivalent of the BASIC INPUT statement).
 * If the user types just "Q" (or "q"), the program exits.
 */
static void readLine(char *buf, int maxlen)
{
    if (fgets(buf, maxlen, stdin) == (char *)NULL)
        buf[0] = '\0';
    buf[strcspn(buf, "\r\n")] = '\0';

    if ((buf[0] == 'q' || buf[0] == 'Q') && buf[1] == '\0')
    {
        printf("\n     Quitting.\n");
        exit(0);
    }
}

/* Reads a numeric option selection and validates it. */
static int readOption(const char *prompt, int total)
{
    char inputVal[LINE_LEN];
    int position;

    if (total == 0)
    {
        printf("\n     No files found.\n");
        return 0;
    }

    printf("\n%s", prompt);
    readLine(inputVal, sizeof(inputVal));
    position = atoi(inputVal);
    if (position < 1 || position > total)
    {
        printf("\n     Invalid selection, picking the first entry.\n");
        position = 1;
    }
    return position;
}

/* Prints the numbered list, pausing every 10 entries if pauseEvery is set. */
static void printMenu(char (*files)[NAME_LEN], int total, int pauseEvery)
{
    char inputVal[LINE_LEN];
    int position;

    for (position = 1; position <= total; position++)
    {
        printf("     %d) %s\n", position, files[position - 1]);
        if (pauseEvery > 0 && position % pauseEvery == 0)
        {
            printf("\n");
            printf("     Press any key to continue . . . ");
            readLine(inputVal, sizeof(inputVal));
            cleanScreen();
        }
    }
}

/* Converts a string to lowercase in place. */
static void toLower(char *s)
{
    while (*s)
    {
        *s = (char)tolower((unsigned char)*s);
        s++;
    }
}

int main(void)
{
    static char benchmarkWads[MAX_ITEMS][NAME_LEN];
    static char benchmarkFiles[MAX_ITEMS][NAME_LEN];
    static char benchmarkExecutables[MAX_ITEMS][NAME_LEN];

    char inputVal[LINE_LEN];
    char benchmarkDemo[LINE_LEN];
    char benchmarkOptions[32];
    char arguments[256];
    char text[512];
    FILE *fp;
    int wadTotal;
    int filesTotal;
    int executablesTotal;
    int position;
    int c;

    /* Flush keyboard */
    while (kbhit())
        (void)getch();

    cleanScreen();
    checkQuit();

    /* ---- Choose an IWAD ---- */
    printf("     Choose an IWAD (type Q to quit)\n");

    wadTotal = findFiles("*.WAD", 1, benchmarkWads);
    printMenu(benchmarkWads, wadTotal, 0);
    position = readOption("     Please enter option: ", wadTotal);
    if (position == 0)
        return 1;

    cleanScreen();
    checkQuit();

    /* ---- Choose a benchmark file ---- */
    printf("     Choose a benchmark (type Q to quit)\n");

    filesTotal = findFiles("BENCH\\*.BNC", 0, benchmarkFiles);
    printMenu(benchmarkFiles, filesTotal, 10);
    position = readOption("     Please enter option: ", filesTotal);
    if (position == 0)
        return 1;

    cleanScreen();
    checkQuit();

    /* ---- Choose a demo file ---- */
    printf("     Choose a demo file (or type any demo you want, Q to quit)\n");
    printf("\n");
    printf("      1) DEMO1\n");
    printf("      2) DEMO2\n");
    printf("      3) DEMO3\n");
    printf("      4) DEMO4\n");
    printf("\n");
    printf("     Please enter option: ");
    readLine(inputVal, sizeof(inputVal));

    if (strcmp(inputVal, "1") == 0)
        strcpy(benchmarkDemo, "demo1");
    else if (strcmp(inputVal, "2") == 0)
        strcpy(benchmarkDemo, "demo2");
    else if (strcmp(inputVal, "3") == 0)
        strcpy(benchmarkDemo, "demo3");
    else if (strcmp(inputVal, "4") == 0)
        strcpy(benchmarkDemo, "demo4");
    else
    {
        strncpy(benchmarkDemo, inputVal, sizeof(benchmarkDemo) - 1);
        benchmarkDemo[sizeof(benchmarkDemo) - 1] = '\0';
    }

    cleanScreen();
    checkQuit();

    /* ---- Choose a FastDoom executable ---- */
    printf("     Choose a FastDoom executable (type Q to quit)\n");

    executablesTotal = findFiles("FDOOM*.EXE", 0, benchmarkExecutables);
    {
        static char extra[MAX_ITEMS][NAME_LEN];
        int more = findFiles("FDM*.EXE", 0, extra);

        if (executablesTotal + more > MAX_ITEMS)
            more = MAX_ITEMS - executablesTotal;
        for (c = 0; c < more; c++)
            strcpy(benchmarkExecutables[executablesTotal + c], extra[c]);
        executablesTotal += more;
    }

    printMenu(benchmarkExecutables, executablesTotal, 10);
    position = readOption("     Please enter option: ", executablesTotal);
    if (position == 0)
        return 1;

    cleanScreen();
    checkQuit();

    /* ---- Choose additional options ---- */
    printf("     Choose additional options (type Q to quit)\n");
    printf("\n");
    printf("      A) Advanced benchmark (frametimes)\n");
    printf("\n");
    printf("     Please enter option: ");
    readLine(inputVal, sizeof(inputVal));

    if (inputVal[0] == 'A' || inputVal[0] == 'a')
        strcpy(benchmarkOptions, "-advanced");
    else
        benchmarkOptions[0] = '\0';

    /* ---- Build the command line and write BENCH2.BAT ---- */
    if (benchmarkOptions[0])
        sprintf(arguments,
                "-iwad %s -benchmark file %s BENCH\\%s %s",
                benchmarkWads[position - 1],
                benchmarkDemo,
                benchmarkFiles[position - 1],
                benchmarkOptions);
    else
        sprintf(arguments,
                "-iwad %s -benchmark file %s BENCH\\%s",
                benchmarkWads[position - 1],
                benchmarkDemo,
                benchmarkFiles[position - 1]);

    /* Lowercase the whole line, like LCASE$ in the BASIC original */
    strcpy(text, benchmarkExecutables[position - 1]);
    strcat(text, " ");
    strcat(text, arguments);
    toLower(text);

    fp = fopen("BENCH2.BAT", "w");
    if (fp == (FILE *)NULL)
    {
        printf("\n     Could not write BENCH2.BAT\n");
        return 1;
    }
    fputs(text, fp);
    fputc('\n', fp);
    fclose(fp);

    (void)system("CLS");
    return 0;
}
