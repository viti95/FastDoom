#include <stdio.h>
#include <string.h>
#include "doomtype.h"
#include "fastmath.h"

void I_DebugClearLine(){
    int i;

    byte *bwscreen = (byte *)0xB0000;

    for (i = 0; i < 80; i++)
    {
        bwscreen[i * 2] = ' ';
        bwscreen[i * 2 + 1] = 7;
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

void I_DebugWriteString(int x, int y, char *message)
{
    int i = 0;
    byte *bwscreen = (byte *)0xB0000;

    while (*message)
    {
        bwscreen[y * 160 + x * 2 + i * 2] = *(message);
        bwscreen[y * 160 + x * 2 + i * 2 + 1] = 7;
        message++;
        i++;
    }
}

void I_DebugWriteFixed(int x, int y, fixed_t value){
    char message[32];
    sprintf(message, "%i.%04i", value >> FRACBITS, ((value & 65535)*10000) >> FRACBITS);
    I_DebugWriteString(x, y, message);
}

void I_DebugWriteInteger(int x, int y, int value)
{
    char message[11];
    sprintf(message, "%ld", value);
    I_DebugWriteString(x, y, message);
}

void I_DebugWriteLineString(char *message)
{
    int i = 0;
    byte *bwscreen = (byte *)0xB0000;

    _fmemcpy(bwscreen + 2 * 80, bwscreen, 2 * 80 * 24);
    I_DebugClearLine();

    while (*message)
    {
        bwscreen[i * 2] = *(message);
        bwscreen[i * 2 + 1] = 7;
        message++;
        i++;
    }
}

void I_DebugWriteLineInteger(int value)
{
    char message[11];
    sprintf(message, "%ld", value);
    I_DebugWriteLineString(message);
}

void I_DebugWriteLineFixed(fixed_t value){
    char message[32];
    sprintf(message, "%i.%04i", value >> FRACBITS, ((value & 65535)*10000) >> FRACBITS);
    I_DebugWriteLineString(message);
}
