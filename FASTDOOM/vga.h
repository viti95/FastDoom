#include "doomtype.h"

#define PEL_WRITE_ADR 0x3c8
#define PEL_READ_ADR 0x3c7
#define PEL_DATA 0x3c9
#define PEL_MASK 0x3c6

#define SC_INDEX 0x3C4
#define SC_DATA 0x3C5

#define CRTC_INDEX 0x3D4
#define CRTC_DATA 0x3D5

#define GC_INDEX 0x3CE
#define GC_DATA 0x3CF

#define SYNC_RESET 0
#define MAP_MASK 2
#define MEMORY_MODE 4

#define READ_MAP 4
#define GRAPHICS_MODE 5
#define MISCELLANOUS 6

#define MISC_OUTPUT 0x3C2

#define MAX_SCAN_LINE 9
#define UNDERLINE 0x14
#define MODE_CONTROL 0x17

void VGA_TestFastSetPalette(void);
