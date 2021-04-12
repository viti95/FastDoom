//
// Enter network config info
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

#ifndef DOOM2
#define	NET_EPY	3			// Y-coord for Episode group
#define	NET_NPY	7			// Number of players
#define	NET_SKY  11			// Skill level
#define	NET_DMY	17			// Deathmatch
#define	NET_STY  20			// Socket value
#else
#define	NET_NPY	5
#define	NET_SKY  9
#define	NET_DMY	15
#define	NET_STY  18
#endif

#ifndef DOOM2
// # of players radio group
radio_t netepi[]=
{
	{28,NET_EPY,1},
	{28,NET_EPY+1,2},
	{28,NET_EPY+2,3}
};
radiogroup_t netepig={&netepi[0],3,&info.episode,15,1};
#endif

// # of players radio group
radio_t netp[]=
{
	{28,NET_NPY,2},
	{28,NET_NPY+1,3},
	{28,NET_NPY+2,4}
};
radiogroup_t netpg={&netp[0],3,&info.numberOfPlayers,15,1};

// SKILL radio group
radio_t netskill[]=
{
	{28,NET_SKY,1},
	{28,NET_SKY+1,2},
	{28,NET_SKY+2,3},
	{28,NET_SKY+3,4},
	{28,NET_SKY+4,5}
};
radiogroup_t netskillg={&netskill[0],5,&info.skillLevel,15,1};

// DEATHMATCH radio group
radio_t netdeath[]=
{
	{28,NET_DMY,0},
	{28,NET_DMY+1,1}
};
radiogroup_t netdeathg={&netdeath[0],2,&info.deathMatch,15,1};

enum
{
	#ifndef DOOM2
	NET_EPISODE0, NET_EPISODE1, NET_EPISODE2,
	#endif
	NET_P2, NET_P3, NET_P4,
	NET_SKILL1, NET_SKILL2, NET_SKILL3, NET_SKILL4, NET_SKILL5,
	NET_DEATHNO, NET_DEATHYES,
	NET_SOCKET,
	NET_MAX
};
item_t netwkitems[]=
{
#ifndef DOOM2
	{NET_EPISODE0,	31,NET_EPY,21,		-1,-1},
	{NET_EPISODE1,	31,NET_EPY+1,21,	-1,-1},
	{NET_EPISODE2,	31,NET_EPY+2,21,	-1,-1},
#endif
	{NET_P2,			31,NET_NPY,21,		-1,-1},
	{NET_P3,			31,NET_NPY+1,21,	-1,-1},
	{NET_P4,			31,NET_NPY+2,21,	-1,-1},

	{NET_SKILL1,	31,NET_SKY,21,		-1,-1},
	{NET_SKILL2,	31,NET_SKY+1,21,	-1,-1},
	{NET_SKILL3,	31,NET_SKY+2,21,	-1,-1},
	{NET_SKILL4,	31,NET_SKY+3,21,	-1,-1},
	{NET_SKILL5,	31,NET_SKY+4,21,	-1,-1},

	{NET_DEATHNO,	31,NET_DMY,20,		-1,-1},
	{NET_DEATHYES,	31,NET_DMY+1,20,	-1,-1},

	{NET_SOCKET,	28,NET_STY,12,		-1,-1},
};
menu_t netwkmenu=
{
	&netwkitems[0],
	#ifndef DOOM2
	NET_EPISODE0,
	#else
	NET_P2,
	#endif
	NET_MAX,
	0x7f
};

