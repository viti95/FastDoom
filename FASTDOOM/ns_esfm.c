/*---------------------------------------------------------------------
   ns_esfm.c

   Native-mode driver for the ESFM synthesizer on ESS AudioDrive cards.

   ESFM is register-compatible with the OPL3 until "native mode" is
   enabled, after which it exposes 18 independent four-operator channels
   (72 operators) with per-operator frequency, level and modulation.
   This driver programs the chip natively and plays a General MIDI patch
   set stored as raw operator-register blocks (see esfmbank_data.h).

   Register layout and the native programming model follow the
   reverse-engineered documentation in ESFM_specs.md (repo root).  The
   melodic/percussion instrument logic mirrors the behaviour of the ESS
   reference MIDI driver, reimplemented here in the FastDOOM idiom.
---------------------------------------------------------------------*/

#include <dos.h>
#include "stdio.h"
#include "ns_esfm.h"
#include "esfmbank_data.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

#define NUM_ESFM_VOICES 18
#define ESFM_OPS 4
#define NUM_MIDI_CHANNELS 16
#define VOICE_NONE 255

typedef struct
{
    unsigned char bChannel;
    unsigned char bNote;
    unsigned char bVelocity; /* packed 2-bit relative-velocity per operator */
    unsigned char bPatch;    /* patch header byte 0 (op mode + fixed-pitch) */
    unsigned char flags1;    /* bit0 active, bit1 released, bit2 sustained, bit3 second voice */
    unsigned short wTime;    /* allocation timestamp for LRU stealing */
    unsigned char reg1[ESFM_OPS];  /* cached level register (0x40) per op */
    unsigned char reg5[ESFM_OPS];  /* cached block/delay register (0x05) per op */
    unsigned short detune[ESFM_OPS];
} voiceStruct;

int ESFM_PORT = 0x388;

static const unsigned char *gBankMem = ESFM_DefaultBank;

/* Linear velocity -> logarithmic attenuation (.75 dB steps). */
static const unsigned char gbVelocityAtten[32] = {
    40, 36, 32, 28, 23, 21, 19, 17,
    15, 14, 13, 12, 11, 10, 9, 8,
    7, 6, 5, 5, 4, 4, 3, 3,
    2, 2, 1, 1, 1, 0, 0, 0};

static const unsigned char pmask_MidiPitchBend[ESFM_OPS] = {0x10, 0x20, 0x40, 0x80};

static const unsigned short NATV_table1[64] = {
    1024, 1025, 1026, 1027, 1028, 1029, 1030, 1030, 1031, 1032,
    1033, 1034, 1035, 1036, 1037, 1038, 1039, 1040, 1041, 1042,
    1043, 1044, 1045, 1045, 1046, 1047, 1048, 1049, 1050, 1051,
    1052, 1053, 1054, 1055, 1056, 1057, 1058, 1059, 1060, 1061,
    1062, 1063, 1064, 1065, 1065, 1066, 1067, 1068, 1069, 1070,
    1071, 1072, 1073, 1074, 1075, 1076, 1077, 1078, 1079, 1080,
    1081, 1082, 1083, 1084};

static const unsigned short NATV_table2[49] = {
    256, 271, 287, 304, 323, 342, 362, 384, 406, 431,
    456, 483, 512, 542, 575, 609, 645, 683, 724, 767,
    813, 861, 912, 967, 1024, 1085, 1149, 1218, 1290, 1367,
    1448, 1534, 1625, 1722, 1825, 1933, 2048, 2170, 2299, 2435,
    2580, 2734, 2896, 3069, 3251, 3444, 3649, 3866, 4096};

static const int td_adjust_setup_operator[12] = {
    256, 242, 228, 215, 203, 192,
    181, 171, 161, 152, 144, 136};

static const short fnum[12] = {
    514, 544, 577, 611,  /* G , G#, A , A# */
    647, 686, 727, 770,  /* B , C , C#, D  */
    816, 864, 916, 970}; /* D#, E , F , F# */

/* per-MIDI-channel state */
static unsigned char pan_mask[NUM_MIDI_CHANNELS];
static unsigned char gbChanAtten[NUM_MIDI_CHANNELS];
static short giBend[NUM_MIDI_CHANNELS];
static unsigned char hold_table[NUM_MIDI_CHANNELS];
static unsigned char gbChanVolume[NUM_MIDI_CHANNELS];
static unsigned char program_table[NUM_MIDI_CHANNELS];
static unsigned char gbChanExpr[NUM_MIDI_CHANNELS];
static unsigned char gbChanBendRange[NUM_MIDI_CHANNELS];

