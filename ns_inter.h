#ifndef __INTERRUPT_H
#define __INTERRUPT_H

unsigned long DisableInterrupts(void);
void RestoreInterrupts(unsigned long flags);

#pragma aux DisableInterrupts = \
    "pushfd",                   \
            "pop    eax",       \
            "cli" modify[eax];

#pragma aux RestoreInterrupts = \
    "push   eax",               \
            "popfd" parm[eax];

#endif
