//
// Copyright (C) 2015-2017 Alexey Khokholov (Nuke.YKT)
// Copyright (C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dmx.h"
#include "doomdef.h"
#include "ns_fxm.h"
#include "ns_music.h"
#include "ns_task.h"
#include "ns_multi.h"
#include "mus2mid.h"
#include "ns_pcfx.h"
#include "doomstat.h"
#include "ns_scape.h"
#include "ns_sb.h"
#include "ns_sbmus.h"
#include "ns_muldf.h"
#include "i_sound.h"
#include "m_misc.h"
#include "options.h"

typedef struct
{
    unsigned int length;
    unsigned short priority;
    unsigned short data[256];
} pcspkmuse_t;

typedef struct
{
    unsigned short id;
    unsigned short length;
    unsigned char data[];
} dmxpcs_t;

pcspkmuse_t pcspkmuse;
int pcshandle = 0;

fx_blaster_config dmx_blaster;

void *mus_data = NULL;
char *mid_data = NULL;

int mus_loop = 0;
int dmx_mus_port = 0;
int dmx_snd_port = 0;

int MUS_RegisterSong(void *data)
{
    FILE *mus;
    FILE *mid;
    unsigned int midlen;
    unsigned short len;
    mus_data = NULL;
    len = ((unsigned short *)data)[2] + ((unsigned short *)data)[3];
    if (mid_data)
    {
        free(mid_data);
    }
    if (memcmp(data, "MThd", 4))
    {
        mus = fopen("temp.mus", "wb");
        if (!mus)
        {
            return 0;
        }
        fwrite(data, 1, len, mus);
        fclose(mus);
        mus = fopen("temp.mus", "rb");
        if (!mus)
        {
            return 0;
        }
        mid = fopen("temp.mid", "wb");
        if (!mid)
        {
            fclose(mus);
            return 0;
        }
        if (mus2mid(mus, mid))
        {
            fclose(mid);
            fclose(mus);
            return 0;
        }
        fclose(mid);
        fclose(mus);
        mid = fopen("temp.mid", "rb");
        if (!mid)
        {
            return 0;
        }
        fseek(mid, 0, SEEK_END);
        midlen = ftell(mid);
        rewind(mid);
        mid_data = malloc(midlen);
        if (!mid_data)
        {
            fclose(mid);
            return 0;
        }
        fread(mid_data, 1, midlen, mid);
        fclose(mid);
        mus_data = mid_data;
        remove("temp.mid");
        remove("temp.mus");
        return 0;
    }
    mus_data = data;
    return 0;
}
int MUS_ChainSong(int handle, int next)
{
    mus_loop = (next == handle);
    return 0;
}

void MUS_PlaySong(int handle, int volume)
{
    long status;
    if (mus_data == NULL)
    {
        return;
    }
    MUSIC_PlaySong((unsigned char *)mus_data, mus_loop);
    MUSIC_SetVolume(volume);
}

