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
// DESCRIPTION:
//	Main loop menu stuff.
//	Default Config File.
//

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "std_func.h"

#include "doomdef.h"

#include "z_zone.h"

#include "w_wad.h"

#include "i_system.h"
#include "v_video.h"

#include "hu_stuff.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "m_misc.h"

int myargc;
char **myargv;

extern patch_t *hu_font[HU_FONTSIZE];

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
int M_CheckParm(char *check)
{
    int i;

    for (i = 1; i < myargc; i++)
    {
        if (!strcasecmp(check, myargv[i]))
            return i;
    }

    return 0;
}

int M_CheckParmOptional(char *check, int *variable)
{
    int i;

    for (i = 1; i < myargc; i++)
    {
        if (!strcasecmp(check, myargv[i])){
            *variable = i;
            return 1;
        }            
    }
    
    return 0;
}

int M_CheckParmDisable(char *check, int *variable)
{
    int i;

    for (i = 1; i < myargc; i++)
    {
        if (!strcasecmp(check, myargv[i])){
            *variable = 0;
            return 1;
        }
            
    }

    return 0;
}

void M_AddToBox(fixed_t *box,
                fixed_t x,
                fixed_t y)
{
    if (box[BOXLEFT] > x)
        box[BOXLEFT] = x;
    else
        if (box[BOXRIGHT] < x)
            box[BOXRIGHT] = x;

    if (box[BOXBOTTOM] > y)
        box[BOXBOTTOM] = y;
    else 
        if (box[BOXTOP] < y)
            box[BOXTOP] = y;
}

//
// M_WriteFile
//
#ifndef O_BINARY
#define O_BINARY 0
#endif

boolean
M_WriteFile(char const *name,
            void *source,
            int length)
{
    int handle;
    int count;

    handle = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);

    if (handle == -1)
        return false;

    count = write(handle, source, length);
    close(handle);

    if (count < length)
        return false;

    return true;
}

//
// M_ReadFile
//
int M_ReadFile(char const *name,
               byte **buffer)
{
    int handle, count, length;
    struct stat fileinfo;
    byte *buf;

    handle = open(name, O_RDONLY | O_BINARY, 0666);
    fstat(handle, &fileinfo);
    length = fileinfo.st_size;
    buf = Z_MallocUnowned(length, PU_STATIC);
    count = read(handle, buf, length);
    close(handle);

    *buffer = buf;
    return length;
}

//
// DEFAULTS
//
int usemouse;

extern int key_right;
extern int key_left;
extern int key_up;
extern int key_down;

extern int key_strafeleft;
extern int key_straferight;

extern int key_fire;
extern int key_use;
extern int key_strafe;
extern int key_speed;

extern int mousebfire;
extern int mousebstrafe;
extern int mousebforward;
extern int viewwidth;
extern int viewwidthlimit;
extern int viewheight;

extern int mouseSensitivity;
extern int showMessages;

extern int detailLevel;

extern int screenblocks;

extern int showMessages;

// machine-independent sound params
int numChannels;

extern int sfxVolume;
extern int musicVolume;
extern int snd_SBport, snd_SBirq, snd_SBdma;
extern int snd_Mport;

typedef struct
{
    char *name;
    int *location;
    int defaultvalue;
    int scantranslate; // PC scan code hack
    int untranslated;  // lousy hack
} default_t;

#define SC_UPARROW 0x48
#define SC_DOWNARROW 0x50
#define SC_LEFTARROW 0x4b
#define SC_RIGHTARROW 0x4d
#define SC_RCTRL 0x1d
#define SC_RALT 0x38
#define SC_RSHIFT 0x36
#define SC_SPACE 0x39
#define SC_COMMA 0x33
#define SC_PERIOD 0x34
#define SC_PAGEUP 0x49
#define SC_INSERT 0x52
#define SC_HOME 0x47
#define SC_PAGEDOWN 0x51
#define SC_DELETE 0x53
#define SC_END 0x4f
#define SC_ENTER 0x1c

#define SC_KEY_A 0x1e
#define SC_KEY_B 0x30
#define SC_KEY_C 0x2e
#define SC_KEY_D 0x20
#define SC_KEY_E 0x12
#define SC_KEY_F 0x21
#define SC_KEY_G 0x22
#define SC_KEY_H 0x23
#define SC_KEY_I 0x17
#define SC_KEY_J 0x24
#define SC_KEY_K 0x25
#define SC_KEY_L 0x26
#define SC_KEY_M 0x32
#define SC_KEY_N 0x31
#define SC_KEY_O 0x18
#define SC_KEY_P 0x19
#define SC_KEY_Q 0x10
#define SC_KEY_R 0x13
#define SC_KEY_S 0x1f
#define SC_KEY_T 0x14
#define SC_KEY_U 0x16
#define SC_KEY_V 0x2f
#define SC_KEY_W 0x11
#define SC_KEY_X 0x2d
#define SC_KEY_Y 0x15
#define SC_KEY_Z 0x2c
#define SC_BACKSPACE 0x0e

