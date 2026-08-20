#include "doomtype.h"

#if defined(MODE_VT100)

void VT100_InitGraphics(void);
void I_ProcessPalette(byte *palette);
void I_SetPalette(int numpalette);
void I_FinishUpdate(void);
void VT100_ShutdownTerminal(void);

#endif
