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