int SFX_PlayPatch(void *vdata, int sep, int vol)
{
    const unsigned short divisors[] = {
        0, 6818, 6628, 6449, 6279, 6087, 5906, 5736, 5575, 5423, 5279, 5120, 4971, 4830, 4697, 4554, 4435, 4307, 4186, 4058, 3950,3836,
        3728, 3615, 3519, 3418, 3323, 3224, 3131, 3043, 2960, 2875, 2794, 2711, 2633, 2560, 2485, 2415,
        2348, 2281, 2213, 2153, 2089, 2032, 1975, 1918, 1864, 1810, 1757, 1709, 1659, 1612, 1565, 1521,
        1478, 1435, 1395, 1355, 1316, 1280, 1242, 1207, 1173, 1140, 1107, 1075, 1045, 1015, 986, 959,
        931, 905, 879,854, 829, 806, 783, 760, 739, 718, 697, 677, 658, 640,621, 604, 586, 570, 553, 538,
        522, 507, 493, 479, 465, 452, 439, 427, 415, 403, 391, 380, 369, 359, 348, 339, 329, 319, 310, 302, 293,
        285, 276, 269, 261, 253, 246, 239, 232, 226, 219, 213, 207, 201, 195, 190, 184, 179,
    };

    unsigned int rate;
    unsigned long len;
    unsigned char *data = (unsigned char *)vdata;
    unsigned int type = data[0] | (data[1] << 8);
    dmxpcs_t *dmxpcs = (dmxpcs_t *)vdata;
    unsigned short i;
    if (type == 0)
    {
        pcspkmuse.length = dmxpcs->length * 2;
        pcspkmuse.priority = 100;
        for (i = 0; i < dmxpcs->length; i++)
        {
            pcspkmuse.data[i] = divisors[dmxpcs->data[i]];
        }
        pcshandle = PCFX_Play((PCSound *)&pcspkmuse, 100);
        return pcshandle | 0x8000;
    }
    else if (type == 3)
    {
        rate = (data[3] << 8) | data[2];
        len = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];
        if (len <= 48)
        {
            return -1;
        }
        len -= 32;

        return MV_PlayRaw(data + 24, len, rate, vol * 2, Div63((254 - sep) * vol), Div63((sep)*vol), 100);
    }

    return -1;
}
void SFX_StopPatch(int handle)
{
    if (handle & 0x8000)
    {
        PCFX_Stop(handle & 0x7fff);
        return;
    }
    MV_Kill(handle);
}
int SFX_Playing(int handle)
{
    if (handle & 0x8000)
    {
        return PCFX_SoundPlaying(handle & 0x7fff);
    }
    return MV_VoicePlaying(handle);
}
void SFX_SetOrigin(int handle, int sep, int vol)
{
    VoiceNode *voice;

    if (handle & 0x8000)
        return;

    voice = MV_GetVoice(handle);
    if (voice == NULL)
        return;

    MV_SetVoiceVolume(voice, vol * 2, Div63((254 - sep) * vol), Div63((sep)*vol));
}
int GF1_Detect(void)
{
    return 0; //FIXME
}
void GF1_SetMap(void *data, int len)
{
    FILE *ini = fopen("ULTRAMID.INI", "wb");
    if (ini)
    {
        fwrite(data, 1, len, ini);
        fclose(ini);
    }
}

void SB_Detect(void)
{
    FX_GetBlasterSettings(&dmx_blaster);
}

int AL_Detect(void)
{
    return !AL_DetectFM();
}

void AL_SetCard(void *data)
{
    unsigned char *cdata;
    unsigned char *tmb;
    int i;
    cdata = (unsigned char *)data;
    tmb = malloc(13 * 256);
    if (!tmb)
    {
        return;
    }
    SetDWords(tmb, 0, 13 * 64);
    for (i = 0; i < 128; i++)
    {
        tmb[i * 13 + 0] = cdata[8 + i * 36 + 4 + 0];
        tmb[i * 13 + 1] = cdata[8 + i * 36 + 4 + 7];
        tmb[i * 13 + 2] = cdata[8 + i * 36 + 4 + 4] | cdata[8 + i * 36 + 4 + 5];
        tmb[i * 13 + 3] = cdata[8 + i * 36 + 4 + 11] & 192;
        tmb[i * 13 + 4] = cdata[8 + i * 36 + 4 + 1];
        tmb[i * 13 + 5] = cdata[8 + i * 36 + 4 + 8];
        tmb[i * 13 + 6] = cdata[8 + i * 36 + 4 + 2];
        tmb[i * 13 + 7] = cdata[8 + i * 36 + 4 + 9];
        tmb[i * 13 + 8] = cdata[8 + i * 36 + 4 + 3];
        tmb[i * 13 + 9] = cdata[8 + i * 36 + 4 + 10];
        tmb[i * 13 + 10] = cdata[8 + i * 36 + 4 + 6];
        tmb[i * 13 + 11] = cdata[8 + i * 36 + 4 + 14] + 12;
        tmb[i * 13 + 12] = 0;
    }
    for (i = 128; i < 175; i++)
    {
        tmb[(i + 35) * 13 + 0] = cdata[8 + i * 36 + 4 + 0];
        tmb[(i + 35) * 13 + 1] = cdata[8 + i * 36 + 4 + 7];
        tmb[(i + 35) * 13 + 2] = cdata[8 + i * 36 + 4 + 4] | cdata[8 + i * 36 + 4 + 5];
        tmb[(i + 35) * 13 + 3] = cdata[8 + i * 36 + 4 + 11] & 192;
        tmb[(i + 35) * 13 + 4] = cdata[8 + i * 36 + 4 + 1];
        tmb[(i + 35) * 13 + 5] = cdata[8 + i * 36 + 4 + 8];
        tmb[(i + 35) * 13 + 6] = cdata[8 + i * 36 + 4 + 2];
        tmb[(i + 35) * 13 + 7] = cdata[8 + i * 36 + 4 + 9];
        tmb[(i + 35) * 13 + 8] = cdata[8 + i * 36 + 4 + 3];
        tmb[(i + 35) * 13 + 9] = cdata[8 + i * 36 + 4 + 10];
        tmb[(i + 35) * 13 + 10] = cdata[8 + i * 36 + 4 + 6];
        tmb[(i + 35) * 13 + 11] = cdata[8 + i * 36 + 3] + cdata[8 + i * 36 + 4 + 14] + 12;
        tmb[(i + 35) * 13 + 12] = 0;
    }
    AL_RegisterTimbreBank(tmb);
    free(tmb);
}
int MPU_Detect(int *port)
{
    if (port == NULL)
    {
        return -1;
    }
    return MPU_Init(*port);
}
void MPU_SetCard(int port)
{
    dmx_mus_port = port;
}