static voiceStruct voice_table[NUM_ESFM_VOICES];
static unsigned short gwTimer;
static int voice1, voice2;

static void ESFM_delay(int count)
{
    for (; count > 0; count--)
        inp(0xE1);
}

/*---------------------------------------------------------------------
   Function: ESFM_fmwrite

   Writes a value to a native ESFM register through the 16-bit index
   ports (index low -> base+2, index high -> base+3, data -> base+1).
---------------------------------------------------------------------*/

static void ESFM_fmwrite(unsigned index, unsigned char data)
{
    int delay;

    outp(ESFM_PORT + 2, index & 0xff);
    ESFM_delay(2);
    outp(ESFM_PORT + 3, index >> 8);
    ESFM_delay(2);
    outp(ESFM_PORT + 1, data);
    ESFM_delay(2);
}

/*---------------------------------------------------------------------
   Function: MidiCalcFAndB

   Converts a pitch value into an F-number / block pair.  The high byte
   of the result carries the block and the top two F-number bits, the
   low byte the remaining F-number bits.
---------------------------------------------------------------------*/

static unsigned MidiCalcFAndB(unsigned wPitch, unsigned char bBlock)
{
    for (; wPitch >= 0x400; wPitch >>= 1, bBlock++)
        ;

    if (bBlock > 0x07)
        bBlock = 0x07;

    return ((unsigned)bBlock << 10) | wPitch;
}

/*---------------------------------------------------------------------
   Function: NATV_CalcBend

   Applies a channel pitch bend (0x2000 = centre) to a detune value.
---------------------------------------------------------------------*/

static short NATV_CalcBend(unsigned detune, unsigned iBend, unsigned iBendRange)
{
    int v5;

    if (iBend == 0x2000)
        return detune;

    if (iBend >= 0x3F80)
        iBend = 0x4000;

    v5 = ((iBendRange * (((int)iBend - 0x2000))) >> 5) + 0x1800;

    return (detune * (unsigned)((NATV_table1[(v5 >> 2) & 0x3F] * NATV_table2[v5 >> 8]) >> 10) + 512) >> 10;
}

/*---------------------------------------------------------------------
   Function: NATV_CalcVolume

   Combines channel volume, expression and per-operator relative
   velocity into an operator level register value (0x40).
---------------------------------------------------------------------*/

static unsigned char NATV_CalcVolume(unsigned char reg1, unsigned char bVelocity, unsigned char bChannel)
{
    unsigned char vol;

    if (!gbChanVolume[bChannel])
        return 63;

    switch (bVelocity)
    {
    case 1:
        vol = ((127 - gbChanExpr[bChannel]) >> 4) + ((127 - gbChanVolume[bChannel]) >> 4);
        break;
    case 2:
        vol = ((127 - gbChanExpr[bChannel]) >> 3) + ((127 - gbChanVolume[bChannel]) >> 3);
        break;
    case 3:
        vol = gbChanVolume[bChannel];
        if (vol < 64)
            vol = ((63 - vol) >> 1) + 16;
        else
            vol = (127 - vol) >> 2;
        if (gbChanExpr[bChannel] < 64)
            vol += ((63 - gbChanExpr[bChannel]) >> 1) + 16;
        else
            vol += ((127 - gbChanExpr[bChannel]) >> 2);
        break;
    case 0:
    default:
        vol = 0;
        break;
    }

    vol += (reg1 & 0x3F);
    if (vol > 63)
        vol = 63;
    return vol | (reg1 & 0xC0);
}

/*---------------------------------------------------------------------
   Function: NATV_CalcNewVolume

   Rewrites operator levels for every active voice on a channel (or all
   channels when bChannel == 0xFF).
---------------------------------------------------------------------*/

static void NATV_CalcNewVolume(unsigned char bChannel)
{
    int i, j;

    for (i = 0; i < NUM_ESFM_VOICES; i++)
    {
        voiceStruct *voice = &voice_table[i];
        if ((voice->flags1 & 1) && (voice->bChannel == bChannel || bChannel == 0xFF))
        {
            for (j = 0; j < ESFM_OPS; j++)
                ESFM_fmwrite(i * 32 + j * 8 + 1,
                             NATV_CalcVolume(voice->reg1[j], (voice->bVelocity >> (j * 2)) & 3, voice->bChannel));
        }
    }
}

