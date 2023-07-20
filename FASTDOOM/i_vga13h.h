#include "doomtype.h"

void I_FinishUpdateDifferential(void);
void I_FinishUpdateDirect(void);
void VGA_13H_InitGraphics(void);
void I_CopyLine(unsigned int position, unsigned int count);
void I_CopyLine486(unsigned int position, unsigned int count);
void I_UpdateCopyLineFunc(void);
