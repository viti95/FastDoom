//
// Copyright (C) 1993-1996 Id Software, Inc.
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
// DESCRIPTION:  none
//

#include <stdio.h>
#include <stdlib.h>
#include "options.h"
#include "i_system.h"
#include "i_sound.h"
#include "sounds.h"
#include "s_sound.h"

#include "z_zone.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"
#include "p_local.h"

#include "doomstat.h"
#include "dmx.h"
#include "ns_music.h"

#include "p_mobj.h"

#include "std_func.h"

#include "ns_cd.h"
#include "ns_multi.h"

// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
extern int snd_MusicDevice;
extern int snd_SfxDevice;
// Config file? Same disclaimer as above.
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;

int cdlooping = 0;
int cdmusicnum = 0;

int wavhandle;
int wavmusicnum = 0;
int wavlooping = 0;
unsigned char *wavfileptr;

typedef struct
{
    // sound information (if null, channel avail.)
    sfxinfo_t *sfxinfo;

    // origin of sound
    void *origin;

    // handle of the sound being played
    int handle;

} channel_t;

// the set of channels available
static channel_t *channels;

// These are not used, but should be (menu).
// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
static int snd_SfxVolume;

// Maximum volume of music. Useless so far.
static int snd_MusicVolume;

extern int sfxVolume;
extern int musicVolume;

// whether songs are mus_paused
static byte mus_paused;

// music currently being played
static musicinfo_t *mus_playing = 0;

int snd_clipping = S_CLIPPING_DIST;

//
// Internals.
//
int S_getChannel(void *origin, sfxinfo_t *sfxinfo);
byte S_AdjustSoundParams(mobj_t *source, int *vol, int *sep);
void S_StopChannel(int cnum);

void S_SetMusicVolumeCD(int volume)
{
    CD_SetVolume(volume);
    snd_MusicVolume = volume;
}

void S_SetMusicVolumeMIDI(int volume)
{
    I_SetMusicVolume(volume);
    snd_MusicVolume = volume;
}

void S_SetMusicVolume(int volume)
{
    switch (snd_MusicDevice)
    {
    case snd_CD:
        S_SetMusicVolumeCD(volume);
        return;
    case snd_WAV:
        return;
    default:
        S_SetMusicVolumeMIDI(volume);
        return;
    }
}

void S_StopMusicMIDI(void)
{
    if (mus_playing)
    {
        if (mus_paused)
            MUSIC_Continue();

        MUSIC_StopSong();
        Z_Free(mus_playing->data);

        mus_playing->data = 0;
        mus_playing = 0;
    }
}

