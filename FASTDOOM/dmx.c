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
#include "i_system.h"
#include "i_sound.h"
#include "z_zone.h"

#include "i_file.h"

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
        Z_Free(mid_data);
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
        mid_data = Z_MallocUnowned(midlen, PU_STATIC);
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

unsigned int MUS_ChecksumRoland(unsigned char *buf, int cnt)
{
    unsigned int cksum = 0;
    /* Add up all the bytes */
    while (cnt-- > 0)
    {
        cksum += *buf++;
    }
    /* Use only the 7 least significant bits */
    cksum &= 0x7F;
    /* The checksum when added with the bytes over
    ** which it is calculated must result in a zero
    ** sum (in the least significant 7 bits).
    ** If the value calculated so far is non-zero
    ** subtract it from 0x80 to produce the proper
    ** checksum byte (if the value calculated so far
    ** is zero then zero is the proper checksum).
    */
    if (cksum)
    {
        cksum = 0x80 - cksum;
    }
    return cksum;
}

unsigned char *LogoSC55;

void MUS_ImgSC55(void)
{
    LogoSC55 = I_ReadBinaryStatic("DATA\\SC55.BIN", 72);
    MUSIC_SysEx(LogoSC55, 72);
    Z_Free(LogoSC55);
}

unsigned char *LogoTG300;

void MUS_ImgTG300(void)
{
    LogoTG300 = I_ReadBinaryStatic("DATA\\TG300.BIN", 55);
    MUSIC_SysEx(LogoTG300, 55);
    Z_Free(LogoTG300);
}

