#include "i_vesa.h"
#include <stdio.h>
#include <malloc.h>
#include <conio.h>
#include <string.h>

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

#pragma pack(1)

#define VBE2SIGNATURE "VBE2"
#define VBE3SIGNATURE "VBE3"

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
  * Some informations 'bout the last mode which was set
  * These informations are required to compensate some differencies
  * between the normal and direct PM calling methods
  */

static signed short vbelastbank = -1;
static unsigned long BytesPerScanline;
static unsigned char BitsPerPixel;

/*
  * Protected Mode Direct Call Informations
  */

void *pmcode;
void *pm_setwindowcall;
void *pm_setdisplaystartcall;
void VBE_InitPM(void);

/*
 *  This function pointers will be initialized after you called
 *  VBE_Init. It'll be set to the realmode or protected mode call
 *  code
 */

tagSetDisplayStartType VBE_SetDisplayStart;
tagSetBankType VBE_SetBank;

#ifdef DEBUG_VESA

static FILE *dfile;
char currentsection[100];

static void __log(char *a)
{
  fprintf(dfile, a);
  fflush(dfile);
}

static void __dpmilog(int a)
{
  if (a)
  {
    __log("-------------------------------------------------\n");
    __log("section = ");
    __log(currentsection);
    __log("\n");
    __log("Error: DPMI interrupt returns zero carry\n");
    __log("-------------------------------------------------\n");
#ifdef DEBUG_SOUND
    sound(1000);
    delay(10);
    nosound();
#endif
  }
}

static void __vbelog(int a)
{
  int code = ((a >> 8) & 0xff);
  if (code != VESA_OK)
  {
    __log("-------------------------------------------------\n");
    __log("section = ");
    __log(currentsection);
    __log("\n");
    __log("Error: VESA returned error code: ");
    switch (code)
    {
    case VESA_OK:
      __log("VESA_OK\n");
      break;
    case VESA_FAIL:
      __log("VESA_FAIL\n");
      break;
    case VESA_NOTSUPPORTED:
      __log("VESA_NOTSUPPORTED\n");
      break;
    case VESA_INVALIDMODE:
      __log("VESA_INVALIDMODE\n");
      break;
    default:
      __log("Unknown error code\n");
      break;
    }
    __log("-------------------------------------------------\n");
#ifdef DEBUG_SOUND
    sound(1000);
    delay(10);
    nosound();
#endif
  }
}

static int hirarchie = 0;

void __logspaces(void)
{
  int a;
  for (a = 0; a < hirarchie * 2; a++)
    __log(" ");
}

#define log(a) __log((a))
#define dpmilog(a) __dpmilog(a)
#define vbelog(a) __vbelog(a)
#define entersection(a)        \
  strcpy(currentsection, (a)); \
  __logspaces();               \
  hirarchie++;                 \
  log("enter ");               \
  log(a);                      \
  log("\n");
#define leavesection(a) \
  hirarchie--;          \
  __logspaces();        \
  log("leave ");        \
  log(a);               \
  log("\n")
#else

#define log (a)
#define dpmilog(a)
#define vbelog(a)
#define entersection(a)
#define leavesection(a)

#endif

static void PrepareRegisters(void)
{
  memset(&RMI, 0, sizeof(RMI));
  memset(&sregs, 0, sizeof(sregs));
  memset(&regs, 0, sizeof(regs));
}

static void RMIRQ(char irq)
{
  memset(&regs, 0, sizeof(regs));
  regs.w.ax = 0x0300; // Simulate Real-Mode interrupt
  regs.h.bl = irq;
  sregs.es = FP_SEG(&RMI);
  regs.x.edi = FP_OFF(&RMI);
  int386x(0x31, &regs, &regs, &sregs);
  dpmilog(regs.x.cflag);
}

