#include "i_vesa.h"
#include <stdio.h>
#include <malloc.h>
#include <conio.h>
#include <string.h>
#include "options.h"
#include "r_defs.h"
#include "i_ibm.h"
#include "m_menu.h"
#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "z_zone.h"
#include "math.h"
#include "i_gamma.h"
#include "fpummx.h"
// #include "i_debug.h"

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

/*
 * Structure to do the Realmode Interrupt Calls.
 */

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)

#pragma pack(1)

#define VBE2SIGNATURE "VBE2"
#define VBE3SIGNATURE "VBE3"

void (*finishfunc)(void);
void (*processpalette)(byte *palette);
void (*setpalette)(int numpalette);
byte *processedpalette;
byte *ptrprocessedpalette;

#define PEL_WRITE_ADR 0x3c8
#define PEL_DATA 0x3c9

unsigned char vesaScaleMax;
unsigned char* vesaScaleStart;
unsigned int vesaScanlineSize;

/*-----------------07-26-97 11:04am-----------------
 * Realmode Callback Structure!
 * --------------------------------------------------*/

static struct rminfo
{
  unsigned long EDI;
  unsigned long ESI;
  unsigned long EBP;
  unsigned long reserved_by_system;
  unsigned long EBX;
  unsigned long EDX;
  unsigned long ECX;
  unsigned long EAX;
  unsigned short flags;
  unsigned short ES, DS, FS, GS, IP, CS, SP, SS;
} RMI;

/*
 * Structures that hold the preallocated DOS-Memory Aeras
 * and their translated near-pointers!
 */

static struct DPMI_PTR VbeInfoPool = {0, 0};
static struct DPMI_PTR VbeModePool = {0, 0};
static struct VBE_VbeInfoBlock *VBE_Controller_Info_Pointer;
static struct VBE_ModeInfoBlock *VBE_ModeInfo_Pointer;
static void *LastPhysicalMapping = NULL;
/*
 * Structures, that hold the Informations needed to invoke
 * PM-Interrupts
 */

static union REGS regs;
static struct SREGS sregs;

/*
 *  This function pointers will be initialized after you called
 *  VBE_Init. It'll be set to the realmode or protected mode call
 *  code
 */

static void PrepareRegisters(void)
{
  memset(&RMI, 0, sizeof(RMI));
  memset(&sregs, 0, sizeof(sregs));
  memset(&regs, 0, sizeof(regs));
}

static void RMIRQ10()
{
  memset(&regs, 0, sizeof(regs));
  regs.w.ax = 0x0300; // Simulate Real-Mode interrupt
  regs.h.bl = 0x10;
  sregs.es = FP_SEG(&RMI);
  regs.x.edi = FP_OFF(&RMI);
  int386x(0x31, &regs, &regs, &sregs);
}

void DPMI_AllocDOSMem(short int paras, struct DPMI_PTR *p)
{
  /* DPMI call 100h allocates DOS memory */
  PrepareRegisters();
  regs.w.ax = 0x0100;
  regs.w.bx = paras;
  int386x(0x31, &regs, &regs, &sregs);
  p->segment = regs.w.ax;
  p->selector = regs.w.dx;
}

void DPMI_FreeDOSMem(struct DPMI_PTR *p)
{
  /* DPMI call 101h free DOS memory */
  memset(&sregs, 0, sizeof(sregs));
  regs.w.ax = 0x0101;
  regs.w.dx = p->selector;
  int386x(0x31, &regs, &regs, &sregs);
}

void DPMI_UNMAP_PHYSICAL(void *p)
{
  /* DPMI call 800h map physical memory*/
  PrepareRegisters();
  regs.w.ax = 0x0801;
  regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
  regs.w.cx = (unsigned short)(((unsigned long)p) & 0xffff);
  int386x(0x31, &regs, &regs, &sregs);
}

#define MEMORY_LIMIT (128 * 64 * 1024)

void *DPMI_MAP_PHYSICAL(void *p, unsigned long size)
{
  // Limit memory to 8Mb, mapping 16Mb crashes
  if (size > MEMORY_LIMIT)
    size = MEMORY_LIMIT;

  /* DPMI call 800h map physical memory*/
  PrepareRegisters();
  regs.w.ax = 0x0800;
  regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
  regs.w.cx = (unsigned short)(((unsigned long)p) & 0xffff);
  regs.w.si = (unsigned short)(size >> 16);
  regs.w.di = (unsigned short)(size & 0xffff);
  int386x(0x31, &regs, &regs, &sregs);
  return (void *)(((unsigned long)regs.w.bx << 16) | regs.w.cx);
}

void VBE_Controller_Information(struct VBE_VbeInfoBlock *a)
{
  memcpy(a, VBE_Controller_Info_Pointer, sizeof(struct VBE_VbeInfoBlock));
}

void VBE_Mode_Information(short Mode, struct VBE_ModeInfoBlock *a)
{
  PrepareRegisters();
  RMI.EAX = 0x00004f01; // Get SVGA-Mode Information
  RMI.ECX = Mode;
  RMI.ES = VbeModePool.segment; // Segment of realmode data
  RMI.EDI = 0;                  // offset of realmode data
  RMIRQ10();
  memcpy(a, VBE_ModeInfo_Pointer, sizeof(struct VBE_ModeInfoBlock));
}

int VBE_IsModeLinear(short Mode)
{
  struct VBE_ModeInfoBlock a;
  if (Mode == 0x13)
  {
    return 1;
  }

  VBE_Mode_Information(Mode, &a);
  return ((a.ModeAttributes & 128) == 128);
}

void VBE_SetDisplayStart(short x, short y)
{
  PrepareRegisters();
  RMI.EAX = 0x00004f07;
  RMI.EBX = 0;
  RMI.ECX = x;
  RMI.EDX = y;
  RMIRQ10();
}

void VBE_SetDisplayStart_Y(short y)
{
  PrepareRegisters();
  RMI.EAX = 0x00004f07;
  RMI.EBX = 0;
  RMI.ECX = 0;
  RMI.EDX = y;
  RMIRQ10();
}

void VBE_SetBank(short bank)
{
  PrepareRegisters();
  RMI.EAX = 0x00004f05;
  RMI.EBX = 0;
  RMI.EDX = bank;
  RMIRQ10();
}

void setbiosmode(unsigned short c);
#pragma aux setbiosmode = "int 0x10" parm[ax] modify[eax ebx ecx edx esi edi];

void VBE_SetMode(short Mode, int linear, int clear)
{
  struct VBE_ModeInfoBlock a;
  int rawmode;
  /* Is it a normal textmode? if so set it directly! */
  if (Mode == 3)
  {
    setbiosmode(Mode);
    return;
  }
  if (Mode == 0x13)
  {
    setbiosmode(Mode);
    return;
  }
  rawmode = Mode & 0x0fff;
  if (linear)
    rawmode |= 1 << 14;
  if (!clear)
    rawmode |= 1 << 15;

  /* Ordinary ModeSwitch call without S3Fix */
  PrepareRegisters();
  RMI.EAX = 0x00004f02;
  RMI.EBX = rawmode;
  RMIRQ10();

  // get the current mode-info block and set some parameters...
  VBE_Mode_Information(Mode, &a);
}

char *VBE_GetVideoPtr(short mode)
{
  void *phys;
  struct VBE_ModeInfoBlock ModeInfo;
  if (mode == 13)
    return (char *)0xa0000;
  VBE_Mode_Information(mode, &ModeInfo);
  /* Unmap the last physical mapping (if there is one...) */
  if (LastPhysicalMapping)
  {
    DPMI_UNMAP_PHYSICAL(LastPhysicalMapping);
    LastPhysicalMapping = NULL;
  }
  LastPhysicalMapping = DPMI_MAP_PHYSICAL((void *)ModeInfo.PhysBasePtr,
                                          ((unsigned long)VBE_Controller_Info_Pointer->TotalMemory) * 64 * 1024);
  return (char *)LastPhysicalMapping;
}

void VBE_SetDACWidth(char bits)
{
  PrepareRegisters();
  RMI.EAX = 0x00004f08;
  RMI.EBX = bits << 8;
  RMIRQ10();
}

