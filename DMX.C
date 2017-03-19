//
// Copyright (C) 2015 Alexey Khokholov (Nuke.YKT)
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

#include "dmx.h"
#include <stdio.h>
#include <stdlib.h>
#include "doomdef.h"
#include "fx_man.h"
#include "music.h"
#include "task_man.h"
#include "mus2mid.h"

void TSM_Install(int rate) {

}

fx_blaster_config dmx_blaster;
int(*tsm_func)(void);
task *tsm_task = NULL;

void tsm_funch() {
    tsm_func();
}

void *mus_data = NULL;
char *mid_data = NULL;

int mus_loop = 0;
int dmx_mus_port = 0;
int dmx_sdev = NumSoundCards;

int TSM_NewService(int(*function)(void), int rate, int unk1, int unk2) {
    tsm_func = function;
    tsm_task = TS_ScheduleTask(tsm_funch, rate, 1, NULL);
    TS_Dispatch();
    return 0;
}
void TSM_DelService(int unk1) {
    if (tsm_task) {
        TS_Terminate(tsm_task);
    }
    tsm_task = NULL;
}
void TSM_Remove(void) {
    TS_Shutdown();
}
void MUS_PauseSong(int handle) {
    MUSIC_Pause();
}
void MUS_ResumeSong(int handle) {
    MUSIC_Continue();
}
void MUS_SetMasterVolume(int volume) {
    MUSIC_SetVolume(volume * 2);
}
int MUS_RegisterSong(void *data) {
    FILE *mus;
    FILE *mid;
    unsigned int midlen;
    unsigned short len;
    mus_data = NULL;
    len = ((unsigned short*)data)[2]
        + ((unsigned short*)data)[3];
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
int MUS_UnregisterSong(int handle) {
    return 0;
}
int MUS_QrySongPlaying(int handle) {
    return 0;
}
int MUS_StopSong(int handle) {
    long status = MUSIC_StopSong();
    return (status != MUSIC_Ok);
}
int MUS_ChainSong(int handle, int next) {
    mus_loop = (next == handle);
    return 0;
}

int MUS_PlaySong(int handle, int volume) {
    long status;
    if (mus_data == NULL)
    {
        return 1;
    }
    status = MUSIC_PlaySong((unsigned char*)mus_data, mus_loop);
    MUSIC_SetVolume(volume * 2);
    return (status != MUSIC_Ok);
}

int SFX_PlayPatch(void *vdata, int pitch, int sep, int vol, int unk1, int unk2) {
    unsigned char *data = (unsigned char*)vdata;
    unsigned int type = (data[1] << 8) | data[0];
    unsigned int rate = (data[3] << 8) | data[2];
    unsigned long len = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];
    if (type != 3) {
        return -1;
    }
    if (len <= 48) {
        return -1;
    }
    len -= 32;
    return FX_PlayRaw(data + 24, len, rate, ((pitch - 128) * 2400) / 128, vol * 2, ((254 - sep) * vol) / 63, ((sep)* vol) / 63, 100, 0);
}
void SFX_StopPatch(int handle) {
    FX_StopSound(handle);
}
int SFX_Playing(int handle) {
    return FX_SoundActive(handle);
}
void SFX_SetOrigin(int handle, int  pitch, int sep, int vol) {
    FX_SetPan(handle, vol * 2, ((254 - sep) * vol) / 63, ((sep)* vol) / 63);
    FX_SetPitch(handle, ((pitch - 128) * 2400) / 128);
}
int GF1_Detect(void) {
    return 0; //FIXME
}
void GF1_SetMap(void *data, int len) {
    FILE *ini = fopen("ULTRAMID.INI", "wb");
    if (ini) {
        fwrite(data, 1, len, ini);
        fclose(ini);
    }
}
int SB_Detect(int *port, int *irq, int *dma, int *unk) {
    if (FX_GetBlasterSettings(&dmx_blaster)) {
        if (!port || !irq || !dma) {
            return -1;
        }
        dmx_blaster.Type = fx_SB;
        dmx_blaster.Address = *port;
        dmx_blaster.Interrupt = *irq;
        dmx_blaster.Dma8 = *dma;
    }
    return 0;
}
void SB_SetCard(int port, int irq, int dma) { } //FIXME
int AL_Detect(int *port, int *unk) {
    return !AL_DetectFM();
}
void AL_SetCard(int port, void *data) {
    unsigned char *cdata;
    unsigned char *tmb;
    int i;
    cdata = (unsigned char *)data;
    tmb = malloc(13 * 256);
    memset(tmb, 0, 13 * 256);
    if (!tmb)
    {
        return;
    }
    for (i = 0; i < 128; i++)
    {
        tmb[i * 13 + 0] = cdata[8 + i * 36 + 4 + 0];
        tmb[i * 13 + 1] = cdata[8 + i * 36 + 4 + 7];
        tmb[i * 13 + 2] = cdata[8 + i * 36 + 4 + 4]
                        | cdata[8 + i * 36 + 4 + 5];
        tmb[i * 13 + 3] = cdata[8 + i * 36 + 4 + 11]
                        | cdata[8 + i * 36 + 4 + 12];
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
        tmb[(i + 35) * 13 + 2] = cdata[8 + i * 36 + 4 + 4]
                               | cdata[8 + i * 36 + 4 + 5];
        tmb[(i + 35) * 13 + 3] = cdata[8 + i * 36 + 4 + 11]
                               | cdata[8 + i * 36 + 4 + 12];
        tmb[(i + 35) * 13 + 4] = cdata[8 + i * 36 + 4 + 1];
        tmb[(i + 35) * 13 + 5] = cdata[8 + i * 36 + 4 + 8];
        tmb[(i + 35) * 13 + 6] = cdata[8 + i * 36 + 4 + 2];
        tmb[(i + 35) * 13 + 7] = cdata[8 + i * 36 + 4 + 9];
        tmb[(i + 35) * 13 + 8] = cdata[8 + i * 36 + 4 + 3];
        tmb[(i + 35) * 13 + 9] = cdata[8 + i * 36 + 4 + 10];
        tmb[(i + 35) * 13 + 10] = cdata[8 + i * 36 + 4 + 6];
        tmb[(i + 35) * 13 + 11] = cdata[8 + i * 36 + 3]
                                + cdata[8 + i * 36 + 4 + 14] + 12;
        tmb[(i + 35) * 13 + 12] = 0;
    }
    AL_RegisterTimbreBank(tmb);
    free(tmb);
}
int MPU_Detect(int *port, int *unk) {
    if (port == NULL) {
        return -1;
    }
    return MPU_Init(*port);
}
void MPU_SetCard(int port) {
    dmx_mus_port = port;
}
int DMX_Init(int rate, int maxsng, int mdev, int sdev) {
    long status, device;
    dmx_sdev = sdev;

    switch (mdev) {
    case 0:
        device = NumSoundCards;
        break;
    case AHW_ADLIB:
        device = Adlib;
        break;
    case AHW_SOUND_BLASTER:
        device = SoundBlaster;
        break;
    case AHW_MPU_401:
        device = GenMidi;
        break;
    case AHW_ULTRA_SOUND:
        device = UltraSound;
        break;
    default:
        return -1;
        break;
    }
    if (device == SoundBlaster) {

        int MaxVoices;
        int MaxBits;
        int MaxChannels;

        FX_SetupSoundBlaster(dmx_blaster, (int *)&MaxVoices, (int *)&MaxBits, (int *)&MaxChannels);
    }
    status = MUSIC_Init(device, dmx_mus_port);
    if (status != MUSIC_Ok) {
        return -1;
    }
    MUSIC_SetVolume(0);
    return 0;
}

void DMX_DeInit(void) {
    MUSIC_Shutdown();
    FX_Shutdown();
    remove("ULTRAMID.INI");
    if (mid_data)
    {
        free(mid_data);
    }
}

void WAV_PlayMode(int channels, int samplerate) {
    long device, status;
    char tmp[300];
    switch (dmx_sdev) {
    case 0:
        device = NumSoundCards;
        break;
    case AHW_SOUND_BLASTER:
        device = SoundBlaster;
        break;
    case AHW_ULTRA_SOUND:
        device = UltraSound;
        break;
    default:
        return;
        break;
    }
    if (device == SoundBlaster) {

        int MaxVoices;
        int MaxBits;
        int MaxChannels;

        FX_SetupSoundBlaster(dmx_blaster, (int *)&MaxVoices, (int *)&MaxBits, (int *)&MaxChannels);
    }
    status = FX_Init(device, channels, 2, 16, samplerate);
    FX_SetVolume(255);
    printf("Status %i %i %i %i\n", device, device + 10, SoundBlaster, status);
}

int CODEC_Detect(int *a, int *b)
{
    return 1;
}
int ENS_Detect(void)
{
    return 1;
}
