#ifndef __OPTIONS__
#define __OPTIONS__

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_CGA) || defined(MODE_CVB) || defined(MODE_CGA_BW) || defined(MODE_PCP) || defined(MODE_EGA) || defined(MODE_EGA640)
#define SUPPORTS_HERCULES_AUTOMAP
#endif

#if defined(MODE_13H) || defined(MODE_CGA_BW) || defined(MODE_CGA16) || defined(MODE_CGA) || defined(MODE_CVB) || defined(MODE_HERC) || defined(MODE_INCOLOR) || defined(MODE_PCP) || defined(MODE_VBE2) || defined(MODE_EGA) || defined(MODE_CGA_AFH) || defined(MODE_CGA512) || defined(MODE_SIGMA) || defined(MODE_EGA640)
#define USE_BACKBUFFER
#endif

#if defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T8025) || defined(MODE_T8043) || defined(MODE_T8050)
#define TEXT_MODE
#endif

#endif
