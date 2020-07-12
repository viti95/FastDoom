#include <dos.h>
#include <stdlib.h>
#include <conio.h>
#include "ns_dpmi.h"
#include "task_man.h"
#include "ns_inter.h"
#include "ns_pcfx.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

static void PCFX_Service(task *Task);

static long PCFX_LengthLeft;
static char *PCFX_Sound = NULL;
static int PCFX_LastSample;
static short PCFX_Lookup[256];
static int PCFX_UseLookupFlag = FALSE;
static int PCFX_Priority;
static unsigned long PCFX_CallBackVal;
static void (*PCFX_CallBackFunc)(unsigned long) = NULL;
static int PCFX_TotalVolume = PCFX_MaxVolume;
static task *PCFX_ServiceTask = NULL;
static int PCFX_VoiceHandle = PCFX_MinVoiceHandle;

int PCFX_Installed = FALSE;

int PCFX_ErrorCode = PCFX_Ok;

#define PCFX_SetErrorCode(status) PCFX_ErrorCode = (status);

#define PCFX_LockStart PCFX_Stop

int PCFX_Stop(int handle)
{
    unsigned flags;

    if ((handle != PCFX_VoiceHandle) || (PCFX_Sound == NULL))
    {
        PCFX_SetErrorCode(PCFX_VoiceNotFound);
        return (PCFX_Warning);
    }

    flags = DisableInterrupts();

    // Turn off speaker
    outp(0x61, inp(0x61) & 0xfc);

    PCFX_Sound = NULL;
    PCFX_LengthLeft = 0;
    PCFX_Priority = 0;
    PCFX_LastSample = 0;

    RestoreInterrupts(flags);

    if (PCFX_CallBackFunc)
    {
        PCFX_CallBackFunc(PCFX_CallBackVal);
    }

    return (PCFX_Ok);
}

static void PCFX_Service(task *Task)
{
    unsigned value;

    if (PCFX_Sound)
    {
        if (PCFX_UseLookupFlag)
        {
            value = PCFX_Lookup[*PCFX_Sound];
            PCFX_Sound++;
        }
        else
        {
            value = *(short int *)PCFX_Sound;
            PCFX_Sound += sizeof(short int);
        }

        if ((PCFX_TotalVolume > 0) && (value != PCFX_LastSample))
        {
            PCFX_LastSample = value;
            if (value)
            {
                outp(0x43, 0xb6);
                outp(0x42, value);
                outp(0x42, value >> 8);
                outp(0x61, inp(0x61) | 0x3);
            }
            else
            {
                outp(0x61, inp(0x61) & 0xfc);
            }
        }
        if (--PCFX_LengthLeft == 0)
        {
            PCFX_Stop(PCFX_VoiceHandle);
        }
    }
}

int PCFX_Play(PCSound *sound, int priority, unsigned long callbackval)
{
    unsigned flags;

    if (priority < PCFX_Priority)
    {
        PCFX_SetErrorCode(PCFX_NoVoices);
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

    if (!PCFX_UseLookupFlag)
    {
        PCFX_LengthLeft >>= 1;
    }

    PCFX_Priority = priority;

    PCFX_Sound = &sound->data;
    PCFX_CallBackVal = callbackval;

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
        outp(0x61, inp(0x61) & 0xfc);
    }

    RestoreInterrupts(flags);

    return (PCFX_Ok);
}

static void PCFX_LockEnd(void){}

void PCFX_UseLookup(int use, unsigned value)
{
    int pitch;
    int index;

    PCFX_Stop(PCFX_VoiceHandle);

    PCFX_UseLookupFlag = use;
    if (use)
    {
        pitch = 0;
        for (index = 0; index < 256; index++)
        {
            PCFX_Lookup[index] = pitch;
            pitch += value;
        }
    }
}

int PCFX_Init(void){
    int status;

    if (PCFX_Installed)
    {
        PCFX_Shutdown();
    }

    status = PCFX_LockMemory();
    if (status != PCFX_Ok)
    {
        PCFX_UnlockMemory();
        return (status);
    }

    PCFX_UseLookup(TRUE, 60);
    PCFX_Stop(PCFX_VoiceHandle);
    PCFX_ServiceTask = TS_ScheduleTask(&PCFX_Service, 140, 2, NULL);
    TS_Dispatch();

    PCFX_CallBackFunc = NULL;
    PCFX_Installed = TRUE;

    PCFX_SetErrorCode(PCFX_Ok);
    return (PCFX_Ok);
}

int PCFX_Shutdown(void)
{
    if (PCFX_Installed)
    {
        PCFX_Stop(PCFX_VoiceHandle);
        TS_Terminate(PCFX_ServiceTask);
        PCFX_UnlockMemory();
        PCFX_Installed = FALSE;
    }

    PCFX_SetErrorCode(PCFX_Ok);
    return (PCFX_Ok);
}

void PCFX_UnlockMemory(void)
{
    DPMI_UnlockMemoryRegion(PCFX_LockStart, PCFX_LockEnd);
    DPMI_Unlock(PCFX_LengthLeft);
    DPMI_Unlock(PCFX_Sound);
    DPMI_Unlock(PCFX_LastSample);
    DPMI_Unlock(PCFX_Lookup);
    DPMI_Unlock(PCFX_UseLookupFlag);
    DPMI_Unlock(PCFX_Priority);
    DPMI_Unlock(PCFX_CallBackVal);
    DPMI_Unlock(PCFX_CallBackFunc);
    DPMI_Unlock(PCFX_TotalVolume);
    DPMI_Unlock(PCFX_ServiceTask);
    DPMI_Unlock(PCFX_VoiceHandle);
    DPMI_Unlock(PCFX_Installed);
    DPMI_Unlock(PCFX_ErrorCode);
}

int PCFX_LockMemory(void)
{
    int status;

    status = DPMI_LockMemoryRegion(PCFX_LockStart, PCFX_LockEnd);
    status |= DPMI_Lock(PCFX_LengthLeft);
    status |= DPMI_Lock(PCFX_Sound);
    status |= DPMI_Lock(PCFX_LastSample);
    status |= DPMI_Lock(PCFX_Lookup);
    status |= DPMI_Lock(PCFX_UseLookupFlag);
    status |= DPMI_Lock(PCFX_Priority);
    status |= DPMI_Lock(PCFX_CallBackVal);
    status |= DPMI_Lock(PCFX_CallBackFunc);
    status |= DPMI_Lock(PCFX_TotalVolume);
    status |= DPMI_Lock(PCFX_ServiceTask);
    status |= DPMI_Lock(PCFX_VoiceHandle);
    status |= DPMI_Lock(PCFX_Installed);
    status |= DPMI_Lock(PCFX_ErrorCode);

    if (status != DPMI_Ok)
    {
        PCFX_UnlockMemory();
        PCFX_SetErrorCode(PCFX_DPMI_Error);
        return (PCFX_Error);
    }

    return (PCFX_Ok);
}