void VBE_Init(void)
{
  FILE *f;

  /* Allocate the Dos Memory for Mode and Controller Information Blocks */
  /* and translate their pointers into flat memory space                */
  DPMI_AllocDOSMem(512 / 16, &VbeInfoPool);
  DPMI_AllocDOSMem(256 / 16, &VbeModePool);
  VBE_ModeInfo_Pointer = (struct VBE_ModeInfoBlock *)(VbeModePool.segment * 16);
  VBE_Controller_Info_Pointer = (struct VBE_VbeInfoBlock *)(VbeInfoPool.segment * 16);

  /* Get Controller Information Block only once and copy this block on  */
  /* all requests                                                       */
  memset(VBE_Controller_Info_Pointer, 0, sizeof(struct VBE_VbeInfoBlock));
  memcpy(VBE_Controller_Info_Pointer->vbeSignature, VBE2SIGNATURE, 4);
  PrepareRegisters();
  RMI.EAX = 0x00004f00;         // Get SVGA-Information
  RMI.ES = VbeInfoPool.segment; // Segment of realmode data
  RMI.EDI = 0;                  // offset of realmode data
  RMIRQ10();
  // Translate the Realmode Pointers into flat-memory address space
  VBE_Controller_Info_Pointer->OemStringPtr = (char *)((((unsigned long)VBE_Controller_Info_Pointer->OemStringPtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->OemStringPtr);
  VBE_Controller_Info_Pointer->VideoModePtr = (unsigned short *)((((unsigned long)VBE_Controller_Info_Pointer->VideoModePtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->VideoModePtr);
  VBE_Controller_Info_Pointer->OemVendorNamePtr = (char *)((((unsigned long)VBE_Controller_Info_Pointer->OemVendorNamePtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->OemVendorNamePtr);
  VBE_Controller_Info_Pointer->OemProductNamePtr = (char *)((((unsigned long)VBE_Controller_Info_Pointer->OemProductNamePtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->OemProductNamePtr);
  VBE_Controller_Info_Pointer->OemProductRevPtr = (char *)((((unsigned long)VBE_Controller_Info_Pointer->OemProductRevPtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->OemProductRevPtr);
}

void VBE_Done(void)
{
  if (LastPhysicalMapping)
  {
    DPMI_UNMAP_PHYSICAL(LastPhysicalMapping);
  }
  DPMI_FreeDOSMem(&VbeModePool);
  DPMI_FreeDOSMem(&VbeInfoPool);
}

static struct VBE_VbeInfoBlock vbeinfo;
static struct VBE_ModeInfoBlock vbemode;
unsigned short vesavideomode = 0xFFFF;
unsigned char vesabitsperpixel = -1;
int vesalinear = -1;
int vesamode = -1;
unsigned long vesamemory = -1;
char *vesavideoptr;
unsigned int vesascanlinefix = 0;
unsigned int vesaWidth = SCREENWIDTH;
unsigned int vesaHeight = SCREENHEIGHT;

int VBE2_FindVideoMode(unsigned short screenwidth, unsigned short screenheight, char bitsperpixel, int isLinear)
{
  int mode;

  // Find a suitable video mode
  for (mode = 0; vbeinfo.VideoModePtr[mode] != 0xFFFF; mode++)
  {
    unsigned short localvesavideomode = vbeinfo.VideoModePtr[mode];

    VBE_Mode_Information(localvesavideomode, &vbemode);
    if (vbemode.XResolution == screenwidth && vbemode.YResolution == screenheight && vbemode.BitsPerPixel == bitsperpixel)
    {
      int isVesaLinear;

      if ((bitsperpixel == 16) && (vbemode.GreenMaskSize == 5))
      {
        // Fix for AMD Radeon (GCN?) cards
        continue;
      }

      isVesaLinear = VBE_IsModeLinear(localvesavideomode);

      if (isVesaLinear == isLinear)
      {
        vesamode = mode;
        vesavideomode = localvesavideomode;
        vesalinear = isVesaLinear;
        vesabitsperpixel = bitsperpixel;

        if (vbemode.BytesPerScanline != vesaWidth*bitsperpixel)
        {
          int bpp = bitsperpixel;

          if (bpp == 15)
            bpp = 16;

          vesascanlinefix = vbemode.BytesPerScanline - vesaWidth*(bpp/8);
        }

        return 1;
      }
    }
  }

  return 0;
}

void VBE2_InitGraphics(void)
{

  char bitsperpixel[] = {8, 16, 15, 24, 32}; // Modes to test
  int i;
  int linearModeFound = 0;

#if defined(MODE_VBE2)
  if (vesaScaleOutput && vesaScaleOutputHeight > SCREENHEIGHT && vesaScaleOutputWidth > SCREENWIDTH) {
    vesaWidth = vesaScaleOutputWidth;
    vesaHeight = vesaScaleOutputHeight;
  }
#endif
  
  VBE_Init();

  // Get VBE info
  VBE_Controller_Information(&vbeinfo);
  vesamemory = ((unsigned long)vbeinfo.TotalMemory) * 64;
  // Get VBE modes

  if (forceVesaBitsPerPixel > 0)
  {
    // Test for requested linear mode
    if (forceVesaNonLinear == 0)
    {
      if (VBE2_FindVideoMode(vesaWidth, vesaHeight, forceVesaBitsPerPixel, 1))
      {
        linearModeFound = 1;
      }
    }

    // If not found, test for requested non-linear modes
    if (!linearModeFound)
    {
      VBE2_FindVideoMode(vesaWidth, vesaHeight, forceVesaBitsPerPixel, 0);
    }

  } else {

    // Test for linear VBE compatible modes
    if (forceVesaNonLinear == 0)
    {
      for (i = 0; i < 5; i++) // Test each bit depth
      {
        if (VBE2_FindVideoMode(vesaWidth, vesaHeight, bitsperpixel[i], 1))
        {
          linearModeFound = 1;
          break;
        }
      }
    }
    
    if (!linearModeFound)
    {
      // Test for non-linear vesa modes
      for (i = 0; i < 5; i++) // Test each bit depth
      {
        if (VBE2_FindVideoMode(vesaWidth, vesaHeight, bitsperpixel[i], 0))
        {
          break;
        }
      }
    }
  }

  /*I_Printf("VESA mode: %d\n", vesamode);
  I_Printf("VESA video mode: %d\n", vesavideomode);
  I_Printf("VESA isLinear: %d\n", vesalinear);
  I_Printf("VESA width: %d\n", SCREENWIDTH);
  I_Printf("VESA height: %d\n", SCREENHEIGHT);
  I_Printf("VESA bits per pixel: %d\n", vesabitsperpixel);*/

#if defined(MODE_VBE2_DIRECT)
  if (vesabitsperpixel > 8)
  {
    I_Error(10);
  }
#endif

  // If a VESA compatible mode is found, use it!
  if (vesavideomode != 0xFFFF)
  {
#if defined(MODE_VBE2_DIRECT)
    // Check for available offscreen memory for tripple buffering + border on fourth vram buffer
    if (vesamemory < SCREENWIDTH * SCREENHEIGHT * 4 / 1024)
    {
      I_Error(19, SCREENWIDTH * SCREENHEIGHT * 4 / 1024, vesamemory);
    }
#endif
    VBE_SetMode(vesavideomode, vesalinear, 1);

    if (vesalinear == 1)
    {
      pcscreen = destscreen = (void *)VBE_GetVideoPtr(vesavideomode);
    }
    else
    {
      pcscreen = destscreen = (void *)0xA0000;
    }

    // Force 6 bits resolution per color
    VBE_SetDACWidth(6);

#if defined(MODE_VBE2)
    // Set finish function

    if (vesaScaleOutput) {
      // Scale backbuffer mode
      int maxScaleWidth = vesaWidth / SCREENWIDTH;
      int maxScaleHeight = vesaHeight / SCREENHEIGHT;
      vesaScaleMax = (maxScaleWidth < maxScaleHeight) ? maxScaleWidth : maxScaleHeight;
      vesaScanlineSize = vbemode.BytesPerScanline;

      if (pcscreen == (void *)0xA0000) {
        // Banked scaled

        switch (vesabitsperpixel)
        {
        case 8:
          vesaScaleStart = pcscreen + ((vesaHeight - SCREENHEIGHT * vesaScaleMax) / 2) * vesaScanlineSize + (vesaScaleOutputWidth - SCREENWIDTH * vesaScaleMax) / 2;
          switch (vesaScaleMax) {
            case 1: finishfunc = I_FinishUpdate8bppBankedScale1x; break;
            case 2: finishfunc = I_FinishUpdate8bppBankedScale2x; break;
            case 3: finishfunc = I_FinishUpdate8bppBankedScale3x; break;
            case 4: finishfunc = I_FinishUpdate8bppBankedScale4x; break;
            case 5: finishfunc = I_FinishUpdate8bppBankedScale5x; break;
          }
          processpalette = I_ProcessPalette8bpp;
          setpalette = I_SetPalette8bpp;
          processedpalette = Z_MallocUnowned(14 * 768, PU_STATIC);
          break;
        case 15:
          vesaScaleStart = pcscreen + ((vesaHeight - SCREENHEIGHT * vesaScaleMax) / 2) * vesaScanlineSize + (vesaScaleOutputWidth - SCREENWIDTH * vesaScaleMax) / 2 * 2;
          switch (vesaScaleMax) {
            case 1: finishfunc = I_FinishUpdate15bppBankedScale1x; break;
            case 2: finishfunc = I_FinishUpdate15bppBankedScale2x; break;
            case 3: finishfunc = I_FinishUpdate15bppBankedScale3x; break;
            case 4: finishfunc = I_FinishUpdate15bppBankedScale4x; break;
            case 5: finishfunc = I_FinishUpdate15bppBankedScale5x; break;
          }
          processpalette = I_ProcessPalette15bpp;
          setpalette = I_SetPalette15bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 2, PU_STATIC);
          break;
        case 16:
          vesaScaleStart = pcscreen + ((vesaHeight - SCREENHEIGHT * vesaScaleMax) / 2) * vesaScanlineSize + (vesaScaleOutputWidth - SCREENWIDTH * vesaScaleMax) / 2 * 2;
          switch (vesaScaleMax) {
            case 1: finishfunc = I_FinishUpdate16bppBankedScale1x; break;
            case 2: finishfunc = I_FinishUpdate16bppBankedScale2x; break;
            case 3: finishfunc = I_FinishUpdate16bppBankedScale3x; break;
            case 4: finishfunc = I_FinishUpdate16bppBankedScale4x; break;
            case 5: finishfunc = I_FinishUpdate16bppBankedScale5x; break;
          }
          processpalette = I_ProcessPalette16bpp;
          setpalette = I_SetPalette16bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 2, PU_STATIC);
          break;
        case 24:
          vesaScaleStart = pcscreen + ((vesaHeight - SCREENHEIGHT * vesaScaleMax) / 2) * vesaScanlineSize + (vesaScaleOutputWidth - SCREENWIDTH * vesaScaleMax) / 2 * 3;
          switch (vesaScaleMax) {
            case 1: finishfunc = I_FinishUpdate24bppBankedScale1x; break;
            case 2: finishfunc = I_FinishUpdate24bppBankedScale2x; break;
            case 3: finishfunc = I_FinishUpdate24bppBankedScale3x; break;
            case 4: finishfunc = I_FinishUpdate24bppBankedScale4x; break;
            case 5: finishfunc = I_FinishUpdate24bppBankedScale5x; break;
          }
          processpalette = I_ProcessPalette24bpp;
          setpalette = I_SetPalette24bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 3, PU_STATIC);
          break;
        case 32:
          vesaScaleStart = pcscreen + ((vesaHeight - SCREENHEIGHT * vesaScaleMax) / 2) * vesaScanlineSize + (vesaScaleOutputWidth - SCREENWIDTH * vesaScaleMax) / 2 * 4;
          switch (vesaScaleMax) {
            case 1: finishfunc = I_FinishUpdate32bppBankedScale1x; break;
            case 2: finishfunc = I_FinishUpdate32bppBankedScale2x; break;
            case 3: finishfunc = I_FinishUpdate32bppBankedScale3x; break;
            case 4: finishfunc = I_FinishUpdate32bppBankedScale4x; break;
            case 5: finishfunc = I_FinishUpdate32bppBankedScale5x; break;
          }
          processpalette = I_ProcessPalette32bpp;
          setpalette = I_SetPalette32bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 4, PU_STATIC);
          break;
        }

      } else {
        // Linear
        
        switch (vesabitsperpixel)
        {
          case 8:
            vesaScaleStart = pcscreen + ((vesaHeight - SCREENHEIGHT * vesaScaleMax) / 2) * (vesaScanlineSize) + (vesaScaleOutputWidth - SCREENWIDTH * vesaScaleMax) / 2;

            switch (vesaScaleMax) {
              case 1:
                finishfunc = I_FinishUpdate8bppLinearScale1x;
                break;
              case 2:
                finishfunc = I_FinishUpdate8bppLinearScale2x;
                break;
              case 3:
                finishfunc = I_FinishUpdate8bppLinearScale3x;
                break;
              case 4:
                finishfunc = I_FinishUpdate8bppLinearScale4x;
                break;
              case 5:
                finishfunc = I_FinishUpdate8bppLinearScale5x;
                break;
            }

            processpalette = I_ProcessPalette8bpp;
            setpalette = I_SetPalette8bpp;
            processedpalette = Z_MallocUnowned(14 * 768, PU_STATIC);
            break;
          case 15:
          case 16:
            vesaScaleStart = pcscreen + ((vesaHeight - SCREENHEIGHT * vesaScaleMax) / 2) * vesaScanlineSize + (vesaScaleOutputWidth - SCREENWIDTH * vesaScaleMax) / 2 * 2;

            switch (vesaScaleMax) {
              case 1:
                finishfunc = I_FinishUpdate15bpp16bppLinearScale1x;
                break;
              case 2:
                finishfunc = I_FinishUpdate15bpp16bppLinearScale2x;
                break;
              case 3:
                finishfunc = I_FinishUpdate15bpp16bppLinearScale3x;
                break;
              case 4:
                finishfunc = I_FinishUpdate15bpp16bppLinearScale4x;
                break;
              case 5:
                finishfunc = I_FinishUpdate15bpp16bppLinearScale5x;
                break;
            }

            processpalette = (vesabitsperpixel == 15) ? I_ProcessPalette15bpp : I_ProcessPalette16bpp;
            setpalette = (vesabitsperpixel == 15) ? I_SetPalette15bpp : I_SetPalette16bpp;
            processedpalette = Z_MallocUnowned(14 * 256 * 2, PU_STATIC);
            break;
          case 24:
            vesaScaleStart = pcscreen + ((vesaHeight - SCREENHEIGHT * vesaScaleMax) / 2) * vesaScanlineSize + (vesaScaleOutputWidth - SCREENWIDTH * vesaScaleMax) / 2 * 3;

            switch (vesaScaleMax) {
              case 1:
                finishfunc = I_FinishUpdate24bppLinearScale1x;
                break;
              case 2:
                finishfunc = I_FinishUpdate24bppLinearScale2x;
                break;
              case 3:
                finishfunc = I_FinishUpdate24bppLinearScale3x;
                break;
              case 4:
                finishfunc = I_FinishUpdate24bppLinearScale4x;
                break;
              case 5:
                finishfunc = I_FinishUpdate24bppLinearScale5x;
                break;
            }

            processpalette = I_ProcessPalette24bpp;
            setpalette = I_SetPalette24bpp;
            processedpalette = Z_MallocUnowned(14 * 256 * 3, PU_STATIC);
            break;
          case 32:
            vesaScaleStart = pcscreen + ((vesaHeight - SCREENHEIGHT * vesaScaleMax) / 2) * vesaScanlineSize + (vesaScaleOutputWidth - SCREENWIDTH * vesaScaleMax) / 2 * 4;

            switch (vesaScaleMax) {
              case 1:
                finishfunc = I_FinishUpdate32bppLinearScale1x;
                break;
              case 2:
                finishfunc = I_FinishUpdate32bppLinearScale2x;
                break;
              case 3:
                finishfunc = I_FinishUpdate32bppLinearScale3x;
                break;
              case 4:
                finishfunc = I_FinishUpdate32bppLinearScale4x;
                break;
              case 5:
                finishfunc = I_FinishUpdate32bppLinearScale5x;
                break;
            }

            processpalette = I_ProcessPalette32bpp;
            setpalette = I_SetPalette32bpp;
            processedpalette = Z_MallocUnowned(14 * 256 * 4, PU_STATIC);
            break;
        }
      }

    } else {
      if (pcscreen == (void *)0xA0000)
      {
        // Banked
        switch (vesabitsperpixel)
        {
        case 8:
          finishfunc = I_FinishUpdate8bppBanked;
          processpalette = I_ProcessPalette8bpp;
          setpalette = I_SetPalette8bpp;
          processedpalette = Z_MallocUnowned(14 * 768, PU_STATIC);
          break;
        case 15:
          finishfunc = I_FinishUpdate15bpp16bppBanked;
          processpalette = I_ProcessPalette15bpp;
          setpalette = I_SetPalette15bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 2, PU_STATIC);
          break;
        case 16:
          finishfunc = I_FinishUpdate15bpp16bppBanked;
          processpalette = I_ProcessPalette16bpp;
          setpalette = I_SetPalette16bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 2, PU_STATIC);
          break;
        case 24:
          finishfunc = I_FinishUpdate24bppBanked;
          processpalette = I_ProcessPalette24bpp;
          setpalette = I_SetPalette24bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 3, PU_STATIC);
          break;
        case 32:
          finishfunc = I_FinishUpdate32bppBanked;
          processpalette = I_ProcessPalette32bpp;
          setpalette = I_SetPalette32bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 4, PU_STATIC);
          break;
        }
      }
      else
      {
        // Linear
        switch (vesabitsperpixel)
        {
        case 8:
          if (vesascanlinefix) {
            finishfunc = I_FinishUpdate8bppLinearFix;
          } else {
            switch(selectedCPU) {
              case CYRIX_6X86:
              case CYRIX_6X86MX:
              case INTEL_PENTIUM_P5_P54C:
              case INTEL_PENTIUM_P54CS:
                finishfunc = I_FinishUpdate8bppLinearFPU;
                break;
              case RISE_MP6:
              case INTEL_PENTIUM_MMX:
              case INTEL_PENTIUM_II:
              case AMD_K6:
                finishfunc = I_FinishUpdate8bppLinearMMX;
                break;
              default:
                finishfunc = I_FinishUpdate8bppLinear;
                break;
            }
          }

          if (!hasFPU && finishfunc == I_FinishUpdate8bppLinearFPU)
            finishfunc = I_FinishUpdate8bppLinear;

          if (!hasMMX && finishfunc == I_FinishUpdate8bppLinearMMX)
            finishfunc = I_FinishUpdate8bppLinear;

          processpalette = I_ProcessPalette8bpp;
          setpalette = I_SetPalette8bpp;
          processedpalette = Z_MallocUnowned(14 * 768, PU_STATIC);
          break;
        case 15:
          if (vesascanlinefix) {
            finishfunc = I_FinishUpdate15bpp16bppLinearFix;
          } else {
            finishfunc = I_FinishUpdate15bpp16bppLinear;
            I_PatchFinishUpdate15bpp16bppLinear();
          }
          
          processpalette = I_ProcessPalette15bpp;
          setpalette = I_SetPalette15bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 2, PU_STATIC);
          break;
        case 16:
          if (vesascanlinefix) {
            finishfunc = I_FinishUpdate15bpp16bppLinearFix;
          } else {
            finishfunc = I_FinishUpdate15bpp16bppLinear;
            I_PatchFinishUpdate15bpp16bppLinear();
          }
          
          processpalette = I_ProcessPalette16bpp;
          setpalette = I_SetPalette16bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 2, PU_STATIC);
          
          break;
        case 24:
          if (vesascanlinefix) {
            finishfunc = I_FinishUpdate24bppLinearFix;
          } else {
            finishfunc = I_FinishUpdate24bppLinear;
            I_PatchFinishUpdate24bppLinear();
          }
          
          processpalette = I_ProcessPalette32bpp;
          setpalette = I_SetPalette32bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 4, PU_STATIC);

          break;
        case 32:
          if (vesascanlinefix) {
            finishfunc = I_FinishUpdate32bppLinearFix;
          } else {
            finishfunc = I_FinishUpdate32bppLinear;
            I_PatchFinishUpdate32bppLinear();
          }
          
          processpalette = I_ProcessPalette32bpp;
          setpalette = I_SetPalette32bpp;
          processedpalette = Z_MallocUnowned(14 * 256 * 4, PU_STATIC);
          
          break;
        }
      }
    }
#endif

#if defined(MODE_VBE2_DIRECT)

    processedpalette = Z_MallocUnowned(14 * 768, PU_STATIC);

    // Check banked video modes that don't fit on the 64 Kb window
    if (pcscreen == (void *)0xA0000)
    {
      if (SCREENWIDTH * SCREENHEIGHT > 65536)
      {
        I_Error(20, SCREENWIDTH * SCREENHEIGHT / 1024);
      }
    }

    // Initialize VRAM
    if (pcscreen == (void *)0xA0000)
    {
      // Banked
      VBE_SetDisplayStart_Y(0);

      VBE_SetBank(0);
      SetDWords((void *)0xA0000, 0, 65536 / 4);
      VBE_SetBank(1);
      SetDWords((void *)0xA0000, 0, 65536 / 4);
      VBE_SetBank(2);
      SetDWords((void *)0xA0000, 0, 65536 / 4);
    }
    else
    {
      // LFB
      VBE_SetDisplayStart_Y(0);
      SetDWords(pcscreen, 0, SCREENWIDTH * SCREENHEIGHT * 3 / 4);
    }

#endif
  }
  else
  {
    I_Error(21, vesaWidth, vesaHeight);
  }
}