/*---------------------------------------------------------------------
   Function: voice_on / voice_off

   Trigger / release a physical channel's key-on registers.  Channels 16
   and 17 have two key-on registers each (operator pairs).
---------------------------------------------------------------------*/

static void voice_on(int voiceNr)
{
    if (voiceNr == 16)
    {
        ESFM_fmwrite(0x250, 1);
        ESFM_fmwrite(0x251, 1);
    }
    else if (voiceNr == 17)
    {
        ESFM_fmwrite(0x252, 1);
        ESFM_fmwrite(0x253, 1);
    }
    else
    {
        ESFM_fmwrite((unsigned)voiceNr + 0x240, 1);
    }
}

static void voice_off(int voiceNr)
{
    if (voiceNr == 16)
    {
        ESFM_fmwrite(0x250, 0);
        ESFM_fmwrite(0x251, 0);
    }
    else if (voiceNr == 17)
    {
        ESFM_fmwrite(0x252, 0);
        ESFM_fmwrite(0x253, 0);
    }
    else
    {
        ESFM_fmwrite((unsigned)voiceNr + 0x240, 0);
    }

    voice_table[voiceNr].flags1 = 2;
    voice_table[voiceNr].wTime = gwTimer;
    gwTimer++;
}

/*---------------------------------------------------------------------
   Function: find_voice

   Locates up to two free voices (voice1/voice2) using an LRU policy.
   Voices 16 and 17 are only used when the patch permits it.
---------------------------------------------------------------------*/

static void find_voice(int allow1617_v1, int allow1617_v2, unsigned char bChannel, unsigned char bNote)
{
    int i;
    unsigned short td, timediff1 = 0, timediff2 = 0;

    voice1 = voice2 = VOICE_NONE;

    for (i = 0; i < 16; i++)
    {
        voiceStruct *voice = &voice_table[i];
        if (voice->flags1 & 1)
        {
            if (voice->bChannel == bChannel && voice->bNote == bNote)
                voice_off(i);
        }
        else
        {
            td = gwTimer - voice->wTime;
            if (td < timediff1)
            {
                if (td >= timediff2)
                {
                    timediff2 = td;
                    voice2 = i;
                }
            }
            else
            {
                timediff2 = timediff1;
                voice2 = voice1;
                timediff1 = td;
                voice1 = i;
            }
        }
    }

    /* voice 16 */
    if (voice_table[16].flags1 & 1)
    {
        if (voice_table[16].bChannel == bChannel && voice_table[16].bNote == bNote)
            voice_off(16);
    }
    else
    {
        td = gwTimer - voice_table[16].wTime;
        if (allow1617_v1 || td < timediff1)
        {
            if (!allow1617_v2 && td >= timediff2)
            {
                timediff2 = gwTimer - voice_table[16].wTime;
                voice2 = 16;
            }
        }
        else
        {
            timediff2 = timediff1;
            voice2 = voice1;
            timediff1 = td;
            voice1 = 16;
        }
    }

    /* voice 17 */
    if (voice_table[17].flags1 & 1)
    {
        if (voice_table[17].bChannel == bChannel && voice_table[17].bNote == bNote)
            voice_off(17);
    }
    else
    {
        td = gwTimer - voice_table[17].wTime;
        if (allow1617_v1 || td < timediff1)
        {
            if (!allow1617_v2 && td >= timediff2)
                voice2 = 17;
        }
        else
        {
            if (voice1 != 16 || !allow1617_v2)
                voice2 = voice1;
            voice1 = 17;
        }
    }
}

/*---------------------------------------------------------------------
   Function: steal_voice

   Frees the least important active voice and returns its index.
---------------------------------------------------------------------*/

