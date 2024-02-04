#include <dos.h>
#include <stdlib.h>
#include "ns_irq.h"
#include "options.h"
#include "i_ibm.h"

#define D32RealSeg(P) ((((unsigned long)(P)) >> 4) & 0xFFFF)
#define D32RealOff(P) (((unsigned long)(P)) & 0xF)

typedef struct
{
    unsigned long drdi;
    unsigned long drsi;
    unsigned long drbp;
    unsigned long drxx;
    unsigned long drbx;
    unsigned long drdx;
    unsigned long drcx;
    unsigned long drax;

    unsigned short drflags;
    unsigned short dres;
    unsigned short drds;
    unsigned short drfs;
    unsigned short drgs;
    unsigned short drip;
    unsigned short drcs;
    unsigned short drsp;
    unsigned short drss;
} DPMI_REGS;

static DPMI_REGS rmregs = {0};
static void(__interrupt __far *IRQ_Callback)(void) = NULL;

static char *IRQ_RealModeCode = NULL;

static unsigned short IRQ_CallBackSegment;
static unsigned short IRQ_CallBackOffset;
static unsigned short IRQ_RealModeSegment;
static unsigned short IRQ_RealModeOffset;
static unsigned long IRQ_ProtectedModeOffset;
static unsigned short IRQ_ProtectedModeSelector;

static union REGS Regs;
static struct SREGS SegRegs;

// Intermediary function: DPMI calls this, making it
// easier to write in C
// handle 16-bit incoming stack

void fixebp(void);

#pragma aux fixebp = \
    "mov   bx,  ss"  \
    "lar   ebx, ebx" \
    "bt    ebx, 22"  \
    "jc    bigstk"   \
    "and   esp, 0x0000FFFF" \
    "mov   ebp, esp" \
    "bigstk:" modify exact[ebx];

#pragma aux rmcallback parm[];

void rmcallback(
    unsigned short _far *stkp)

{
    // "Pop" the real mode return frame so we
    // can resume where we left off
    rmregs.drip = *stkp++;
    rmregs.drcs = *stkp++;

    rmregs.drsp = FP_OFF(stkp);

    // Call protected-mode handler
    IRQ_Callback();
}

static void _interrupt _cdecl callback_x(
    // regs pushed in this order by prologue

    int rgs,
    int rfs,
    int res,
    int rds,
    int rdi,
    int rsi,
    int rbp,
    int rsp,
    int rbx,
    int rdx,
    int rcx,
    int rax)

{
    //   unsigned short _far *stkp;
    //   return;

    fixebp();
    rmcallback(MK_FP(rds, rsi));
}

int IRQ_SetVector(int vector, void(__interrupt __far *function)(void))
{
    void far *fp;

    IRQ_Callback = function;

    // Save the starting real-mode and protected-mode handler addresses

    // DPMI get protected mode vector */
    Regs.w.ax = 0x0204;
    Regs.w.bx = vector;
    int386(0x31, &Regs, &Regs);
    IRQ_ProtectedModeSelector = Regs.w.cx;
    IRQ_ProtectedModeOffset = Regs.x.edx;

    // DPMI get real mode vector
    Regs.w.ax = 0x0200;
    Regs.w.bx = vector;
    int386(0x31, &Regs, &Regs);
    IRQ_RealModeSegment = Regs.w.cx;
    IRQ_RealModeOffset = Regs.w.dx;

    // Set up callback
    // DPMI allocate real mode callback
    Regs.w.ax = 0x0303;
    fp = (void far *)callback_x;
    SegRegs.ds = FP_SEG(fp);
    Regs.x.esi = FP_OFF(fp);
    fp = (void _far *)&rmregs;
    SegRegs.es = FP_SEG(fp);
    Regs.x.edi = FP_OFF(fp);
    int386x(0x31, &Regs, &Regs, &SegRegs);

    IRQ_CallBackSegment = Regs.w.cx;
    IRQ_CallBackOffset = Regs.w.dx;

    if (Regs.x.cflag != 0)
    {
        return (IRQ_Error);
    }

    if (IRQ_RealModeCode == NULL)
    {
        // Allocate 6 bytes of low memory for real mode interrupt handler
        IRQ_RealModeCode = I_DosMemAlloc(6);
        if (IRQ_RealModeCode == NULL)
        {
            // Free callback
            Regs.w.ax = 0x304;
            Regs.w.cx = IRQ_CallBackSegment;
            Regs.w.dx = IRQ_CallBackOffset;
            int386x(0x31, &Regs, &Regs, &SegRegs);

            return (IRQ_Error);
        }
    }

    // Poke code (to call callback) into real mode handler

    // CALL FAR PTR (callback)
    IRQ_RealModeCode[0] = '\x9A';
    *((unsigned short *)&IRQ_RealModeCode[1]) = IRQ_CallBackOffset;
    *((unsigned short *)&IRQ_RealModeCode[3]) = IRQ_CallBackSegment;

    // IRET
    IRQ_RealModeCode[5] = '\xCF';

    // Install protected mode handler
    // DPMI set protected mode vector
    Regs.w.ax = 0x0205;
    Regs.w.bx = vector;
    fp = function;

    Regs.w.cx = FP_SEG(fp);
    Regs.x.edx = FP_OFF(fp);
    int386(0x31, &Regs, &Regs);

    // Install callback address as real mode handler
    // DPMI set real mode vector
    Regs.w.ax = 0x0201;
    Regs.w.bx = vector;
    Regs.w.cx = D32RealSeg(IRQ_RealModeCode);
    Regs.w.dx = D32RealOff(IRQ_RealModeCode);
    int386(0x31, &Regs, &Regs);

    return (IRQ_Ok);
}

int IRQ_RestoreVector(int vector)
{
    // Restore original interrupt handlers
    // DPMI set real mode vector
    Regs.w.ax = 0x0201;
    Regs.w.bx = vector;
    Regs.w.cx = IRQ_RealModeSegment;
    Regs.w.dx = IRQ_RealModeOffset;
    int386(0x31, &Regs, &Regs);

    Regs.w.ax = 0x0205;
    Regs.w.bx = vector;
    Regs.w.cx = IRQ_ProtectedModeSelector;
    Regs.x.edx = IRQ_ProtectedModeOffset;
    int386(0x31, &Regs, &Regs);

    // Free callback
    Regs.w.ax = 0x304;
    Regs.w.cx = IRQ_CallBackSegment;
    Regs.w.dx = IRQ_CallBackOffset;
    int386x(0x31, &Regs, &Regs, &SegRegs);

    if (Regs.x.cflag)
    {
        return (IRQ_Error);
    }

    return (IRQ_Ok);
}