#define NUM_BANKS_8BPP ((SCREENHEIGHT * SCREENWIDTH) / (64 * 1024))
#define LAST_BANK_SIZE_8BPP ((SCREENHEIGHT * SCREENWIDTH) - (NUM_BANKS_8BPP * 64 * 1024))

#define NUM_BANKS_15BPP_16BPP ((SCREENHEIGHT * SCREENWIDTH * 2) / (64 * 1024))
#define LAST_BANK_SIZE_15BPP_16BPP ((SCREENHEIGHT * SCREENWIDTH * 2) - (NUM_BANKS_15BPP_16BPP * 64 * 1024))

#define NUM_BANKS_24BPP ((SCREENHEIGHT * SCREENWIDTH * 3) / (64 * 1024))
#define LAST_BANK_SIZE_24BPP ((SCREENHEIGHT * SCREENWIDTH * 3) - (NUM_BANKS_24BPP * 64 * 1024))

#define NUM_BANKS_32BPP ((SCREENHEIGHT * SCREENWIDTH * 4) / (64 * 1024))
#define LAST_BANK_SIZE_32BPP ((SCREENHEIGHT * SCREENWIDTH * 4) - (NUM_BANKS_32BPP * 64 * 1024))

#if defined(MODE_VBE2) || defined(MODE_VBE2_DIRECT)

#if defined(MODE_VBE2_DIRECT)
#define I_ProcessPalette8bpp I_ProcessPalette
#define I_SetPalette8bpp I_SetPalette
#endif

void I_ProcessPalette8bpp(byte *palette)
{
  int i;

  byte *ptr = gammatable;

  for (i = 0; i < 14 * 768; i += 4, palette += 4)
  {
    processedpalette[i] = ptr[*palette];
    processedpalette[i + 1] = ptr[*(palette + 1)];
    processedpalette[i + 2] = ptr[*(palette + 2)];
    processedpalette[i + 3] = ptr[*(palette + 3)];
  }
}

void I_SetPalette8bpp(int numpalette)
{
  int pos = Mul768(numpalette);

  if (VGADACfix)
  {
    int i;
    byte *ptrprocessedpalette = processedpalette + pos;

    I_WaitSingleVBL();

    outp(PEL_WRITE_ADR, 0);

    for (i = 0; i < 768; i += 4)
    {
      outp(PEL_DATA, *(ptrprocessedpalette));
      outp(PEL_DATA, *(ptrprocessedpalette + 1));
      outp(PEL_DATA, *(ptrprocessedpalette + 2));
      outp(PEL_DATA, *(ptrprocessedpalette + 3));
      ptrprocessedpalette += 4;
    }
  }
  else
  {
    FastPaletteOut(((unsigned char *)processedpalette) + pos);
  }
}

