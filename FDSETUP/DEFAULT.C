//
// Save & Load defaults
//
#include <dir.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <bios.h>

#include "default.h"
#include "keys.h"
#include "main.h"

int usemouse;

extern net_t netinfo;
extern DMXINFO newc;
extern CONTS curk;

int mouseSensitivity = 4;
int showMessages = 1;
int sfxVolume = 15;
int musicVolume = 15;
int detailLevel = 1;
int screenblocks = 10;
int usegamma = 0;

int showFPS = 0;
int uncappedFPS = 0;
int flatSky = 0;
int nearSprites = 0;
int noMelt = 0;
int flatShadows = 0;
int saturnShadows = 0;
int untexturedSurfaces = 0;
int flatSurfaces = 0;
int waitVsync = 0;
int monoSound = 0;
int autorun = 0;

default_t defaults[] =
	{
		{"mouse_sensitivity", &mouseSensitivity, 5},
		{"sfx_volume", &sfxVolume, 8},
		{"music_volume", &musicVolume, 8},
		{"show_messages", &showMessages, 1},

		{"key_right", &curk.right, SC_RIGHT},
		{"key_left", &curk.left, SC_LEFT},
		{"key_up", &curk.up, SC_UP},
		{"key_down", &curk.down, SC_DOWN},
		{"key_strafeleft", &curk.key3, SC_COMMA},
		{"key_straferight", &curk.key4, SC_PERIOD},
		{"key_fire", &curk.fire, SC_CTRL},
		{"key_use", &curk.use, SC_SPACE},
		{"key_strafe", &curk.key1, SC_ALT},
		{"key_speed", &curk.key2, SC_RIGHT_SHIFT},

		{"use_mouse", &usemouse, 1},
		{"mouseb_fire", &curk.mouse[ID_FIRE], ID_FIRE},
		{"mouseb_strafe", &curk.mouse[ID_STRAFE], ID_STRAFE},
		{"mouseb_forward", &curk.mouse[ID_FORWARD], ID_FORWARD},

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

		{"autorun", &autorun, 0},

		{"monosound", &monoSound, 0},

		{"snd_channels", (int *)&newc.numdig, 3},
		{"snd_musicdevice", (int *)&newc.m.card, 0},
		{"snd_sfxdevice", (int *)&newc.d.card, 0},
		{"snd_mport", (int *)&newc.m.midiport, 0x330},
		{"snd_lptport", (int *)&newc.d.lptport, 0x378},

		{"usegamma", &usegamma, 0},
};

int numdefaults;
char *defaultfile;

/*
==============
=
= M_SaveDefaults
=
==============
*/

void M_SaveDefaults(void)
{
	int i;
	FILE *f;

	if (newc.m.lptport == -1)
	{
		if (newc.d.lptport == -1)
			newc.m.lptport = 0x378;
		else
			newc.m.lptport = newc.d.lptport;
	}

	if (newc.control != C_KEY)
	{
		if (newc.control == C_MOUSE)
			usemouse = 1;
	}

	numdefaults = sizeof(defaults) / sizeof(default_t);

	f = fopen(defaultfile, "w");
	if (!f)
		return; // can't write the file, but don't complain

	for (i = 0; i < numdefaults; i++)
		fprintf(f, "%s\t\t%i\n", defaults[i].name, *defaults[i].location);

	fclose(f);
}

int CheckParm(char *string)
{
	int i;

	for (i = 1; i < myargc; i++)
		if (!strcmp(myargv[i], string))
			return i;
	return 0;
}

/*
==============
=
= M_LoadDefaults
=
==============
*/
int M_LoadDefaults(void)
{
	int i;
	FILE *f;
	char def[80];
	char strparm[50];
	int parm;
	char macro[40];

	//
	// set everything to base values
	//
	numdefaults = sizeof(defaults) / sizeof(defaults[0]);
	for (i = 0; i < numdefaults; i++)
		*defaults[i].location = defaults[i].defaultvalue;

	defaultfile = DEFAULTNAME; // hard-coded path GONE!

	i = CheckParm("-config");
	if (i)
		defaultfile = myargv[i + 1];

	//
	// read the file in, overriding any set defaults
	//
	f = fopen(defaultfile, "r");
	if (!f)
		return (0); // no overrides

	while (!feof(f))
	{
		fscanf(f, "%79s %[^\n]", def, strparm);

		macro[0] = 0;
		if (strparm[0] == '0' && strparm[1] == 'x')
			sscanf(strparm + 2, "%x", &parm);
		else
			sscanf(strparm, "%i", &parm);

		for (i = 0; i < numdefaults; i++)
			if (!strcmp(def, defaults[i].name))
			{
				*defaults[i].location = parm;
				break;
			}
	}

	fclose(f);

	newc.d.lptport = newc.m.lptport;

	if (usemouse)
		newc.control = C_MOUSE;
	else
		newc.control = C_KEY;

	return (1);
}
