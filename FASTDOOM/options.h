#ifndef __OPTIONS__
#define __OPTIONS__

#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_V2) || defined(MODE_CGA) || defined(MODE_EGA640) || defined(MODE_CVB) || defined(MODE_CGA_BW) || defined(MODE_PCP) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA14)
#define SUPPORTS_HERCULES_AUTOMAP
#endif

#if defined(MODE_13H) || defined(MODE_ATI640) || defined(MODE_CGA_BW) || defined(MODE_CGA16) || defined(MODE_CGA136) || defined(MODE_CGA) || defined(MODE_CVB) || defined(MODE_EGA640) || defined(MODE_EGA16) || defined(MODE_EGA136) || defined(MODE_HERC) || defined(MODE_PCP) || defined(MODE_V2) || defined(MODE_VGA16) || defined(MODE_VGA136) || defined(MODE_VBE2) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA14) || defined(MODE_CGA_AFH)
#define USE_BACKBUFFER
#endif

#endif