unsigned char S_MapMusicCD(int musicnum)
{
    unsigned char DoomMusicMap[33] = {
        1,  // mus_None,
        1,  // mus_e1m1
        2,  // mus_e1m2
        3,  // mus_e1m3
        4,  // mus_e1m4
        5,  // mus_e1m5
        6,  // mus_e1m6
        7,  // mus_e1m7
        8,  // mus_e1m8
        9,  // mus_e1m9
        10, // mus_e2m1
        11, // mus_e2m2
        12, // mus_e2m3
        13, // mus_e2m4
        7,  // mus_e2m5
        14, // mus_e2m6
        15, // mus_e2m7
        16, // mus_e2m8
        17, // mus_e2m9
        17, // mus_e3m1
        18, // mus_e3m2
        19, // mus_e3m3
        8,  // mus_e3m4
        7,  // mus_e3m5
        6,  // mus_e3m6
        15, // mus_e3m7
        20, // mus_e3m8
        9,  // mus_e3m9
        12, // mus_inter
        21, // mus_intro
        22, // mus_bunny
        23, // mus_victor
        21  // mus_introa
    };

    unsigned char Doom2MusicMap[36] = {
        1,  // mus_None,
        1,  // mus_runnin - MAP1
        2,  // mus_stalks - MAP2
        3,  // mus_countd - MAP3
        4,  // mus_betwee - MAP4
        5,  // mus_doom - MAP5
        6,  // mus_the_da - MAP6
        7,  // mus_shawn - MAP7
        8,  // mus_ddtblu - MAP8
        9,  // mus_in_cit - MAP9
        10, // mus_dead - MAP10
        2,  // mus_stlks2 - MAP11
        6,  // mus_theda2 - MAP12
        5,  // mus_doom2 - MAP13
        8,  // mus_ddtbl2 - MAP14
        1,  // mus_runni2 - MAP15
        10, // mus_dead2 - MAP16
        2,  // mus_stlks3 - MAP17
        11, // mus_romero - MAP18
        7,  // mus_shawn2 - MAP19
        12, // mus_messag - MAP20
        3,  // mus_count2 - MAP21
        8,  // mus_ddtbl3 - MAP22
        13, // mus_ampie - MAP23
        6,  // mus_theda3 - MAP24
        14, // mus_adrian - MAP25
        12, // mus_messg2 - MAP26
        11, // mus_romer2 - MAP27
        15, // mus_tense - MAP28
        7,  // mus_shawn3 - MAP29
        16, // mus_openin - MAP30
        17, // mus_evil - MAP31
        18, // mus_ultima - MAP32
        19, // mus_read_m
        20, // mus_dm2ttl
        21  // mus_dm2int
    };

    unsigned char TNTMusicMap[36] = {
        1,  // mus_None,
        1,  // mus_runnin - MAP1
        2,  // mus_stalks - MAP2
        3,  // mus_countd - MAP3
        4,  // mus_betwee - MAP4
        5,  // mus_doom - MAP5
        6,  // mus_the_da - MAP6
        7,  // mus_shawn - MAP7
        8,  // mus_ddtblu - MAP8
        1,  // mus_in_cit - MAP9
        9,  // mus_dead - MAP10
        10, // mus_stlks2 - MAP11
        11, // mus_theda2 - MAP12
        4,  // mus_doom2 - MAP13
        12, // mus_ddtbl2 - MAP14
        2,  // mus_runni2 - MAP15
        13, // mus_dead2 - MAP16
        5,  // mus_stlks3 - MAP17
        9,  // mus_romero - MAP18
        14, // mus_shawn2 - MAP19
        15, // mus_messag - MAP20
        16, // mus_count2 - MAP21
        17, // mus_ddtbl3 - MAP22
        18, // mus_ampie - MAP23
        19, // mus_theda3 - MAP24
        20, // mus_adrian - MAP25
        13, // mus_messg2 - MAP26
        8,  // mus_romer2 - MAP27
        17, // mus_tense - MAP28
        4,  // mus_shawn3 - MAP29
        8,  // mus_openin - MAP30
        21, // mus_evil - MAP31
        16, // mus_ultima - MAP32
        22, // mus_read_m
        23, // mus_dm2ttl
        21  // mus_dm2int
    };

    unsigned char PlutoniaMusicMap[36] = {
        1,  // mus_None,
        1,  // mus_runnin - MAP1
        2,  // mus_stalks - MAP2
        3,  // mus_countd - MAP3
        4,  // mus_betwee - MAP4
        5,  // mus_doom - MAP5
        6,  // mus_the_da - MAP6
        7,  // mus_shawn - MAP7
        8,  // mus_ddtblu - MAP8
        9,  // mus_in_cit - MAP9
        10, // mus_dead - MAP10
        11, // mus_stlks2 - MAP11
        12, // mus_theda2 - MAP12
        13, // mus_doom2 - MAP13
        14, // mus_ddtbl2 - MAP14
        15, // mus_runni2 - MAP15
        16, // mus_dead2 - MAP16
        17, // mus_stlks3 - MAP17
        10, // mus_romero - MAP18
        18, // mus_shawn2 - MAP19
        19, // mus_messag - MAP20
        20, // mus_count2 - MAP21
        21, // mus_ddtbl3 - MAP22
        22, // mus_ampie - MAP23
        23, // mus_theda3 - MAP24
        24, // mus_adrian - MAP25
        19, // mus_messg2 - MAP26
        7,  // mus_romer2 - MAP27
        8,  // mus_tense - MAP28
        17, // mus_shawn3 - MAP29
        25, // mus_openin - MAP30
        6,  // mus_evil - MAP31
        14, // mus_ultima - MAP32
        20, // mus_read_m
        26, // mus_dm2ttl
        27  // mus_dm2int
    };

    switch (gamemission)
    {
    case doom:
        return DoomMusicMap[musicnum];
    case doom2:
        return Doom2MusicMap[musicnum - mus_introa];
    case pack_plut:
        return PlutoniaMusicMap[musicnum - mus_introa];
    case pack_tnt:
        return TNTMusicMap[musicnum - mus_introa];
    default:
        return 1;
    }
}

