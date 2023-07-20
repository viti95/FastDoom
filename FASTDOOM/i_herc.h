#include "doomtype.h"

void I_ProcessPalette(byte *palette);
void I_SetPalette(int numpalette);
void I_FinishUpdate386(void);
void I_FinishUpdate486(void);
void HERC_InitGraphics(void);
void HERC_ShutdownGraphics(void);
void I_UpdateFinishFunc(void);
extern void (*finishfunc)(void);
