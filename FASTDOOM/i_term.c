//
// Serial Terminal Output for MDA Mode (VT100-compatible)
//
// Provides text output over a serial port (COM1-COM4).
// Compatible with DEC VT100 and VT100-emulation terminals.
// Only VT100-standard escapes are used: ED, CH, CUP.
//

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <dos.h>

#include "i_term.h"
#include "i_system.h"
#include "options.h"
#include "ns_inter.h"

/* Serial port registers */
#define UART_THR  0x00   /* Transmit Holding Register (write) */
#define UART_IER  0x01   /* Interrupt Enable Register */
#define UART_IIR  0x02   /* Interrupt Identification Register (read) */
#define UART_FCR  0x02   /* FIFO Control Register (write) */
#define UART_LCR  0x03   /* Line Control Register */
#define UART_MCR  0x04   /* Modem Control Register */
#define UART_LSR  0x05   /* Line Status Register */

#define LSR_THRE  0x20   /* Transmit Holding Register Empty */

#define MCR_DTR   0x01   /* Data Terminal Ready */
#define MCR_RTS   0x02   /* Request To Send */
#define MCR_OUT2  0x08   /* Out2 */

/* Module state */
static int    term_port       = 0;
static int    term_baud       = 0;
static int    term_active     = 0;

/* Output buffer - flushed during I_FinishUpdate to avoid
   blocking the game loop on slow serial writes. */
static char   term_buf[TERM_BUF_SIZE];
static int    term_buf_len    = 0;

/*
 * CP437 -> ASCII translation table for VT100.
 * VT100 uses US ASCII (0x20-0x7E) and does not have a CP437
 * character set.  Box-drawing, block, and special CP437 chars
 * are mapped to the closest readable ASCII equivalent.
 *
 * Indices 0x20-0x7E are identity (standard ASCII).
 * All values are single-byte ASCII (0x00-0x7F).
 */
static const byte cp437_to_ascii[256] =
{
    /* 0x00 - 0x1F : control -> space (32) */
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,

    /* 0x20 - 0x3F : standard ASCII (32) */
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
    0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,

    /* 0x40 - 0x5F : standard ASCII (32) */
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
    0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
    0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,

    /* 0x60 - 0x7F : standard ASCII (32) */
    0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
    0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
    0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,

    /* 0x80 - 0x9F : extended Latin -> ASCII (32) */
    0x43,0x75,0x65,0x65,0x61,0x61,0x65,0x63,
    0x65,0x65,0x65,0x69,0x69,0x6f,0x75,0x75,
    0x6f,0x6f,0x75,0x6f,0x6e,0x4f,0x5a,0x7a,
    0x41,0x41,0x41,0x43,0x41,0x41,0x41,0x53,

    /* 0xA0 - 0xAF : misc -> ASCII (16) */
    0x54,0x74,0x22,0x53,0x73,0x53,0x61,0x54,
    0x74,0x7a,0x5a,0x52,0x72,0x49,0x69,0x50,

    /* 0xB0 - 0xBF : misc -> ASCII (16) */
    0x70,0x48,0x55,0x75,0x3e,0x49,0x69,0x53,
    0x73,0x6f,0x6f,0x4f,0x4f,0x4c,0x6c,0x3c,

    /* 0xC0 - 0xCF : box-drawing (single) -> ASCII (16) */
    0x2b,0x2d,0x7c,0x2b,0x2b,0x2d,0x2b,0x7c,
    0x7c,0x2b,0x2d,0x7c,0x2d,0x2b,0x7c,0x2b,

    /* 0xD0 - 0xDF : box-drawing (double/triple/quad) -> ASCII (16) */
    0x2b,0x2d,0x7c,0x2b,0x2b,0x2d,0x2b,0x7c,
    0x7c,0x2b,0x2d,0x7c,0x2d,0x2b,0x7c,0x2b,

    /* 0xE0 - 0xEF : block elements -> ASCII (16) */
    0x6f,0x6f,0x6f,0x23,0x23,0x23,0x4f,0x4f,
    0x4f,0x6f,0x6f,0x6f,0x6f,0x6f,0x6f,0x6f,

    /* 0xF0 - 0xFF : symbols / arrows -> ASCII (16) */
    0x5e,0x76,0x3c,0x3e,0x21,0x3f,0x5e,0x23,
    0x7b,0x7d,0x7e,0x7e,0x7e,0x7e,0x7e,0x20,
};

/*
 * TERM_TranslateChar
 * Map a CP437 character code to the nearest VT100-safe ASCII
 * character.  Characters < 0x20 become space.
 */