void I_ProcessPalette15bpp(byte *palette)
{
  int i, j;

  byte *ptr = gammatable;

  for (i = 0; i < 14 * 256 * 2; i += 2, palette += 3)
  {
    unsigned short r, g, b;
    unsigned short color = 0;

    r = (ptr[*palette] >> 1) & 0x1F;
    g = (ptr[*(palette + 1)] >> 1) & 0x1F;
    b = (ptr[*(palette + 2)] >> 1) & 0x1F;

    // RGB555
    color = (r << 10) | (g << 5) | b;

    processedpalette[i] = color & 0xFF;
    processedpalette[i + 1] = color >> 8;
  }
}

void I_SetPalette15bpp(int numpalette)
{
  ptrprocessedpalette = processedpalette + (numpalette * 256 * 2);
}

void I_ProcessPalette16bpp(byte *palette)
{
  int i, j;

  byte *ptr = gammatable;

  for (i = 0; i < 14 * 256 * 2; i += 2, palette += 3)
  {
    unsigned short r, g, b;
    unsigned short color = 0;

    r = (ptr[*palette] >> 1) & 0x1F;
    g = (ptr[*(palette + 1)]) & 0x3F;
    b = (ptr[*(palette + 2)] >> 1) & 0x1F;

    // RGB565
    color = (r << 11) | (g << 5) | b;

    processedpalette[i] = color & 0xFF;
    processedpalette[i + 1] = color >> 8;
  }
}

void I_SetPalette16bpp(int numpalette)
{
  ptrprocessedpalette = processedpalette + (numpalette * 256 * 2);
}

void I_ProcessPalette24bpp(byte *palette)
{
  int i, j;

  byte *ptr = gammatable;

  for (i = 0; i < 14 * 256 * 3; i += 3, palette += 3)
  {
    unsigned int r, g, b;

    r = ptr[*palette] << 2;
    g = ptr[*(palette + 1)] << 2;
    b = ptr[*(palette + 2)] << 2;

    // RGB888
    processedpalette[i] = b;
    processedpalette[i + 1] = g;
    processedpalette[i + 2] = r;
  }
}

void I_SetPalette24bpp(int numpalette)
{
  ptrprocessedpalette = processedpalette + (numpalette * 256 * 3);
}

void I_ProcessPalette32bpp(byte *palette)
{
  int i, j;

  byte *ptr = gammatable;

  for (i = 0; i < 14 * 256 * 4; i += 4, palette += 3)
  {
    unsigned int r, g, b;

    r = ptr[*palette] << 2;
    g = ptr[*(palette + 1)] << 2;
    b = ptr[*(palette + 2)] << 2;

    // ARGB888
    processedpalette[i] = b;
    processedpalette[i + 1] = g;
    processedpalette[i + 2] = r;
    processedpalette[i + 3] = 0;
  }
}

void I_SetPalette32bpp(int numpalette)
{
  ptrprocessedpalette = processedpalette + (numpalette * 256 * 4);
}

#endif

#if defined(MODE_VBE2)

void I_ProcessPalette(byte *palette)
{
  processpalette(palette);
}

void I_SetPalette(int numpalette)
{
  setpalette(numpalette);
}

void I_FinishUpdate8bppBanked(void)
{
  int i = 0;

  for (i = 0; i < NUM_BANKS_8BPP; i++)
  {
    VBE_SetBank(i);
    CopyDWords(backbuffer + ((64 * 1024) * i), (void *)0xA0000, 64 * 1024 / 4);
  }

#if LAST_BANK_SIZE_8BPP > 0
  VBE_SetBank(NUM_BANKS_8BPP);
  CopyDWords(backbuffer + (NUM_BANKS_8BPP * 64 * 1024), (void *)0xA0000, LAST_BANK_SIZE_8BPP / 4);
#endif
}

void I_FinishUpdate8bppLinearFix(void)
{
  int i,j;

  unsigned char *ptrVRAM = (unsigned char *) pcscreen;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM++)
    {
      *(ptrVRAM) = backbuffer[i + j];
    }

    ptrVRAM+=vesascanlinefix;
  }
}

void I_FinishUpdate8bppLinearScale1x(void)
{
  int i,j;

  unsigned int *ptrVRAM = vesaScaleStart;
  unsigned int *ptrBackbuffer = backbuffer;
  unsigned int vesaScanlineSizeQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j+=4, ptrBackbuffer++, ptrVRAM++)
    {
      *(ptrVRAM) = *(ptrBackbuffer);
    }

    ptrVRAM += vesaScanlineSizeQuarter - SCREENWIDTH/4;
  }
}

void I_FinishUpdate8bppLinearScale2x(void)
{
  int i,j;

  unsigned int *ptrVRAM = vesaScaleStart;
  unsigned int vesaScanlineSizeHalf = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j+=2, ptrVRAM++)
    {
      unsigned char data = backbuffer[i + j];
      unsigned char data2 = backbuffer[i + j + 1];

      unsigned int first = data | data << 8 | data2 << 16 | data2 << 24;

      *(ptrVRAM) = first;
      *(ptrVRAM+vesaScanlineSizeHalf) = first;
    }

    ptrVRAM += 2*vesaScanlineSizeHalf - SCREENWIDTH/2;
  }
}

void I_FinishUpdate8bppLinearScale3x(void)
{
  int i,j;

  unsigned int *ptrVRAM = vesaScaleStart;
  unsigned int vesaScanlineSizeQuarter = vesaScanlineSize/4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j += 4, ptrVRAM += 3)
    {
      unsigned char data = backbuffer[i + j];
      unsigned char data2 = backbuffer[i + j + 1];
      unsigned char data3 = backbuffer[i + j + 2];
      unsigned char data4 = backbuffer[i + j + 3];

      unsigned int first = data2 << 24 | data << 16 | data << 8 | data;
      unsigned int second = data3 << 24 | data3 << 16 | data2 << 8 | data2;
      unsigned int third = data4 << 24 | data4 << 16 | data4 << 8 | data3;

      *(ptrVRAM) = first;
      *(ptrVRAM+1) = second;
      *(ptrVRAM+2) = third;

      *(ptrVRAM+vesaScanlineSizeQuarter) = first;
      *(ptrVRAM+vesaScanlineSizeQuarter+1) = second;
      *(ptrVRAM+vesaScanlineSizeQuarter+2) = third;

      *(ptrVRAM+2*vesaScanlineSizeQuarter) = first;
      *(ptrVRAM+2*vesaScanlineSizeQuarter+1) = second;
      *(ptrVRAM+2*vesaScanlineSizeQuarter+2) = third;
    }

    ptrVRAM += 3*vesaScanlineSizeQuarter - SCREENWIDTH*3/4;
  }
}

void I_FinishUpdate8bppLinearScale4x(void)
{
  int i,j;

  unsigned int *ptrVRAM = vesaScaleStart;
  unsigned int vesaScanlineSizeQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM++)
    {
      unsigned int data = backbuffer[i + j];
      data |= data << 8;
      data |= data << 16;

      *(ptrVRAM) = data;
      *(ptrVRAM+vesaScanlineSizeQuarter) = data;
      *(ptrVRAM+2*vesaScanlineSizeQuarter) = data;
      *(ptrVRAM+3*vesaScanlineSizeQuarter) = data;
    }

    ptrVRAM += 4*vesaScanlineSizeQuarter - SCREENWIDTH;
  }
}

void I_FinishUpdate8bppLinearScale5x(void)
{
  int i,j;

  unsigned int *ptrVRAM = vesaScaleStart;
  unsigned int vesaScanlineSizeQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j+=4, ptrVRAM += 5)
    {
      unsigned char data1 = backbuffer[i + j];
      unsigned char data2 = backbuffer[i + j + 1];
      unsigned char data3 = backbuffer[i + j + 2];
      unsigned char data4 = backbuffer[i + j + 3];

      unsigned int first = data1 << 24 | data1 << 16 | data1 << 8 | data1;
      unsigned int second = data2 << 24 | data2 << 16 | data2 << 8 | data1;
      unsigned int third = data3 << 24 | data3 << 16 | data2 << 8 | data2;
      unsigned int fourth = data4 << 24 | data3 << 16 | data3 << 8 | data3;
      unsigned int fifth = data4 << 24 | data4 << 16 | data4 << 8 | data4;

      *(ptrVRAM) = first;
      *(ptrVRAM+1) = second;
      *(ptrVRAM+2) = third;
      *(ptrVRAM+3) = fourth;
      *(ptrVRAM+4) = fifth;
      *(ptrVRAM+vesaScanlineSizeQuarter) = first;
      *(ptrVRAM+vesaScanlineSizeQuarter+1) = second;
      *(ptrVRAM+vesaScanlineSizeQuarter+2) = third;
      *(ptrVRAM+vesaScanlineSizeQuarter+3) = fourth;
      *(ptrVRAM+vesaScanlineSizeQuarter+4) = fifth;
      *(ptrVRAM+2*vesaScanlineSizeQuarter) = first;
      *(ptrVRAM+2*vesaScanlineSizeQuarter+1) = second;
      *(ptrVRAM+2*vesaScanlineSizeQuarter+2) = third;
      *(ptrVRAM+2*vesaScanlineSizeQuarter+3) = fourth;
      *(ptrVRAM+2*vesaScanlineSizeQuarter+4) = fifth;
      *(ptrVRAM+3*vesaScanlineSizeQuarter) = first;
      *(ptrVRAM+3*vesaScanlineSizeQuarter+1) = second;
      *(ptrVRAM+3*vesaScanlineSizeQuarter+2) = third;
      *(ptrVRAM+3*vesaScanlineSizeQuarter+3) = fourth;
      *(ptrVRAM+3*vesaScanlineSizeQuarter+4) = fifth;
      *(ptrVRAM+4*vesaScanlineSizeQuarter) = first;
      *(ptrVRAM+4*vesaScanlineSizeQuarter+1) = second;
      *(ptrVRAM+4*vesaScanlineSizeQuarter+2) = third;
      *(ptrVRAM+4*vesaScanlineSizeQuarter+3) = fourth;
      *(ptrVRAM+4*vesaScanlineSizeQuarter+4) = fifth;
    }

    ptrVRAM += 5*vesaScanlineSizeQuarter - SCREENWIDTH*5/4;
  }
}

void I_FinishUpdate15bpp16bppLinearScale1x(void)
{
  int i,j;

  unsigned short *ptrPalette = (unsigned short *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineSizeQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j += 2, ptrVRAM++)
    {
      unsigned short data1 = ptrPalette[backbuffer[i + j]];
      unsigned short data2 = ptrPalette[backbuffer[i + j + 1]];

      *(ptrVRAM) = data1 | (data2 << 16);
    }

    ptrVRAM += vesaScanlineSizeQuarter - SCREENWIDTH / 2;
  }
}

void I_FinishUpdate15bpp16bppLinearScale2x(void)
{
  int i,j;

  unsigned short *ptrPalette = (unsigned short *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM++)
    {
      unsigned short data = ptrPalette[backbuffer[i + j]];
      unsigned int packet = data | data << 16;

      *(ptrVRAM) = packet;
      *(ptrVRAM+vesaScanlineQuarter) = packet;
    }

    ptrVRAM += 2*vesaScanlineQuarter - SCREENWIDTH;
  }
}