void DPMI_AllocDOSMem(short int paras, struct DPMI_PTR *p)
{
  entersection("DPMI_AllocDOSMem");
  /* DPMI call 100h allocates DOS memory */
  PrepareRegisters();
  regs.w.ax = 0x0100;
  regs.w.bx = paras;
  int386x(0x31, &regs, &regs, &sregs);
  p->segment = regs.w.ax;
  p->selector = regs.w.dx;
  dpmilog(regs.x.cflag);
  leavesection("DPMI_AllocDOSMem");
}

void DPMI_FreeDOSMem(struct DPMI_PTR *p)
{
  entersection("DPMI_FreeDOSMem");
  /* DPMI call 101h free DOS memory */
  memset(&sregs, 0, sizeof(sregs));
  regs.w.ax = 0x0101;
  regs.w.dx = p->selector;
  int386x(0x31, &regs, &regs, &sregs);
  dpmilog(regs.x.cflag);
  leavesection("DPMI_FreeDOSMem");
}

void DPMI_UNMAP_PHYSICAL(void *p)
{
  entersection("DPMI_UNMAP_PHYSICAL");
  /* DPMI call 800h map physical memory*/
  PrepareRegisters();
  regs.w.ax = 0x0801;
  regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
  regs.w.cx = (unsigned short)(((unsigned long)p) & 0xffff);
  int386x(0x31, &regs, &regs, &sregs);
  dpmilog(regs.x.cflag);
  leavesection("DPMI_UNMAP_PHYSICAL");
}

void *DPMI_MAP_PHYSICAL(void *p, unsigned long size)
{
  entersection("DPMI_MAP_PHYSICAL");
  /* DPMI call 800h map physical memory*/
  PrepareRegisters();
  regs.w.ax = 0x0800;
  regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
  regs.w.cx = (unsigned short)(((unsigned long)p) & 0xffff);
  regs.w.si = (unsigned short)(((unsigned long)size) >> 16);
  regs.w.di = (unsigned short)(((unsigned long)size) & 0xffff);
  int386x(0x31, &regs, &regs, &sregs);
  dpmilog(regs.x.cflag);
  leavesection("DPMI_MAP_PHYSICAL");
  return (void *)((regs.w.bx << 16) + regs.w.cx);
}

void VBE_Controller_Information(struct VBE_VbeInfoBlock *a)
{
  memcpy(a, VBE_Controller_Info_Pointer, sizeof(struct VBE_VbeInfoBlock));
}

void VBE_Mode_Information(short Mode, struct VBE_ModeInfoBlock *a)
{
  entersection("VBE_Mode_Information");
  PrepareRegisters();
  RMI.EAX = 0x00004f01; // Get SVGA-Mode Information
  RMI.ECX = Mode;
  RMI.ES = VbeModePool.segment; // Segment of realmode data
  RMI.EDI = 0;                  // offset of realmode data
  RMIRQ(0x10);
  vbelog(RMI.EAX);
  memcpy(a, VBE_ModeInfo_Pointer, sizeof(struct VBE_ModeInfoBlock));
  leavesection("VBE_Mode_Information");
  ;
}

int VBE_IsModeLinear(short Mode)
{
  struct VBE_ModeInfoBlock a;
  entersection("VBE_IsModeLinear");
  if (Mode == 0x13)
  {
    leavesection("VBE_IsModeLinear");
    return 1;
  }
#ifdef DISABLE_LFB
  leavesection("VBE_IsModeLinear");
  return 0;
#else
  VBE_Mode_Information(Mode, &a);
  leavesection("VBE_IsModeLinear");
  return ((a.ModeAttributes & 128) == 128);
#endif
}

#ifndef DISABLE_PM_EXTENSIONS

void asmSDS(short lowaddr, short hiaddr);
#pragma aux asmSDS = "mov ax, 0x4f07" \
                     "xor ebx, ebx"   \
                     "call [pm_setdisplaystartcall]" parm[cx][dx] modify[eax ebx ecx edx esi edi];

static void PM_SetDisplayStart(short x, short y)
{
  unsigned long addr = (y * BytesPerScanline + x);
  unsigned short loaddr = addr & 0xffff;
  unsigned short hiaddr = (addr >> 16);
  asmSDS(loaddr, hiaddr);
}