static byte TERM_TranslateChar(byte c)
{
    return cp437_to_ascii[c];
}

/*
 * TERM_WaitTransmit
 * Busy-wait until the THR is empty.  Timeout prevents hard
 * lock-up if the port is mis-configured.
 */
static void TERM_WaitTransmit(void)
{
    unsigned int timeout = 0x10000;

    while ((inp(term_port + UART_LSR) & LSR_THRE) == 0)
    {
        if (--timeout == 0)
            return;
    }
}

/*
 * TERM_WriteByte
 * Write one byte directly to the UART THR register.
 */
static void TERM_WriteByte(byte c)
{
    TERM_WaitTransmit();
    outp(term_port + UART_THR, c);
}

/*
 * TERM_WriteBuf
 * Drain the software output buffer to the hardware.
 */
static void TERM_WriteBuf(void)
{
    while (term_buf_len > 0)
    {
        TERM_WriteByte((byte)term_buf[0]);
        memmove(term_buf, term_buf + 1, term_buf_len);
        term_buf_len--;
    }
}

/*
 * TERM_BaudToDivisor
 * Standard 1.8432 MHz crystal:  divisor = 115200 / baud
 */
static int TERM_BaudToDivisor(int baud)
{
    return 115200 / baud;
}

/*
 * TERM_FormatInt
 * Convert signed integer to decimal string in the output buffer.
 */
static void TERM_FormatInt(int val)
{
    char tmp[16];
    int i = 0;

    if (val == 0)
    {
        TERM_SendByte('0');
        return;
    }

    if (val < 0)
    {
        TERM_SendByte('-');
        val = -val;
    }

    while (val > 0)
    {
        tmp[i++] = (val % 10) + '0';
        val /= 10;
    }

    while (i > 0)
        TERM_SendByte(tmp[--i]);
}

/*
 * TERM_FormatUnsigned
 * Convert unsigned integer to decimal string in the output buffer.
 */
static void TERM_FormatUnsigned(unsigned int val)
{
    char tmp[16];
    int i = 0;

    if (val == 0)
    {
        TERM_SendByte('0');
        return;
    }

    while (val > 0)
    {
        tmp[i++] = (val % 10) + '0';
        val /= 10;
    }

    while (i > 0)
        TERM_SendByte(tmp[--i]);
}

/*
 * TERM_ParseFormat
 * Minimal printf:  %s, %d, %u, %c, %x, %%
 */
static void TERM_ParseFormat(char *fmt, va_list args)
{
    while (*fmt)
    {
        if (*fmt == '%')
        {
            fmt++;
            switch (*fmt)
            {
            case 's':
            {
                char *s = va_arg(args, char *);
                if (s)
                    TERM_SendString(s);
                else
                    TERM_SendString("(null)");
                break;
            }
            case 'd':
            {
                int val = va_arg(args, int);
                TERM_FormatInt(val);
                break;
            }
            case 'u':
            {
                unsigned int val = va_arg(args, unsigned int);
                TERM_FormatUnsigned(val);
                break;
            }
            case 'c':
            {
                int c = va_arg(args, int);
                TERM_SendByte((byte)c);
                break;
            }
            case 'x':
            {
                unsigned int val = va_arg(args, unsigned int);
                char hexchars[] = "0123456789abcdef";
                int shift;

                TERM_SendByte('0');
                TERM_SendByte('x');
                for (shift = 28; shift >= 0; shift -= 4)
                    TERM_SendByte(hexchars[(val >> shift) & 0x0F]);
                break;
            }
            case '%':
                TERM_SendByte('%');
                break;
            case '\0':
                TERM_SendByte('%');
                return;
            default:
                TERM_SendByte('%');
                TERM_SendByte((byte)*fmt);
                break;
            }
        }
        else
            TERM_SendByte((byte)*fmt);
        fmt++;
    }
}

/* ------------------------------------------------------------------
   Public API
   ------------------------------------------------------------------ */

/*
 * TERM_Init
 * Configure the UART for 8N1 output at the requested baud rate.
 *
 * port  - 0x3F8 (COM1), 0x2F8 (COM2), 0x3E8 (COM3), 0x2E8 (COM4)
 * baud  - 9600, 19200, 38400, 57600, 115200
 *
 * Returns 0 on success, -1 on invalid port.
 */
