#ifndef __OPTIONS__
#define __OPTIONS__

#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(MODE_13H) || defined(MODE_VBE2) || defined(MODE_V2) || defined(MODE_CGA) || defined(MODE_EGA640) || defined(MODE_CVB) || defined(MODE_CGA_BW) || defined(MODE_PCP) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA)
#define SUPPORTS_HERCULES_AUTOMAP
#endif

#if defined(MODE_13H) || defined(MODE_ATI640) || defined(MODE_CGA_BW) || defined(MODE_CGA16) || defined(MODE_CGA) || defined(MODE_CVB) || defined(MODE_EGA640) || defined(MODE_HERC) || defined(MODE_PCP) || defined(MODE_V2) || defined(MODE_VGA16) || defined(MODE_VBE2) || defined(MODE_EGA80) || defined(MODE_EGAW1) || defined(MODE_EGA) || defined(MODE_CGA_AFH) || defined(MODE_CGA512)
#define USE_BACKBUFFER
#endif

#if defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T8025) || defined(MODE_T8043) || defined(MODE_T8050)
#define TEXT_MODE
#endif

#endif