static int steal_voice(int allow1617)
{
    int i, last_voice = 0, max_voices = allow1617 ? 18 : 16;
    unsigned char chn, chncmp = 0, bit3 = 0;
    unsigned short timediff = 0;

    for (i = 0; i < max_voices; i++)
    {
        chn = voice_table[i].bChannel == 9 ? 1 : voice_table[i].bChannel + 2;
        if (bit3 == (voice_table[i].flags1 & 8))
        {
            if (chn <= chncmp)
            {
                if (chn != chncmp || (gwTimer - voice_table[i].wTime) <= timediff)
                    continue;
            }
            else
            {
                chncmp = chn;
            }
        }
        else if (!bit3)
        {
            bit3 = 8;
            chncmp = chn;
        }
        else
            continue;

        timediff = gwTimer - voice_table[i].wTime;
        last_voice = i;
    }

    voice_off(last_voice);
    return last_voice;
}

/*---------------------------------------------------------------------
   Function: setup_operator

   Programs one operator (8 registers) from a patch's operator block.
---------------------------------------------------------------------*/

static void setup_operator(int offset, int bNote, int bVelocity, unsigned reg,
                           int fixed_pitch, int rel_velocity, int bChannel, int oper, int voicenr)
{
    int note, transpose, block, notemod12, reg1, detune;
    unsigned fnum_block;
    unsigned char reg4, reg5, reg6, panmask;

    panmask = pan_mask[bChannel];
    ESFM_fmwrite(reg + 7, 0);

    note = bNote;
    if (!fixed_pitch)
    {
        transpose = (((gBankMem[offset + 5]) << 2) & 0x7F) | (gBankMem[offset + 4] & 3);
        if (gBankMem[offset + 5] & 0x10)
            transpose |= ~0x7F;
        note += transpose;
    }

    if (note < 19)
        note += 12 * (((18 - note) / 12) + 1);
    if (note > 114)
        note -= 12 * (((note - 115) / 12) + 1);
    block = (note - 19) / 12;
    notemod12 = (note - 19) % 12;

    ESFM_fmwrite(reg + 0, gBankMem[offset]);

    switch (rel_velocity)
    {
    case 1:
        reg1 = (127 - bVelocity) >> 4;
        break;
    case 2:
        reg1 = (127 - bVelocity) >> 3;
        break;
    case 3:
        if (bVelocity < 64)
            reg1 = ((63 - bVelocity) >> 1) + 16;
        else
            reg1 = (127 - bVelocity) >> 2;
        break;
    case 0:
    default:
        reg1 = 0;
        break;
    }
    reg1 += (gBankMem[offset + 1] & 0x3F);
    if (reg1 > 63)
        reg1 = 63;
    reg1 += (gBankMem[offset + 1] & 0xC0);
    voice_table[voicenr].reg1[oper] = (unsigned char)reg1;

    ESFM_fmwrite(reg + 1, NATV_CalcVolume((unsigned char)reg1, (unsigned char)rel_velocity, (unsigned char)bChannel));
    ESFM_fmwrite(reg + 2, gBankMem[offset + 2]);
    ESFM_fmwrite(reg + 3, gBankMem[offset + 3]);

    if (fixed_pitch)
    {
        reg4 = gBankMem[offset + 4];
        reg5 = gBankMem[offset + 5];
    }
    else
    {
        detune = ((int)*((signed char *)&gBankMem[offset + 4])) & (~3);
        if (detune)
        {
            detune = ((detune >> 2) * td_adjust_setup_operator[notemod12]) >> 8;
            if (block > 1)
                detune >>= block - 1;
        }
        detune += fnum[notemod12];
        voice_table[voicenr].reg5[oper] =
            (unsigned char)(((detune >> 8) & 3) | (gBankMem[offset + 5] & 0xE0) | (block << 2));
        fnum_block = MidiCalcFAndB(NATV_CalcBend((unsigned)detune, giBend[bChannel], gbChanBendRange[bChannel]),
                                   (unsigned char)block);
        reg4 = fnum_block & 0xFF;
        reg5 = (fnum_block >> 8) | (voice_table[voicenr].reg5[oper] & 0xE0);
        voice_table[voicenr].detune[oper] = (unsigned short)detune;
    }

    reg6 = gBankMem[offset + 6];
    if ((reg6 & 0x30) && panmask != 0x30)
        reg6 = panmask | (reg6 & 0xCF);
    ESFM_fmwrite(reg + 4, reg4);
    ESFM_fmwrite(reg + 5, reg5);
    ESFM_fmwrite(reg + 6, reg6);
    ESFM_fmwrite(reg + 7, gBankMem[offset + 7]);
}

/*---------------------------------------------------------------------
   Function: setup_voice

   Programs all four operators of a physical voice from a patch.
---------------------------------------------------------------------*/

