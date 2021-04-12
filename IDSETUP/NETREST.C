//
// Enter network savegame info
//
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

char  savenames[6][24];
int   saveslot;

enum {NETS_0,NETS_1,NETS_2,NETS_3,NETS_4,NETS_5,
	NETS_DEATHNO,NETS_DEATHYES,
	NETS_SOCKET,NETS_MAX};
item_t netsaveitems[]=
{
	{NETS_0,		29,7,24,		-1,-1},
	{NETS_1,		29,8,24,		-1,-1},
	{NETS_2,		29,9,24,		-1,-1},
	{NETS_3,		29,10,24,	-1,-1},
	{NETS_4,		29,11,24,	-1,-1},
	{NETS_5,		29,12,24,	-1,-1},

	{NETS_DEATHNO,	29,14,24,	-1,-1},
	{NETS_DEATHYES,29,15,24,	-1,-1},

	{NETS_SOCKET,	26,17,16,	-1,-1}
};
menu_t netsavemenu=
{
	&netsaveitems[0],
	NETS_0,
	NETS_MAX,
	0x7f
};

// SAVEGAME radio group
radio_t netslots[]=
{
	{26,7,0},
	{26,8,1},
	{26,9,2},
	{26,10,3},
	{26,11,4},
	{26,12,5}
};
radiogroup_t netslotsg=	{ &netslots[0],6,&saveslot,15,1 };

// DEATHMATCH radio group
radio_t netsdeath[]=
{
	{26,14,0},
	{26,15,1}
};
radiogroup_t netsdeathg=	{ &netsdeath[0],2,&info.deathMatch,15,1 };

int RestoreNetwork(void)
{
	short field;
	short	key;
	int   rval = 0;
	char  *args[MAXARGS];
	char  string[MAXARGS*2][16];
	int   argcount;
	int   i;
	int   numplayers[6];
	int   handle;
	char  name[32];
	char  p1,p2,p3,p4;
	char	tempstring[10];


	info = netinfo;       // save copy of netinfo

	SaveScreen();
	DrawPup(&netsave);

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

		Clear(&netsaveitems[i]);
		Pos(&netsaveitems[i]);
		cprintf("%s",savenames[i]);
	}

	//
	// Set defaults
	//
	info.networkSocket = 0;
	Clear(&netsaveitems[NETS_SOCKET]);
	Pos(&netsaveitems[NETS_SOCKET]);
	cprintf("%u",info.networkSocket);
	gotoxy(1,25);

	info.deathMatch = 0;
	DrawRadios(&netsdeathg);

	while(1)
	{
		SetupMenu(&netsavemenu);
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

				#pragma warn -cln
				if (info.networkSocket > 64000)
				#pragma warn +cln
				{
					sound(100);
					delay(6);
					nosound();
					textbackground(7);
					textcolor(0);
					gotoxy(2,25);
					gotoxy(1,25);
					netsavemenu.startitem = NETS_SOCKET;
					break;
            }

            netinfo = info;
            
            M_SaveDefaults();
				RestoreScreen();
				
				argcount = 1;

            args[0] = "ipxsetup.exe ";

               if (cdrom)
						args[argcount++] = "-cdrom";

            args[argcount++] = "-nodes";
            sprintf(string[argcount],"%d",numplayers[saveslot]);
            args[argcount] = string[argcount];
            argcount++;

				if (netinfo.networkSocket)
            {
               args[argcount++] = "-port";
               sprintf(string[argcount],"%u",netinfo.networkSocket);
               args[argcount] = string[argcount];
					argcount++;
            }

            if (netinfo.deathMatch)
               args[argcount++] = "-deathmatch";

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

            execv("ipxsetup.exe",args);

            //
            // ERROR EXECing!
            //
            printf("Problem EXECing IPXSETUP for netplay. Need to be in same directory!");
            exit(0);

			case KEY_ENTER:
				switch ( field )
				{
					case NETS_0:
					case NETS_1:
					case NETS_2:
					case NETS_3:
					case NETS_4:
					case NETS_5:
						if (!savenames[field][0])
						{
							sound(1000);
							delay(12);
							nosound();
							break;
						}
						saveslot = field;
						break;

					//
					// Deathmatch
					//
					case NETS_DEATHNO: info.deathMatch = 0; break;
					case NETS_DEATHYES: info.deathMatch = 1; break;

					//
					// Network socket #
					//
					case NETS_SOCKET:
						ltoa(info.networkSocket,tempstring,10);
						key = EditLine(&netsaveitems[NETS_SOCKET],tempstring,8);
						if (key == KEY_ENTER)
							info.networkSocket = atoi(tempstring);
						textcolor(15);
						textbackground(1);
						Clear(&netsaveitems[NETS_SOCKET]);
						Pos(&netsaveitems[NETS_SOCKET]);
						cprintf("%u",info.networkSocket);
						gotoxy(1,25);
						break;
  
					default:
						break;
				}
			DrawRadios(&netslotsg);
			DrawRadios(&netsdeathg);
			break;
		}
	}

	func_exit:

	RestoreScreen();
	return ( rval );
}

//
// Choose which type of netplay for SAVEGAME RESTORE
//
enum {NETCH_IPX,NETCH_MOD,NETCH_SER, NETCH_MAX};
item_t netplay2items[]=
{
	{NETCH_IPX,	27,11,26,	-1,-1},
	{NETCH_MOD,	27,12,26,	-1,-1},
	{NETCH_SER,	27,13,26,	-1,-1}
};
menu_t netplay2menu=
{
	&netplay2items[0],
	NETCH_IPX,
	NETCH_MAX,
	0x7f
};

void ChooseNetrestore(void)
{
	short   key;
	short   field;

	SaveScreen();
	DrawPup(&netplay2);

	while(1)
	{
		SetupMenu(&netplay2menu);
		field = GetMenuInput();
		key = menukey;

		switch(key)
		{
			case KEY_ESC:
				RestoreScreen();
				return;

			case KEY_ENTER:
				RestoreScreen();
				switch(field)
				{
					case NETCH_IPX:
						RestoreNetwork();
						break;
					case NETCH_MOD:
						RestoreModem();
						break;
					case NETCH_SER:
						RestoreSerial();
						break;
				}
				return;
		}
	}
}