void asmSB(short bnk);
#pragma aux asmSB = "mov ax, 0x4f05" \
                    "mov bx, 0"      \
                    "call [pm_setwindowcall]" parm[dx] modify[eax ebx ecx edx esi edi];

static void PM_SetBank(short bnk)
{
  if (bnk == vbelastbank)
    return;
  vbelastbank = bnk;
  asmSB(bnk);
}
#endif

static void RM_SetDisplayStart(short x, short y)
{
  entersection("RM_SetDisplayStart");
  PrepareRegisters();
  RMI.EAX = 0x00004f07;
  RMI.EBX = 0;
  RMI.ECX = x;
  RMI.EDX = y;
  RMIRQ(0x10);
  vbelog(RMI.EAX);
  leavesection("RM_SetDisplayStart");
}

void setbiosmode(unsigned short c);
#pragma aux setbiosmode = "int 0x10" parm[ax] modify[eax ebx ecx edx esi edi];

void VBE_SetMode(short Mode, int linear, int clear)
{
  struct VBE_ModeInfoBlock a;
  int rawmode;
  entersection("VBE_SetMode");
  /* Is it a normal textmode? if so set it directly! */
  if (Mode == 3)
  {
    setbiosmode(Mode);
    leavesection("VBE_SetMode");
    ;
    return;
  }
  if (Mode == 0x13)
  {
    setbiosmode(Mode);
    leavesection("VBE_SetMode");
    ;
    return;
  }
  rawmode = Mode & 0x0fff;
  if (linear)
    rawmode |= 1 << 14;
  if (!clear)
    rawmode |= 1 << 15;
#ifdef S3FIX
  if (linear && clear)
  {
    /*
     * in case of a S3 card the following bugfix prevents a system-crash
     * when a mode with lfb+clearscreen will be set. We simulate the same
     * behaviour doing the clearscreen without LFB and setting the LFB
     * without clearscreen afterwards (a little bit messy...)
     */

    PrepareRegisters();
    RMI.EAX = 0x00004f02;
    RMI.EBX = Mode & 0x0fff; // mode without LFB but clear
    RMIRQ(0x10);
    vbelog(RMI.EAX);

    PrepareRegisters();
    RMI.EAX = 0x00004f02;
    RMI.EBX = (Mode & 0x0fff) | (1 << 14) | (1 << 15); // mode with LFB but without clear
    RMIRQ(0x10);
    vbelog(RMI.EAX);
  }
  else
  {
    PrepareRegisters();
    RMI.EAX = 0x00004f02;
    RMI.EBX = rawmode;
    RMIRQ(0x10);
    vbelog(RMI.EAX);
  }
#else
  /* Ordinary ModeSwitch call without S3Fix */
  PrepareRegisters();
  RMI.EAX = 0x00004f02;
  RMI.EBX = rawmode;
  RMIRQ(0x10);
  vbelog(RMI.EAX);
#endif
  // get the current mode-info block and set some parameters...
  VBE_Mode_Information(Mode, &a);
  BytesPerScanline = a.BytesPerScanline;
  BitsPerPixel = a.BitsPerPixel;
  /* Reinitalize the Protected Mode part. This hasn't been defined by VESA,
   * but it avoids a lot of errors and incompatibilities */
  VBE_InitPM();
  leavesection("VBE_SetMode");
}

int VBE_Test(void)
{
  entersection("VBE_Test");
  leavesection("VBE_Test");
  ;
  return (VBE_Controller_Info_Pointer->vbeVersion.hi >= 0x2);
}

unsigned int VBE_VideoMemory(void)
{
  entersection("VBE_VideoMemory");
  leavesection("VBE_VideoMemory");
  return (VBE_Controller_Info_Pointer->TotalMemory * 1024 * 64);
}

