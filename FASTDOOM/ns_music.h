#ifndef __MUSIC_H
#define __MUSIC_H

#include "ns_cards.h"

enum MUSIC_ERRORS
{
    MUSIC_Warning = -2,
    MUSIC_Error = -1,
    MUSIC_Ok = 0,
    MUSIC_ASSVersion,
    MUSIC_SoundCardError,
    MUSIC_MPU401Error,
    MUSIC_InvalidCard,
    MUSIC_MidiError,
    MUSIC_TaskManError,
    MUSIC_FMNotDetected,
    MUSIC_DPMI_Error
};

typedef struct
{
    unsigned long tickposition;
    unsigned long milliseconds;
    unsigned int measure;
    unsigned int beat;
    unsigned int tick;
} songposition;

int MUSIC_Init(int SoundCard, int Address);
int MUSIC_Shutdown(void);
void MUSIC_SetVolume(int volume);
void MUSIC_Continue(void);
void MUSIC_Pause(void);
int MUSIC_SongPlaying(void);
int MUSIC_StopSong(void);
int MUSIC_PlaySong(unsigned char *song, int loopflag);

#endif