void I_FinishUpdate15bpp16bppLinearScale3x(void)
{
  int i,j;

  unsigned short *ptrPalette = (unsigned short *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j+=2, ptrVRAM += 3)
    {
      unsigned short data1 = ptrPalette[backbuffer[i + j]];
      unsigned short data2 = ptrPalette[backbuffer[i + j + 1]];

      unsigned int packet1 = data1 | data1 << 16;
      unsigned int packet2 = data1 | data2 << 16;
      unsigned int packet3 = data2 | data2 << 16;

      *(ptrVRAM) = packet1; 
      *(ptrVRAM+1) = packet2; 
      *(ptrVRAM+2) = packet3;
      *(ptrVRAM+vesaScanlineQuarter) = packet1; 
      *(ptrVRAM+vesaScanlineQuarter+1) = packet2; 
      *(ptrVRAM+vesaScanlineQuarter+2) = packet3;
      *(ptrVRAM+2*vesaScanlineQuarter) = packet1; 
      *(ptrVRAM+2*vesaScanlineQuarter+1) = packet2; 
      *(ptrVRAM+2*vesaScanlineQuarter+2) = packet3;
    }

    ptrVRAM += 3*vesaScanlineQuarter - SCREENWIDTH * 3 / 2;
  }
}

void I_FinishUpdate15bpp16bppLinearScale4x(void)
{
  int i,j;

  unsigned short *ptrPalette = (unsigned short *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM += 2)
    {
      unsigned char ptrLUT = backbuffer[i + j];
      unsigned int data = ptrPalette[ptrLUT];
      data |= data << 16;

      *(ptrVRAM) = data; 
      *(ptrVRAM+1) = data;
      *(ptrVRAM+vesaScanlineQuarter) = data;
      *(ptrVRAM+vesaScanlineQuarter+1) = data;
      *(ptrVRAM+2*vesaScanlineQuarter) = data;
      *(ptrVRAM+2*vesaScanlineQuarter+1) = data;
      *(ptrVRAM+3*vesaScanlineQuarter) = data;
      *(ptrVRAM+3*vesaScanlineQuarter+1) = data;
    }

    ptrVRAM += 4*vesaScanlineQuarter - SCREENWIDTH * 2;
  }
}

void I_FinishUpdate15bpp16bppLinearScale5x(void)
{
  int i,j;

  unsigned short *ptrPalette = (unsigned short *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j+=2, ptrVRAM += 5)
    {
      unsigned short data1 = ptrPalette[backbuffer[i + j]];
      unsigned short data2 = ptrPalette[backbuffer[i + j + 1]];

      unsigned int packet1 = data1 | data1 << 16;
      unsigned int packet2 = data1 | data2 << 16;
      unsigned int packet3 = data2 | data2 << 16;

      *(ptrVRAM) = packet1; 
      *(ptrVRAM+1) = packet1; 
      *(ptrVRAM+2) = packet2; 
      *(ptrVRAM+3) = packet3; 
      *(ptrVRAM+4) = packet3;
      *(ptrVRAM+vesaScanlineQuarter) = packet1; 
      *(ptrVRAM+vesaScanlineQuarter+1) = packet1; 
      *(ptrVRAM+vesaScanlineQuarter+2) = packet2; 
      *(ptrVRAM+vesaScanlineQuarter+3) = packet3; 
      *(ptrVRAM+vesaScanlineQuarter+4) = packet3;
      *(ptrVRAM+2*vesaScanlineQuarter) = packet1; 
      *(ptrVRAM+2*vesaScanlineQuarter+1) = packet1; 
      *(ptrVRAM+2*vesaScanlineQuarter+2) = packet2; 
      *(ptrVRAM+2*vesaScanlineQuarter+3) = packet3; 
      *(ptrVRAM+2*vesaScanlineQuarter+4) = packet3;
      *(ptrVRAM+3*vesaScanlineQuarter) = packet1; 
      *(ptrVRAM+3*vesaScanlineQuarter+1) = packet1; 
      *(ptrVRAM+3*vesaScanlineQuarter+2) = packet2; 
      *(ptrVRAM+3*vesaScanlineQuarter+3) = packet3; 
      *(ptrVRAM+3*vesaScanlineQuarter+4) = packet3;
      *(ptrVRAM+4*vesaScanlineQuarter) = packet1; 
      *(ptrVRAM+4*vesaScanlineQuarter+1) = packet1; 
      *(ptrVRAM+4*vesaScanlineQuarter+2) = packet2; 
      *(ptrVRAM+4*vesaScanlineQuarter+3) = packet3; 
      *(ptrVRAM+4*vesaScanlineQuarter+4) = packet3;
    }

    ptrVRAM += 5*vesaScanlineQuarter - SCREENWIDTH * 5 / 2;
  }
}

void I_FinishUpdate24bppLinearScale1x(void)
{
  int i,j;

  unsigned char *ptrPalette = (unsigned char *) ptrprocessedpalette;
  unsigned char *ptrVRAM = (unsigned char *) vesaScaleStart;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM += 3)
    {
      unsigned short ptrLUT = backbuffer[i + j] * 3;
      *(ptrVRAM) = ptrPalette[ptrLUT];
      *(ptrVRAM+1) = ptrPalette[ptrLUT + 1];
      *(ptrVRAM+2) = ptrPalette[ptrLUT + 2];
    }

    ptrVRAM += vesaScanlineSize - SCREENWIDTH * 3;
  }
}

void I_FinishUpdate24bppLinearScale2x(void)
{
  int i,j;

  unsigned char *ptrPalette = (unsigned char *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineSizeQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j+=2, ptrVRAM += 3)
    {
      unsigned short ptrLUT1 = backbuffer[i + j] * 3;
      unsigned short ptrLUT2 = backbuffer[i + j + 1] * 3;

      unsigned char b1 = ptrPalette[ptrLUT1];
      unsigned char g1 = ptrPalette[ptrLUT1 + 1];
      unsigned char r1 = ptrPalette[ptrLUT1 + 2];

      unsigned char b2 = ptrPalette[ptrLUT2];
      unsigned char g2 = ptrPalette[ptrLUT2 + 1];
      unsigned char r2 = ptrPalette[ptrLUT2 + 2];

      unsigned int packet1 = b1 | g1 << 8 | r1 << 16 | b1 << 24; 
      unsigned int packet2 = g1 | r1 << 8 | b2 << 16 | g2 << 24;
      unsigned int packet3 = r2 | b2 << 8 | g2 << 16 | r2 << 24;

      *(ptrVRAM) = packet1;
      *(ptrVRAM+1) = packet2;
      *(ptrVRAM+2) = packet3;
      *(ptrVRAM+vesaScanlineSizeQuarter) = packet1;
      *(ptrVRAM+vesaScanlineSizeQuarter+1) = packet2;
      *(ptrVRAM+vesaScanlineSizeQuarter+2) = packet3;
    }

    ptrVRAM += 2*vesaScanlineSizeQuarter - SCREENWIDTH * 3 / 2;
  }
}

void I_FinishUpdate24bppLinearScale3x(void)
{
  int i,j;

  unsigned char *ptrPalette = (unsigned char *) ptrprocessedpalette;
  unsigned short *ptrVRAM = (unsigned short *) vesaScaleStart;
  unsigned int vesaScanlineSizeHalf = vesaScanlineSize / 2;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j+=2, ptrVRAM += 9)
    {
      unsigned short ptrLUT1 = backbuffer[i + j] * 3;
      unsigned short ptrLUT2 = backbuffer[i + j + 1] * 3;

      unsigned char b1 = ptrPalette[ptrLUT1];
      unsigned char g1 = ptrPalette[ptrLUT1 + 1];
      unsigned char r1 = ptrPalette[ptrLUT1 + 2];

      unsigned char b2 = ptrPalette[ptrLUT2];
      unsigned char g2 = ptrPalette[ptrLUT2 + 1];
      unsigned char r2 = ptrPalette[ptrLUT2 + 2];

      unsigned short packet1 = b1 | g1 << 8; 
      unsigned short packet2 = r1 | b1 << 8;
      unsigned short packet3 = g1 | r1 << 8;
      //unsigned short packet4 = b1 | g1 << 8;
      unsigned short packet5 = r1 | b2 << 8;
      unsigned short packet6 = g2 | r2 << 8;
      unsigned short packet7 = b2 | g2 << 8;
      unsigned short packet8 = r2 | b2 << 8;
      //unsigned short packet9 = g2 | r2 << 8;

      *(ptrVRAM) = packet1;
      *(ptrVRAM+1) = packet2;
      *(ptrVRAM+2) = packet3;
      *(ptrVRAM+3) = packet1;
      *(ptrVRAM+4) = packet5;
      *(ptrVRAM+5) = packet6;
      *(ptrVRAM+6) = packet7;
      *(ptrVRAM+7) = packet8;
      *(ptrVRAM+8) = packet6;
      *(ptrVRAM+vesaScanlineSizeHalf) = packet1;
      *(ptrVRAM+vesaScanlineSizeHalf+1) = packet2;
      *(ptrVRAM+vesaScanlineSizeHalf+2) = packet3;
      *(ptrVRAM+vesaScanlineSizeHalf+3) = packet1;
      *(ptrVRAM+vesaScanlineSizeHalf+4) = packet5;
      *(ptrVRAM+vesaScanlineSizeHalf+5) = packet6;
      *(ptrVRAM+vesaScanlineSizeHalf+6) = packet7;
      *(ptrVRAM+vesaScanlineSizeHalf+7) = packet8;
      *(ptrVRAM+vesaScanlineSizeHalf+8) = packet6;
      *(ptrVRAM+2*vesaScanlineSizeHalf) = packet1;
      *(ptrVRAM+2*vesaScanlineSizeHalf+1) = packet2;
      *(ptrVRAM+2*vesaScanlineSizeHalf+2) = packet3;
      *(ptrVRAM+2*vesaScanlineSizeHalf+3) = packet1;
      *(ptrVRAM+2*vesaScanlineSizeHalf+4) = packet5;
      *(ptrVRAM+2*vesaScanlineSizeHalf+5) = packet6;
      *(ptrVRAM+2*vesaScanlineSizeHalf+6) = packet7;
      *(ptrVRAM+2*vesaScanlineSizeHalf+7) = packet8;
      *(ptrVRAM+2*vesaScanlineSizeHalf+8) = packet6;
    }

    ptrVRAM += 3*vesaScanlineSizeHalf - SCREENWIDTH * 9 / 2;
  }
}

