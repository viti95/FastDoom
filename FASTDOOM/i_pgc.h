#ifndef __I_PGC_H__
#define __I_PGC_H__

#include "doomtype.h"

void PGC_InitGraphics(void);
void PGC_ShutdownGraphics(void);

// PGC palette hooks (see I_ProcessPalette/I_SetPalette in i_system.h)
void I_ProcessPalette(byte *palette);
void I_SetPalette(int numpalette);

void I_FinishUpdate(void);

#endif