default_t defaults[] =
    {
        {"mouse_sensitivity", &mouseSensitivity, 5},
        {"sfx_volume", &sfxVolume, 8},
        {"music_volume", &musicVolume, 8},
        {"show_messages", &showMessages, 1},

        {"key_right", &key_right, SC_RIGHTARROW, 1},
        {"key_left", &key_left, SC_LEFTARROW, 1},
        {"key_up", &key_up, SC_UPARROW, 1},
        {"key_down", &key_down, SC_DOWNARROW, 1},
        {"key_strafeleft", &key_strafeleft, SC_COMMA, 1},
        {"key_straferight", &key_straferight, SC_PERIOD, 1},

        {"key_fire", &key_fire, SC_RCTRL, 1},
        {"key_use", &key_use, SC_SPACE, 1},
        {"key_strafe", &key_strafe, SC_RALT, 1},
        {"key_speed", &key_speed, SC_RSHIFT, 1},

        {"use_mouse", &usemouse, 1},
        {"mouseb_fire", &mousebfire, 0},
        {"mouseb_strafe", &mousebstrafe, 1},
        {"mouseb_forward", &mousebforward, 2},

        {"screenblocks", &screenblocks, 9},
        {"detaillevel", &detailLevel, 0},
        {"showfps", &showFPS, 0},
        {"uncapped", &uncappedFPS, 0},
        {"flatsky", &flatSky, 0},
        {"near", &nearSprites, 0},
        {"nomelt", &noMelt, 0},
        {"flatShadows", &flatShadows, 0},
        {"saturnShadows", &saturnShadows, 0},
        {"untexturedSurfaces", &untexturedSurfaces, 0},
        {"flatSurfaces", &flatSurfaces, 0},
        {"vsync", &waitVsync, 0},

        {"monosound", &monoSound, 0},

        {"snd_channels", &numChannels, 3},
        {"snd_musicdevice", &snd_DesiredMusicDevice, 0},
        {"snd_sfxdevice", &snd_DesiredSfxDevice, 0},
        {"snd_sbport", &snd_SBport, 0x220},
        {"snd_sbirq", &snd_SBirq, 5},
        {"snd_sbdma", &snd_SBdma, 1},
        {"snd_mport", &snd_Mport, 0x330},

        {"usegamma", &usegamma, 0}

};

int numdefaults;
char *defaultfile;

//
// M_SaveDefaults
//
void M_SaveDefaults(void)
{
    int i;
    int v;
    FILE *f;

    f = fopen(defaultfile, "w");
    if (!f)
        return; // can't write the file, but don't complain

    for (i = 0; i < numdefaults; i++)
    {
        if (defaults[i].scantranslate)
            defaults[i].location = &defaults[i].untranslated;

        if (defaults[i].defaultvalue > -0xfff && defaults[i].defaultvalue < 0xfff)
        {
            v = *defaults[i].location;
            fprintf(f, "%s\t\t%i\n", defaults[i].name, v);
        }
        else
        {
            fprintf(f, "%s\t\t\"%s\"\n", defaults[i].name,
                    *(char **)(defaults[i].location));
        }
    }

    fclose(f);
}

//
// M_LoadDefaults
//
extern byte scantokey[128];

void M_LoadDefaults(void)
{
    int i;
    int len;
    FILE *f;
    char def[80];
    char strparm[100];
    char *newstring;
    int parm;
    boolean isstring;

    // set everything to base values
    numdefaults = sizeof(defaults) / sizeof(defaults[0]);
    for (i = 0; i < numdefaults; i++)
        *defaults[i].location = defaults[i].defaultvalue;

    // check for a custom default file
    i = M_CheckParm("-config");
    if (i && i < myargc - 1)
    {
        defaultfile = myargv[i + 1];
        printf("	default file: %s\n", defaultfile);
    }
    else
        defaultfile = basedefault;

    // read the file in, overriding any set defaults
    f = fopen(defaultfile, "r");
    if (f)
    {
        while (!feof(f))
        {
            isstring = false;
            if (fscanf(f, "%79s %99[^\n]\n", def, strparm) == 2)
            {
                if (strparm[0] == '"')
                {
                    // get a string default
                    isstring = true;
                    len = strlen(strparm);
                    newstring = (char *)malloc(len);
                    strparm[len - 1] = 0;
                    strcpy(newstring, strparm + 1);
                }
                else if (strparm[0] == '0' && strparm[1] == 'x')
                    sscanf(strparm + 2, "%x", &parm);
                else
                    sscanf(strparm, "%i", &parm);
                for (i = 0; i < numdefaults; i++)
                    if (!strcmp(def, defaults[i].name))
                    {
                        if (!isstring)
                            *defaults[i].location = parm;
                        else
                            *defaults[i].location = (int)newstring;
                        break;
                    }
            }
        }

        fclose(f);
    }

    for (i = 0; i < numdefaults; i++)
    {
        if (defaults[i].scantranslate)
        {
            parm = *defaults[i].location;
            defaults[i].untranslated = parm;
            *defaults[i].location = scantokey[parm];
        }
    }
}