static void setup_voice(int voicenr, int offset, int bChannel, int bNote, int bVelocity)
{
    unsigned char rel_vel, bPatch;

    bPatch = gBankMem[offset];
    rel_vel = gBankMem[offset + 3];
    offset += 4;

    setup_operator(offset + 0, bNote, bVelocity, 32 * voicenr + 0, bPatch & 0x10, (rel_vel >> 0) & 3, bChannel, 0, voicenr);
    setup_operator(offset + 8, bNote, bVelocity, 32 * voicenr + 8, bPatch & 0x20, (rel_vel >> 2) & 3, bChannel, 1, voicenr);
    setup_operator(offset + 16, bNote, bVelocity, 32 * voicenr + 16, bPatch & 0x40, (rel_vel >> 4) & 3, bChannel, 2, voicenr);
    setup_operator(offset + 24, bNote, bVelocity, 32 * voicenr + 24, bPatch & 0x80, (rel_vel >> 6) & 3, bChannel, 3, voicenr);

    voice_table[voicenr].bPatch = bPatch;
    voice_table[voicenr].bVelocity = rel_vel;
    voice_table[voicenr].wTime = gwTimer;
    voice_table[voicenr].bNote = (unsigned char)bNote;
    voice_table[voicenr].flags1 = 1;
    voice_table[voicenr].bChannel = (unsigned char)bChannel;

    gwTimer++;
}

/*---------------------------------------------------------------------
   Function: note_on / note_off
---------------------------------------------------------------------*/

static void note_on(unsigned char bChannel, unsigned char bNote, unsigned char bVelocity)
{
    int patch, offset, fixed_pitch;
    unsigned char flags_voice1;

    if (bChannel == 9)
        patch = bNote + 128;
    else
        patch = program_table[bChannel];

    offset = gBankMem[2 * patch] | ((int)gBankMem[2 * patch + 1] << 8);
    if (!offset)
        return;

    flags_voice1 = gBankMem[offset];
    fixed_pitch = (flags_voice1 >> 1) & 3;

    switch (fixed_pitch)
    {
    case 0:
        find_voice(flags_voice1 & 1, 0, bChannel, bNote);
        if (voice1 == VOICE_NONE)
            voice1 = steal_voice(gBankMem[offset] & 1);
        setup_voice(voice1, offset, bChannel, bNote, bVelocity);
        voice_on(voice1);
        break;
    case 1:
        find_voice(flags_voice1 & 1, gBankMem[offset + 36] & 1, bChannel, bNote);
        if (voice1 == VOICE_NONE)
            voice1 = steal_voice(gBankMem[offset] & 1);
        setup_voice(voice1, offset, bChannel, bNote, bVelocity);
        if (voice2 != VOICE_NONE)
        {
            setup_voice(voice2, offset + 36, bChannel, bNote, bVelocity);
            voice_table[voice2].flags1 |= 8;
            voice_on(voice2);
        }
        voice_on(voice1);
        break;
    case 2:
        find_voice(flags_voice1 & 1, gBankMem[offset + 36] & 1, bChannel, bNote);
        if (voice1 == VOICE_NONE)
            voice1 = steal_voice(gBankMem[offset] & 1);
        if (voice2 == VOICE_NONE)
            voice2 = steal_voice(gBankMem[offset + 36] & 1);
        setup_voice(voice1, offset, bChannel, bNote, bVelocity);
        setup_voice(voice2, offset + 36, bChannel, bNote, bVelocity);
        voice_on(voice1);
        voice_on(voice2);
        break;
    }
}

static void note_off(unsigned char bChannel, unsigned char bNote)
{
    int i;

    for (i = 0; i < NUM_ESFM_VOICES; i++)
    {
        voiceStruct *voice = &voice_table[i];
        if ((voice->flags1 & 1) && voice->bChannel == bChannel && voice->bNote == bNote)
        {
            if (hold_table[bChannel] & 1)
                voice->flags1 |= 4;
            else
                voice_off(i);
        }
    }
}

static void hold_controller(unsigned char bChannel, unsigned char value)
{
    if (value < 64)
    {
        int i;
        hold_table[bChannel] &= ~1;
        for (i = 0; i < NUM_ESFM_VOICES; i++)
        {
            if ((voice_table[i].flags1 & 4) && voice_table[i].bChannel == bChannel)
                voice_off(i);
        }
    }
    else
    {
        hold_table[bChannel] |= 1;
    }
}

