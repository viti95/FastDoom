#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include "ns_dpmi.h"
#include "ns_sb.h"
#include "ns_ctawe.h"
#include "ns_awe32.h"
#include "options.h"
#include "doomstat.h"
#include "z_zone.h"
#include "i_system.h"

/*  DSP defines  */
#define MPU_ACK_OK 0xfe
#define MPU_RESET_CMD 0xff
#define MPU_ENTER_UART 0x3f

static int wSBCBaseAddx; /* Sound Blaster base address */
static int wEMUBaseAddx; /* EMU8000 subsystem base address */

#define TOTALNOTEFLAGS 128

unsigned short *NoteFlags;

static SOUND_PACKET spSound =
    {
        0};

static char* pPresets[MAXBANKS]    = {0};

static LONG lBankSizes[MAXBANKS];

char *Packet;

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

void AWE32_NoteOff(
    int channel,
    int key,
    int velocity)

{
    unsigned temp;

    temp = SetES();
    awe32NoteOff(channel, key, velocity);
    RestoreES(temp);
    NoteFlags[key] ^= (1 << channel);
}

void AWE32_NoteOn(
    int channel,
    int key,
    int velocity)

{
    unsigned temp;

    temp = SetES();
    awe32NoteOn(channel, key, velocity);
    RestoreES(temp);
    NoteFlags[key] |= (1 << channel);
}

void AWE32_PolyAftertouch(
    int channel,
    int key,
    int pressure)

{
    unsigned temp;

    temp = SetES();
    awe32PolyKeyPressure(channel, key, pressure);
    RestoreES(temp);
}

void AWE32_ChannelAftertouch(
    int channel,
    int pressure)

{
    unsigned temp;

    temp = SetES();
    awe32ChannelPressure(channel, pressure);
    RestoreES(temp);
}

void AWE32_ControlChange(
    int channel,
    int number,
    int value)

{
    unsigned temp;
    int i;
    unsigned channelmask;

    temp = SetES();

    if (number == 0x7b)
    {
        channelmask = 1 << channel;
        for (i = 0; i < 128; i++)
        {
            if (NoteFlags[i] & channelmask)
            {
                awe32NoteOff(channel, i, 0);
                NoteFlags[i] ^= channelmask;
            }
        }
    }
    else
    {
        awe32Controller(channel, number, value);
    }
    RestoreES(temp);
}

void AWE32_ProgramChange(
    int channel,
    int program)

{
    unsigned temp;

    temp = SetES();
    awe32ProgramChange(channel, program);
    RestoreES(temp);
}

void AWE32_PitchBend(
    int channel,
    int lsb,
    int msb)

{
    unsigned temp;

    temp = SetES();
    awe32PitchBend(channel, lsb, msb);
    RestoreES(temp);
}

static void LoadSBK(void)
{
    int i;
    FILE* fp;
    Packet = Z_MallocUnowned(PACKETSIZE, PU_STATIC);

    fp = fopen(sbkfile, "rb");
    if (!fp) {
            I_Error(29, sbkfile);

    					/* use embeded preset objects */
    		spSound.bank_no = 0;     /* load as Bank 0 */
    		spSound.total_banks = 1; /* use 1 bank first */
    		lBankSizes[0] = 0;       /* ram is not needed */

    		spSound.banksizes = lBankSizes;
    		awe32DefineBankSizes(&spSound);
    		awe32SoundPad.SPad1 = awe32SPad1Obj;
    		awe32SoundPad.SPad2 = awe32SPad2Obj;
    		awe32SoundPad.SPad3 = awe32SPad3Obj;
    		awe32SoundPad.SPad4 = awe32SPad4Obj;
    		awe32SoundPad.SPad5 = awe32SPad5Obj;
    		awe32SoundPad.SPad6 = awe32SPad6Obj;
    		awe32SoundPad.SPad7 = awe32SPad7Obj;

        	return ;
    }
    
    printf("Loading SoundFont: %s\n", sbkfile);

    /* allocate ram */
    spSound.bank_no = 0;                        /* load as Bank 0 */
    spSound.total_banks = 1;                    /* use 1 bank first */
    lBankSizes[0] = spSound.total_patch_ram;    /* use all available ram */
    spSound.banksizes = lBankSizes;
    awe32DefineBankSizes(&spSound);

    /* request to load */
    SetDWords(Packet, 0, PACKETSIZE / 4);

    spSound.data = Packet;
    fread(Packet, 1, PACKETSIZE, fp);
    if (awe32SFontLoadRequest(&spSound)) {
        fclose(fp);
        I_Error(29, sbkfile);
    }
    
    /* stream samples */
    fseek(fp, spSound.sample_seek, SEEK_SET);
    for (i=0; i<spSound.no_sample_packets; i++) {
        fread(Packet, 1, PACKETSIZE, fp);
        awe32StreamSample(&spSound);
    }
    
    /* setup SoundFont preset objects */
    fseek(fp, spSound.preset_seek, SEEK_SET);
    pPresets[0] = (char*) Z_MallocUnowned((unsigned) spSound.preset_read_size, PU_STATIC);
    fread(pPresets[0], 1, (unsigned) spSound.preset_read_size, fp);
    spSound.presets = pPresets[0];
    if (awe32SetPresets(&spSound)) {
        fclose(fp);
        I_Error(30, sbkfile);
    }
    fclose(fp);
    
    /* calculate actual ram used */
    if (spSound.no_sample_packets) {
        lBankSizes[0] = spSound.preset_seek - spSound.sample_seek + 160;
        spSound.total_patch_ram -= lBankSizes[0];
    }
    else
        lBankSizes[0] = 0;          /* no sample in SBK file */

    Z_Free(Packet);

    return;
}

int AWE32_Init(void)
{
    int status;
    BLASTER_CONFIG Blaster;

    wSBCBaseAddx = 0x220;
    wEMUBaseAddx = 0x620;

    SetDWords(lBankSizes, 0, sizeof(lBankSizes) / 4);

    status = BLASTER_GetCardSettings(&Blaster);
    if (status != BLASTER_Ok)
    {
        status = BLASTER_GetEnv(&Blaster);
        if (status != BLASTER_Ok)
        {
            return (AWE32_Error);
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
        return (AWE32_Error);
    }

    status = awe32InitHardware();
    if (status)
    {
        return (AWE32_Error);
    }

    status = awe32InitMIDI();
    if (status)
    {
        AWE32_Shutdown();
        return (AWE32_Error);
    }

    if (status != DPMI_Ok)
    {
        awe32Terminate();
        return (AWE32_Error);
    }

    // Tronix: don't needed for SBK. Voices must be set to 30 by default
    // Set the number of voices to use to 32
    //awe32NumG = 32;

    awe32TotalPatchRam(&spSound);
   
    LoadSBK();
    awe32InitMIDI();
    awe32InitNRPN();

    NoteFlags = Z_MallocUnowned(TOTALNOTEFLAGS * 2, PU_STATIC);
    SetDWords(NoteFlags, 0, (TOTALNOTEFLAGS * 2) / 4);
    
    return (AWE32_Ok);
}

void AWE32_Shutdown(
    void)

{
    int i;

    /* free allocated memory */
    awe32ReleaseAllBanks(&spSound);
    for (i=0; i<spSound.total_banks; i++)
        if (pPresets[i]) Z_Free(pPresets[i]);

    awe32Terminate();

    Z_Free(NoteFlags);
}
