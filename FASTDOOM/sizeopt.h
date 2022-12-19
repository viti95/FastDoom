#ifndef __SIZEOPT__
#define __SIZEOPT__

#if defined(MODE_T4025)
#define scaledviewwidth 40
#define viewwidth 40
#define viewwidthhalf 20
#define viewwidthlimit 39
#define viewheight 25
#define viewheightshift 25 << 16
#define viewheightopt (25 << 16) - 25
#define viewheight32 (25 << 16 | 25)
#define viewwindowx 0
#define viewwindowy 0
#define centerx 20
#define centery 12
#define centerxfrac 1310720
#define centeryfrac 786432
#define centeryfracshifted 49152
#define projection 1310720
#define pspritescale 8192
#define pspriteiscale 524288
#define pspriteiscaleneg -524288
#define pspriteiscaleshifted 524288
#endif

#if defined(MODE_T4050)
#define scaledviewwidth 80
#define viewwidth 40
#define viewwidthhalf 20
#define viewwidthlimit 39
#define viewheight 50
#define viewheightshift 50 << 16
#define viewheightopt (50 << 16) - 50
#define viewheight32 (50 << 16 | 50)
#define viewwindowx 0
#define viewwindowy 0
#define centerx 20
#define centery 25
#define centerxfrac 1310720
#define centeryfrac 1638400
#define centeryfracshifted 102400
#define projection 1310720
#define pspritescale 8192
#define pspriteiscale 524288
#define pspriteiscaleneg -524288
#define pspriteiscaleshifted 262144
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_MDA)
#define scaledviewwidth 80
#define viewwidth 80
#define viewwidthhalf 40
#define viewwidthlimit 79
#define viewheight 50
#define viewheightshift 50 << 16
#define viewheightopt (50 << 16) - 50
#define viewheight32 (50 << 16 | 50)
#define viewwindowx 0
#define viewwindowy 0
#define centerx 40
#define centery 25
#define centerxfrac 2621440
#define centeryfrac 1638400
#define centeryfracshifted 102400
#define projection 2621440
#define pspritescale 16384
#define pspriteiscale 262144
#define pspriteiscaleneg -262144
#define pspriteiscaleshifted 262144
#endif

#if defined(MODE_T8043)
#define scaledviewwidth 80
#define viewwidth 80
#define viewwidthhalf 40
#define viewwidthlimit 79
#define viewheight 43
#define viewheightshift 43 << 16
#define viewheightopt (43 << 16) - 43
#define viewheight32 (43 << 16 | 43)
#define viewwindowx 0
#define viewwindowy 0
#define centerx 40
#define centery 21
#define centerxfrac 2621440
#define centeryfrac 1376256
#define centeryfracshifted 86016
#define projection 2621440
#define pspritescale 16384
#define pspriteiscale 262144
#define pspriteiscaleneg -262144
#define pspriteiscaleshifted 262144
#endif

#if defined(MODE_T8086)
#define scaledviewwidth 160
#define viewwidth 80
#define viewwidthhalf 40
#define viewwidthlimit 79
#define viewheight 86
#define viewheightshift 86 << 16
#define viewheightopt (86 << 16) - 86
#define viewheight32 (86 << 16 | 86)
#define viewwindowx 0
#define viewwindowy 0
#define centerx 40
#define centery 43
#define centerxfrac 2621440
#define centeryfrac 2752512
#define centeryfracshifted 172032
#define projection 2621440
#define pspritescale 16384
#define pspriteiscale 262144
#define pspriteiscaleneg -262144
#define pspriteiscaleshifted 131072
#endif

#endif
