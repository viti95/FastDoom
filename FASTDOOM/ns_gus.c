#include <conio.h>
#include <dos.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "ns_usrho.h"
#include "ns_inter.h"
#include "ns_gf1.h"
#include "ns_gusmi.h"
#include "ns_gusau.h"
#include "ns_gusdf.h"

// size of DMA buffer for patch loading
#define DMABUFFSIZE 2048U

struct gf1_dma_buff GUS_HoldBuffer;
static int HoldBufferAllocated = FALSE;

static int GUS_Installed = 0;

extern VoiceNode GUSWAVE_Voices[VOICES];
extern int GUSWAVE_Installed;

unsigned long GUS_TotalMemory;
int GUS_MemConfig;

/*---------------------------------------------------------------------
   Function: D32DosMemAlloc

   Allocate a block of Conventional memory.
---------------------------------------------------------------------*/

void *D32DosMemAlloc(
    unsigned size)

{
    union REGS r;

    // DPMI allocate DOS memory
    r.x.ax = 0x0100;

    // Number of paragraphs requested
    r.x.bx = (size + 15) >> 4;
    int386(0x31, &r, &r);
    if (r.x.cflag)
    {
        // Failed
        return (NULL);
    }

    return ((void *)((r.x.ax & 0xFFFF) << 4));
}

/*---------------------------------------------------------------------
   Function: GUS_Init

   Initializes the Gravis Ultrasound for sound and music playback.
---------------------------------------------------------------------*/

int GUS_Init(
    void)

{
    struct load_os os;
    int ret;

    if (GUS_Installed > 0)
    {
        GUS_Installed++;
        return (GUS_Ok);
    }

    GUS_Installed = 0;

    //GetUltraCfg(&os);

    if (os.forced_gf1_irq > 7)
    {
        return (GUS_Error);
    }

    if (!HoldBufferAllocated)
    {
        GUS_HoldBuffer.vptr = D32DosMemAlloc(DMABUFFSIZE);
        if (GUS_HoldBuffer.vptr == NULL)
        {
            return (GUS_Error);
        }
        GUS_HoldBuffer.paddr = (unsigned long)GUS_HoldBuffer.vptr;

        HoldBufferAllocated = TRUE;
    }

    os.voices = 24;
    //ret = gf1_load_os(&os);
    if (ret)
    {
        return (GUS_Error);
    }

    //GUS_TotalMemory = gf1_mem_avail();
    GUS_MemConfig = (GUS_TotalMemory - 1) >> 18;

    GUS_Installed = 1;
    return (GUS_Ok);
}

/*---------------------------------------------------------------------
   Function: GUS_Shutdown

   Ends use of the Gravis Ultrasound.  Must be called the same number
   of times as GUS_Init.
---------------------------------------------------------------------*/

void GUS_Shutdown(
    void)

{
    if (GUS_Installed > 0)
    {
        GUS_Installed--;
        if (GUS_Installed == 0)
        {
            //gf1_unload_os();
        }
    }
}

/*---------------------------------------------------------------------
   Function: GUSWAVE_Shutdown

   Terminates use of the Gravis Ultrasound for digitized sound playback.
---------------------------------------------------------------------*/

void GUSWAVE_Shutdown(
    void)

{
    int i;

    if (GUSWAVE_Installed)
    {
        GUSWAVE_KillAllVoices();

        // free memory
        for (i = 0; i < VOICES; i++)
        {
            if (GUSWAVE_Voices[i].mem != 0) // Compare to NULL
            {
                //gf1_free(GUSWAVE_Voices[i].mem);
                GUSWAVE_Voices[i].mem = 0;
            }
        }

        GUS_Shutdown();
        GUSWAVE_Installed = FALSE;
    }
}