unsigned char TextSC55[] = {0x41, 0x10, 0x45, 0x12, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void MUS_TextSC55(unsigned char *text, int size)
{
    int i;
    unsigned char *sc55ptr = TextSC55 + 7;

    if (size < 1 || size > 32)
        return;

    // The SC55 supports messages with lenght 16 to 32. It scrolls automatically if more
    // than 16 characters are useds
    for (i = 0; i < size; i++)
    {
        *(sc55ptr) = *(text);
        sc55ptr++;
        text++;
    }

    *(sc55ptr) = MUS_ChecksumRoland(TextSC55 + 4, size+3);

    MUSIC_SysEx(TextSC55, size+8);
}

unsigned char YamahaXG[] = {0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00};

void MUS_YamahaXG(void)
{
    MUSIC_SysEx(YamahaXG, 7);
}

unsigned char TextMU80[] = {0x43, 0x10, 0x4C, 0x06, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void MUS_TextMU80(unsigned char *text, int size)
{
    int i;
    unsigned char *mu80ptr = TextMU80 + 6;

    if (size < 1)
        return;

    if (size > 32) 
    {
        size = 32;
    }
    else
    {
        unsigned char minimumStr[32];
        int i;
        for (i = 0; i < size; i++)
        {
            minimumStr[i] = text[i];
        }
        for (i = size; i < 32; i++)
        {
            minimumStr[i] = ' ';
        }
        text = minimumStr;
        size = 32;
    }

    // The MU80 only supports text messages with 32 characters
    for (i = 0; i < size; i++)
    {
        *(mu80ptr) = *(text);
        mu80ptr++;
        text++;
    }

    MUSIC_SysEx(TextMU80, size + 6);
}

unsigned char TextTG300[] = {0x43, 0x10, 0x2B, 0x07, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
                             0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                             0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                             0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void MUS_TextTG300(unsigned char *text, int size)
{
    int i;
    unsigned char *tg300ptr = TextTG300 + 6;

    if (size < 1)
        return;

    if (size > 32) 
    {
        size = 32;
    }
    else
    {
        unsigned char minimumStr[32];
        int i;
        for (i = 0; i < size; i++)
        {
            minimumStr[i] = text[i];
        }
        for (i = size; i < 32; i++)
        {
            minimumStr[i] = ' ';
        }
        text = minimumStr;
        size = 32;
    }

    // The TG300 only supports text messages with 32 characters
    for (i = 0; i < size; i++)
    {
        *(tg300ptr) = *(text);
        tg300ptr++;
        text++;
    }

    *(tg300ptr) = MUS_ChecksumRoland(TextTG300 + 3, size+3);

    MUSIC_SysEx(TextTG300, size + 7);
}

unsigned char TextMT32[] = {0x41, 0x10, 0x16, 0x12, 0x20, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void MUS_TextMT32(unsigned char *text, int size)
{
    int i;
    unsigned char *mt32ptr = TextMT32 + 7;
    
    if (size < 1)
        return;

    if (size > 20)
    {
        size = 20;
    }
    else if (size < 20) 
    {
        unsigned char minimumStr[20];
        int i;
        for (i = 0; i < size; i++)
        {
            minimumStr[i] = text[i];
        }
        for (i = size; i < 20; i++)
        {
            minimumStr[i] = ' ';
        }
        text = minimumStr;
        size = 20;
    }

    // The MT32 only supports text messages with 20 characters
    for (i = 0; i < size; i++)
    {
        *(mt32ptr) = *(text);
        mt32ptr++;
        text++;
    }

    *(mt32ptr) = MUS_ChecksumRoland(TextMT32 + 4, size + 3);

    MUSIC_SysEx(TextMT32, size + 8);
}

char mt32file[13] = "MT32GM.MID";

int MUS_LoadMT32(void)
{
    FILE *mid;
    unsigned int midlen;

    if (mid_data)
    {
        Z_Free(mid_data);
    }
        
    mid = fopen(mt32file, "rb");
    if (!mid)
    {
        return 0;
    }
    fseek(mid, 0, SEEK_END);
    midlen = ftell(mid);
    rewind(mid);
    mid_data = Z_MallocUnowned(midlen, PU_STATIC);
    if (!mid_data)
    {
        fclose(mid);
        return 0;
    }
    fread(mid_data, 1, midlen, mid);
    fclose(mid);
    mus_data = mid_data;
    return 0;
}

void MUS_ReleaseData(void)
{
    if (!MUSIC_SongPlaying())
    {
        if (mid_data)
        {
            Z_Free(mid_data);
        }
    }
}

int MUS_SongPlaying()
{
    return MUSIC_SongPlaying();
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

unsigned short *divisors;

int SFX_PlayPatch(void *vdata, int sep, int vol)
{
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

void AL_SetCard(void *data)
{
    unsigned char *cdata;
    unsigned char *tmb;
    int i;
    cdata = (unsigned char *)data;
    tmb = Z_MallocUnowned(13 * 256, PU_STATIC);
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
    Z_Free(tmb);
}
int MPU_Detect(int *port)
{
    if (port == NULL)
    {
        return -1;
    }
    return MPU_Init(*port);
}

void SetSNDPort(int port)
{
    dmx_snd_port = port;
}

void SetMUSPort(int port)
{
    dmx_mus_port = port;
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
    case snd_DISNEY:
        return SoundSource;
    case snd_TANDY:
        return Tandy3Voice;
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
    case snd_OPL2LPT:
        return OPL2LPT;
    case snd_OPL3LPT:
        return OPL3LPT;
    case snd_CD:
        return AudioCD;
    case snd_WAV:
        return FileWAV;
    case snd_SBMIDI:
        return SBMIDI;
    case snd_RS232MIDI:
        return RS232MIDI;
    case snd_LPTMIDI:
        return LPTMIDI;
    default:
        return -1;
    }
}

void ASS_Init(int rate, int mdev, int sdev)
{
    int status, music_device, sound_device;
    unsigned int sample_rate;    
    int finalNumChannels = numChannels;

    fx_device fx_device;

    if (mdev != snd_none)
    {
        status = MUSIC_Ok;

        music_device = ASS_GetSoundCardCode(mdev);

        if (mdev == snd_SBMIDI)
        {
            status = FX_GetBlasterSettings(&dmx_blaster);

            if (status != FX_Ok)
            {
                I_Error(12, status);
            }

            status = FX_SetupSoundBlaster(dmx_blaster);

            if (status != FX_Ok)
            {
                I_Error(13, status);
            }
        }

        status = MUSIC_Init(music_device, dmx_mus_port);

        if (status != MUSIC_Ok)
        {
            I_Error(14, status);
        }

        if (status == MUSIC_Ok)
        {
            MUSIC_SetVolume(0);
        }
    }

    if (sdev != snd_none)
    {
        status = FX_Ok;

        sound_device = ASS_GetSoundCardCode(sdev);

        switch (sound_device)
        {
        case PC:
            // Load divisors
            divisors = (unsigned short *)I_ReadBinaryStatic("DATA\\PCSPK.BIN", 256);
            status = PCFX_Init();
            PCFX_SetTotalVolume(255);
            return;
        case SoundBlaster:
        case Awe32:
        case SoundBlasterDirect:
            status = FX_SetupSoundBlaster(dmx_blaster);
            printf("Sound Blaster DSP %01X.%02X\n", BLASTER_Version >> 8, BLASTER_Version && 7);
            printf("ADDR: %03X, IRQ: %u, DMA LOW: %u, DMA HIGH: %u\n", BLASTER_Config.Address, BLASTER_Config.Interrupt, BLASTER_Config.Dma8, BLASTER_Config.Dma16);
            break;
        case SoundSource:
        case LPTDAC:
        case CMS:
        case Adlib:
        case OPL2LPT:
        case OPL3LPT:
            status = FX_SetupCard(sound_device, &fx_device, dmx_snd_port);
            break;
        default:
            status = FX_SetupCard(sound_device, &fx_device, -1);
            break;
        }

        if (status != FX_Ok)
        {
            I_Error(15, status);
        }

        switch (snd_Rate)
        {
            case 0:
            sample_rate = 7000;
            break;
            case 1:
            sample_rate = 8000;
            break;
            case 2:
            sample_rate = 11025;
            break;
            case 3:
            sample_rate = 12000;
            break;
            case 4:
            sample_rate = 16000;
            break;
            case 5:
            sample_rate = 22050;
            break;
            case 6:
            sample_rate = 24000;
            break;
            case 7:
            sample_rate = 32000;
            break;
            case 8:
            sample_rate = 44100;
            break;
        }

        if (mdev = snd_WAV)
            finalNumChannels += 1; // Extra sound channel for PCM music

        status = FX_Init(sound_device, finalNumChannels, 2, sample_rate);

        if (status != FX_Ok)
        {
            I_Error(16, status);
        }

        FX_SetVolume(255);

        if (reverseStereo)
            MV_ReverseStereo();
    }
}

void ASS_DeInit(void)
{
    MUSIC_Shutdown();
    FX_Shutdown();
    PCFX_Shutdown();
    remove("ULTRAMID.INI");
    if (mid_data)
    {
        Z_Free(mid_data);
    }
}

int ENS_Detect(void)
{
    return SOUNDSCAPE_FindCard() != 0;
}
