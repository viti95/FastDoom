#define TRUE (1 == 1)
#define FALSE (!TRUE)

#define LOCKMEMORY
#define NOINTS

#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <string.h>
#include "ns_inter.h"
#include "ns_ll.h"
#include "ns_task.h"
#include "options.h"
#include "fastmath.h"
#include "ns_dpmi.h"
#include "z_zone.h"

#ifdef LOCKMEMORY
#include "ns_dpmi.h"
#endif

typedef struct
{
    task *start;
    task *end;
} tasklist;

/*---------------------------------------------------------------------
   Global variables
---------------------------------------------------------------------*/
// adequate stack size
#define kStackSize 2048

static unsigned short StackSelector = NULL;
static unsigned long StackPointer;

static unsigned short oldStackSelector;
static unsigned long oldStackPointer;

extern  unsigned short  GetDS( void );
#pragma aux GetDS = \
        "mov    ax,ds" value[ax]

static task HeadTask;
static task *TaskList = &HeadTask;

static void(__interrupt __far *OldInt8)(void);

static volatile long TaskServiceRate = 0x10000L;
static volatile long TaskServiceCount = 0;

#ifndef NOINTS
static volatile int TS_TimesInInterrupt;
#endif

static char TS_Installed = FALSE;

volatile int TS_InInterrupt = FALSE;

/*---------------------------------------------------------------------
   Function prototypes
---------------------------------------------------------------------*/

static void TS_FreeTaskList(void);
static void TS_SetClockSpeed(long speed);
static long TS_SetTimer(long TickBase);
static void TS_SetTimerToMaxTaskRate(void);
static void __interrupt __far TS_ServiceSchedule(void);
static void __interrupt __far TS_ServiceScheduleIntEnabled(void);
static void TS_AddTask(task *ptr);
static int TS_Startup(void);
static void RestoreRealTimeClock(void);

// These declarations are necessary to use the inline assembly pragmas.

extern void GetStack(unsigned short *selptr, unsigned long *stackptr);
extern void SetStack(unsigned short selector, unsigned long stackptr);

// This function will get the current stack selector and pointer and save
// them off.
#pragma aux GetStack = \
    "mov  [edi],esp"   \
    "mov	ax,ss"        \
    "mov  [esi],ax" parm[esi][edi] modify[eax esi edi];

// This function will set the stack selector and pointer to the specified
// values.
#pragma aux SetStack = \
    "mov  ss,ax"       \
    "mov  esp,edx" parm[ax][edx] modify[eax edx];

static void TS_FreeTaskList(void)
{
    task *node;
    task *next;
    unsigned flags;

    flags = DisableInterrupts();

    node = TaskList->next;
    while (node != TaskList)
    {
        next = node->next;
        Z_Free(node);
        node = next;
    }

    TaskList->next = TaskList;
    TaskList->prev = TaskList;

    RestoreInterrupts(flags);
}

static void TS_SetClockSpeed(long speed)
{
    unsigned flags;

    flags = DisableInterrupts();

    if ((speed > 0) && (speed < 0x10000L))
    {
        TaskServiceRate = speed;
    }
    else
    {
        TaskServiceRate = 0x10000L;
    }

    outp(0x43, 0x36);
    outp(0x40, TaskServiceRate);
    outp(0x40, TaskServiceRate >> 8);

    RestoreInterrupts(flags);
}

static long TS_SetTimer(long TickBase)
{
    long speed;

    // VITI95: OPTIMIZE
    speed = 1192030L / TickBase;
    if (speed < TaskServiceRate)
    {
        TS_SetClockSpeed(speed);
    }

    return (speed);
}

static void TS_SetTimerToMaxTaskRate(void)
{
    task *ptr;
    long MaxServiceRate;
    unsigned flags;

    flags = DisableInterrupts();

    MaxServiceRate = 0x10000L;

    ptr = TaskList->next;
    while (ptr != TaskList)
    {
        if (ptr->rate < MaxServiceRate)
        {
            MaxServiceRate = ptr->rate;
        }

        ptr = ptr->next;
    }

    if (TaskServiceRate != MaxServiceRate)
    {
        TS_SetClockSpeed(MaxServiceRate);
    }

    RestoreInterrupts(flags);
}

