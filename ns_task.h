#ifndef __TASK_MAN_H
#define __TASK_MAN_H

enum TASK_ERRORS
{
   TASK_Warning = -2,
   TASK_Error = -1,
   TASK_Ok = 0
};

typedef struct task
{
   struct task *next;
   struct task *prev;
   void (*TaskService)(struct task *);
   void *data;
   long rate;
   volatile long count;
   int priority;
   int active;
} task;

// TS_InInterrupt is TRUE during a taskman interrupt.
// Use this if you have code that may be used both outside
// and within interrupts.

extern volatile int TS_InInterrupt;

void TS_Shutdown(void);
task *TS_ScheduleTask(void (*Function)(task *), int rate,
                      int priority, void *data);
int TS_Terminate(task *ptr);
void TS_Dispatch(void);
void TS_SetTaskRate(task *Task, int rate);
void TS_UnlockMemory(void);
int TS_LockMemory(void);

#endif
