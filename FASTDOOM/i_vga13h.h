#include "doomtype.h"

void I_FinishUpdateDifferential386(void);
void I_FinishUpdateDifferential486(void);
void I_FinishUpdateDirect(void);
void VGA_13H_InitGraphics(void);
void I_CopyLine386(unsigned int position, unsigned int count);
void I_CopyLine486(unsigned int position, unsigned int count);
void I_UpdateFinishFunc(void);
extern void (*finishfunc)(void);