void S_ChangeMusicCD(int musicnum, int looping)
{
    int cdtrack;

    cdmusicnum = musicnum;
    cdtrack = S_MapMusicCD(musicnum);
    cdlooping = looping;

    // Not enough CD tracks. At least not crash.
    if (cdtrack > CD_Cdrom_data.High_audio)
        cdtrack = cdtrack % CD_Cdrom_data.High_audio;

    CD_StopAudio();
    CD_PlayAudio(TrackBeginPosition[cdtrack], TrackLength[cdtrack]);
}

void S_CheckCD(void)
{
    if (cdlooping && !mus_paused)
    {
        CD_GetAudioStatus();
        if ((CD_Cdrom_data.Status & (1 << 9)) ? 0 : 1)
            S_ChangeMusicCD(cdmusicnum, cdlooping);
    }
}

void S_CheckWAV(void)
{
    if (wavlooping)
    {
        if (!MV_VoicePlaying(wavhandle))
            S_ChangeMusicWAV(wavmusicnum, wavlooping);
    }
}

unsigned char *LoadFile(char *filename, int *length)
{
    FILE *in;
    long size;
    unsigned char *ptr;

    if ((in = fopen(filename, "rb")) == NULL)
        I_Error("FILE NOT FOUND");

    fseek(in, 0, SEEK_END);
    size = ftell(in);
    fseek(in, 0, SEEK_SET);

    ptr = (unsigned char *)malloc(size);
    if (ptr == NULL)
        I_Error("OUT OF MEMORY");

    if (fread(ptr, size, 1, in) != 1)
        I_Error("UNEXPECTED END OF FILE");

    fclose(in);

    *length = size;

    return (ptr);
}

void S_ChangeMusicWAV(int musicnum, int looping)
{
    int length;
    

    char filename[80];
    char subfolder[5];

    if (MV_VoicePlaying(wavhandle))
    {
        MV_Kill(wavhandle);
    }

    if (wavfileptr != NULL)
    {
        free(wavfileptr);
    }

    memset(filename, 0, sizeof(filename));
    memset(subfolder, 0, sizeof(subfolder));

    wavmusicnum = musicnum;
    wavlooping = looping;

    switch (gamemission)
    {
    case doom:
        sprintf(subfolder, "DOOM1");
        break;
    case doom2:
        sprintf(subfolder, "DOOM2");
        break;
    case pack_plut:
        sprintf(subfolder, "PLUTONIA");
        break;
    case pack_tnt:
        sprintf(subfolder, "TNT");
        break;
    }

    sprintf(filename, "MUSIC/%s/mus_%u.raw", subfolder, S_MapMusicCD(musicnum));

    wavfileptr = LoadFile(filename, &length);

    wavhandle = MV_PlayRaw(wavfileptr, length, 22050, 255, 255, 255, 0);
}

void S_ChangeMusicMIDI(int musicnum, int looping)
{
    musicinfo_t *music;
    char namebuf[9];

    if (snd_MusicDevice == snd_none)
        return;

    if ((snd_MusicDevice == snd_Adlib || snd_MusicDevice == snd_OPL2LPT || snd_MusicDevice == snd_SB || snd_MusicDevice == snd_OPL3LPT || snd_MusicDevice == snd_CMS) && musicnum == mus_intro)
    {
        musicnum = mus_introa;
    }

    music = &S_music[musicnum];

    if (mus_playing == music)
        return;

    // shutdown old music
    S_StopMusicMIDI();

    // get lumpnum if neccessary
    if (!music->lumpnum)
    {
        sprintf(namebuf, "D_%s", music->name);
        music->lumpnum = W_GetNumForName(namebuf);
    }

    // load & register it
    music->data = (void *)W_CacheLumpNum(music->lumpnum, PU_MUSIC);
    music->handle = MUS_RegisterSong(music->data);

    // play it
    MUS_ChainSong(music->handle, looping ? music->handle : -1);
    MUS_PlaySong(music->handle, snd_MusicVolume);

    mus_playing = music;
}

void S_ChangeMusic(int musicnum, int looping)
{
    switch (snd_MusicDevice)
    {
    case snd_CD:
        S_ChangeMusicCD(musicnum, looping);
        break;
    case snd_WAV:
        S_ChangeMusicWAV(musicnum, looping);
        break;
    default:
        S_ChangeMusicMIDI(musicnum, looping);
        break;
    }
}

void S_StopChannel(int cnum)
{

    int i;
    channel_t *c = &channels[cnum];

    if (c->sfxinfo)
    {
        // stop the sound playing
        if (SFX_Playing(c->handle))
        {
            SFX_StopPatch(c->handle);
        }
        c->sfxinfo = 0;
    }
}

