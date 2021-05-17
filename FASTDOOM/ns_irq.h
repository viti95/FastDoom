#ifndef __IRQ_H
#define __IRQ_H

enum IRQ_ERRORS
{
    IRQ_Warning = -2,
    IRQ_Error = -1,
    IRQ_Ok = 0,
};

#define VALID_IRQ(irq) (((irq) >= 0) && ((irq) <= 15))

int IRQ_SetVector(int vector, void(__interrupt *function)(void));
int IRQ_RestoreVector(int vector);

#endif