/*---------------------------------------------------------------------
   Function: MidiPitchBend

   Re-programs the F-number/block of every active operator on a channel.
---------------------------------------------------------------------*/

static void MidiPitchBend(unsigned char bChannel, unsigned iBend)
{
    int i, j;
    short bnd;

    giBend[bChannel] = iBend;

    for (i = 0; i < NUM_ESFM_VOICES; i++)
    {
        if (voice_table[i].bChannel == bChannel && (voice_table[i].flags1 & 1))
        {
            for (j = 0; j < ESFM_OPS; j++)
            {
                if (pmask_MidiPitchBend[j] & voice_table[i].bPatch)
                    continue;
                bnd = NATV_CalcBend(voice_table[i].detune[j], iBend, gbChanBendRange[bChannel]);
                bnd = MidiCalcFAndB(bnd, (unsigned char)((voice_table[i].reg5[j] >> 2) & 7));
                ESFM_fmwrite(32 * i + 8 * j + 5, (unsigned char)((bnd >> 8) | (voice_table[i].reg5[j] & 0xE0)));
                ESFM_fmwrite(32 * i + 8 * j + 4, (unsigned char)(bnd & 0xFF));
            }
        }
    }
}

/*---------------------------------------------------------------------
   Function: fmreset

   Resets driver state and silences every voice.
---------------------------------------------------------------------*/

static void fmreset(void)
{
    int i;

    for (i = 0; i < NUM_MIDI_CHANNELS; i++)
    {
        giBend[i] = 0x2000;
        gbChanBendRange[i] = 0x02;
        hold_table[i] = 0x00;
        gbChanExpr[i] = 0x7F;
        gbChanVolume[i] = 0x7F;
        gbChanAtten[i] = 0x04;
        pan_mask[i] = 0x30;
    }

    for (i = 0; i < NUM_ESFM_VOICES; i++)
    {
        voice_off(i);
        voice_table[i].wTime = 0;
        voice_table[i].flags1 = 0;
    }

    gwTimer = 0;
}

void ESFM_SetVolume(int volume)
{
    int i;
    /* Scale 0-255 MIDI master volume to 0-127 per-channel volume. */
    unsigned char vol = (unsigned char)((volume * 127) / 255);
    for (i = 0; i < NUM_MIDI_CHANNELS; i++)
        gbChanVolume[i] = vol;
    NATV_CalcNewVolume(0xFF);
}

/*---------------------------------------------------------------------
   Public MIDI interface (matches midifuncs).
---------------------------------------------------------------------*/

void ESFM_NoteOn(int channel, int key, int velocity)
{
    if (velocity == 0)
        note_off((unsigned char)channel, (unsigned char)key);
    else
        note_on((unsigned char)channel, (unsigned char)key, (unsigned char)velocity);
}

void ESFM_NoteOff(int channel, int key, int velocity)
{
    (void)velocity;
    note_off((unsigned char)channel, (unsigned char)key);
}

void ESFM_ProgramChange(int channel, int program)
{
    program_table[channel] = (unsigned char)program;
}

void ESFM_SetPitchBend(int channel, int lsb, int msb)
{
    MidiPitchBend((unsigned char)channel, (unsigned)lsb | ((unsigned)msb << 7));
}