//
// Changes volume, stereo-separation, and pitch variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
byte S_AdjustSoundParams(mobj_t *source, int *vol, int *sep)
{
    fixed_t approx_dist;
    fixed_t adx;
    fixed_t ady;
    angle_t angle;

    fixed_t optSine;

    // calculate the distance to sound origin
    //  and clip it if necessary
    adx = abs(players_mo->x - source->x);
    ady = abs(players_mo->y - source->y);

    // From _GG1_ p.428. Appox. eucledian distance fast.
    approx_dist = adx + ady - ((adx < ady ? adx : ady) >> 1);

    if (approx_dist > snd_clipping)
    {
        return 1; // Non audible
    }

    // volume calculation
    if (approx_dist < S_CLOSE_DIST)
    {
        *vol = snd_SfxVolume;
    }
    else
    {
        // distance effect
        *vol = Div1000(snd_SfxVolume * ((snd_clipping - approx_dist) >> FRACBITS));
    }

    if (*vol == 0)
    {
        return 1; // Non audible
    }

    if (monoSound)
    {
        *sep = NORM_SEP;
    }
    else
    {
        // angle of source to listener
        angle = R_PointToAngle2(players_mo->x, players_mo->y, source->x, source->y);

        angle -= players_mo->angle;
        if (angle <= players_mo->angle)
            angle += 0xffffffff;

        angle >>= ANGLETOFINESHIFT;

        // stereo separation (S_STEREO_SWING == 2^21 + 2^22)
        optSine = finesine[angle];
        *sep = 128 - (((optSine << 6) + (optSine << 5)) >> FRACBITS);
    }

    return 0; // Audible
}

