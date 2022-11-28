#include "doomtype.h"

#if defined(TEXT_MODE)

extern byte lut16colors[14 * 256];
extern byte *ptrlut16colors;

void TEXT_40x25_InitGraphics(void);
void TEXT_80x25_InitGraphics(void);
void TEXT_80x25_Double_InitGraphics(void);
void TEXT_ProcessPalette(byte *palette);
void TEXT_SetPalette(int numpalette);
void TEXT_40x25_ChangeVideoPage(void);
void TEXT_80x25_ChangeVideoPage(void);
void TEXT_80x25_EGA_Double_ChangeVideoPage(void);
void TEXT_80x25_Double_ChangeVideoPage(void);

#endif
