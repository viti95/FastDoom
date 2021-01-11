#include <string.h>
#include "doomtype.h"
#include "fastmath.h"

#define COLOURS 0xFF
#define COLS 80
#define ROWS 25

unsigned short *Scrn = (unsigned short *)0xB0000;
int Curx, Cury = 0;
unsigned short EmptySpace = COLOURS << 8 | 0x20;

void I_Putchar(byte c);

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
void I_Putchar(byte c)
{
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
}

void I_SetCursor(int x, int y){
    Curx = x;
    Cury = y;
}
