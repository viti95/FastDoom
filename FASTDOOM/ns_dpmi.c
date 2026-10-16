#include <dos.h>
#include <string.h>
#include "ns_dpmi.h"
#include "options.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

static union REGS Regs;
static struct SREGS SegRegs;

int DPMI_CallRealModeFunction(dpmi_regs *callregs)
{
    // Setup our registers to call DPMI
    Regs.w.ax = 0x0301;
    Regs.h.bl = 0;
    Regs.h.bh = 0;
    Regs.w.cx = 0;

    SegRegs.es = FP_SEG(callregs);
    Regs.x.edi = FP_OFF(callregs);

    // Call Real-mode procedure with Far Return Frame
    int386x(0x31, &Regs, &Regs, &SegRegs);

    if (Regs.x.cflag)
    {
        return (DPMI_Error);
    }

    return (DPMI_Ok);
}

void DPMI_AllocDOSMem(short int paras, struct DPMI_PTR *p)
{
    // DPMI call 100h allocates DOS memory
    memset(&Regs, 0, sizeof(Regs));
    memset(&SegRegs, 0, sizeof(SegRegs));

    Regs.w.ax = 0x0100;
    Regs.w.bx = paras;

    int386x(0x31, &Regs, &Regs, &SegRegs);

    p->segment = Regs.w.ax;
    p->selector = Regs.w.dx;
}

void DPMI_FreeDOSMem(struct DPMI_PTR *p)
{
    // DPMI call 101h frees an allocated DOS memory block
    memset(&Regs, 0, sizeof(Regs));
    memset(&SegRegs, 0, sizeof(SegRegs));

    Regs.w.ax = 0x0101;
    Regs.w.dx = p->selector;

    int386x(0x31, &Regs, &Regs, &SegRegs);
}

/*---------------------------------------------------------------------
   Function: DPMI_AllocTimerStack

   Allocate a block of memory from conventional (low) memory and return
   the selector (which can go directly into a segment register) of the
   memory block or 0 if an error occured.
---------------------------------------------------------------------*/
unsigned short DPMI_AllocTimerStack(
    unsigned short size)

{
    union REGS regs;

    // clear all registers
    memset(&regs, 0, sizeof(regs));

    // DPMI allocate conventional memory
    regs.w.ax = 0x100;

    // size in paragraphs
    regs.w.bx = (size + 15) / 16;

    int386(0x31, &regs, &regs);
    if (!regs.w.cflag)
    {
        // DPMI call returns selector in dx
        // (ax contains real mode segment
        // which is ignored here)

        return (regs.w.dx);
    }

    // Couldn't allocate memory.
    return (NULL);
}

/*---------------------------------------------------------------------
   Function: DPMI_FreeTimerStack

   Deallocate a block of conventional (low) memory given a selector to
   it.  Assumes the block was allocated with DPMI function 0x100.
---------------------------------------------------------------------*/
void DPMI_FreeTimerStack(
    unsigned short selector)

{
    union REGS regs;

    if (selector != NULL)
    {
        // clear all registers
        memset(&regs, 0, sizeof(regs));

        regs.w.ax = 0x101;
        regs.w.dx = selector;
        int386(0x31, &regs, &regs);
    }
}

int DPMI_LockMemory(void *address, unsigned length)
{
    unsigned linear;

    // Thanks to DOS/4GW's zero-based flat memory model, converting
    // a pointer of any type to a linear address is trivial.

    linear = (unsigned)address;

    // DPMI Lock Linear Region
    Regs.w.ax = 0x600;

    // Linear address in BX:CX
    Regs.w.bx = (linear >> 16);
    Regs.w.cx = (linear & 0xFFFF);

    // Length in SI:DI
    Regs.w.si = (length >> 16);
    Regs.w.di = (length & 0xFFFF);

    int386(0x31, &Regs, &Regs);

    // Return 0 if can't lock
    if (Regs.w.cflag)
    {
        return (DPMI_Error);
    }

    return (DPMI_Ok);
}
