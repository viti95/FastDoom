//
// Enter serial config info
//
#include <process.h>
#include <dos.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <bios.h>

#include "main.h"
#include "default.h"

enum
{
#ifndef DOOM2
	SER_EPISODE0, SER_EPISODE1, SER_EPISODE2,
#endif
	SER_SKILL1, SER_SKILL2, SER_SKILL3, SER_SKILL4,
	SER_DEATHNO, SER_DEATHYES,
	SER_COM1, SER_COM2, SER_COM3, SER_COM4,
	SER_MAX
};
item_t cserialitems[]=
{
#ifndef DOOM2
	{SER_EPISODE0,	31,5,21,		-1,-1},
	{SER_EPISODE1,	31,6,21,		-1,-1},
	{SER_EPISODE2,	31,7,21,		-1,-1},

	{SER_SKILL1,	31,9,21,		-1,-1},
	{SER_SKILL2,	31,10,21,		-1,-1},
	{SER_SKILL3,	31,11,21,		-1,-1},
	{SER_SKILL4,	31,12,21,	-1,-1},

	{SER_DEATHNO,	31,14,20,	-1,-1},
	{SER_DEATHYES,	31,15,20,	-1,-1},

	{SER_COM1,		31,17,4,		-1,		SER_COM3},
	{SER_COM2,		31,18,4,		-1,		SER_COM4},
	{SER_COM3,		45,17,4,		SER_COM1,-1,SER_DEATHYES},
	{SER_COM4,		45,18,4,		SER_COM2,-1}
#else
	{SER_SKILL1,	31,7,20,		-1,-1},
	{SER_SKILL2,	31,8,20,		-1,-1},
	{SER_SKILL3,	31,9,20,		-1,-1},
	{SER_SKILL4,	31,10,20,	-1,-1},

	{SER_DEATHNO,	31,12,20,	-1,-1},
	{SER_DEATHYES,	31,13,20,	-1,-1},

	{SER_COM1,		31,15,4,		-1,		SER_COM3},
	{SER_COM2,		31,16,4,		-1,		SER_COM4},
	{SER_COM3,		45,15,4,		SER_COM1,-1,SER_DEATHYES},
	{SER_COM4,		45,16,4,		SER_COM2,-1}
#endif
};
menu_t cserialmenu=
{
	&cserialitems[0],
#ifndef DOOM2
	SER_EPISODE0,
#else
	SER_SKILL1,
#endif
	SER_MAX,
	0x7f
};

#ifndef DOOM2
// EPISODE radio buttons
radio_t sepi[]=
{
	{28,5,1},
	{28,6,2},
	{28,7,3}
};
radiogroup_t sepig={&sepi[0],3,&sinfo.episode,15,1};

// SKILL radio buttons
radio_t sskill[]=
{
	{28,9,2},
	{28,10,3},
	{28,11,4},
	{28,12,5}
};
radiogroup_t sskillg={&sskill[0],4,&sinfo.skillLevel,15,1};

// DEATHMATCH radio buttons
radio_t sdeath[]=
{
	{28,14,0},
	{28,15,1}
};
radiogroup_t sdeathg={&sdeath[0],2,&sinfo.deathMatch,15,1};

// COMPORT radio buttons
radio_t scom[]=
{
	{28,17,1},
	{28,18,2},
	{42,17,3},
	{42,18,4}
};
radiogroup_t scomg={&scom[0],4,&sinfo.comport,15,1};
#else
// SKILL radio buttons
radio_t sskill[]=
{
	{28,7,2},
	{28,8,3},
	{28,9,4},
	{28,10,5}
};
radiogroup_t sskillg={&sskill[0],4,&sinfo.skillLevel,15,1};

// DEATHMATCH radio buttons
radio_t sdeath[]=
{
	{28,12,0},
	{28,13,1}
};
radiogroup_t sdeathg={&sdeath[0],2,&sinfo.deathMatch,15,1};

// COMPORT radio buttons
radio_t scom[]=
{
	{28,15,1},
	{28,16,2},
	{42,15,3},
	{42,16,4}
};
radiogroup_t scomg={&scom[0],4,&sinfo.comport,15,1};
#endif