void I_FinishUpdate24bppLinearScale4x(void)
{
  int i,j;

  unsigned char *ptrPalette = (unsigned char *) ptrprocessedpalette;
  unsigned char *ptrVRAM = (unsigned char *) vesaScaleStart;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM += 12)
    {
      unsigned short ptrLUT = backbuffer[i + j] * 3;
      unsigned char b = ptrPalette[ptrLUT];
      unsigned char g = ptrPalette[ptrLUT + 1];
      unsigned char r = ptrPalette[ptrLUT + 2];

      *(ptrVRAM) = b;
      *(ptrVRAM+1) = g;
      *(ptrVRAM+2) = r;
      *(ptrVRAM+3) = b;
      *(ptrVRAM+4) = g;
      *(ptrVRAM+5) = r;
      *(ptrVRAM+6) = b;
      *(ptrVRAM+7) = g;
      *(ptrVRAM+8) = r;
      *(ptrVRAM+9) = b;
      *(ptrVRAM+10) = g;
      *(ptrVRAM+11) = r;
      *(ptrVRAM+vesaScanlineSize) = b;
      *(ptrVRAM+vesaScanlineSize+1) = g;
      *(ptrVRAM+vesaScanlineSize+2) = r;
      *(ptrVRAM+vesaScanlineSize+3) = b;
      *(ptrVRAM+vesaScanlineSize+4) = g;
      *(ptrVRAM+vesaScanlineSize+5) = r;
      *(ptrVRAM+vesaScanlineSize+6) = b;
      *(ptrVRAM+vesaScanlineSize+7) = g;
      *(ptrVRAM+vesaScanlineSize+8) = r;
      *(ptrVRAM+vesaScanlineSize+9) = b;
      *(ptrVRAM+vesaScanlineSize+10) = g;
      *(ptrVRAM+vesaScanlineSize+11) = r;
      *(ptrVRAM+2*vesaScanlineSize) = b;
      *(ptrVRAM+2*vesaScanlineSize+1) = g;
      *(ptrVRAM+2*vesaScanlineSize+2) = r;
      *(ptrVRAM+2*vesaScanlineSize+3) = b;
      *(ptrVRAM+2*vesaScanlineSize+4) = g;
      *(ptrVRAM+2*vesaScanlineSize+5) = r;
      *(ptrVRAM+2*vesaScanlineSize+6) = b;
      *(ptrVRAM+2*vesaScanlineSize+7) = g;
      *(ptrVRAM+2*vesaScanlineSize+8) = r;
      *(ptrVRAM+2*vesaScanlineSize+9) = b;
      *(ptrVRAM+2*vesaScanlineSize+10) = g;
      *(ptrVRAM+2*vesaScanlineSize+11) = r;
      *(ptrVRAM+3*vesaScanlineSize) = b;
      *(ptrVRAM+3*vesaScanlineSize+1) = g;
      *(ptrVRAM+3*vesaScanlineSize+2) = r;
      *(ptrVRAM+3*vesaScanlineSize+3) = b;
      *(ptrVRAM+3*vesaScanlineSize+4) = g;
      *(ptrVRAM+3*vesaScanlineSize+5) = r;
      *(ptrVRAM+3*vesaScanlineSize+6) = b;
      *(ptrVRAM+3*vesaScanlineSize+7) = g;
      *(ptrVRAM+3*vesaScanlineSize+8) = r;
      *(ptrVRAM+3*vesaScanlineSize+9) = b;
      *(ptrVRAM+3*vesaScanlineSize+10) = g;
      *(ptrVRAM+3*vesaScanlineSize+11) = r;
    }

    ptrVRAM += 4*vesaScanlineSize - SCREENWIDTH * 12;
  }
}

void I_FinishUpdate24bppLinearScale5x(void)
{
  int i,j;

  unsigned char *ptrPalette = (unsigned char *) ptrprocessedpalette;
  unsigned char *ptrVRAM = (unsigned char *) vesaScaleStart;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM += 15)
    {
      unsigned short ptrLUT = backbuffer[i + j] * 3;
      unsigned char b = ptrPalette[ptrLUT];
      unsigned char g = ptrPalette[ptrLUT + 1];
      unsigned char r = ptrPalette[ptrLUT + 2];

      *(ptrVRAM) = b;
      *(ptrVRAM+1) = g;
      *(ptrVRAM+2) = r;
      *(ptrVRAM+3) = b;
      *(ptrVRAM+4) = g;
      *(ptrVRAM+5) = r;
      *(ptrVRAM+6) = b;
      *(ptrVRAM+7) = g;
      *(ptrVRAM+8) = r;
      *(ptrVRAM+9) = b;
      *(ptrVRAM+10) = g;
      *(ptrVRAM+11) = r;
      *(ptrVRAM+12) = b;
      *(ptrVRAM+13) = g;
      *(ptrVRAM+14) = r;
      *(ptrVRAM+vesaScanlineSize) = b;
      *(ptrVRAM+vesaScanlineSize+1) = g;
      *(ptrVRAM+vesaScanlineSize+2) = r;
      *(ptrVRAM+vesaScanlineSize+3) = b;
      *(ptrVRAM+vesaScanlineSize+4) = g;
      *(ptrVRAM+vesaScanlineSize+5) = r;
      *(ptrVRAM+vesaScanlineSize+6) = b;
      *(ptrVRAM+vesaScanlineSize+7) = g;
      *(ptrVRAM+vesaScanlineSize+8) = r;
      *(ptrVRAM+vesaScanlineSize+9) = b;
      *(ptrVRAM+vesaScanlineSize+10) = g;
      *(ptrVRAM+vesaScanlineSize+11) = r;
      *(ptrVRAM+vesaScanlineSize+12) = b;
      *(ptrVRAM+vesaScanlineSize+13) = g;
      *(ptrVRAM+vesaScanlineSize+14) = r;
      *(ptrVRAM+2*vesaScanlineSize) = b;
      *(ptrVRAM+2*vesaScanlineSize+1) = g;
      *(ptrVRAM+2*vesaScanlineSize+2) = r;
      *(ptrVRAM+2*vesaScanlineSize+3) = b;
      *(ptrVRAM+2*vesaScanlineSize+4) = g;
      *(ptrVRAM+2*vesaScanlineSize+5) = r;
      *(ptrVRAM+2*vesaScanlineSize+6) = b;
      *(ptrVRAM+2*vesaScanlineSize+7) = g;
      *(ptrVRAM+2*vesaScanlineSize+8) = r;
      *(ptrVRAM+2*vesaScanlineSize+9) = b;
      *(ptrVRAM+2*vesaScanlineSize+10) = g;
      *(ptrVRAM+2*vesaScanlineSize+11) = r;
      *(ptrVRAM+2*vesaScanlineSize+12) = b;
      *(ptrVRAM+2*vesaScanlineSize+13) = g;
      *(ptrVRAM+2*vesaScanlineSize+14) = r;
      *(ptrVRAM+3*vesaScanlineSize) = b;
      *(ptrVRAM+3*vesaScanlineSize+1) = g;
      *(ptrVRAM+3*vesaScanlineSize+2) = r;
      *(ptrVRAM+3*vesaScanlineSize+3) = b;
      *(ptrVRAM+3*vesaScanlineSize+4) = g;
      *(ptrVRAM+3*vesaScanlineSize+5) = r;
      *(ptrVRAM+3*vesaScanlineSize+6) = b;
      *(ptrVRAM+3*vesaScanlineSize+7) = g;
      *(ptrVRAM+3*vesaScanlineSize+8) = r;
      *(ptrVRAM+3*vesaScanlineSize+9) = b;
      *(ptrVRAM+3*vesaScanlineSize+10) = g;
      *(ptrVRAM+3*vesaScanlineSize+11) = r;
      *(ptrVRAM+3*vesaScanlineSize+12) = b;
      *(ptrVRAM+3*vesaScanlineSize+13) = g;
      *(ptrVRAM+3*vesaScanlineSize+14) = r;
      *(ptrVRAM+4*vesaScanlineSize) = b;
      *(ptrVRAM+4*vesaScanlineSize+1) = g;
      *(ptrVRAM+4*vesaScanlineSize+2) = r;
      *(ptrVRAM+4*vesaScanlineSize+3) = b;
      *(ptrVRAM+4*vesaScanlineSize+4) = g;
      *(ptrVRAM+4*vesaScanlineSize+5) = r;
      *(ptrVRAM+4*vesaScanlineSize+6) = b;
      *(ptrVRAM+4*vesaScanlineSize+7) = g;
      *(ptrVRAM+4*vesaScanlineSize+8) = r;
      *(ptrVRAM+4*vesaScanlineSize+9) = b;
      *(ptrVRAM+4*vesaScanlineSize+10) = g;
      *(ptrVRAM+4*vesaScanlineSize+11) = r;
      *(ptrVRAM+4*vesaScanlineSize+12) = b;
      *(ptrVRAM+4*vesaScanlineSize+13) = g;
      *(ptrVRAM+4*vesaScanlineSize+14) = r;
    }

    ptrVRAM += 5*vesaScanlineSize - SCREENWIDTH * 15;
  }
}

void I_FinishUpdate32bppLinearScale1x(void)
{
  int i,j;

  unsigned int *ptrPalette = (unsigned int *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM++)
    {
      unsigned char ptrLUT = backbuffer[i + j];
      *(ptrVRAM) = ptrPalette[ptrLUT];
    }

    ptrVRAM += vesaScanlineSize / 4 - SCREENWIDTH;
  }
}

void I_FinishUpdate32bppLinearScale2x(void)
{
  int i,j;

  unsigned int *ptrPalette = (unsigned int *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM += 2)
    {
      unsigned char ptrLUT = backbuffer[i + j];
      unsigned int data = ptrPalette[ptrLUT];

      *(ptrVRAM) = data; 
      *(ptrVRAM+1) = data;
      *(ptrVRAM+vesaScanlineQuarter) = data;
      *(ptrVRAM+vesaScanlineQuarter+1) = data;
    }

    ptrVRAM += 2*vesaScanlineQuarter - SCREENWIDTH * 2;
  }
}

void I_FinishUpdate32bppLinearScale3x(void)
{
  int i,j;

  unsigned int *ptrPalette = (unsigned int *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM += 3)
    {
      unsigned char ptrLUT = backbuffer[i + j];
      unsigned int data = ptrPalette[ptrLUT];

      *(ptrVRAM) = data; 
      *(ptrVRAM+1) = data; 
      *(ptrVRAM+2) = data;
      *(ptrVRAM+vesaScanlineQuarter) = data; 
      *(ptrVRAM+vesaScanlineQuarter+1) = data; 
      *(ptrVRAM+vesaScanlineQuarter+2) = data;
      *(ptrVRAM+2*vesaScanlineQuarter) = data; 
      *(ptrVRAM+2*vesaScanlineQuarter+1) = data; 
      *(ptrVRAM+2*vesaScanlineQuarter+2) = data;
    }

    ptrVRAM += 3*vesaScanlineQuarter - SCREENWIDTH * 3;
  }
}

