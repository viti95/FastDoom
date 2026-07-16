#include "doomtype.h"

#if defined(MODE_MDA)

void MDA_InitGraphics(void);
void I_ProcessPalette(byte *palette);
void I_SetPalette(int numpalette);
void I_FinishUpdate(void);
void MDA_ShutdownTerminal(void);

#endif
