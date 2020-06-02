#include "doomtype.h"

void I_DebugWrite(int x, int y, char *message)
{
    int i = 0;
    byte *bwscreen = (byte *)0xB0000;

    while (*message)
    {
        bwscreen[y * 160 + x * 2 + i * 2] = *(bwscreen);
        bwscreen[y * 160 + x * 2 + i * 2 + 1] = 7;
        message++;
        i++;
    }
}

void I_DebugClear()
{
    int i;

    byte *bwscreen = (byte *)0xB0000;

    for (i = 0; i < 80 * 25; i++)
    {
        bwscreen[i * 2] = ' ';
        bwscreen[i * 2 + 1] = 7;
    }
}