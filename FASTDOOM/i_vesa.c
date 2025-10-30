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
int vesascanlinefix = 0;

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

        if (vbemode.BytesPerScanline != SCREENWIDTH*bitsperpixel)
        {
          int bpp = bitsperpixel;

          if (bpp == 15)
            bpp = 16;

          vesascanlinefix = vbemode.BytesPerScanline - SCREENWIDTH*(bpp/8);
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
      if (VBE2_FindVideoMode(SCREENWIDTH, SCREENHEIGHT, forceVesaBitsPerPixel, 1))
      {
        linearModeFound = 1;
      }
    }

    // If not found, test for requested non-linear modes
    if (!linearModeFound)
    {
      VBE2_FindVideoMode(SCREENWIDTH, SCREENHEIGHT, forceVesaBitsPerPixel, 0);
    }

  } else {

    // Test for linear VBE compatible modes
    if (forceVesaNonLinear == 0)
    {
      for (i = 0; i < 5; i++) // Test each bit depth
      {
        if (VBE2_FindVideoMode(SCREENWIDTH, SCREENHEIGHT, bitsperpixel[i], 1))
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
        if (VBE2_FindVideoMode(SCREENWIDTH, SCREENHEIGHT, bitsperpixel[i], 0))
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
            case INTEL_PENTIUM:
              finishfunc = I_FinishUpdate8bppLinearFPU;
              break;
            case RISE_MP6:
            case INTEL_PENTIUM_MMX:
              finishfunc = I_FinishUpdate8bppLinearMMX;
              break;
            default:
              finishfunc = I_FinishUpdate8bppLinear;
              break;
          }
        }

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
    I_Error(21, SCREENWIDTH, SCREENHEIGHT);
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