#ifdef NOINTS

static void __interrupt __far TS_ServiceSchedule(void)
{
    task *ptr;
    task *next;

    TS_InInterrupt = TRUE;

    // save stack
    GetStack(&oldStackSelector, &oldStackPointer);

    // set our stack
    SetStack(StackSelector, StackPointer);

    ptr = TaskList->next;
    while (ptr != TaskList)
    {
        next = ptr->next;

        if (ptr->active)
        {
            ptr->count += TaskServiceRate;
            //JIM
            //         if ( ptr->count >= ptr->rate )
            while (ptr->count >= ptr->rate)
            {
                ptr->count -= ptr->rate;
                ptr->TaskService(ptr);
            }
        }
        ptr = next;
    }

    // restore stack
    SetStack(oldStackSelector, oldStackPointer);

    TaskServiceCount += TaskServiceRate;
    if (TaskServiceCount > 0xffffL)
    {
        TaskServiceCount &= 0xffff;
        _chain_intr(OldInt8);
    }

    OutByte20h(0x20);

    TS_InInterrupt = FALSE;
}

#else

static void __interrupt __far TS_ServiceScheduleIntEnabled(void)
{
    task *ptr;
    task *next;

    TS_TimesInInterrupt++;
    TaskServiceCount += TaskServiceRate;
    if (TaskServiceCount > 0xffffL)
    {
        TaskServiceCount &= 0xffff;
        _chain_intr(OldInt8);
    }

    OutByte20h(0x20);

    if (TS_InInterrupt)
    {
        return;
    }

    TS_InInterrupt = TRUE;
    _enable();

    // save stack
    GetStack(&oldStackSelector, &oldStackPointer);

    // set our stack
    SetStack(StackSelector, StackPointer);

    while (TS_TimesInInterrupt)
    {
        ptr = TaskList->next;
        while (ptr != TaskList)
        {
            next = ptr->next;

            if (ptr->active)
            {
                ptr->count += TaskServiceRate;
                if (ptr->count >= ptr->rate)
                {
                    ptr->count -= ptr->rate;
                    ptr->TaskService(ptr);
                }
            }
            ptr = next;
        }
        TS_TimesInInterrupt--;
    }

    _disable();

    // restore stack
    SetStack(oldStackSelector, oldStackPointer);

    TS_InInterrupt = FALSE;
}
#endif

/*---------------------------------------------------------------------
   Function: allocateTimerStack

   Allocate a block of protected mode memory which can be accessed by DS selector.
---------------------------------------------------------------------*/

static unsigned long allocateTimerStack(
    unsigned short size)

{
    return (unsigned long)Z_MallocUnowned(size, PU_STATIC);
}

/*---------------------------------------------------------------------
   Function: deallocateTimerStack

---------------------------------------------------------------------*/

static void deallocateTimerStack(
    unsigned long pointer)

{
    Z_Free((void*)pointer);
}

/*---------------------------------------------------------------------
   Function: TS_Startup

   Sets up the task service routine.
---------------------------------------------------------------------*/

static int TS_Startup(
    void)

{
    if (!TS_Installed)
    {

        StackPointer = allocateTimerStack(kStackSize);
        if (StackPointer == 0)
        {
            return (TASK_Error);
        }

        // Leave a little room at top of stack just for the hell of it...
        StackPointer += kStackSize - sizeof(long);

        //SS=DS so it's still FLAT during interrupts, and be safe for watcom to generate optimized code using EBP to access static/global vars.
        //this is essential when DPMI interfaces are provided by a DPMI host rather than DOS32A,
        //as a DPMI host will provide another stack making SS != DS.
        StackSelector = GetDS();

        //static const task *TaskList = &HeadTask;
        TaskList->next = TaskList;
        TaskList->prev = TaskList;

        TaskServiceRate = 0x10000L;
        TaskServiceCount = 0;

#ifndef NOINTS
        TS_TimesInInterrupt = 0;
#endif

        OldInt8 = _dos_getvect(0x08);
#ifdef NOINTS
        _dos_setvect(0x08, TS_ServiceSchedule);
#else
        _dos_setvect(0x08, TS_ServiceScheduleIntEnabled);
#endif

        TS_Installed = TRUE;
    }

    return (TASK_Ok);
}