int SerialConfig(void)
{
	short	key;
	short field;
	int   rval = 0;
	char  *args[MAXARGS];
	int   argcount;
	char  string[MAXARGS*2][12];
	int   i;
	int   level;


	sinfo = serialinfo;       // save copy of modeminfo

	SaveScreen();
	DrawPup(&cserial);

	//
	// Set defaults
	//
	sinfo.skillLevel = 3;
	DrawRadios(&sskillg);

	sinfo.episode = 1;
	#ifndef DOOM2
	DrawRadios(&sepig);
	#endif

	sinfo.deathMatch = 1;
	DrawRadios(&sdeathg);

	sinfo.comport = comport;
	DrawRadios(&scomg);

	while(1)
	{
		SetupMenu(&cserialmenu);
		field = GetMenuInput();
		key = menukey;
		switch ( key )
		{
			case KEY_ESC:
				rval = -1;
				goto func_exit;

			//
			// Secret WARP code: F5+warp
			//
			case KEY_F1:
			{
				level = WarpTime();
				if (!level)
					continue;

				serialinfo = sinfo;

				M_SaveDefaults();
				RestoreScreen();

				argcount = 1;

				args[0] = "sersetup.exe ";

				if (cdrom)
					args[argcount++] = "-cdrom";

				args[argcount++] = "-skill";
				sprintf(string[argcount],"%d",serialinfo.skillLevel);
				args[argcount] = string[argcount];
				argcount++;

				if (!level)
				{
					#ifndef DOOM2
					 args[argcount++] = "-episode";
					 sprintf(string[argcount],"%d",serialinfo.episode);
					 args[argcount] = string[argcount];
					 argcount++;
					#endif
				}
				else
				{
					args[argcount++] = "-warp";
					#ifdef DOOM2
					sprintf(string[argcount],"%d",level);
					args[argcount] = string[argcount];
					#else
					sprintf(string[argcount],"%d",level>>8);
					args[argcount] = string[argcount];
					argcount++;
					sprintf(string[argcount],"%d",level&0x0f);
					args[argcount] = string[argcount];
					#endif
					argcount++;
				}

				if (serialinfo.deathMatch)
					args[argcount++] = "-deathmatch";

				if (nomonsters)
					args[argcount++] = "-nomonsters";

				if (respawn)
					args[argcount++] = "-respawn";

				if (deathmatch2 && serialinfo.deathMatch)
					args[argcount++] = "-altdeath";

				sprintf(string[argcount],"-com%d",serialinfo.comport);
				args[argcount] = string[argcount];
				argcount++;

				for (i = 1;i < myargc; i++)
					args[argcount++] = myargv[i];

				args[argcount] = NULL;

				textbackground(0);
				textcolor(7);
				clrscr();

				execv("sersetup.exe",args);

				//
				// ERROR EXECing!
				//
				printf("Problem EXECing SERSETUP for netplay. Need to be in same directory!");
				exit(1);

			}
			break;

exitandsave:
			case KEY_F10:

				serialinfo = sinfo;

				M_SaveDefaults();
				RestoreScreen();

				argcount = 1;

				args[0] = "sersetup.exe ";

					if (cdrom)
						args[argcount++] = "-cdrom";

				args[argcount++] = "-skill";
				sprintf(string[argcount],"%d",serialinfo.skillLevel);
				args[argcount] = string[argcount];
				argcount++;

				if (serialinfo.deathMatch)
					args[argcount++] = "-deathmatch";

				#ifndef DOOM2
				args[argcount++] = "-episode";
				sprintf(string[argcount],"%d",serialinfo.episode);
				args[argcount] = string[argcount];
				argcount++;
				#endif

				sprintf(string[argcount],"-com%d",serialinfo.comport);
				args[argcount] = string[argcount];
				argcount++;
            
            for (i = 1;i < myargc; i++)
               args[argcount++] = myargv[i];

            args[argcount] = NULL;

				textbackground(0);
				textcolor(7);
				clrscr();

				execv("sersetup.exe",args);

				//
				// ERROR EXECing!
				//
				printf("Problem EXECing SERSETUP for netplay. Need to be in same directory!");
				exit(1);

			case KEY_ENTER:
			case 32:
			switch ( field )
			{
				#ifndef DOOM2
				//
				// Episode
				//
				case SER_EPISODE0:
				case SER_EPISODE1:
				case SER_EPISODE2:
					sinfo.episode = field - SER_EPISODE0 + 1;
					DrawRadios(&sepig);
					break;
				#endif

				//
				// Skill level
				//
				case SER_SKILL1:
				case SER_SKILL2:
				case SER_SKILL3:
				case SER_SKILL4:
					sinfo.skillLevel = field - SER_SKILL1 + 2; break;

				//
				// Deathmatch
				//
				case SER_DEATHNO:
				case SER_DEATHYES:
					sinfo.deathMatch = field - SER_DEATHNO;
					break;

				//
				// COM port
				//
				case SER_COM1:
				case SER_COM2:
				case SER_COM3:
				case SER_COM4:
					comport = sinfo.comport = field - SER_COM1 + 1;
					break;

				default:
					break;
			}
			DrawRadios(&sskillg);
			DrawRadios(&sdeathg);
			DrawRadios(&scomg);
			break;
		}
	}

	func_exit:

	RestoreScreen();
	return ( rval );
}