void S_SetSfxVolume(int volume)
{
    snd_SfxVolume = volume;
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseMusic(void)
{
    switch (snd_MusicDevice)
    {
    case snd_CD:
        CD_StopAudio();
        mus_paused = 1;
        return;
    case snd_WAV:
        MV_Kill(wavhandle); // Apogee Sound System doesn't support pause audio samples
        return;
    default:
        if (mus_playing && !mus_paused)
        {
            MUSIC_Pause();
            mus_paused = 1;
        }
        return;
    }
}

void S_ResumeMusic(void)
{

    switch (snd_MusicDevice)
    {
    case snd_CD:
        CD_ResumeAudio();
        mus_paused = 0;
        return;
    case snd_WAV:
        S_ChangeMusicWAV(wavmusicnum, wavlooping);
        return;
    default:
        if (mus_playing && mus_paused)
        {
            MUSIC_Continue();
            mus_paused = 0;
        }
        return;
    }
}

void S_StopSound(void *origin)
{
    int cnum;

    if (snd_SfxDevice == snd_none)
        return;

    for (cnum = 0; cnum < numChannels; cnum++)
    {
        if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
        {
            S_StopChannel(cnum);
            break;
        }
    }
}

//
// S_getChannel :
//   If none available, return -1.  Otherwise channel #.
//
int S_getChannel(void *origin, sfxinfo_t *sfxinfo)
{
    // channel number to use
    int cnum;

    channel_t *c;

    // Find an open channel
    for (cnum = 0; cnum < numChannels; cnum++)
    {
        if (!channels[cnum].sfxinfo)
            break;
        else if (origin && channels[cnum].origin == origin)
        {
            S_StopChannel(cnum);
            break;
        }
    }

    // None available
    if (cnum == numChannels)
    {
        // Look for lower priority
        for (cnum = 0; cnum < numChannels; cnum++)
            if (channels[cnum].sfxinfo->priority >= sfxinfo->priority)
                break;

        if (cnum == numChannels)
        {
            // FUCK!  No lower priority.  Sorry, Charlie.
            return -1;
        }
        else
        {
            // Otherwise, kick out lower priority.
            S_StopChannel(cnum);
        }
    }

    c = &channels[cnum];

    // channel is decided to be cnum.
    c->sfxinfo = sfxinfo;
    c->origin = origin;

    return cnum;
}

void S_StartSound(mobj_t *origin, byte sfx_id)
{
    int sep;
    sfxinfo_t *sfx;
    int cnum;
    int volume;

    if (snd_SfxDevice == snd_none)
        return;

    volume = snd_SfxVolume;

    // Check to see if it is audible,
    //  and if not, modify the params
    if (origin && origin != players_mo)
    {
        byte rc = S_AdjustSoundParams(origin, &volume, &sep);

        if (rc)
        {
            return;
        }

        if (origin->x == players_mo->x && origin->y == players_mo->y)
        {
            sep = NORM_SEP;
        }
    }
    else
    {
        sep = NORM_SEP;
    }

    // kill old sound
    S_StopSound(origin);

    sfx = &S_sfx[sfx_id];

    // try to find a channel
    cnum = S_getChannel(origin, sfx);

    if (cnum < 0)
        return;

    //
    // This is supposed to handle the loading/caching.
    // For some odd reason, the caching is done nearly
    //  each time the sound is needed?
    //

    // get lumpnum if necessary
    if (sfx->lumpnum < 0)
        sfx->lumpnum = I_GetSfxLumpNum(sfx);

    // cache data if necessary
    if (!sfx->data)
    {
        sfx->data = (void *)W_CacheLumpNum(sfx->lumpnum, PU_SOUND);
    }

    // Assigns the handle to one of the channels in the
    //  mix/output buffer.
    channels[cnum].handle = SFX_PlayPatch(sfx->data, sep, volume);
}

//
// Updates music & sounds
//
void S_UpdateSounds(void)
{
    int cnum;
    int volume;
    int sep;

    if (snd_SfxDevice == snd_none)
        return;

    for (cnum = 0; cnum < numChannels; cnum++)
    {
        channel_t *c = &channels[cnum];
        if (c->sfxinfo)
        {
            if (SFX_Playing(c->handle))
            {
                // initialize parameters
                volume = snd_SfxVolume;
                sep = NORM_SEP;

                // check non-local sounds for distance clipping
                //  or modify their params
                if (c->origin && players_mo != c->origin)
                {
                    byte audible = S_AdjustSoundParams(c->origin, &volume, &sep);

                    if (audible)
                    {
                        S_StopChannel(cnum);
                    }
                    else
                        SFX_SetOrigin(c->handle, sep, volume);
                }
            }
            else
            {
                // if channel is allocated but sound has stopped,
                //  free it
                S_StopChannel(cnum);
            }
        }
    }
}

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int musicVolume)
{
    int i;

    S_SetSfxVolume(sfxVolume);
    // No music with Linux - another dummy.
    S_SetMusicVolume(musicVolume);

    // Allocating the internal channels for mixing
    // (the maximum numer of sounds rendered
    // simultaneously) within zone memory.
    channels = (channel_t *)Z_MallocUnowned(numChannels * sizeof(channel_t), PU_STATIC);

    // Free all channels for use
    for (i = 0; i < numChannels; i++)
        channels[i].sfxinfo = 0;

    // no sounds are playing, and they are not mus_paused
    mus_paused = 0;

    // Note that sounds have not been cached (yet).
    for (i = 1; i < NUMSFX; i++)
        S_sfx[i].lumpnum = -1;
}

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
    int cnum;
    int mnum;

    // kill all playing sounds at start of level
    //  (trust me - a good idea)
    for (cnum = 0; cnum < numChannels; cnum++)
        if (channels[cnum].sfxinfo)
            S_StopChannel(cnum);

    // start new music for the level
    mus_paused = 0;

    if (gamemode == commercial)
        mnum = mus_runnin + gamemap - 1;
    else
    {
        int spmus[] =
            {
                // Song - Who? - Where?

                mus_e3m4, // American	e4m1
                mus_e3m2, // Romero	e4m2
                mus_e3m3, // Shawn	e4m3
                mus_e1m5, // American	e4m4
                mus_e2m7, // Tim 	e4m5
                mus_e2m4, // Romero	e4m6
                mus_e2m6, // J.Anderson	e4m7 CHIRON.WAD
                mus_e2m5, // Shawn	e4m8
                mus_e1m9  // Tim		e4m9
            };

        if (gameepisode < 4)
            mnum = mus_e1m1 + (gameepisode - 1) * 9 + gamemap - 1;
        else
            mnum = spmus[gamemap - 1];
    }

    // HACK FOR COMMERCIAL
    //  if (gamemode == commercial && mnum > mus_e3m9)
    //      mnum -= mus_e3m9;

    S_ChangeMusic(mnum, true);
}

void S_ClearSounds(void)
{
    unsigned short i;

    if (snd_SfxDevice == snd_none)
        return;

    // Clean up unused data.
    for (i = 1; i < NUMSFX; i++)
    {
        if (S_sfx[i].data != 0)
        {
            Z_Free(S_sfx[i].data);
            S_sfx[i].data = 0;
        }
    }
}
