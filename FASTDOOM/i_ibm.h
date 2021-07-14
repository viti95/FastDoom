#include "fastmath.h"
#include "ns_task.h"
#include "doomtype.h"


extern int ticcount;
extern fixed_t fps;

extern byte *currentscreen;

#if defined(MODE_Y) || defined(MODE_13H) || (defined(MODE_VBE2) && !defined(MODE_PM))
extern byte processedpalette[14 * 768];
#endif
#if defined(MODE_VBE2) && defined(MODE_PM)
extern byte processedpalette[14 * 1024];
#endif

#if defined(MODE_T25) || defined(MODE_T50) || defined(MODE_EGA) || defined(MODE_PCP) || defined(MODE_CVB)
extern byte lut16colors[14 * 256];
extern byte *ptrlut16colors;
#endif

#if defined(MODE_CGA_BW) || defined(MODE_HERC)
extern byte sumcolors00[14 * 256];
extern byte sumcolors01[14 * 256];
extern byte sumcolors10[14 * 256];
extern byte sumcolors11[14 * 256];
extern byte *ptrsumcolors00;
extern byte *ptrsumcolors01;
extern byte *ptrsumcolors10;
extern byte *ptrsumcolors11;
#endif

#if defined(MODE_13H) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_CGA_BW) || defined(MODE_HERC) || defined(MODE_VBE2) || defined(MODE_PCP) || defined(MODE_CVB)
extern int updatestate;
#endif

#define I_NOUPDATE	0
#define I_FULLVIEW	1
#define I_STATBAR	2
#define I_MESSAGES	4
#define I_FULLSCRN	8

extern void I_TimerISR(task *task);
