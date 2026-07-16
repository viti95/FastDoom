//
// Serial Terminal Output for MDA Mode (VT100-compatible)
//
// Provides text output over a serial port (COM1-COM4).
// Compatible with DEC VT100 and VT100-emulation terminals.
//

#ifndef __I_TERM__
#define __I_TERM__

#include "doomtype.h"

#define TERM_DEFAULT_BAUD    38400
#define TERM_DEFAULT_PORT    0x3F8   /* COM1 */
#define TERM_BUF_SIZE        1024

/* VT100 display size (VT100 is 80x24, not 80x25) */
#define TERM_ROWS            24
#define TERM_COLS            80

/* VT100 escape sequences */
#define TERM_ESC             0x1B
#define TERM_CLEAR_SCREEN    "\x1B[2J"   /* ED - Erase Display        */
#define TERM_HOME            "\x1B[H"    /* CH - Cursor Home (1,1)    */
#define TERM_USASCII         "\x1B(B"    /* SCS - Select US ASCII     */

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

/* Clear the terminal screen and home cursor. */
void TERM_Clear(void);

/* Move cursor to column x, row y (0-based, clamped to TERM_ROWS-1). */
void TERM_SetCursor(int x, int y);

/* Translate a CP437 character to VT100-safe ASCII and send it. */
void TERM_SendChar(byte c);

/* Shutdown the serial port. */
void TERM_Shutdown(void);

/* Set the 80x25 text backbuffer to mirror.
   Called by the video mode driver (e.g. MDA_InitGraphics). */
void TERM_SetBackbuffer(const unsigned short *buf);

/* Update the VT100 display from the registered backbuffer.
   Only changed cells are transmitted (incremental diff). */
void TERM_UpdateFromBuffer(const unsigned short *buf);

/* Returns non-zero if the terminal is active. */
int  TERM_IsActive(void);

/* ------------------------------------------------------------------
   Global flags set by d_main.c from -term command line.
   Read by MDA_InitGraphics() to decide whether to call TERM_Init().
   ------------------------------------------------------------------ */
extern boolean term_enabled;
extern int     term_port;
extern int     term_baud;

#endif /* __I_TERM__ */