void ESFM_ControlChange(int channel, int number, int value)
{
    int i;

    switch (number)
    {
    case 6: /* data entry MSB: pitch-bend range when RPN 0 selected */
        if ((hold_table[channel] & 6) == 6)
            gbChanBendRange[channel] = value;
        break;
    case 7: /* channel volume */
        gbChanAtten[channel] = gbVelocityAtten[value >> 1];
        gbChanVolume[channel] = value;
        NATV_CalcNewVolume(channel);
        break;
    case 8:
    case 10: /* pan */
        if (value <= 80)
            pan_mask[channel] = (value >= 48) ? 0x30 : 0x10;
        else
            pan_mask[channel] = 0x20;
        break;
    case 11: /* expression */
        gbChanExpr[channel] = value;
        NATV_CalcNewVolume(channel);
        break;
    case 64: /* sustain */
        hold_controller((unsigned char)channel, (unsigned char)value);
        break;
    case 100: /* RPN LSB */
        if (value == 0)
        {
            hold_table[channel] |= 2;
            break;
        }
        /* fall through */
    case 98:
        hold_table[channel] &= ~2;
        break;
    case 101: /* RPN MSB */
        if (value == 0)
        {
            hold_table[channel] |= 4;
            break;
        }
        /* fall through */
    case 99:
        hold_table[channel] &= ~4;
        break;
    case 120: /* all sound off */
    case 124:
    case 125:
        for (i = 0; i < NUM_ESFM_VOICES; i++)
            if ((voice_table[i].flags1 & 1) && voice_table[i].bChannel == channel)
                voice_off(i);
        break;
    case 121: /* reset all controllers */
        if (hold_table[channel] & 1)
        {
            for (i = 0; i < NUM_ESFM_VOICES; i++)
                if ((voice_table[i].flags1 & 1) && voice_table[i].bChannel == channel && (voice_table[i].flags1 & 4))
                    voice_off(i);
        }
        hold_table[channel] &= ~1u;
        gbChanVolume[channel] = 100;
        gbChanExpr[channel] = 127;
        giBend[channel] = 0x2000;
        pan_mask[channel] = 0x30;
        gbChanBendRange[channel] = 2;
        break;
    case 123: /* all notes off */
    case 126:
    case 127:
        for (i = 0; i < NUM_ESFM_VOICES; i++)
            if ((voice_table[i].flags1 & 1) && voice_table[i].bChannel == channel && (voice_table[i].flags1 & 4) == 0)
                voice_off(i);
        break;
    }
}

/*---------------------------------------------------------------------
   Function: ESFM_DetectFM

   ESS AudioDrive presents an OPL3 before native mode is enabled, so the
   standard Adlib timer-based probe applies.
---------------------------------------------------------------------*/

int ESFM_DetectFM(void)
{
    int ii;
    outp(ESFM_PORT + 2, 0x05);
    ESFM_delay(128);
    outp(ESFM_PORT + 3, 0x05);
    ESFM_delay(1024);
    ii = inp(ESFM_PORT + 1);

    if ((ii & ~1) == 0x80)
    {
        outp(ESFM_PORT, 0);
        return 1;
    }

    outp(ESFM_PORT + 2, 0x05);
    ESFM_delay(128);
    outp(ESFM_PORT + 3, 0x01);
    ESFM_delay(1024);
    outp(ESFM_PORT + 2, 0x05);
    ESFM_delay(128);
    outp(ESFM_PORT + 3, 0x80);
    ESFM_delay(1024);

    outp(ESFM_PORT + 2, 0x05);
    ESFM_delay(128);
    outp(ESFM_PORT + 3, 0x05);
    ESFM_delay(1024);
    ii = inp(ESFM_PORT + 1);

    if (ii == 0x80)
    {
        outp(ESFM_PORT, 0);
        return 1;
    }

    return 0;
}

/*---------------------------------------------------------------------
   Function: ESFM_RegisterBank
---------------------------------------------------------------------*/

void ESFM_RegisterBank(const unsigned char *bank)
{
    gBankMem = bank ? bank : ESFM_DefaultBank;
}

/*---------------------------------------------------------------------
   Function: ESFM_Init

   Enables native ESFM mode and silences the chip.  Address is the SB/FM
   base port; 0 selects the default Adlib port (0x388).
---------------------------------------------------------------------*/

int ESFM_Init(int Address)
{
    int delay;

    ESFM_PORT = Address ? Address : 0x388;

    /* Enter native mode: set bit 7 of OPL3 register 0x105.
       In OPL3 mode base+2 is the high-bank address port, base+3 is its
       data port.  Spec requires a delay between address and data writes. */
    outp(ESFM_PORT + 2, 0x05);
    ESFM_delay(2);
    outp(ESFM_PORT + 3, 0x81);
    ESFM_delay(2);

    fmreset();

    return ESFM_Ok;
}

/*---------------------------------------------------------------------
   Function: ESFM_Shutdown

   Silences all voices and returns the chip to OPL3-compatible mode.
---------------------------------------------------------------------*/

void ESFM_Shutdown(void)
{
    int i;

    for (i = 0; i < NUM_ESFM_VOICES; i++)
        voice_off(i);

    /* Any write to the reset port (base+0) leaves native mode. */
    outp(ESFM_PORT, 0);
}