/*---------------------------------------------------------------------
   Function: TS_Shutdown

   Ends processing of all tasks.
---------------------------------------------------------------------*/

void TS_Shutdown(
    void)

{
    if (TS_Installed)
    {
        TS_FreeTaskList();

        TS_SetClockSpeed(0);

        _dos_setvect(0x08, OldInt8);

        deallocateTimerStack(StackPointer - (kStackSize-sizeof(long)));
        StackSelector = NULL;

        // Set Date and Time from CMOS
        //      RestoreRealTimeClock();

        TS_Installed = FALSE;
    }
}

/*---------------------------------------------------------------------
   Function: TS_ScheduleTask

   Schedules a new task for processing.
---------------------------------------------------------------------*/

task *TS_ScheduleTask(
    void (*Function)(task *),
    int rate,
    int priority,
    void *data)

{
    task *ptr;
    int status;

    ptr = Z_MallocUnowned(sizeof(task), PU_STATIC);

    if (!TS_Installed)
    {
        status = TS_Startup();
        if (status != TASK_Ok)
        {
            Z_Free(ptr);
            return (NULL);
        }
    }

    ptr->TaskService = Function;
    ptr->data = data;
    ptr->rate = TS_SetTimer(rate);
    ptr->count = 0;
    ptr->priority = priority;
    ptr->active = FALSE;

    TS_AddTask(ptr);
    

    return (ptr);
}

/*---------------------------------------------------------------------
   Function: TS_AddTask

   Adds a new task to our list of tasks.
---------------------------------------------------------------------*/

static void TS_AddTask(
    task *node)

{
    LL_SortedInsertion(TaskList, node, next, prev, task, priority);
}

/*---------------------------------------------------------------------
   Function: TS_Terminate

   Ends processing of a specific task.
---------------------------------------------------------------------*/

int TS_Terminate(
    task *NodeToRemove)

{
    task *ptr;
    task *next;
    unsigned flags;

    flags = DisableInterrupts();

    ptr = TaskList->next;
    while (ptr != TaskList)
    {
        next = ptr->next;

        if (ptr == NodeToRemove)
        {
            LL_RemoveNode(NodeToRemove, next, prev);
            NodeToRemove->next = NULL;
            NodeToRemove->prev = NULL;
            Z_Free(NodeToRemove);

            TS_SetTimerToMaxTaskRate();

            RestoreInterrupts(flags);

            return (TASK_Ok);
        }

        ptr = next;
    }

    RestoreInterrupts(flags);

    return (TASK_Warning);
}

/*---------------------------------------------------------------------
   Function: TS_Dispatch

   Begins processing of all inactive tasks.
---------------------------------------------------------------------*/

void TS_Dispatch(
    void)

{
    task *ptr;
    unsigned flags;

    flags = DisableInterrupts();

    ptr = TaskList->next;
    while (ptr != TaskList)
    {
        ptr->active = TRUE;
        ptr = ptr->next;
    }

    RestoreInterrupts(flags);
}

/*---------------------------------------------------------------------
   Function: TS_SetTaskRate

   Sets the rate at which the specified task is serviced.
---------------------------------------------------------------------*/

void TS_SetTaskRate(
    task *Task,
    int rate)

{
    unsigned flags;

    flags = DisableInterrupts();

    Task->rate = TS_SetTimer(rate);
    TS_SetTimerToMaxTaskRate();

    RestoreInterrupts(flags);
}