int NetworkConfig(void)
{
	short	key;
	short field;
	int   rval = 0;
	char  *args[MAXARGS];
	int   argcount;
	char  string[MAXARGS*2][12];
	int   i;
	int   level;
	char	tempstring[10];


	info = netinfo;       // save copy of netinfo

	SaveScreen();
	DrawPup(&netwk2);
	//
	// Set defaults
	//
	info.networkSocket = 0;
	textbackground(1);
	textcolor(15);
	Clear(&netwkitems[NET_SOCKET]);
	Pos(&netwkitems[NET_SOCKET]);
	cprintf("%u",info.networkSocket);
	gotoxy(1,25);

	info.numberOfPlayers = 2;
	DrawRadios(&netpg);

	info.skillLevel = 3;
	DrawRadios(&netskillg);

	info.episode = 1;
	#ifndef DOOM2
	DrawRadios(&netepig);
	#endif

	info.deathMatch = 0;
	DrawRadios(&netdeathg);

	while(1)
	{
		SetupMenu(&netwkmenu);
		field = GetMenuInput();
		key = menukey;
		switch ( key )
		{
			case KEY_ESC:
				rval = -1;
				goto func_exit;

			//
			// WARP
			//
			case KEY_F1:
				{
					level = WarpTime();
					if (!level)
						continue;

					#pragma warn -cln
					if (info.networkSocket > 64000)
					#pragma warn +cln
					{
						sound(100);
						delay(3);
						nosound();
						textcolor(0);
						textbackground(7);
						gotoxy(2,25);
						cprintf("Invalid port socket value! Re-enter it.");
						netwkmenu.startitem = NET_SOCKET;
						gotoxy(1,25);
						goto func_exit;
					}

					netinfo = info;

					M_SaveDefaults();
					RestoreScreen();

					argcount = 1;

					args[0] = "ipxsetup.exe";

					if (cdrom)
						args[argcount++] = "-cdrom";

					args[argcount++] = "-nodes";
					sprintf(string[argcount],"%d",netinfo.numberOfPlayers);
					args[argcount] = string[argcount];
					argcount++;

					args[argcount++] = "-skill";
					sprintf(string[argcount],"%d",netinfo.skillLevel);
					args[argcount] = string[argcount];
					argcount++;


					if (!level)
               {
                   #ifndef DOOM2
                   args[argcount++] = "-episode";
                   sprintf(string[argcount],"%d",netinfo.episode);
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


					if (netinfo.networkSocket)
               {
                  args[argcount++] = "-port";
                  sprintf(string[argcount],"%u",netinfo.networkSocket);
						args[argcount] = string[argcount];
						argcount++;
					}

               if (netinfo.deathMatch)
                  args[argcount++] = "-deathmatch";

               if (nomonsters)
						args[argcount++] = "-nomonsters";

               if (respawn)
                  args[argcount++] = "-respawn";

               if (deathmatch2)
                  args[argcount++] = "-altdeath";

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
				}
				break;

exitandsave:
			case KEY_F10:

				#pragma warn -cln
				if (info.networkSocket > 64000)
				#pragma warn +cln
				{
					sound(100);
					delay(3);
					nosound();
					textcolor(0);
					textbackground(7);
					gotoxy(2,25);
					cprintf("Invalid port socket value! Re-enter it.");
					gotoxy(1,25);
					netwkmenu.startitem = NET_SOCKET;
					break;
            }

            netinfo = info;
            
				M_SaveDefaults();
				RestoreScreen();

            argcount = 1;

            args[0] = "ipxsetup.exe";

            if (cdrom)
               args[argcount++] = "-cdrom";

            args[argcount++] = "-nodes";
            sprintf(string[argcount],"%d",netinfo.numberOfPlayers);
            args[argcount] = string[argcount];
            argcount++;

				args[argcount++] = "-skill";
            sprintf(string[argcount],"%d",netinfo.skillLevel);
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

            #ifndef DOOM2
            args[argcount++] = "-episode";
            sprintf(string[argcount],"%d",netinfo.episode);
				args[argcount] = string[argcount];
            argcount++;
            #endif
            
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
				#ifndef DOOM2
				//
				// Episode
				//
				case NET_EPISODE0:
				case NET_EPISODE1:
				case NET_EPISODE2:
					info.episode = field - NET_EPISODE0 + 1;
					break;
				#endif

				//
				// Number of players
				//
				case NET_P2:
				case NET_P3:
				case NET_P4:
					info.numberOfPlayers = field - NET_P2 + 2;
					break;

				//
				// Skill level
				//
				case NET_SKILL1:
				case NET_SKILL2:
				case NET_SKILL3:
				case NET_SKILL4:
				case NET_SKILL5:
					info.skillLevel = field - NET_SKILL1 + 1; break;

				//
				// Deathmatch
				//
				case NET_DEATHNO:
				case NET_DEATHYES:
					info.deathMatch = field - NET_DEATHNO; break;

				//
				// Network socket #
				//
				case NET_SOCKET:
					ltoa(info.networkSocket,tempstring,10);
					key = EditLine(&netwkitems[NET_SOCKET],tempstring,8);
					if (key == KEY_ENTER)
						info.networkSocket = atoi(tempstring);
					textcolor(15);
					textbackground(1);
					Clear(&netwkitems[NET_SOCKET]);
					Pos(&netwkitems[NET_SOCKET]);
					cprintf("%u",info.networkSocket);
					gotoxy(1,25);
					break;

				default:
					break;
			}
				#ifndef DOOM2
				DrawRadios(&netepig);
				#endif
				DrawRadios(&netpg);
				DrawRadios(&netskillg);
				DrawRadios(&netdeathg);
				break;
		}
	}

	func_exit:

	RestoreScreen();
	return ( rval );
}