void SND_SetPort(int port)
{
    dmx_snd_port = port;
}

int ASS_GetSoundCardCode(int sndDevice)
{
    switch (sndDevice)
    {
    case snd_PC:
        return PC;
    case snd_Adlib:
        return Adlib;
    case snd_SB:
        return SoundBlaster;
    case snd_PAS:
        return ProAudioSpectrum;
    case snd_GUS:
        return UltraSound;
    case snd_MPU:
        return GenMidi;
    case snd_AWE:
        return Awe32;
    case snd_ENSONIQ:
        return SoundScape;
    case snd_CODEC:
        return -1;
    case snd_DISNEY:
        return SoundSource;
    case snd_TANDY:
        return TandySoundSource;
    case snd_PC1BIT:
        return PC1bit;
    case snd_PCPWM:
        return PCPWM;
    case snd_CMS:
        return CMS;
    case snd_LPTDAC:
        return LPTDAC;
    case snd_SBDirect:
        return SoundBlasterDirect;
    case snd_AdlibFX:
        return AdlibFX;
    case snd_AC97:
    	return AC97;
    default:
        return -1;
    }
}

void ASS_Init(int rate, int maxsng, int mdev, int sdev)
{
    int status, music_device, sound_device;
    int sample_rate;

    int SbMaxVoices;
    int SbMaxBits;
    int SbMaxChannels;

    fx_device fx_device;

    status = 0;

    music_device = ASS_GetSoundCardCode(mdev);
    sound_device = ASS_GetSoundCardCode(sdev);

    status = MUSIC_Init(music_device, dmx_mus_port);
    if (status == MUSIC_Ok)
    {
        MUSIC_SetVolume(0);
    }

    switch (sound_device)
    {
    case PC:
        PCFX_Init();
        PCFX_SetTotalVolume(255);
        return;
    case SoundBlaster:
    case Awe32:
    case SoundBlasterDirect:
        FX_SetupSoundBlaster(dmx_blaster, (int *)&SbMaxVoices, (int *)&SbMaxBits, (int *)&SbMaxChannels);
        printf("Sound Blaster DSP %01X.%02X\n", BLASTER_Version >> 8, BLASTER_Version && 7);
        printf("ADDR: %03X, IRQ: %u, DMA LOW: %u, DMA HIGH: %u\n", BLASTER_Config.Address, BLASTER_Config.Interrupt, BLASTER_Config.Dma8, BLASTER_Config.Dma16);
        break;
    default:
        break;
    }

    sample_rate = lowSound ? 8000 : 11025;

    status = FX_Init(sound_device, numChannels, 2, 8, sample_rate);

    FX_SetVolume(255);

    if (reverseStereo)
        MV_SetReverseStereo(true);
}

void ASS_DeInit(void)
{
    MUSIC_Shutdown();
    FX_Shutdown();
    PCFX_Shutdown();
    remove("ULTRAMID.INI");
    if (mid_data)
    {
        free(mid_data);
    }
}

int ENS_Detect(void)
{
    return SOUNDSCAPE_FindCard() != 0;
}
