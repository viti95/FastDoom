//
// Serial Terminal Output for MDA Mode
//
// Provides a text-only terminal output over COM1 serial port.
// Used alongside the MDA text display to pipe game text to an
// external terminal (e.g. for accessibility, headless operation).
//

#ifndef __I_TERM__
#define __I_TERM__

#include "doomtype.h"

#define TERM_DEFAULT_BAUD    38400
#define TERM_DEFAULT_PORT    0x3F8   /* COM1 */
#define TERM_MAX_PORT        0x2F8   /* COM4 */
#define TERM_BUF_SIZE        1024

/* ANSI-compatible cursor / display escape sequences */
#define TERM_ESC             0x1B
#define TERM_CSI             "\x1B["
#define TERM_CLEAR_LINE      "\x1B[2K\r"
#define TERM_HOME            "\x1B[H"
#define TERM_CLEAR_SCREEN    "\x1B[2J\x1B[H"
#define TERM_CUR_UP(n)       "\x1B[" #n "A"
#define TERM_CUR_DOWN(n)     "\x1B[" #n "B"
#define TERM_CUR_RIGHT(n)    "\x1B[" #n "C"
#define TERM_CUR_LEFT(n)     "\x1B[" #n "D"
#define TERM_CUR_POS(x, y)   "\x1B[" #y ";" #x "H"

/* Initialise the serial port. Returns 0 on success, -1 on failure. */
int  TERM_Init(int port, int baud);

/* Send a single byte to the terminal. */
void TERM_SendByte(byte c);

/* Send a null-terminated string to the terminal. */
void TERM_SendString(char *s);

/* Printf-style output to the terminal. */
void TERM_Printf(char *fmt, ...);

/* Flush any buffered output to the serial port. */
void TERM_Flush(void);

/* Clear the terminal screen (ANSI escape). */
void TERM_Clear(void);

/* Move cursor to column x, row y (0-based). */
void TERM_SetCursor(int x, int y);

/* Shutdown the serial port. */
void TERM_Shutdown(void);

/* Returns non-zero if the terminal is active. */
int  TERM_IsActive(void);

#endif /* __I_TERM__ */