void I_FinishUpdate32bppLinearScale4x(void)
{
  int i,j;

  unsigned int *ptrPalette = (unsigned int *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM += 4)
    {
      unsigned char ptrLUT = backbuffer[i + j];
      unsigned int data = ptrPalette[ptrLUT];

      *(ptrVRAM) = data; 
      *(ptrVRAM+1) = data; 
      *(ptrVRAM+2) = data; 
      *(ptrVRAM+3) = data;
      *(ptrVRAM+vesaScanlineQuarter) = data; 
      *(ptrVRAM+vesaScanlineQuarter+1) = data; 
      *(ptrVRAM+vesaScanlineQuarter+2) = data; 
      *(ptrVRAM+vesaScanlineQuarter+3) = data;
      *(ptrVRAM+2*vesaScanlineQuarter) = data; 
      *(ptrVRAM+2*vesaScanlineQuarter+1) = data; 
      *(ptrVRAM+2*vesaScanlineQuarter+2) = data; 
      *(ptrVRAM+2*vesaScanlineQuarter+3) = data;
      *(ptrVRAM+3*vesaScanlineQuarter) = data; 
      *(ptrVRAM+3*vesaScanlineQuarter+1) = data; 
      *(ptrVRAM+3*vesaScanlineQuarter+2) = data; 
      *(ptrVRAM+3*vesaScanlineQuarter+3) = data;
    }

    ptrVRAM += vesaScanlineSize - SCREENWIDTH * 4;
  }
}

void I_FinishUpdate32bppLinearScale5x(void)
{
  int i,j;

  unsigned int *ptrPalette = (unsigned int *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) vesaScaleStart;
  unsigned int vesaScanlineQuarter = vesaScanlineSize / 4;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM += 5)
    {
      unsigned char ptrLUT = backbuffer[i + j];
      unsigned int data = ptrPalette[ptrLUT];

      *(ptrVRAM) = data;
      *(ptrVRAM+1) = data;
      *(ptrVRAM+2) = data;
      *(ptrVRAM+3) = data;
      *(ptrVRAM+4) = data;
      *(ptrVRAM+vesaScanlineQuarter) = data;
      *(ptrVRAM+vesaScanlineQuarter+1) = data;
      *(ptrVRAM+vesaScanlineQuarter+2) = data;
      *(ptrVRAM+vesaScanlineQuarter+3) = data;
      *(ptrVRAM+vesaScanlineQuarter+4) = data;
      *(ptrVRAM+2*vesaScanlineQuarter) = data;
      *(ptrVRAM+2*vesaScanlineQuarter+1) = data;
      *(ptrVRAM+2*vesaScanlineQuarter+2) = data;
      *(ptrVRAM+2*vesaScanlineQuarter+3) = data;
      *(ptrVRAM+2*vesaScanlineQuarter+4) = data;
      *(ptrVRAM+3*vesaScanlineQuarter) = data; 
      *(ptrVRAM+3*vesaScanlineQuarter+1) = data; 
      *(ptrVRAM+3*vesaScanlineQuarter+2) = data; 
      *(ptrVRAM+3*vesaScanlineQuarter+3) = data; 
      *(ptrVRAM+3*vesaScanlineQuarter+4) = data;
      *(ptrVRAM+4*vesaScanlineQuarter) = data; 
      *(ptrVRAM+4*vesaScanlineQuarter+1) = data; 
      *(ptrVRAM+4*vesaScanlineQuarter+2) = data; 
      *(ptrVRAM+4*vesaScanlineQuarter+3) = data; 
      *(ptrVRAM+4*vesaScanlineQuarter+4) = data;
    }

    ptrVRAM += 5*vesaScanlineQuarter - SCREENWIDTH * 5;
  }
}

void I_FinishUpdate8bppLinear(void)
{
  if (updatestate & I_FULLSCRN)
  {
    CopyDWords(backbuffer, pcscreen, SCREENHEIGHT * SCREENWIDTH / 4);
    updatestate = I_NOUPDATE; // clear out all draw types
  }
  if (updatestate & I_FULLVIEW)
  {
    if (updatestate & I_MESSAGES && screenblocks > 7)
    {
      int i;
      for (i = 0; i < endscreen; i += SCREENWIDTH)
      {
        CopyDWords(backbuffer + i, pcscreen + i, SCREENWIDTH / 4);
      }
      updatestate &= ~(I_FULLVIEW | I_MESSAGES);
    }
    else
    {
      int i;
      for (i = startscreen; i < endscreen; i += SCREENWIDTH)
      {
        CopyDWords(backbuffer + i, pcscreen + i, SCREENWIDTH / 4);
      }
      updatestate &= ~I_FULLVIEW;
    }
  }
  if (updatestate & I_STATBAR)
  {
    CopyDWords(backbuffer + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), pcscreen + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), SCREENWIDTH * SBARHEIGHT / 4);
    updatestate &= ~I_STATBAR;
  }
  if (updatestate & I_MESSAGES)
  {
    CopyDWords(backbuffer, pcscreen, (SCREENWIDTH * 28) / 4);
    updatestate &= ~I_MESSAGES;
  }
}

void I_FinishUpdate8bppLinearMMX(void)
{
  if (updatestate & I_FULLSCRN)
  {
    CopyQWordsMMX(backbuffer, pcscreen, SCREENHEIGHT * SCREENWIDTH / 8);
    updatestate = I_NOUPDATE; // clear out all draw types
  }
  if (updatestate & I_FULLVIEW)
  {
    if (updatestate & I_MESSAGES && screenblocks > 7)
    {
      int i;
      for (i = 0; i < endscreen; i += SCREENWIDTH)
      {
        CopyQWordsMMX(backbuffer + i, pcscreen + i, SCREENWIDTH / 8);
      }
      updatestate &= ~(I_FULLVIEW | I_MESSAGES);
    }
    else
    {
      int i;
      for (i = startscreen; i < endscreen; i += SCREENWIDTH)
      {
        CopyQWordsMMX(backbuffer + i, pcscreen + i, SCREENWIDTH / 8);
      }
      updatestate &= ~I_FULLVIEW;
    }
  }
  if (updatestate & I_STATBAR)
  {
    CopyQWordsMMX(backbuffer + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), pcscreen + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), SCREENWIDTH * SBARHEIGHT / 8);
    updatestate &= ~I_STATBAR;
  }
  if (updatestate & I_MESSAGES)
  {
    CopyQWordsMMX(backbuffer, pcscreen, (SCREENWIDTH * 28) / 8);
    updatestate &= ~I_MESSAGES;
  }
}

void I_FinishUpdate8bppLinearFPU(void)
{
  if (updatestate & I_FULLSCRN)
  {
    CopyQWordsFPU(backbuffer, pcscreen, SCREENHEIGHT * SCREENWIDTH / 8);
    updatestate = I_NOUPDATE; // clear out all draw types
  }
  if (updatestate & I_FULLVIEW)
  {
    if (updatestate & I_MESSAGES && screenblocks > 7)
    {
      int i;
      for (i = 0; i < endscreen; i += SCREENWIDTH)
      {
        CopyQWordsFPU(backbuffer + i, pcscreen + i, SCREENWIDTH / 8);
      }
      updatestate &= ~(I_FULLVIEW | I_MESSAGES);
    }
    else
    {
      int i;
      for (i = startscreen; i < endscreen; i += SCREENWIDTH)
      {
        CopyQWordsFPU(backbuffer + i, pcscreen + i, SCREENWIDTH / 8);
      }
      updatestate &= ~I_FULLVIEW;
    }
  }
  if (updatestate & I_STATBAR)
  {
    CopyQWordsFPU(backbuffer + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), pcscreen + SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT), SCREENWIDTH * SBARHEIGHT / 8);
    updatestate &= ~I_STATBAR;
  }
  if (updatestate & I_MESSAGES)
  {
    CopyQWordsFPU(backbuffer, pcscreen, (SCREENWIDTH * 28) / 8);
    updatestate &= ~I_MESSAGES;
  }
}

void I_FinishUpdate15bpp16bppBanked(void)
{
  int i, j;

  unsigned short *ptrPalette = (unsigned short *) ptrprocessedpalette;
  unsigned short *ptrVRAM = (unsigned short *) pcscreen;

  for (i = 0; i < NUM_BANKS_15BPP_16BPP; i++)
  {
    VBE_SetBank(i);

    for (j = 0; j < 64 * 1024 / 2; j++)
    {
      unsigned char ptrLUT = backbuffer[32 * 1024 * i + j];

      ptrVRAM[j] = ptrPalette[ptrLUT];
    }
  }

#if LAST_BANK_SIZE_15BPP_16BPP > 0

  VBE_SetBank(NUM_BANKS_15BPP_16BPP);

  for (j = 0; j < LAST_BANK_SIZE_15BPP_16BPP / 2; j++)
  {
    unsigned char ptrLUT = backbuffer[NUM_BANKS_15BPP_16BPP * 32 * 1024 + j];

    ptrVRAM[j] = ptrPalette[ptrLUT];
  }

#endif
}

void I_FinishUpdate15bpp16bppLinearFix(void)
{
  int i,j;

  unsigned short *ptrPalette = (unsigned short *) ptrprocessedpalette;
  unsigned short *ptrVRAM = (unsigned short *) pcscreen;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM++)
    {
      unsigned char ptrLUT = backbuffer[i + j];
      *(ptrVRAM) = ptrPalette[ptrLUT];
    }

    ptrVRAM+=(vesascanlinefix / 2);
  }
}

void I_FinishUpdate24bppBanked(void)
{
  int i;
  int ptrPCscreen = 0;
  int numBank = 0;

  VBE_SetBank(numBank);

  for (i = 0; i < SCREENWIDTH * SCREENHEIGHT; i++)
  {
    unsigned short ptrLUT = backbuffer[i] * 3;

    if (ptrPCscreen + 3 < 64 * 1024)
    {
      pcscreen[ptrPCscreen] = ptrprocessedpalette[ptrLUT];
      pcscreen[ptrPCscreen + 1] = ptrprocessedpalette[ptrLUT + 1];
      pcscreen[ptrPCscreen + 2] = ptrprocessedpalette[ptrLUT + 2];
      ptrPCscreen += 3;
    }
    else
    {
      int count = (64 * 1024) - ptrPCscreen;
      int countLUT = 3;

      while (count > 0)
      {
        pcscreen[ptrPCscreen] = ptrprocessedpalette[ptrLUT];
        ptrPCscreen++;
        ptrLUT++;
        countLUT--;
        count--;
      }

      numBank++;
      VBE_SetBank(numBank);

      ptrPCscreen = 0;

      while (countLUT > 0)
      {
        pcscreen[ptrPCscreen] = ptrprocessedpalette[ptrLUT];
        ptrPCscreen++;
        ptrLUT++;
        countLUT--;
      }
    }
  }
}

void I_FinishUpdate24bppLinearFix(void)
{
  int i,j;
  int vramposition = 0;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, vramposition+=3)
    {
      unsigned short ptrLUT = backbuffer[i + j] * 4;

      pcscreen[vramposition] = ptrprocessedpalette[ptrLUT];
      pcscreen[vramposition + 1] = ptrprocessedpalette[ptrLUT + 1];
      pcscreen[vramposition + 2] = ptrprocessedpalette[ptrLUT + 2];
    }

    vramposition += vesascanlinefix;
  }
}