int VBE_FindMode(int xres, int yres, char bpp)
{
  int i;
  int real_bpp; // true number of bits per pixel... fix lots of buggy bioses
  struct VBE_ModeInfoBlock Info;
  entersection("VBE_FindMode");
  // try to find the mode in the ControllerInfoBlock:
  // terminates on a -1 (vesa spec) and on a 0 (lousy bioses)
  for (i = 0; ((VBE_Controller_Info_Pointer->VideoModePtr[i] != 0xffff) &&
               (VBE_Controller_Info_Pointer->VideoModePtr[i] != 0));
       i++)
  {
    VBE_Mode_Information(VBE_Controller_Info_Pointer->VideoModePtr[i], &Info);
    // use the field-masks to calculate the acutal bit-size if we are searching
    // for a 15 or 16 bit mode (fix for lousy bioses)
    if ((bpp == 15) || (bpp == 16))
    {
      real_bpp = Info.RedMaskSize +
                 Info.GreenMaskSize +
                 Info.BlueMaskSize;
    }
    else
    {
      real_bpp = Info.BitsPerPixel;
    }
    if ((xres == Info.XResolution) &&
        (yres == Info.YResolution) &&
        (bpp == real_bpp))
    {
      leavesection("VBE_FindMode");
      return VBE_Controller_Info_Pointer->VideoModePtr[i];
    }
  }
  leavesection("VBE_FindMode");
  return -1;
}

char *VBE_GetVideoPtr(short mode)
{
  void *phys;
  struct VBE_ModeInfoBlock ModeInfo;
  if (mode == 13)
    return (char *)0xa0000;
  entersection("VBE_GetVideoPtr");
  VBE_Mode_Information(mode, &ModeInfo);
  leavesection("VBE_GetVideoPtr");
  /* Unmap the last physical mapping (if there is one...) */
  if (LastPhysicalMapping)
  {
    DPMI_UNMAP_PHYSICAL(LastPhysicalMapping);
    LastPhysicalMapping = NULL;
  }
  LastPhysicalMapping = DPMI_MAP_PHYSICAL((void *)ModeInfo.PhysBasePtr,
                                          (long)(VBE_Controller_Info_Pointer->TotalMemory) * 64 * 1024);
  return (char *)LastPhysicalMapping;
}

void RM_SetBank(short bnk)
{
  entersection("RM_SetBank");
  if (bnk == vbelastbank)
    return;
  PrepareRegisters();
  RMI.EAX = 0x00004f05;
  RMI.EBX = 0;
  RMI.EDX = bnk;
  RMIRQ(0x10);
  vbelog(RMI.EAX);
  vbelastbank = bnk;
  leavesection("RM_SetBank");
}

short VBE_MaxBytesPerScanline(void)
{
  entersection("VBE_MaxBytesPerScanline");
  PrepareRegisters();
  RMI.EAX = 0x00004f06;
  RMI.EBX = 3;
  RMIRQ(0x10);
  vbelog(RMI.EAX);
  leavesection("VBE_MaxBytesPerScanline");
  return regs.w.bx;
}

void VBE_SetPixelsPerScanline(short Pixels)
{
  entersection("VBE_SetPixelsPerScanline");
  PrepareRegisters();
  RMI.EAX = 0x00004f06;
  RMI.EBX = 0;
  RMI.ECX = Pixels;
  RMIRQ(0x10);
  vbelog(RMI.EAX);
  BytesPerScanline = (Pixels * BitsPerPixel / 8);
  leavesection("VBE_SetPixelsPerScanline");
}

void VBE_SetDACWidth(char bits)
{
  entersection("VBE_SetDACWidth");
  PrepareRegisters();
  RMI.EAX = 0x00004f08;
  RMI.EBX = bits << 8;
  RMIRQ(0x10);
  vbelog(RMI.EAX);
  leavesection("VBE_SetDACWidth");
}

int VBE_8BitDAC(void)
{
  entersection("VBE_8BitDAC");
  leavesection("VBE_8BitDAC");
  return (VBE_Controller_Info_Pointer->Capabilities & 1);
}

