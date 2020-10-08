#ifndef __DPMI_H
#define __DPMI_H

enum DPMI_Errors
{
   DPMI_Warning = -2,
   DPMI_Error = -1,
   DPMI_Ok = 0
};

typedef struct
{
   unsigned long EDI;
   unsigned long ESI;
   unsigned long EBP;
   unsigned long Reserved;
   unsigned long EBX;
   unsigned long EDX;
   unsigned long ECX;
   unsigned long EAX;
   unsigned short Flags;
   unsigned short ES;
   unsigned short DS;
   unsigned short FS;
   unsigned short GS;
   unsigned short IP;
   unsigned short CS;
   unsigned short SP;
   unsigned short SS;
} dpmi_regs;

int DPMI_CallRealModeFunction(dpmi_regs *callregs);
int DPMI_GetDOSMemory(void **ptr, int *descriptor, unsigned length);
int DPMI_FreeDOSMemory(int descriptor);
int DPMI_LockMemory(void *address, unsigned length);
int DPMI_LockMemoryRegion(void *start, void *end);

#define DPMI_Lock(variable) \
   (DPMI_LockMemory(&(variable), sizeof(variable)))

#pragma aux DPMI_GetDOSMemory =    \
    "mov    eax, 0100h",           \
            "add    ebx, 15",      \
            "shr    ebx, 4",       \
            "int    31h",          \
            "jc     DPMI_Exit",    \
            "movzx  eax, ax",      \
            "shl    eax, 4",       \
            "mov    [ esi ], eax", \
            "mov    [ edi ], edx", \
            "sub    eax, eax",     \
            "DPMI_Exit:",          \
            parm[esi][edi][ebx] modify exact[eax ebx edx];

#pragma aux DPMI_FreeDOSMemory = \
    "mov    eax, 0101h",         \
            "int    31h",        \
            "jc     DPMI_Exit",  \
            "sub    eax, eax",   \
            "DPMI_Exit:",        \
            parm[edx] modify exact[eax];

#endif