void I_FinishUpdate32bppBanked(void)
{
  int i, j;

  unsigned int *ptrPalette = (unsigned int *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) pcscreen;

  for (i = 0; i < NUM_BANKS_32BPP; i++)
  {
    VBE_SetBank(i);

    for (j = 0; j < 64 * 1024 / 4; j++)
    {
      unsigned char ptrLUT = backbuffer[16 * 1024 * i + j];

      ptrVRAM[j] = ptrPalette[ptrLUT];
    }
  }

#if LAST_BANK_SIZE_32BPP > 0

  VBE_SetBank(NUM_BANKS_32BPP);

  for (j = 0; j < LAST_BANK_SIZE_32BPP / 4; j++)
  {
    unsigned char ptrLUT = backbuffer[NUM_BANKS_32BPP * 16 * 1024 + j];

    ptrVRAM[j] = ptrPalette[ptrLUT];
  }

#endif
}

void I_FinishUpdate32bppLinearFix(void)
{
  int i,j;

  unsigned int *ptrPalette = (unsigned int *) ptrprocessedpalette;
  unsigned int *ptrVRAM = (unsigned int *) pcscreen;

  for (i = 0; i < SCREENHEIGHT * SCREENWIDTH; i += SCREENWIDTH)
  {
    for (j = 0; j < SCREENWIDTH; j++, ptrVRAM++)
    {
      unsigned char ptrLUT = backbuffer[i + j];
      *(ptrVRAM) = ptrPalette[ptrLUT];
    }

    ptrVRAM += (vesascanlinefix/4);
  }
}

/* ---- Banked scaled mode functions ---- */

inline static void BankedWritePixelByte(int offset, byte data)
{
  *(byte *)(0xA0000 + offset) = data;
}

inline static void BankedWritePixelShort(int offset, unsigned short data)
{
  *(unsigned short *)(0xA0000 + offset) = data;
}

inline static void BankedWritePixelInt(int offset, unsigned int data)
{
  *(unsigned int *)(0xA0000 + offset) = data;
}

#define BANKED_SCALE_8BPP(N) \
void I_FinishUpdate8bppBankedScale##N##x(void) \
{ \
  int out_x, out_y; \
  unsigned char *ptrPalette = (unsigned char *) ptrprocessedpalette; \
  int scaleStartOffset = (int)((unsigned long)vesaScaleStart - (unsigned long)0xA0000); \
  int rowOffset, bank, offsetInBank; \
  \
  for (out_y = 0; out_y < SCREENHEIGHT * N; out_y++) \
  { \
    rowOffset = scaleStartOffset + out_y * vesaScanlineSize; \
    bank = rowOffset / 65536; \
    offsetInBank = rowOffset % 65536; \
    VBE_SetBank(bank); \
    \
    for (out_x = 0; out_x < SCREENWIDTH * N; out_x++) \
    { \
      if (offsetInBank + 1 > 65536) \
      { \
        bank++; \
        offsetInBank = 0; \
        VBE_SetBank(bank); \
      } \
      BankedWritePixelByte(offsetInBank, backbuffer[(out_y / N) * SCREENWIDTH + (out_x / N)]); \
      offsetInBank++; \
    } \
  } \
}

BANKED_SCALE_8BPP(1)
BANKED_SCALE_8BPP(2)
BANKED_SCALE_8BPP(3)
BANKED_SCALE_8BPP(4)
BANKED_SCALE_8BPP(5)

#define BANKED_SCALE_15BPP(N) \
void I_FinishUpdate15bppBankedScale##N##x(void) \
{ \
  int out_x, out_y; \
  unsigned short *ptrPalette = (unsigned short *) ptrprocessedpalette; \
  int scaleStartOffset = (int)((unsigned long)vesaScaleStart - (unsigned long)0xA0000); \
  int rowOffset, bank, offsetInBank; \
  \
  for (out_y = 0; out_y < SCREENHEIGHT * N; out_y++) \
  { \
    rowOffset = scaleStartOffset + out_y * vesaScanlineSize; \
    bank = rowOffset / 65536; \
    offsetInBank = rowOffset % 65536; \
    VBE_SetBank(bank); \
    \
    for (out_x = 0; out_x < SCREENWIDTH * N; out_x++) \
    { \
      if (offsetInBank + 2 > 65536) \
      { \
        bank++; \
        offsetInBank = 0; \
        VBE_SetBank(bank); \
      } \
      BankedWritePixelShort(offsetInBank, ptrPalette[backbuffer[(out_y / N) * SCREENWIDTH + (out_x / N)]]); \
      offsetInBank += 2; \
    } \
  } \
}

BANKED_SCALE_15BPP(1)
BANKED_SCALE_15BPP(2)
BANKED_SCALE_15BPP(3)
BANKED_SCALE_15BPP(4)
BANKED_SCALE_15BPP(5)

#define BANKED_SCALE_16BPP(N) \
void I_FinishUpdate16bppBankedScale##N##x(void) \
{ \
  int out_x, out_y; \
  unsigned short *ptrPalette = (unsigned short *) ptrprocessedpalette; \
  int scaleStartOffset = (int)((unsigned long)vesaScaleStart - (unsigned long)0xA0000); \
  int rowOffset, bank, offsetInBank; \
  \
  for (out_y = 0; out_y < SCREENHEIGHT * N; out_y++) \
  { \
    rowOffset = scaleStartOffset + out_y * vesaScanlineSize; \
    bank = rowOffset / 65536; \
    offsetInBank = rowOffset % 65536; \
    VBE_SetBank(bank); \
    \
    for (out_x = 0; out_x < SCREENWIDTH * N; out_x++) \
    { \
      if (offsetInBank + 2 > 65536) \
      { \
        bank++; \
        offsetInBank = 0; \
        VBE_SetBank(bank); \
      } \
      BankedWritePixelShort(offsetInBank, ptrPalette[backbuffer[(out_y / N) * SCREENWIDTH + (out_x / N)]]); \
      offsetInBank += 2; \
    } \
  } \
}

BANKED_SCALE_16BPP(1)
BANKED_SCALE_16BPP(2)
BANKED_SCALE_16BPP(3)
BANKED_SCALE_16BPP(4)
BANKED_SCALE_16BPP(5)

#define BANKED_SCALE_24BPP(N) \
void I_FinishUpdate24bppBankedScale##N##x(void) \
{ \
  int out_x, out_y; \
  unsigned char *ptrPalette = (unsigned char *) ptrprocessedpalette; \
  int scaleStartOffset = (int)((unsigned long)vesaScaleStart - (unsigned long)0xA0000); \
  int rowOffset, bank, offsetInBank; \
  \
  for (out_y = 0; out_y < SCREENHEIGHT * N; out_y++) \
  { \
    rowOffset = scaleStartOffset + out_y * vesaScanlineSize; \
    bank = rowOffset / 65536; \
    offsetInBank = rowOffset % 65536; \
    VBE_SetBank(bank); \
    \
    for (out_x = 0; out_x < SCREENWIDTH * N; out_x++) \
    { \
      int lutIdx = (out_y / N) * SCREENWIDTH + (out_x / N); \
      int ptrLUT = backbuffer[lutIdx] * 3; \
      \
      if (offsetInBank + 3 > 65536) \
      { \
        int remaining = 65536 - offsetInBank; \
        while (offsetInBank < 65536 && remaining > 0) \
        { \
          BankedWritePixelByte(offsetInBank, ptrPalette[ptrLUT]); \
          offsetInBank++; \
          ptrLUT++; \
          remaining--; \
        } \
        bank++; \
        offsetInBank = 0; \
        VBE_SetBank(bank); \
        while (offsetInBank < 65536 && ptrLUT < backbuffer[lutIdx] * 3 + 3) \
        { \
          BankedWritePixelByte(offsetInBank, ptrPalette[ptrLUT]); \
          offsetInBank++; \
          ptrLUT++; \
        } \
      } \
      else \
      { \
        BankedWritePixelByte(offsetInBank, ptrPalette[ptrLUT]); offsetInBank++; \
        BankedWritePixelByte(offsetInBank, ptrPalette[ptrLUT + 1]); offsetInBank++; \
        BankedWritePixelByte(offsetInBank, ptrPalette[ptrLUT + 2]); offsetInBank++; \
      } \
    } \
  } \
}

BANKED_SCALE_24BPP(1)
BANKED_SCALE_24BPP(2)
BANKED_SCALE_24BPP(3)
BANKED_SCALE_24BPP(4)
BANKED_SCALE_24BPP(5)

#define BANKED_SCALE_32BPP(N) \
void I_FinishUpdate32bppBankedScale##N##x(void) \
{ \
  int out_x, out_y; \
  unsigned int *ptrPalette = (unsigned int *) ptrprocessedpalette; \
  int scaleStartOffset = (int)((unsigned long)vesaScaleStart - (unsigned long)0xA0000); \
  int rowOffset, bank, offsetInBank; \
  \
  for (out_y = 0; out_y < SCREENHEIGHT * N; out_y++) \
  { \
    rowOffset = scaleStartOffset + out_y * vesaScanlineSize; \
    bank = rowOffset / 65536; \
    offsetInBank = rowOffset % 65536; \
    VBE_SetBank(bank); \
    \
    for (out_x = 0; out_x < SCREENWIDTH * N; out_x++) \
    { \
      if (offsetInBank + 4 > 65536) \
      { \
        bank++; \
        offsetInBank = 0; \
        VBE_SetBank(bank); \
      } \
      BankedWritePixelInt(offsetInBank, ptrPalette[backbuffer[(out_y / N) * SCREENWIDTH + (out_x / N)]]); \
      offsetInBank += 4; \
    } \
  } \
}

BANKED_SCALE_32BPP(1)
BANKED_SCALE_32BPP(2)
BANKED_SCALE_32BPP(3)
BANKED_SCALE_32BPP(4)
BANKED_SCALE_32BPP(5)

#endif

#if defined(MODE_VBE2_DIRECT)
short page = 0;
short bank = 0;

void I_FinishUpdate(void)
{
  if (pcscreen == (void *)0xA0000)
  {
    // This only works on 320x200 resolution (64000kb per screen)

    switch (bank)
    {
    case 2:
      bank = 0;
      VBE_SetDisplayStart(256, 204);
      break;
    case 0:
      bank++;
      VBE_SetDisplayStart(192, 409);
      break;
    case 1:
      bank++;
      VBE_SetDisplayStart(0, 0);
      break;
    }

    VBE_SetBank(bank);
  }
  else
  {
    VBE_SetDisplayStart_Y(page);
    if (page == SCREENHEIGHT * 2)
    {
      page = 0;
      destscreen -= 2 * SCREENWIDTH * SCREENHEIGHT;
    }
    else
    {
      page += SCREENHEIGHT;
      destscreen += SCREENWIDTH * SCREENHEIGHT;
    }
  }
}
#endif

#endif
