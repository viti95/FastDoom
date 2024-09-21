#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef MAC
#include <dos.h>
#include <conio.h>
#endif

#include <stdarg.h>
#include "doomtype.h"
#include "fastmath.h"
#include "options.h"
#include "i_ibm.h"
#include "i_system.h"
#include "i_debug.h"

#define COLOURS 0x1A
#define COLS 80
#define ROWS 25

#if (DEBUG_ENABLED == 1)

int debug_to_screen = 0;

#if (DEBUG_FILE_ENABLED == 1)

FILE *f_log;

#endif

void I_Putchar(byte c);

#if (DEBUG_MDA_ENABLED == 1)
unsigned short *Scrn = (unsigned short *)0xB0000;
int Curx, Cury = 0;

void I_Clear()
{
    int i;

    Curx = Cury = 0;

    for (i = 0; i < ROWS * COLS; i++)
        I_Putchar(' ');
}

void I_Scroll(void)
{
    if (Cury >= ROWS)
    {
        int tmp;
        for (tmp = 0; tmp < (ROWS - 1) * COLS; tmp++)
        {
            Scrn[tmp] = Scrn[tmp + COLS];
        }
        for (tmp = (ROWS - 1) * COLS; tmp < ROWS * COLS; tmp++)
        {
            Scrn[tmp] = '\0';
        }

        Cury = ROWS - 1;
    }
}

void I_SetCursor(int x, int y) {
    Curx = x;
    Cury = y;
}

#endif

void I_Putchar(byte c)
{
// Output to MDA text mode, if you're lucky enough to have one
#if (DEBUG_MDA_ENABLED == 1)
    unsigned short *addr;
    if (c == '\t')
        Curx = ((Curx + 4) / 4) * 4;
    if (c == '\r')
        Curx = 0;
    if (c == '\n')
    {
        Curx = 0;
        Cury++;
    }
    I_Scroll();
    if (c == 0x08 && Curx != 0)
        Curx--;
    else if (c >= ' ')
    {
        addr = Scrn + (Cury * COLS + Curx);
        *addr = (COLOURS << 8) | c;
        Curx++;
    }
    if (Curx >= COLS)
    {
        Curx = 0;
        Cury++;
    }
    I_Scroll();
#endif // DEBUG_MDA_ENABLED
    // This outputs to the bochs debug console, which is supported by dosbox-x
    // with the config option 'bochs debug port e9 = true'
#if (BOCHS_DEBUG_ENABLED == 1)
    outp(0xE9, c);
#endif // BOCHS_DEBUG_ENABLED
    // This outputs to the serial port, which is all I have for real hardware
#if (DEBUG_SERIAL_ENABLED == 1)
    {
      int status;
      // Yuck, busy wait. Install an interrupt with DPMI?
      do {
        status = inp(DEBUG_SERIAL_BASE_IO + 5);
      } while ((status & 0x20) == 0);
      outp(DEBUG_SERIAL_BASE_IO, c);
    }
#endif
    // Output to text file
#if (DEBUG_FILE_ENABLED == 1)
    fputc(c, f_log);
#endif
    if (debug_to_screen) {
        putchar(c);
    }
}

void I_Puts(char *str)
{
    while (*str)
    {
        I_Putchar(*str);
        str++;
    }
}

void I_Printf(const char *format, ...)
{
    char **arg = (char **)&format;
    int c;
    arg++;
    while ((c = *format++) != 0)
    {
        char buf[80];

        if (c != '%')
            I_Putchar(c);
        else
        {
            char *p;
            fixed_t value;
            c = *format++;
            switch (c)
            {
            case 'd':
            case 'i':
                sprintf(buf, "%d", *((int *)arg++));
                I_Puts(buf);
                break;
            case 'u':
                sprintf(buf, "%u", *((int *)arg++));
                I_Puts(buf);
                break;
            case 'x':
                sprintf(buf, "%x", *((int *)arg++));
                I_Puts(buf);
                break;
            case 'f':
                sprintf(buf, "%f", *((int *)arg++));
                I_Puts(buf);
                break;
            case 'p':
                value = *((fixed_t *)arg++);
                sprintf(buf, "%i.%04i", value >> FRACBITS, ((value & 65535) * 10000) >> FRACBITS);
                I_Puts(buf);
                break;
            case 's':
                p = *arg++;
                if (p == NULL)
                    p = "(null)\0";
                I_Puts(p);
                break;
            case 'c':
                I_Putchar(*((byte *)arg++));
                break;
            default:
                I_Putchar(*((int *)arg++));
                break;
            }
        }
    }

#if (DEBUG_FILE_ENABLED == 1)
    fflush(f_log);
#endif
}

