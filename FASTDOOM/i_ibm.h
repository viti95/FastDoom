#include "fastmath.h"
#include "ns_task.h"
#include "doomtype.h"
#include "options.h"

extern int ticcount;
extern fixed_t fps;

extern unsigned short *currentscreen;
extern byte gammatable[5][256];

#if defined(USE_BACKBUFFER)
extern int updatestate;
#endif

#define I_NOUPDATE	0
#define I_FULLVIEW	1
#define I_STATBAR	2
#define I_MESSAGES	4
#define I_FULLSCRN	8

extern void I_TimerISR(task *task);
