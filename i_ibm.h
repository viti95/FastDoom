#include "fastmath.h"
#include "ns_task.h"
#include "doomtype.h"

extern int ticcount;
extern fixed_t fps;

extern byte *currentscreen;
extern byte processedpalette[14 * 768];
extern byte lut16colors[256];

extern void I_TimerISR(task *task);