static int FramePtr(void);
#pragma aux FramePtr = "mov eax, ebp" value[eax];

static int StackPtr(void);
#pragma aux StackPtr = "mov eax, esp" value[eax];


static void CleanupModules(debugmodule_t *modules, int num_modules) {
    int i;
    if (modules == NULL) {
        return;
    }
    for (i = 0; i < num_modules; i++) {
        free(modules[i].name);
    }
    free(modules);
}

static void CleanupSymbols(debugsymbol_t *symbols, int num_symbols) {
    int i;
    if (symbols == NULL) {
        return;
    }
    for (i = 0; i < num_symbols; i++) {
        free(symbols[i].name);
    }
    free(symbols);
}

#define MAX_MODULES 350
#define MAX_SYMBOLS 3000

void SortSymbols(debugsymbol_t *symbols, int num_symbols) {
    if (num_symbols <= 1) {
        return;
    }
    if (num_symbols == 2) {
        if (symbols[0].addr > symbols[1].addr) {
            debugsymbol_t tmp = symbols[0];
            symbols[0] = symbols[1];
            symbols[1] = tmp;
        }
        return;
    } else {
        int pivot = symbols[0].addr;
        int left = 1;
        int right = num_symbols - 1;
        debugsymbol_t tmp;
        while (left < right) {
            while (left < right && symbols[left].addr < pivot) {
                left++;
            }
            while (left < right && symbols[right].addr >= pivot) {
                right--;
            }
            if (left < right) {
                debugsymbol_t tmp = symbols[left];
                symbols[left] = symbols[right];
                symbols[right] = tmp;
            }
        }
        if (symbols[left].addr >= pivot) {
            left--;
        }
        tmp = symbols[0];
        symbols[0] = symbols[left];
        symbols[left] = tmp;
        SortSymbols(symbols, left);
        SortSymbols(symbols + left + 1, num_symbols - left - 1);
    }
}


void DumpSymbolTable(debugsymbol_t *symbols, int num_symbols) {
    int i;
    if (symbols == NULL) {
        return;
    }
    for (i = 0; i < num_symbols; i++) {
        I_Printf("0x%x: %s\n", symbols[i].addr, symbols[i].name);
    }
}

