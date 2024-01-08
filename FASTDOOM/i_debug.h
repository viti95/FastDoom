#ifndef __I_DEBUG_H__
#define __I_DEBUG_H__
#include "fastmath.h"
#include "i_system.h"

// DEBUG_ENABLED is set by the build.sh

#if (DEBUG_ENABLED == 1)
#include "dbgcfg.h"
#ifdef DEBUG_UNCONFIGURED
#error "DEBUG_ENABLED is set but debugging is not configured in dbgcfg.h"
#endif
#endif // DEBUG_ENABLED

void I_Printf(const char *format, ...);
void I_SetCursor(int x, int y);
void I_Clear();
void I_DebugInit();

typedef struct debugmodule_t {
    const char *name;
} debugmodule_t;

typedef struct debugsymbol_t {
    int addr;
    const char *name;
    debugmodule_t *module;
    int is_data;
} debugsymbol_t;

debugsymbol_t *I_LookupSymbol(int addr);

const char* I_LookupSymbolName(void* addr);

void I_Backtrace(const char *msg, ...);

void I_DebugShutdown(void);


#if BOUNDS_CHECK_ENABLED == 1
#define BOUNDS_CHECK(x, y)                                                     \
    if ((unsigned)(x) >= SCREENWIDTH || (unsigned)(y) >= SCREENHEIGHT) {       \
      I_Backtrace("Bounds check failure at %s:%i, X:%i Y:%i\n", __FILE__,      \
                  __LINE__, x, y);                                             \
    }
#define SCALED_BOUNDS_CHECK(x, y)                                              \
    if ((unsigned)(x) >= SCALED_SCREENWIDTH ||                                 \
        (unsigned)(y) >= SCALED_SCREENHEIGHT) {                                \
      I_Backtrace("Scaled bounds check failure at %s::%i X:%i Y:%i\n",         \
                  __FILE__, __LINE__, x, y);                                   \
    }
#else
#define BOUNDS_CHECK(x, y)
#define SCALED_BOUNDS_CHECK(x, y)
#endif

#if ASSERT_ENABLED == 1
#define ASSERT(x)                                                              \
    if (!(x)) {                                                                \
      I_Backtrace("Assertion failure at %s:%i\n", __FILE__, __LINE__);         \
    }
#else
#define ASSERT(x) do {} while(0)
#endif

#endif // __I_DEBUG_H__