int TERM_Init(int port, int baud)
{
    unsigned flags;
    int divisor;

    if (port != 0x3F8 && port != 0x2F8 &&
        port != 0x3E8 && port != 0x2E8)
        return -1;

    if (baud <= 0)
        baud = TERM_DEFAULT_BAUD;

    term_port  = port;
    term_baud  = baud;
    divisor    = TERM_BaudToDivisor(baud);

    flags = DisableInterrupts();

    /* Disable interrupts, set DLAB = 1 */
    outp(port + UART_IER, 0x00);
    outp(port + UART_LCR, 0x80);

    /* Write divisor (LSB then MSB) */
    outp(port + UART_THR, divisor & 0xFF);
    outp(port + UART_IER, (divisor >> 8) & 0xFF);

    /* 8 data bits, no parity, 1 stop, DLAB = 0 */
    outp(port + UART_LCR, 0x03);

    /* Enable FIFO, clear both */
    outp(port + UART_FCR, 0xC7);

    /* DTR + RTS + OUT2 */
    outp(port + UART_MCR, MCR_DTR | MCR_RTS | MCR_OUT2);

    /* Flush receive buffer */
    outp(port + UART_IIR, 0x00);

    RestoreInterrupts(flags);

    term_active  = 1;
    term_buf_len = 0;

    return 0;
}

/*
 * TERM_SendByte
 * Queue one raw byte.  Flushes on overflow.
 */
void TERM_SendByte(byte c)
{
    if (!term_active)
        return;

    term_buf[term_buf_len] = (char)c;
    term_buf_len++;

    if (term_buf_len >= TERM_BUF_SIZE)
        TERM_Flush();
}

/*
 * TERM_SendString
 * Queue a null-terminated string (raw, no translation).
 */
void TERM_SendString(char *s)
{
    while (*s)
    {
        TERM_SendByte((byte)*s);
        s++;
    }
}

/*
 * TERM_SendChar
 * Translate a single CP437 character to VT100-safe ASCII
 * and queue it.  Control characters become space.
 */
void TERM_SendChar(byte c)
{
    byte translated;

    if (!term_active)
        return;

    /* Fast-path: standard ASCII printable range */
    if (c >= 0x20 && c <= 0x7E)
    {
        TERM_SendByte(c);
        return;
    }

    /* Translate CP437 extended char to ASCII */
    translated = TERM_TranslateChar(c);

    /* Guarantee printable ASCII */
    if (translated < 0x20)
        translated = ' ';

    TERM_SendByte(translated);
}

/*
 * TERM_Printf
 * Printf-style output to the terminal buffer.
 */
void TERM_Printf(char *fmt, ...)
{
    va_list args;

    if (!term_active)
        return;

    va_start(args, fmt);
    TERM_ParseFormat(fmt, args);
    va_end(args);
}

/*
 * TERM_Flush
 * Drain the software buffer to the UART.  Called from
 * I_FinishUpdate() so serial I/O never blocks the game loop.
 */
void TERM_Flush(void)
{
    if (!term_active)
        return;

    TERM_WriteBuf();
}

/*
 * TERM_Clear
 * Send VT100 ED (Erase Display) + CH (Cursor Home).
 * \x1B[2J  \x1B[H   -- standard on VT100.
 */
void TERM_Clear(void)
{
    if (!term_active)
        return;

    TERM_SendByte(TERM_ESC);
    TERM_SendByte('[');
    TERM_SendByte('2');
    TERM_SendByte('J');
    TERM_SendByte(TERM_ESC);
    TERM_SendByte('[');
    TERM_SendByte('H');
}

/*
 * TERM_SetCursor
 * VT100 CUP (Cursor Position): \x1B[y;xH
 *
 * x, y are 0-based; VT100 expects 1-based.
 * y is clamped to TERM_ROWS-1 (23), x to TERM_COLS-1 (79).
 */
void TERM_SetCursor(int x, int y)
{
    if (!term_active)
        return;

    /* Clamp to VT100 display bounds */
    if (x < 0)    x = 0;
    if (x >= 80)  x = 79;
    if (y < 0)    y = 0;
    if (y >= 24)  y = 23;

    /* \x1B[y;xH  (row;col, 1-based) */
    TERM_SendByte(TERM_ESC);
    TERM_SendByte('[');
    TERM_FormatUnsigned((unsigned)(y + 1));
    TERM_SendByte(';');
    TERM_FormatUnsigned((unsigned)(x + 1));
    TERM_SendByte('H');
}

/*
 * TERM_Shutdown
 * Reset the UART and clear internal state.
 */
void TERM_Shutdown(void)
{
    if (!term_active)
        return;

    outp(term_port + UART_MCR, 0x00);
    outp(term_port + UART_IER, 0x00);

    term_active  = 0;
    term_buf_len = 0;
}

/*
 * TERM_IsActive
 * Returns non-zero if the terminal is initialised.
 */
int TERM_IsActive(void)
{
    return term_active;
}