void VBE_InitPM(void)
{
#ifndef DISABLE_PM_EXTENSIONS
  unsigned short *pm_pointer;
#endif
  VBE_SetDisplayStart = RM_SetDisplayStart;
  VBE_SetBank = RM_SetBank;
  entersection("VBE_InitPM");
#ifndef DISABLE_PM_EXTENSIONS
  PrepareRegisters();
  RMI.EAX = 0x00004f0a;
  RMIRQ(0x10);
  vbelog(RMI.EAX);
  if ((RMI.EAX) == 0x004f)
  {
    if (pmcode)
      free(pmcode);
    // get some memory to copy the stuff.
    pmcode = malloc(RMI.ECX & 0x0000ffff);
    pm_pointer = (unsigned short *)(((unsigned long)RMI.ES << 4) | (RMI.EDI));
    memcpy(pmcode, pm_pointer, (RMI.ECX & 0x0000ffff));
    pm_pointer = (unsigned short *)pmcode;
    pm_setwindowcall = (void *)(((unsigned long)RMI.ES << 4) | (RMI.EDI + pm_pointer[0]));
    pm_setdisplaystartcall = (void *)(((unsigned long)RMI.ES << 4) | (RMI.EDI + pm_pointer[1]));
    VBE_SetDisplayStart = PM_SetDisplayStart;
    VBE_SetBank = PM_SetBank;
  }
#endif
  leavesection("VBE_InitPM");
}

void VBE_Init(void)
{
  FILE *f;
#ifdef DEBUG_VESA
  dfile = fopen(DEBUGFILENAME, "wt");
  log("log start\n");
  entersection("VBE_Init");

  log("----------- Library Configuration: ---------------\n");
#ifdef DISABLE_PM_EXTENSIONS
  log("Protected Mode Extensions Disabled\n");
#else
  log("Protected Mode Extensions Enabled\n");
#endif

#ifdef DISABLE_LFB
  log("LFB Support Disabled\n");
#else
  log("LFB Support Enabled\n");
#endif
#ifdef S3FIX
  log("S3 Bugfix Support Enabled\n");
#else
  log("S3 Bugfix Support Disabled\n");
#endif

  log("-----------------------------------------------\n");
#endif

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
  RMIRQ(0x10);
  vbelog(RMI.EAX);
  // Translate the Realmode Pointers into flat-memory address space
  VBE_Controller_Info_Pointer->OemStringPtr = (char *)((((unsigned long)VBE_Controller_Info_Pointer->OemStringPtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->OemStringPtr);
  VBE_Controller_Info_Pointer->VideoModePtr = (unsigned short *)((((unsigned long)VBE_Controller_Info_Pointer->VideoModePtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->VideoModePtr);
  VBE_Controller_Info_Pointer->OemVendorNamePtr = (char *)((((unsigned long)VBE_Controller_Info_Pointer->OemVendorNamePtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->OemVendorNamePtr);
  VBE_Controller_Info_Pointer->OemProductNamePtr = (char *)((((unsigned long)VBE_Controller_Info_Pointer->OemProductNamePtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->OemProductNamePtr);
  VBE_Controller_Info_Pointer->OemProductRevPtr = (char *)((((unsigned long)VBE_Controller_Info_Pointer->OemProductRevPtr >> 16) << 4) + (unsigned short)VBE_Controller_Info_Pointer->OemProductRevPtr);

  VBE_InitPM();

  leavesection("VBE_Init");
  ;
}

void VBE_Done(void)
{
  entersection("VBE_Done");
  if (LastPhysicalMapping)
  {
    DPMI_UNMAP_PHYSICAL(LastPhysicalMapping);
  }
  DPMI_FreeDOSMem(&VbeModePool);
  DPMI_FreeDOSMem(&VbeInfoPool);
#ifndef DISABLE_PM_EXTENSIONS
  if (pmcode)
    free(pmcode);
#endif
#ifdef DEBUG_VESA
  leavesection("VBE_Done");
  log("log end\n");
  fclose(dfile);
#endif
}
