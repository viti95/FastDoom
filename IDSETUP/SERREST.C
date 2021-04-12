//
// Enter serial-link savegame info
//
#include <string.h>
#include <process.h>
#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <mem.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>

#include "main.h"
#include "default.h"

enum {SERS_0,SERS_1,SERS_2,SERS_3,SERS_4,SERS_5,
	SERS_DEATHNO,SERS_DEATHYES,
	SERS_COM1,SERS_COM2,SERS_COM3,SERS_COM4,
	SERS_MAX};
item_t sersaveitems[]=
{
	{SERS_0,		29,6,24,		-1,-1},
	{SERS_1,		29,7,24,		-1,-1},
	{SERS_2,		29,8,24,		-1,-1},
	{SERS_3,		29,9,24,	-1,-1},
	{SERS_4,		29,10,24,	-1,-1},
	{SERS_5,		29,11,24,	-1,-1},

	{SERS_DEATHNO,	29,13,24,	-1,-1},
	{SERS_DEATHYES,29,14,24,	-1,-1},

	{SERS_COM1,	29,16,4,		-1,SERS_COM3},
	{SERS_COM2,	29,17,4,		-1,SERS_COM4},
	{SERS_COM3,	46,16,4,		SERS_COM1,-1,SERS_DEATHYES},
	{SERS_COM4,	46,17,4,		SERS_COM2,-1}
};
menu_t sersavemenu=
{
	&sersaveitems[0],
	SERS_0,
	SERS_MAX,
	0x7f
};

// SAVEGAME radio group
radio_t serslots[]=
{
	{26,6,0},
	{26,7,1},
	{26,8,2},
	{26,9,3},
	{26,10,4},
	{26,11,5}
};
radiogroup_t serslotsg=	{ &serslots[0],6,&saveslot,15,1 };

// DEATHMATCH radio group
radio_t sersdeath[]=
{
	{26,13,0},
	{26,14,1}
};
radiogroup_t sersdeathg=	{ &sersdeath[0],2,&sinfo.deathMatch,15,1 };

// COMPORT radio group
radio_t serscom[]=
{
	{26,16,1},
	{26,17,2},
	{43,16,3},
	{43,17,4}
};
radiogroup_t serscomg=	{ &serscom[0],4,&sinfo.comport,15,1 };


int RestoreSerial(void)
{
	short field;
	short	key;
	int   rval = 0;
	char  *args[MAXARGS];
	int   argcount;
	char  string[MAXARGS*2][16];
	int   i;
	int   numplayers[6];
	int   saveslot;
	int   handle;
	char  name[32];
	char  p1,p2,p3,p4;


	sinfo = serialinfo;       // save copy of netinfo

	SaveScreen();
	DrawPup(&sersave);

	saveslot = -1;
	memset(savenames,0,6*24);

	//
	// Read in savegame strings
	//
	textbackground(1);
	textcolor(15);
	for (i = 0;i < 6;i++)
	{
		sprintf(name,SAVENAME,i);
		handle = open (name, O_BINARY | O_RDONLY);
		if (handle == -1)
			continue;

		read(handle,savenames[i],24);
		lseek(handle,27+16,SEEK_SET);
		read(handle,&p1,1);
		read(handle,&p2,1);
		read(handle,&p3,1);
		read(handle,&p4,1);
		numplayers[i] = p1+p2+p3+p4;
		close(handle);

		Clear(&sersaveitems[i]);
		Pos(&sersaveitems[i]);
		cprintf("%s",savenames[i]);
	}

	//
	// Set defaults
	//
	sinfo.comport = comport;
	DrawRadios(&serscomg);

	sinfo.deathMatch = 0;
	DrawRadios(&sersdeathg);
	gotoxy(1,25);

	while(1)
	{
		SetupMenu(&sersavemenu);
		field = GetMenuInput();
		key = menukey;
      switch ( key )
		{
			case KEY_ESC:
				rval = -1;
				goto func_exit;

exitandsave:
			case KEY_F10:

				if (saveslot < 0)
				{
					ErrorWindow(&netserr);
					break;
				}

				serialinfo = sinfo;

				M_SaveDefaults();
				RestoreScreen();

				argcount = 1;

				args[0] = "sersetup.exe ";

					if (cdrom)
						args[argcount++] = "-cdrom";

				args[argcount++] = "-nodes";
				sprintf(string[argcount],"%d",numplayers[saveslot]);
				args[argcount] = string[argcount];
				argcount++;

				if (modeminfo.deathMatch)
					args[argcount++] = "-deathmatch";

				sprintf(string[argcount],"-com%d",serialinfo.comport);
				args[argcount] = string[argcount];
				argcount++;

				args[argcount++] = "-loadgame";
				sprintf(string[argcount],"%d",saveslot);
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
				exit(0);

			case KEY_ENTER:
				switch ( field )
				{
					case SERS_COM1:
					case SERS_COM2:
					case SERS_COM3:
					case SERS_COM4:
						sinfo.comport = field - SERS_COM1 + 1;
						DrawRadios(&serscomg);
						break;

					case SERS_0:
					case SERS_1:
					case SERS_2:
					case SERS_3:
					case SERS_4:
					case SERS_5:
						if (!savenames[field - SERS_0][0])
						{
							Sound(1000,12);
							break;
						}
						saveslot = field - SERS_0;
						DrawRadios(&serslotsg);
						break;

					//
					// Deathmatch
					//
					case SERS_DEATHNO:
					case SERS_DEATHYES:
						sinfo.deathMatch = field - SERS_DEATHNO;
						DrawRadios(&sersdeathg);

					default:
						break;
				}
            break;
      }
   }
  
   func_exit:
  
	RestoreScreen();
	return ( rval );
}

