#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include "ns_dpmi.h"
#include "ns_sb.h"
#include "ns_ctawe.h"
#include "ns_awe32snd.h"
#include "options.h"
#include "doomstat.h"
#include "z_zone.h"
#include "sounds.h"
#include "w_wad.h"
#include "i_system.h"

/*  DSP defines  */
static int wSBCBaseAddx; /* Sound Blaster base address */
static int wEMUBaseAddx; /* EMU8000 subsystem base address */

static SOUND_PACKET spSound =
    {
        0};

static char *pPresets[MAXBANKS] = {0};

static LONG lBankSizes[MAXBANKS] =
    {
        0};

static char Packet[PACKETSIZE] = {0};

unsigned SetES(void);
#pragma aux SetES = \
    "xor eax, eax"  \
    "mov ax, es"    \
    "mov bx, ds"    \
    "mov es, bx" modify[eax ebx];

void RestoreES(unsigned num);
#pragma aux RestoreES = \
    "mov  es, ax" parm[eax];

/**********************************************************************

   Memory locked functions:

**********************************************************************/

/* SoundFont variables */
WAVE_PACKET wpWave          = {0};

void LoadSamples()
{
    int i;
    int bank;
    long sampsize;

    for (i = 1; i < NUMSFX; i++)
    {
        unsigned int rate;
        unsigned long sampsize;
        unsigned char *data;
        unsigned long total = 0;
        char namebuf[9] = "DS";
        
        strcpy(namebuf+2, S_sfx[i].name);

        data = W_CacheLumpName(namebuf, PU_STATIC);

        rate = (data[3] << 8) | data[2];

        sampsize = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];
        if (sampsize <= 48)
            continue;
        sampsize -= 32;

        data += 24;

        if (sampsize + 160 > spSound.total_patch_ram)
        {
            I_Error("AWE32: Not enough patch ram to load %s. Available: %u, asked: %u", namebuf, spSound.total_patch_ram, sampsize + 160);
        }

        /* allocate patch ram */
        bank = spSound.total_banks;
        lBankSizes[bank] = sampsize * 2;
        lBankSizes[bank] += 160;
        spSound.total_patch_ram -= lBankSizes[bank];
        spSound.total_banks += 1;
        awe32DefineBankSizes(&spSound);

        /* setup WAVE_PACKET */
        wpWave.tag = 0x101;
        wpWave.bank_no = (short)bank;
        wpWave.data = Packet;
        wpWave.sample_size = sampsize;
        wpWave.samples_per_sec = rate;
        wpWave.bits_per_sample = 8;
        wpWave.no_channels = 1;
        wpWave.looping = 0;
        wpWave.startloop = 0;
        wpWave.endloop = wpWave.sample_size;
        wpWave.release = 0;

        /* request to load WAV */
        if (awe32WPLoadRequest(&wpWave))
        {
            I_Error("AWE32: Request to load %s failed\n", namebuf);
        }

        /* stream the raw PCM samples */
        wpWave.data = Packet;
        do
            if (total >= PACKETSIZE)
            {
                memcpy(Packet, data, PACKETSIZE);
                total += PACKETSIZE;
            }else{
                memcpy(Packet, data, total);
            }
                
        while (!awe32WPStreamWave(&wpWave));

        /* build SoundFont preset objects */
        pPresets[bank] = (char *)Z_MallocUnowned(wpWave.preset_size, PU_STATIC);
        wpWave.presets = pPresets[bank];
        if (awe32WPBuildSFont(&wpWave))
        {
            I_Error("AWE32: Cannot build SoundFont preset objects\n");
        }
    }
}

int AWE32SND_Init(void)
{
    int status;
    BLASTER_CONFIG Blaster;

    wSBCBaseAddx = 0x220;
    wEMUBaseAddx = 0x620;

    status = BLASTER_GetCardSettings(&Blaster);
    if (status != BLASTER_Ok)
    {
        status = BLASTER_GetEnv(&Blaster);
        if (status != BLASTER_Ok)
        {
            return (AWE32SND_Error);
        }
    }

    wSBCBaseAddx = Blaster.Address;
    if (wSBCBaseAddx == UNDEFINED)
    {
        wSBCBaseAddx = 0x220;
    }

    wEMUBaseAddx = Blaster.Emu;
    if (wEMUBaseAddx <= 0)
    {
        wEMUBaseAddx = wSBCBaseAddx + 0x400;
    }

    status = awe32Detect(wEMUBaseAddx);
    if (status)
    {
        return (AWE32SND_Error);
    }

    status = awe32InitHardware();
    if (status)
    {
        return (AWE32SND_Error);
    }

    status = awe32InitMIDI();
    if (status)
    {
        AWE32SND_Shutdown();
        return (AWE32SND_Error);
    }

    if (status != DPMI_Ok)
    {
        awe32Terminate();
        return (AWE32SND_Error);
    }

    awe32TotalPatchRam(&spSound);

    LoadSamples();
    awe32InitMIDI();
    awe32InitNRPN();

    // Testing! Play some audios

    awe32Controller(15, 0, (WORD) 1);
    awe32ProgramChange(15, 0);
    awe32NoteOn(15, 60, 127);

    return (AWE32SND_Ok);
}

void AWE32SND_Shutdown(
    void)

{
    int i;

    /* free allocated memory */
    awe32ReleaseAllBanks(&spSound);
    for (i = 0; i < spSound.total_banks; i++)
        if (pPresets[i])
            Z_Free(pPresets[i]);

    awe32Terminate();
}
