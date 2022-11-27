#include "fastmath.h"
#include "ns_task.h"
#include "doomtype.h"
#include "options.h"

extern int ticcount;
extern fixed_t fps;

extern unsigned short *currentscreen;
extern byte gammatable[5][256];

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T8086) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T80100)
extern byte lut16colors[14 * 256];
extern byte *ptrlut16colors;
#endif

#if defined(USE_BACKBUFFER)
extern int updatestate;
#endif

#define I_NOUPDATE	0
#define I_FULLVIEW	1
#define I_STATBAR	2
#define I_MESSAGES	4
#define I_FULLSCRN	8

extern void I_TimerISR(task *task);
