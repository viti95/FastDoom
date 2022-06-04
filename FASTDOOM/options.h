#ifndef __OPTIONS__
#define __OPTIONS__

#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_V2) || defined(MODE_CGA) || defined(MODE_EGA) || defined(MODE_EGA640) || defined(MODE_CVB) || defined(MODE_CGA_BW)
#define SUPPORTS_HERCULES_AUTOMAP
#endif

#endif