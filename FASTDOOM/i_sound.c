//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 1993-2008 Raven Software
// Copyright (C) 2016-2017 Alexey Khokholov (Nuke.YKT)
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
// DESCRIPTION:
//  System interface for sound.
//

#include <stdio.h>

#include "dmx.h"

#include "i_ibm.h"
#include "i_system.h"
#include "s_sound.h"
#include "i_sound.h"
#include "sounds.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#include "doomdef.h"
#include "doomstat.h"

#include "ns_task.h"
#include "ns_music.h"
#include "ns_cms.h"

#include "options.h"

#include "ns_cd.h"

//
// I_StartupTimer
//

task *tsm_task;

void I_StartupTimer(void)
{
    printf("I_StartupTimer()\n");
    // installs master timer.  Must be done before StartupTimer()!
    tsm_task = TS_ScheduleTask(I_TimerISR, 35, 1, NULL);
    TS_Dispatch();
}

void I_ShutdownTimer(void)
{
    if (tsm_task)
    {
        TS_Terminate(tsm_task);
    }
    tsm_task = NULL;
    TS_Shutdown();
}

//
// Sound header & data
//
int snd_Mport; // midi variables
int snd_Sport; // sound port

int snd_MusicVolume; // maximum volume for music
int snd_SfxVolume;   // maximum volume for sound

int snd_SfxDevice;   // current sfx card # (index to dmxCodes)
int snd_MusicDevice; // current music card # (index to dmxCodes)
int snd_DesiredSfxDevice;
int snd_DesiredMusicDevice;

void I_SetMusicVolume(int volume)
{
    MUSIC_SetVolume(volume);
    snd_MusicVolume = volume;
}
//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t *sfx)
{
    const char snd_prefixen[] = {'P', 'P', 'A', 'S', 'S', 'S', 'M', 'M', 'M', 'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S', 'S', 'A', 'A'};
    char namebuf[9];
    sprintf(namebuf, "D%c%s", snd_prefixen[snd_SfxDevice], sfx->name);
    return W_GetNumForName(namebuf);
}

//
// Sound startup stuff
//

void I_sndArbitrateCards(void)
{
    byte gus, adlib, sb, midi, ensoniq, lpt, cmsfx, cmsmus, oplxlpt, audiocd;
    int dmxlump;

    snd_SfxVolume = 127;
    snd_SfxDevice = snd_DesiredSfxDevice;
    snd_MusicDevice = snd_DesiredMusicDevice;

    //
    // check command-line parameters- overrides config file
    //
    if (M_CheckParm("-nosound"))
    {
        snd_MusicDevice = snd_SfxDevice = snd_none;
    }
    if (M_CheckParm("-nosfx"))
    {
        snd_SfxDevice = snd_none;
    }
    if (M_CheckParm("-nomusic"))
    {
        snd_MusicDevice = snd_none;
    }

    if (snd_MusicDevice > snd_MPU && snd_MusicDevice <= snd_MPU3)
    {
        snd_MusicDevice = snd_MPU;
    }

    //
    // figure out what i've got to initialize
    //
    gus = snd_MusicDevice == snd_GUS || snd_SfxDevice == snd_GUS;
    sb = snd_SfxDevice == snd_SB || snd_SfxDevice == snd_SBDirect;
    ensoniq = snd_SfxDevice == snd_ENSONIQ;
    adlib = snd_MusicDevice == snd_Adlib || snd_MusicDevice == snd_SB || snd_MusicDevice == snd_PAS;
    oplxlpt = snd_MusicDevice == snd_OPL2LPT || snd_MusicDevice == snd_OPL3LPT;
    midi = snd_MusicDevice == snd_MPU;
    lpt = snd_SfxDevice == snd_DISNEY || snd_SfxDevice == snd_TANDY || snd_SfxDevice == snd_LPTDAC;
    cmsfx = snd_SfxDevice == snd_CMS;
    cmsmus = snd_MusicDevice == snd_CMS;
    audiocd = snd_MusicDevice == snd_CD;

    //
    // initialize whatever i've got
    //
    if (ensoniq)
    {
        if (ENS_Detect())
        {
            printf("ENSONIQ isn't responding.\n");
        }
    }
    if (gus)
    {
        if (GF1_Detect())
        {
            printf("GUS isn't responding.\n");
        }
        else
        {
            if (gamemode == commercial)
            {
                dmxlump = W_GetNumForName("DMXGUSC");
            }
            else
            {
                dmxlump = W_GetNumForName("DMXGUS");
            }
            GF1_SetMap(W_CacheLumpNum(dmxlump, PU_CACHE), lumpinfo[dmxlump].size);
        }
    }
    if (sb)
    {
        SB_Detect();
    }

    if (adlib)
    {
        if (AL_Detect())
        {
            printf("Adlib isn't responding.\n");
        }
        else
        {
            void *genmidi = W_CacheLumpName("GENMIDI", PU_STATIC);
            AL_SetCard(genmidi);
            Z_Free(genmidi);
        }
    }

    if (oplxlpt)
    {
        void *genmidi = W_CacheLumpName("GENMIDI", PU_STATIC);
        AL_SetCard(genmidi);
        Z_Free(genmidi);
        OPLxLPT_SetCard(snd_Mport);
    }

    if (midi)
    {
        if (MPU_Detect(&snd_Mport))
        {
            printf("MPU-401 isn't reponding @ p=0x%x.\n", snd_Mport);
        }
        else
        {
            MPU_SetCard(snd_Mport);
        }
    }

    if (lpt)
    {
        SND_SetPort(snd_Sport);
    }

    if (cmsfx)
    {
        SND_SetPort(snd_Sport);
    }

    if (cmsmus)
    {
        CMS_SetCard(snd_Mport);
    }

    if (cmsfx && cmsmus)
    {
        CMS_SetMode(CMS_MusicFX);
    }
    else
    {
        if (cmsfx)
        {
            CMS_SetMode(CMS_OnlyFX);
        }

        if (cmsmus)
        {
            CMS_SetMode(CMS_OnlyMusic);
        }
    }

    if (audiocd)
    {
        if(!CD_Init())
        {
            // Error on AudioCD init
            snd_MusicDevice = snd_none;
            return;
        }

        CD_Volumeinfo.Volume0 = 255;
        CD_Volumeinfo.Volume1 = 255;
        CD_Volumeinfo.Volume2 = 255;
        CD_Volumeinfo.Volume3 = 255;
        CD_SetVolume();
        CD_SetTrack(2);                       
        CD_Lock(LOCK);                         
        CD_Seek(CD_Cdrom_data.Track_position); 
        delay(400);                            
                                               
        CD_PlayAudio(CD_Cdrom_data.Track_position, CD_Cdrom_data.Endofdisk);
        CD_GetVolume();
        //CD_StopAudio();
        //CD_Lock(UNLOCK);
        //CD_DeInit();
    }
}

//
// I_StartupSound
// Inits all sound stuff
//
void I_StartupSound(void)
{
    //
    // inits sound library timer stuff
    //
    I_StartupTimer();

    //
    // pick the sound cards i'm going to use
    //
    I_sndArbitrateCards();

    //
    // inits ASS sound library
    //
    printf("  calling ASS_Init\n");

    ASS_Init(SND_TICRATE, SND_MAXSONGS, snd_MusicDevice, snd_SfxDevice);
}
//
// I_ShutdownSound
// Shuts down all sound stuff
//
void I_ShutdownSound(void)
{
    S_PauseSound();
    ASS_DeInit();
}
