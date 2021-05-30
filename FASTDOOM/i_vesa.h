#ifndef _VESAVBE_H
#define _VESAVBE_H

#pragma pack(1)

/*-----------------05-14-97 05:19pm-----------------
  *
  *     SUBMiSSiVES VESA VBE 2.0 Application Core
  *     -----------------------------------------
  *
  * tested and works well under Dos4G, PmodeW and Win95
  *
  * This code also works for buggy VBE's and finds
  * the mode even if the modelist is placed in dos-memory.
  *
  * * Bugfixes:     Now also supports a fix for S3 chips.
  * * Changes:	    Removed Matrox Mystique Support since noone used it..
  * * Bugfixes:	    Now a more clever BPP detection (since some bioses sucks)
  *                 Enhanced FindMode function (now terminates on 0)
  *
  * --------------------------------------------------*/

/*-----------------05-25-97 05:44pm-----------------
  *      Diagnostic and Configuration Switches
  * --------------------------------------------------*/

/* Adds some special code to avid crashes on some old S3 Bioses */
//#define S3FIX

/* Disables the Proteted Mode Extensions (buggy on some cards (e.g. matrox)) */
//#define DISABLE_PM_EXTENSIONS

/* Disables the Linar Frame Buffer */
// #define DISABLE_LFB

/* Enables Debugging Option */
//#define DEBUG_VESA

/* Make a Beep when a function fails */
//#define DEBUG_SOUND

/* Filename of Error and execution logfile */
//#define DEBUGFILENAME ".\\vesavbe.log"

#include <i86.h>
#include <string.h>

#define VESA_OK 0
#define VESA_FAIL 1
#define VESA_NOTSUPPORTED 2
#define VESA_INVALIDMODE 3

/* Bit-Definition of the ControllerInfo Capability fields */
#define CAP_8bit_DAC 1
#define CAP_VGACompatible 2
#define CAP_Use_VBE_DAC_Functions 4

/* Bit-Definition of the ModeInfo Attribute fields */
#define ATTR_HardwareMode 1
#define ATTR_TTY_Support 2
#define ATTR_ColorMode 4
#define ATTR_GraphicsMode 8
#define ATTR_No_VGA_Mode 16
#define ATTR_No_SegA000 32
#define ATTR_LFB_Support 64

/* Bit-Definitions of the Window Attributes */
#define WATTR_Relocatable 1
#define WATTR_Readable 2
#define WATTR_Writeable 4

/* Definitions of the MemoryModel Field */
#define MM_TextMode 0
#define MM_CGA_Graphics 1
#define MM_Hercules_Graphics 2
#define MM_Planar 3
#define MM_PackedPixel 4
#define MM_UnChained 5
#define MM_DirectColor 6
#define MM_YUV 7

#pragma pack(1);

typedef void (*tagSetDisplayStartType)(short x, short y);
typedef void (*tagSetBankType)(short bnk);
typedef void (*tagSetPaletteType)(unsigned char *palptr);

struct bcd16
{
  unsigned char lo;
  unsigned char hi;
};

struct DPMI_PTR
{
  unsigned short int segment;
  unsigned short int selector;
};

struct VBE_VbeInfoBlock
{
  char vbeSignature[4];
  struct bcd16 vbeVersion;
  char *OemStringPtr;
  unsigned long Capabilities;
  unsigned short *VideoModePtr;
  unsigned short TotalMemory;
  unsigned short OemSoftwareRev;
  char *OemVendorNamePtr;
  char *OemProductNamePtr;
  char *OemProductRevPtr;
  char Reserved[222];
  char OemData[256];
};

struct VBE_ModeInfoBlock
{
  unsigned short ModeAttributes;
  char WinAAttributes;
  char WinBAttributes;
  unsigned short Granularity;
  unsigned short WinSize;
  unsigned short WinASegment;
  unsigned short WinBSegment;
  void *WinFuncPtr;
  unsigned short BytesPerScanline;
  unsigned short XResolution;
  unsigned short YResolution;
  char XCharSize;
  char YCharSize;
  char NumberOfPlanes;
  char BitsPerPixel;
  char NumberOfBanks;
  char MemoryModel;
  char BankSize;
  char NumberOfImagePages;
  char Reserved;
  char RedMaskSize;
  char RedFieldPosition;
  char GreenMaskSize;
  char GreenFieldPosition;
  char BlueMaskSize;
  char BlueFieldPosition;
  char RsvdMaskSize;
  char RsvdFieldPosition;
  char DirectColorModeInfo;
  void *PhysBasePtr;
  void *OffScreenMemOffset;
  unsigned short OffScreenMemSize;
  char reserved2[206];
};

