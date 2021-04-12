//
// Enter modem savegame info
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

enum {MODS_0,MODS_1,MODS_2,MODS_3,MODS_4,MODS_5,
	MODS_DEATHNO,MODS_DEATHYES,
	MODS_COM1,MODS_COM2,MODS_COM3,MODS_COM4,
	MODS_CONN1,MODS_CONN2,MODS_CONN3,
	MODS_MAX};
item_t modsaveitems[]=
{
	{MODS_0,		29,4,24,		-1,-1},
	{MODS_1,		29,5,24,		-1,-1},
	{MODS_2,		29,6,24,		-1,-1},
	{MODS_3,		29,7,24,	-1,-1},
	{MODS_4,		29,8,24,	-1,-1},
	{MODS_5,		29,9,24,	-1,-1},

	{MODS_DEATHNO,	29,11,24,	-1,-1},
	{MODS_DEATHYES,29,12,24,	-1,-1},

	{MODS_COM1,	29,14,4,		-1,MODS_COM3},
	{MODS_COM2,	29,15,4,		-1,MODS_COM4,0,MODS_CONN1},
	{MODS_COM3,	46,14,4,		MODS_COM1,-1,MODS_DEATHYES},
	{MODS_COM4,	46,15,4,		MODS_COM2,-1},

	{MODS_CONN1,	29,17,20,	-1,-1,MODS_COM2},
	{MODS_CONN2,	29,18,20,	-1,-1},
	{MODS_CONN3,	29,19,7,		-1,-1}
};
menu_t modsavemenu=
{
	&modsaveitems[0],
	MODS_0,
	MODS_MAX,
	0x7f
};

// SAVEGAME radio group
radio_t modslots[]=
{
	{26,4,0},
	{26,5,1},
	{26,6,2},
	{26,7,3},
	{26,8,4},
	{26,9,5}
};
radiogroup_t modslotsg=	{ &modslots[0],6,&saveslot,15,1 };

// DEATHMATCH radio group
radio_t modsdeath[]=
{
	{26,11,0},
	{26,12,1}
};
radiogroup_t modsdeathg=	{ &modsdeath[0],2,&minfo.deathMatch,15,1 };

// COMPORT radio group
radio_t modscom[]=
{
	{26,14,1},
	{26,15,2},
	{43,14,3},
	{43,15,4}
};
radiogroup_t modscomg=	{ &modscom[0],4,&minfo.comport,15,1 };

// CONNECT TYPE radio group
radio_t modsconn[]=
{
	{26,17,0},
	{26,18,1},
	{26,19,2}
};
radiogroup_t modsconng=	{ &modsconn[0],3,&minfo.comtype,15,1 };

int RestoreModem(void)
{
	short field;
	short key;
	int   rval = 0;
	char  *args[MAXARGS];
	int   argcount;
	char  string[MAXARGS*2][16];
	int   i;
	int   numplayers[6];
	int   handle;
	char  name[32];
	char  p1,p2,p3,p4;
	item_t phonenum = {2,37,19,13,-1,-1};
	char	tempstring[16];

	minfo = modeminfo;       // save copy of netinfo

	SaveScreen();
	DrawPup(&modsave);

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

		Clear(&modsaveitems[i]);
		Pos(&modsaveitems[i]);
		cprintf("%s",savenames[i]);
	}
  	                 	
   //
   // Set defaults
   //
	minfo.comtype = 1;
	DrawRadios(&modsconng);

	minfo.comport = comport;
	DrawRadios(&modscomg);
   
	minfo.deathMatch = 0;
	DrawRadios(&modsdeathg);

   strcpy(minfo.phonenum,"");
	gotoxy(1,25);

	while(1)
	{
		SetupMenu(&modsavemenu);
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

				modeminfo = minfo;

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

				sprintf(string[argcount],"-com%d",modeminfo.comport);
            args[argcount] = string[argcount];
				argcount++;
            
            switch(modeminfo.comtype)
            {
               case 0:     // no parameter if already connected!
                  break;
					case 1:
                  args[argcount++] = "-answer";
                  break;
               case 2:
                  args[argcount++] = "-dial";
                  sprintf(string[argcount],"%s",minfo.phonenum);
                  args[argcount] = string[argcount];
                  argcount++;
                  break;
				}

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

			case KEY_F2:
				if (ChooseOpponentInList())
				{
					strcpy(minfo.phonenum,chosenPhoneNum);
					textcolor(15);
					textbackground(0);
					Clear(&phonenum);
					Pos(&phonenum);
					cprintf("%s",minfo.phonenum);
					minfo.comtype = 2;
					DrawRadios(&modsconng);
					gotoxy(1,25);
				 }
				 break;

			case KEY_ENTER:
				switch ( field )
				{
					case MODS_COM1:
					case MODS_COM2:
					case MODS_COM3:
					case MODS_COM4:
						minfo.comport = field - MODS_COM1 + 1;
						DrawRadios(&modscomg);
						break;

					case MODS_CONN1:
					case MODS_CONN2:
						minfo.comtype = field - MODS_CONN1;
						DrawRadios(&modsconng);
						break;
					case MODS_CONN3:
						minfo.comtype = 2;
						DrawRadios(&modsconng);
						key = EditLine(&phonenum,tempstring,12);
						if (key == KEY_ENTER)
							strcpy(minfo.phonenum,tempstring);
						gotoxy(1,25);
						break;

					case MODS_0:
					case MODS_1:
					case MODS_2:
					case MODS_3:
					case MODS_4:
					case MODS_5:
						if (!savenames[field - MODS_0][0])
						{
							sound(1000);
							delay(12);
							nosound();
							break;
						}
						saveslot = field - MODS_0;
						DrawRadios(&modslotsg);
						break;

					//
					// Deathmatch
					//
					case MODS_DEATHNO:
					case MODS_DEATHYES:
						minfo.deathMatch = field - MODS_DEATHNO;
						DrawRadios(&modsdeathg);
						break;

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

