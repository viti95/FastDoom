#include "fastmath.h"

void I_DebugWriteString(int x, int y, char *message);
void I_DebugWriteInteger(int x, int y, int value);
void I_DebugWriteFixed(int x, int y, fixed_t value);
void I_DebugWriteLineString(char *message);
void I_DebugWriteLineInteger(int value);
void I_DebugWriteLineFixed(fixed_t value);
void I_DebugClear(void);
void I_DebugClearLine(void);
