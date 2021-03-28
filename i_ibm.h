#include "fastmath.h"
#include "ns_task.h"
#include "doomtype.h"
#include "vmode.h"

extern int ticcount;
extern fixed_t fps;

extern byte *currentscreen;
extern byte processedpalette[14 * 768];
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25) || (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
extern byte lut16colors[256];
#endif

extern void I_TimerISR(task *task);
