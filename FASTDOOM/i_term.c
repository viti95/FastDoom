//
// Serial Terminal Output for MDA Mode
//
// Provides text output over a serial port (COM1-COM4).
// Buffers writes and flushes them to the UART to avoid
// slowing down the main game loop with busy-wait I/O.
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
#define UART_RBR  0x00   /* Receive Buffer Register (read) */
#define UART_THR  0x00   /* Transmit Holding Register (write) */
#define UART_IER  0x01   /* Interrupt Enable Register */
#define UART_IIR  0x02   /* Interrupt Identification Register (read) */
#define UART_FCR  0x02   /* FIFO Control Register (write) */
#define UART_LCR  0x03   /* Line Control Register */
#define UART_MCR  0x04   /* Modem Control Register */
#define UART_LSR  0x05   /* Line Status Register */
#define UART_MSR  0x06   /* Modem Status Register */

/* LSR bits */
#define LSR_DR    0x01   /* Data Ready */
#define LSR_THRE  0x20   /* Transmit Holding Register Empty */
#define LSR_TEMT  0x40   /* Transmit Empty */

/* MCR bits */
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
 * TERM_WaitTransmit
 * Busy-wait until the THR is empty, then return.
 * Uses a timeout counter to avoid an infinite loop if the
 * serial port is misconfigured.
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
 * Actually write a byte to the UART hardware.
 */
static void TERM_WriteByte(byte c)
{
    TERM_WaitTransmit();
    outp(term_port + UART_THR, c);
}

/*
 * TERM_WriteBuf
 * Drain the output buffer to the serial port.
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
 * Convert baud rate to UART divisor (clock = 115200).
 */
static int TERM_BaudToDivisor(int baud)
{
    return 115200 / baud;
}

/*
 * TERM_FormatInt
 * Append a decimal integer to the output buffer.
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
    {
        TERM_SendByte(tmp[--i]);
    }
}

/*
 * TERM_FormatUnsigned
 * Append an unsigned decimal integer to the output buffer.
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
    {
        TERM_SendByte(tmp[--i]);
    }
}

/*
 * TERM_ParseFormat
 * Handle format string output (simplified printf).
 * Supports: %s, %d, %u, %c, %x, %%
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
                {
                    TERM_SendByte(hexchars[(val >> shift) & 0x0F]);
                }
                break;
            }
            case '%':
                TERM_SendByte('%');
                break;
            case '\0':
                /* Trailing % */
                TERM_SendByte('%');
                return;
            default:
                TERM_SendByte('%');
                TERM_SendByte((byte)*fmt);
                break;
            }
        }
        else
        {
            TERM_SendByte((byte)*fmt);
        }
        fmt++;
    }
}

/* ------------------------------------------------------------------
   Public API
   ------------------------------------------------------------------ */

/*
 * TERM_Init
 * Initialise the serial port for terminal output.
 *
 * port  - I/O base address (0x3F8, 0x2F8, 0x3E8, 0x2E8)
 * baud  - Baud rate (9600, 19200, 38400, 57600, 115200)
 *
 * Returns 0 on success, -1 if port is invalid.
 */
int TERM_Init(int port, int baud)
{
    unsigned flags;
    int divisor;

    /* Validate port */
    if (port != 0x3F8 && port != 0x2F8 &&
        port != 0x3E8 && port != 0x2E8)
    {
        return -1;
    }

    /* Validate baud rate */
    if (baud <= 0)
        baud = TERM_DEFAULT_BAUD;

    term_port = port;
    term_baud = baud;
    divisor = TERM_BaudToDivisor(baud);

    /* Disable interrupts */
    flags = DisableInterrupts();

    /* DLAB = 0, disable interrupts, 8 bits */
    outp(port + UART_LCR, 0x80);     /* DLAB = 1 */
    outp(port + UART_IER, 0x00);     /* No interrupts */

    /* Set baud rate divisor */
    outp(port + UART_THR, divisor & 0xFF);       /* Divisor Latch LSB */
    outp(port + UART_IER, (divisor >> 8) & 0xFF); /* Divisor Latch MSB */

    /* 8 data bits, no parity, 1 stop bit, DLAB = 0 */
    outp(port + UART_LCR, 0x03);

    /* Clear FIFO (if available) */
    outp(port + UART_FCR, 0xC7);  /* Enable & clear FIFOs */

    /* Enable DTR and RTS */
    outp(port + UART_MCR, MCR_DTR | MCR_RTS | MCR_OUT2);

    /* Flush any pending data */
    outp(port + UART_IIR, 0x00);

    RestoreInterrupts(flags);

    term_active = 1;
    term_buf_len = 0;

    return 0;
}

/*
 * TERM_SendByte
 * Queue a byte for transmission. If the buffer overflows,
 * flush immediately.
 */
void TERM_SendByte(byte c)
{
    if (!term_active)
        return;

    term_buf[term_buf_len] = (char)c;
    term_buf_len++;

    if (term_buf_len >= TERM_BUF_SIZE)
    {
        TERM_Flush();
    }
}

/*
 * TERM_SendString
 * Send a null-terminated string to the terminal buffer.
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
 * Drain the output buffer to the serial port.
 * Called from I_FinishUpdate() to avoid blocking the game loop.
 */
void TERM_Flush(void)
{
    if (!term_active)
        return;

    TERM_WriteBuf();
}

/*
 * TERM_Clear
 * Send ANSI clear-screen escape sequence.
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
 * Move cursor to position (x, y) using ANSI escape.
 * x and y are 1-based for ANSI compatibility.
 */
void TERM_SetCursor(int x, int y)
{
    char numbuf[32];
    int i;

    if (!term_active)
        return;

    /* CSI y;x H */
    TERM_SendByte(TERM_ESC);
    TERM_SendByte('[');

    /* Encode row (y) - use 1-based */
    {
        int val = y + 1;
        i = 0;
        if (val == 0)
        {
            numbuf[i++] = '0';
        }
        else
        {
            char rev[12];
            int rlen = 0;
            while (val > 0)
            {
                rev[rlen++] = (val % 10) + '0';
                val /= 10;
            }
            while (rlen > 0)
            {
                numbuf[i++] = rev[--rlen];
            }
        }
        while (i > 0)
        {
            TERM_SendByte((byte)numbuf[--i]);
        }
    }

    TERM_SendByte(';');

    /* Encode column (x) - use 1-based */
    {
        int val = x + 1;
        i = 0;
        if (val == 0)
        {
            numbuf[i++] = '0';
        }
        else
        {
            char rev[12];
            int rlen = 0;
            while (val > 0)
            {
                rev[rlen++] = (val % 10) + '0';
                val /= 10;
            }
            while (rlen > 0)
            {
                numbuf[i++] = rev[--rlen];
            }
        }
        while (i > 0)
        {
            TERM_SendByte((byte)numbuf[--i]);
        }
    }

    TERM_SendByte('H');
}

/*
 * TERM_Shutdown
 * Disable the serial port and clean up.
 */
void TERM_Shutdown(void)
{
    if (!term_active)
        return;

    /* Disable DTR, RTS, and interrupts */
    outp(term_port + UART_MCR, 0x00);
    outp(term_port + UART_IER, 0x00);

    term_active = 0;
    term_buf_len = 0;
}

/*
 * TERM_IsActive
 * Returns non-zero if the terminal is initialised and active.
 */
int TERM_IsActive(void)
{
    return term_active;
}