static void ReadMapFile(debugmodule_t **modules, int *num_modules, debugsymbol_t **symbols,
                        int *num_symbols) {
    char mapfile[256];
    debugmodule_t *_modules = NULL;
    int _num_modules = 0;
    debugsymbol_t *_symbols = NULL;
    int _num_symbols = 0;
    extern char **__argv;
    char line[512];
    FILE *map;
    debugmodule_t *current_module;
    char *executable = __argv[0];
    char *ext = strrchr(executable, '.');
    if (ext) {
        *ext = '\0';
    }
    sprintf(mapfile, "%s.map", executable);
    map = fopen(mapfile, "r");
    *ext = '.';
    if (!map) {
        I_Printf("Failed to open map file %s, symtable not available\n",
                 mapfile);
        return;
    }
    _modules = malloc(sizeof(debugmodule_t) * MAX_MODULES);
    _symbols = malloc(sizeof(debugsymbol_t) * MAX_SYMBOLS);
    if (modules == NULL || symbols == NULL) {
        I_Printf("Failed to allocate memory for backtrace, symtable not "
                 "available.\n");
        goto fail;
    }
    // Read line by line
    current_module = NULL;
    while (fgets(line, sizeof(line), map) != NULL) {
        if (strncmp(line, "Module: ", 8) == 0) {
            char* obj_part = line + 8;
            char *module_name;
            // The module is formatted obj(source) but it's very long due to full
            // paths. Let's drop the path
            char* filepart = strrchr(obj_part, '(');
            char* filepart_nopath;
            char reformatted[256];
            if (filepart) {
                filepart[0] = '\0';
                filepart++;
                filepart_nopath = strrchr(filepart, '/');
                if (filepart_nopath) {
                    filepart = filepart_nopath + 1;
                }
                // Remove trailing '\n'
                filepart[strlen(filepart) - 1] = '\0';
            }
            snprintf(reformatted, sizeof(reformatted), "%s(%s)", obj_part, filepart);
            module_name = strdup(reformatted);
            // Strip \n
            if (!module_name) {
                I_Printf("Failed to allocate memory for backtrace, symtable "
                         "not available.\n");
                goto fail;
                return;
            }
            module_name[strlen(module_name) - 1] = '\0';
            current_module = &_modules[_num_modules];
            current_module->name = module_name;
            _num_modules++;
            if (_num_modules > MAX_MODULES) {
                I_Printf("Too many modules, symtable incomplete, boost "
                         "MAX_MODULES.\n");
                goto cleanup;
            }
        } else if (strncmp(line, "0001:", 4) == 0 || strncmp(line, "0002:", 4) == 0) {
            // Symbol
            int addr = (int)strtol(line + 5, NULL, 16);
            // Two chars whitespace
            char *name = strdup(line + 5 + 10);
            // Strip \n
            debugsymbol_t *current_symbol = &_symbols[_num_symbols];
            if (!name) {
                I_Printf("Failed to allocate memory for backtrace, symtable "
                         "not available.\n");
                goto fail;
            }
            name[strlen(name) - 1] = '\0';
            current_symbol->addr = addr;
            current_symbol->name = name;
            current_symbol->module = current_module;
            if (_num_symbols > MAX_SYMBOLS) {
                I_Printf("Too many symbols, symtable incomplete, boost "
                         "MAX_SYMBOLS.\n");
                goto cleanup;
            }
            _num_symbols++;
        }
    }
    goto cleanup;
fail:
    CleanupModules(_modules, _num_modules);
    CleanupSymbols(_symbols, _num_symbols);
    _modules = NULL;
    _symbols = NULL;
    _num_modules = 0;
    _num_symbols = 0;
cleanup:
    SortSymbols(_symbols, _num_symbols);
    *modules = _modules;
    *num_modules = _num_modules;
    *symbols = _symbols;
    *num_symbols = _num_symbols;
    if (map) {
        fclose(map);
    }
}

