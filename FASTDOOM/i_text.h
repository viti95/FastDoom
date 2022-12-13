#include "doomtype.h"

#if defined(TEXT_MODE)

extern byte lut16colors[14 * 256];
extern byte *ptrlut16colors;

void TEXT_40x25_InitGraphics(void);
void TEXT_80x25_InitGraphics(void);
void TEXT_80x25_Double_InitGraphics(void);
void TEXT_ProcessPalette(byte *palette);
void I_SetPalette(int numpalette);
void I_FinishUpdate(void);

#endif