// ------------------------------------------------------------------------
//                          Setup Routines
// ------------------------------------------------------------------------

void VBE_Init(void);

/*
   *
   * Must be called before any of the other functions will be called,
   * otherwise your system will crash
   *
   */

void VBE_Done(void);

/*
   *
   * Frees all memory allocated by VBE_Init.. Not a must to call, but
   * it frees about 1k
   *
   */

// ------------------------------------------------------------------------
//                          DPMI Support Functions..
// ------------------------------------------------------------------------

void DPMI_AllocDOSMem(short int paras, struct DPMI_PTR *p);

/*
  *
  * Allocate paras numbers of memory-paragraphs (16 bytes)
  * in realmode memory and store the segment/selector in
  * structure p
  *
  */

void DPMI_FreeDOSMem(struct DPMI_PTR *p);

/*
  *
  * Free a allocated ralmode memory block
  *
  */

void *DPMI_MAP_PHYSICAL(void *p, unsigned long size);
void DPMI_UNMAP_PHYSICAL(void *p);

/*
  *
  * Create a physical mapping for the memory at p to p+size.
  * Returns that pointer (doesn't work with realmode memory)
  *
  */

// ------------------------------------------------------------------------
//                      VBE/VESA Low Level Functions
// ------------------------------------------------------------------------

int VBE_Test(void);

/*
  *
  * Returns true if a VBE BIOS-extension version 2.0 or greater is available
  * Returns zero otherwise
  *
  */

void VBE_Controller_Information(struct VBE_VbeInfoBlock *a);

/*
  *
  * Request the VberInfoBlock. Look into the VBE 2.0 Specs. for more
  * information.
  *
  */

int VBE_IsModeLinear(short Mode);

/*
  *
  * Returns nonzero if specified mode supports a linear framebuffer
  *
  */

extern tagSetBankType VBE_SetBank;

/*
  * void VBE_SetBank (short bnk);
  *
  * Sets the A-Window to the specified bank. 0xa0000 points to another
  * location of the video memory. Be aware, that the bank is dependent
  * of the granularity field in the modeinfoblock.
  *
  */

extern tagSetPaletteType VBE_SetPalette;

/*
  * void VBE_SetPalette (unsigned char *palptr);
  *
  * Sets the video card palette.
  * 4 bytes per palette entry, BGRx format. The x isn't used at all.
  * WTF were they thinking!
  *
  */

unsigned int VBE_VideoMemory(void);

/*
  *
  * Return the amount of display memory available  (in bytes)
  *
  */

void VBE_Mode_Information(short Mode, struct VBE_ModeInfoBlock *a);

/*
  *
  * Request the ModeInfoBlock for the specified mode. Look into the VBE 2.0
  * specs. for more information.
  *
  */

int VBE_FindMode(int xres, int yres, char bpp);

/*
  *
  * Search the modelist for the specified mode. bpp is BytesPerPixel
  * (8 for 256 colors). Returns -1 on error
  *
  */

void VBE_SetMode(short Mode, int linear, int clear);

/*
  *
  * Set VGA to the specified VBE-Mode. Use linear frame buffer if linear==1
  * Clear the video-memory if clear==1
  *
  */

char *VBE_GetVideoPtr(short mode);

/*
  * Get the (mapped) pointer to the linear frame buffer for the specified
  * mode. May fail on weired DPMI-Systems (tested: pmodew, dos4g, win95)
  */

extern tagSetDisplayStartType VBE_SetDisplayStart;

/* void VBE_SetDisplayStart (short x, short y); */

/*
  * Adjust the position of the logical upper left pixel.
  * (for scrolling & page flipping)
  */

void VBE_SetPixelsPerScanline(short Pixels);
/*
  * Sets the number of pixels per scanline. Warning, not all VESA VBE
  * implementations support this feature. However, it might be usefull
  * if you have to deal with non linear frame buffers.
  */

short VBE_MaxBytesPerScanline(void);
/*
  * Returns the maxumum bytes per scanline. Unfortunately this function is
  * more or less useless because most vesa implementations doesn't give valid
  * results.
  */

void VBE_SetDACWidth(char bits);

/*
  * Sets the DAC into the 6 (default) or 8 bit color mode. You have to
  * check first if the DAC supports 8 bit using the VBE_8BitDAC function.
  *
  */

int VBE_8BitDAC(void);

/*
  * Returns non-zero, if the DAC can be switched into the 8-Bit mode.
  * (only usefull in 256 color modes. Gives you a better color-control)
  *
  */

#endif