static debugsymbol_t *LookupSymbol(debugsymbol_t *symbols, int num_symbols, int addr) {
    int left = 0;
    int right = num_symbols - 1;
    int mid;
    if (symbols == NULL || num_symbols == 0) {
        return NULL;
    }
    // Binary search
    while (left <= right) {
        mid = (left + right) / 2;
        if (symbols[mid].addr == addr) {
            return &symbols[mid];
        } else if (symbols[mid].addr < addr) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    if (symbols[left].addr < addr) {
        return &symbols[left];
    } else {
        return &symbols[right];
    }
}

// Global symbol table. Keep it hidden with static, only use module functions
static debugmodule_t *modules = NULL;
static debugsymbol_t *symbols = NULL;
static int num_modules = 0;
static int num_symbols = 0;

extern char __begtext;
#define TEXT_SECTION_START ((int)&__begtext - 3)

debugsymbol_t* I_LookupSymbol(int addr) {
    return LookupSymbol(symbols, num_symbols, addr - TEXT_SECTION_START);
}

const char *I_LookupSymbolName(void* addr) {
    debugsymbol_t *symbol = LookupSymbol(symbols, num_symbols, (int)addr - TEXT_SECTION_START);
    if (symbol) {
        return symbol->name;
    } else {
        return NULL;
    }
}

void I_DebugInit(void) {
#if (DEBUG_SERIAL_ENABLED == 1)
    outp(DEBUG_SERIAL_BASE_IO + 1, 0x00); // Disable all interrupts
    outp(DEBUG_SERIAL_BASE_IO + 3, 0x80); // Enable the baud rate divisor
    outp(DEBUG_SERIAL_BASE_IO + 0, 115200 / DEBUG_SERIAL_BAUD); // Set the baud rate
    outp(DEBUG_SERIAL_BASE_IO + 1, 0x00); // Hi byte
    outp(DEBUG_SERIAL_BASE_IO + 3, 0x03); // 8 bits, no parity, one stop bit
    I_Printf("Serial debug port initialized\n");
#endif
#if (DEBUG_FILE_ENABLED == 1)
    f_log = fopen("fdoom.log", "a");
#endif
    I_Printf("Reading symbol table from map file\n");
    ReadMapFile(&modules, &num_modules, &symbols, &num_symbols);
    if (!modules) {
        I_Printf("Failed to read symbol table\n");
        return;
    }
    I_Printf("Symbol table ready, %i syms %i mods\n", num_symbols, num_modules);
}

void I_DebugShutdown(void) {
    CleanupModules(modules, num_modules);
    CleanupSymbols(symbols, num_symbols);
    modules = NULL;
    symbols = NULL;
    num_modules = 0;
    num_symbols = 0;
}

void I_ShutdownGraphics(void);

#define MAX_BACKTRACE_FRAMES 25

// Print a nice backtrace, reading from the .map file if it exists.
void I_Backtrace(const char *msg, ...) {
    extern char __begtext;
    extern char ___Argc;
    int text_end = (int)&___Argc;
    int text_start = TEXT_SECTION_START;
    int *stack = (int *)StackPtr();
    int *frame = (int *)FramePtr();
    int* backtrace_alloc[MAX_BACKTRACE_FRAMES + 1];
    int* *backtrace = backtrace_alloc;
    int num_backtrace = 0;

    // If defined. pause the game, and print the backtrace to the screen
#if (DEBUG_SHOW_BACKTRACE_ON_SCREEN == 1)
    I_ShutdownGraphics();
    I_ShutdownKeyboard();
    debug_to_screen = 1;
#endif
    // What is the stack size? 0x10000?
    if (frame < stack || frame > stack + 0x10000) {
        I_Printf("Stack frames are omitted, backtrace is not possible. Make "
                 "sure -of+ is set in compiler options for watcom\n");
        return;
    }
    if (!modules) {
        ReadMapFile(&modules, &num_modules, &symbols, &num_symbols);
    }
    // TODO Should we grab the registers here? Probably not needed
    I_Printf("Backtrace EBP: 0x%x, ESP: 0x%x: TEXT: 0x%x", frame,
             stack, text_start);
    // Setup the backtrace buffer pointer
    backtrace[MAX_BACKTRACE_FRAMES] = NULL;
    // We put the most recent frame at the end of the buffer, so we need
    // to walk backwards. We start at the end
    backtrace = &backtrace[MAX_BACKTRACE_FRAMES];
    // The frame is the EBP register, which points to the previous frame
    while (frame != NULL) {
        // THe return pointer is the second thing on the stack frame
        int *retptr = frame + 1;
        // Does the return pointer make seense?
        if (*retptr > text_start && *retptr < text_end) {
          // Record the frame if we have space
          if (num_backtrace >= MAX_BACKTRACE_FRAMES) {
              I_Printf("Notice: backtrace is truncated\n");
              break;
          }
          backtrace--;
          *backtrace = retptr;
          num_backtrace++;
          // Next
        } else {
            break;
        }
        // The next frame is the previous EBP, or the first thing on the stack
        // frame. This forms a linked list
        frame = (int *)frame[0];
        if (frame[0] <= frame) {
            // This is a nonsense frame, abort
            break;
        }
    }
    while(*backtrace) {
        int* retptr = *backtrace;
        int offset_in_text = *retptr - text_start;
        // Try to lookup the symbol info
        debugsymbol_t *symbol = LookupSymbol(symbols, num_symbols, offset_in_text);
        if (symbol) {
          I_Printf("  0x%x[text+0x%x] %s+%x %s \n", *retptr,
                   offset_in_text, symbol->name, offset_in_text - symbol->addr,
                   symbol->module->name);
        } else {
            I_Printf("  0x%x[text+0x%x] \n", *retptr,
                     offset_in_text);
        }
        backtrace++;
    }
    {
      char msgbuf[256];
      va_list args;
      va_start(args, msg);
      vsnprintf(msgbuf, sizeof(msgbuf), msg, args);
      va_end(args);
      I_Puts(msgbuf);
    }
#if (DEBUG_DIE_ON_BACKTRACE == 1)
    exit(1);
#elif (DEBUG_SHOW_BACKTRACE_ON_SCREEN == 1)
    printf("Press any key to continue, ESC to quit\n");
    {
      // TODO this isn't always reliable
      char c = getchar();
      if (c == 27) {
          exit(1);
      }
      debug_to_screen = 0;
      I_InitGraphics();
      I_StartupKeyboard();
    }
#endif
}

#endif // DEBUG_ENABLED
