#include "fastmath.h"
#include "ns_task.h"
#include "doomtype.h"
#include "vmode.h"

extern int ticcount;
extern fixed_t fps;

extern int currentpalette;

extern byte *currentscreen;
extern byte processedpalette[14 * 768];
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25 || EXE_VIDEOMODE == EXE_VIDEOMODE_80X50 || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA)
extern byte lut16colors[256];
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_13H || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA || EXE_VIDEOMODE == EXE_VIDEOMODE_EGA || EXE_VIDEOMODE == EXE_VIDEOMODE_HERC || EXE_VIDEOMODE == EXE_VIDEOMODE_CGA_BW)
extern int updatestate;
#endif

#define I_NOUPDATE	0
#define I_FULLVIEW	1
#define I_STATBAR	2
#define I_MESSAGES	4
#define I_FULLSCRN	8

extern void I_TimerISR(task *task);
