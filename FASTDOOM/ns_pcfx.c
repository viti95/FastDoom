#include <dos.h>
#include <stdlib.h>
#include <conio.h>
#include "ns_dpmi.h"
#include "ns_task.h"
#include "ns_inter.h"
#include "ns_pcfx.h"
#include "options.h"
#include "fastmath.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

static void PCFX_Service(task *Task);

static long PCFX_LengthLeft;
static char *PCFX_Sound = NULL;
static int PCFX_LastSample;
static int PCFX_Priority;
static int PCFX_TotalVolume = PCFX_MaxVolume;
static task *PCFX_ServiceTask = NULL;
static int PCFX_VoiceHandle = PCFX_MinVoiceHandle;

int PCFX_Installed = FALSE;

int PCFX_Stop(int handle)
{
    unsigned flags;

    if ((handle != PCFX_VoiceHandle) || (PCFX_Sound == NULL))
    {
        return (PCFX_Warning);
    }

    flags = DisableInterrupts();

    // Turn off speaker
    OutByte61h(InByte61h() & 0xfc);

    PCFX_Sound = NULL;
    PCFX_LengthLeft = 0;
    PCFX_Priority = 0;
    PCFX_LastSample = 0;

    RestoreInterrupts(flags);

    return (PCFX_Ok);
}

static void PCFX_Service(task *Task)
{
    unsigned value;

    if (PCFX_Sound)
    {
        value = *(short int *)PCFX_Sound;
        PCFX_Sound += sizeof(short int);

        if ((PCFX_TotalVolume > 0) && (value != PCFX_LastSample))
        {
            PCFX_LastSample = value;
            if (value)
            {
                OutByte43h(0xb6);
                OutByte42h(value);
                OutByte42h(value >> 8);
                OutByte61h(InByte61h() | 0x3);
            }
            else
            {
                OutByte61h(InByte61h() & 0xfc);
            }
        }
        if (--PCFX_LengthLeft == 0)
        {
            PCFX_Stop(PCFX_VoiceHandle);
        }
    }
}

int PCFX_Play(PCSound *sound, int priority)
{
    unsigned flags;

    if (priority < PCFX_Priority)
    {
        return (PCFX_Warning);
    }

    PCFX_Stop(PCFX_VoiceHandle);

    PCFX_VoiceHandle++;
    if (PCFX_VoiceHandle < PCFX_MinVoiceHandle)
    {
        PCFX_VoiceHandle = PCFX_MinVoiceHandle;
    }

    flags = DisableInterrupts();

    PCFX_LengthLeft = sound->length;

    PCFX_LengthLeft >>= 1;

    PCFX_Priority = priority;

    PCFX_Sound = &sound->data;

    RestoreInterrupts(flags);

    return (PCFX_VoiceHandle);
}

int PCFX_SoundPlaying(int handle)
{
    int status;

    status = FALSE;
    if ((handle == PCFX_VoiceHandle) && (PCFX_LengthLeft > 0))
    {
        status = TRUE;
    }

    return (status);
}

int PCFX_SetTotalVolume(int volume)
{
    unsigned flags;

    flags = DisableInterrupts();

    volume = max(volume, 0);
    volume = min(volume, PCFX_MaxVolume);

    PCFX_TotalVolume = volume;

    if (volume == 0)
    {
        OutByte61h(InByte61h() & 0xfc);
    }

    RestoreInterrupts(flags);

    return (PCFX_Ok);
}

int PCFX_Init(void){
    int status;

    if (PCFX_Installed)
    {
        PCFX_Shutdown();
    }

    PCFX_Stop(PCFX_VoiceHandle);
    PCFX_ServiceTask = TS_ScheduleTask(&PCFX_Service, 140, 2, NULL);
    TS_Dispatch();

    PCFX_Installed = TRUE;

    return (PCFX_Ok);
}

int PCFX_Shutdown(void)
{
    if (PCFX_Installed)
    {
        PCFX_Stop(PCFX_VoiceHandle);
        TS_Terminate(PCFX_ServiceTask);
        PCFX_Installed = FALSE;
    }

    return (PCFX_Ok);
}
