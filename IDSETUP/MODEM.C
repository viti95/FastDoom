//
// Enter modem config info
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

//
// Choose who to call for modem play
//

phonelist_t phonelist[MAXPHNLIST];
int   numInList;
char  chosenPhoneNum[16];

//
// Parse the MODOOM.NUM file
//
int ParseMODOOMList(void)
{
   int   i;
   int   ok;
   FILE  *fp;
   int   rv;
   int   found;


   fp = fopen("MODEM.NUM","rt");
   if (fp == NULL)
   {
		gotoxy(2,25);
		printf("There's no MODEM.NUM file!  Press a key.                     ");
		sound(2500);
		delay(3);
		nosound();
		getch();
      return 0;
   }

   i = 0;
   ok = 1;
	do
   {
      rv = fscanf(fp,"%[^\n]",&phonelist[i].name);
      if (!rv || rv == EOF)
         ok = 0;
      
      rv = fscanf(fp,"\n");
      if (rv == EOF)
         ok = 0;
      
      rv = fscanf(fp,"%[^\n]",&phonelist[i].number);
		if (!rv || rv == EOF)
			ok = 0;

      rv = fscanf(fp,"\n\n");
      if (rv == EOF)
         ok = 0;
      i++;

   } while(ok);

   numInList = i;
   fclose(fp);


   //
   // Quick little shell sort
   //
   do
	{
		int   j;
      found = 0;

      for (i = 0; i < numInList-1; i++)
         for (j = i+1; j < numInList; j++)
            if (strcmp(phonelist[j].name,phonelist[i].name)<0)
            {
               phonelist_t  temp;

               temp = phonelist[j];
               phonelist[j] = phonelist[i];
					phonelist[i] = temp;
               found = 1;
            }
   } while(found);

   return 1;
}

//
// Blit the phone numbers to the screen
//
void BlitPhoneNumbers(int start)
{
   int   i;
   int   max;
   int   y;
   int   j;
   int   len;

   max = start + 10;
   if (max > numInList)
      max = numInList;

	y = 8;
	for (i = start; i < max; i++)
	{
		gotoxy(17,y);
		cprintf("%s",phonelist[i].name);
		len = 32 - strlen(phonelist[i].name);
		for (j = 0; j < len; j++)
			cprintf(" ");

		gotoxy(49,y++);
		cprintf("%s",phonelist[i].number);
		len = 16 - strlen(phonelist[i].number);
		for (j = 0; j < len; j++)
         cprintf(" ");
   }
	gotoxy(1,25);
}

//
// Choose who to call
//
enum {ADDNAME,ADDNUMBER,MAXADD};
item_t additems[]= {
	{ADDNAME,	17,12,28,-1,ADDNUMBER},
	{ADDNUMBER,	46,12,18,ADDNAME,-1}};
menu_t addmenu = {&additems[0],ADDNAME,MAXADD,0x7f};

int ChooseOpponentInList(void)
{
	int   i;
	int   xit;
	int   blitY;
	int   hliteY;
	int   lastHliteY;
	int   index;

	chosenPhoneNum[0] = 0;

	if (!ParseMODOOMList())
		return 0;

	SaveScreen();
	DrawPup(&phonelst);
	textcolor(11);
	textbackground(1);
	gotoxy(1,25);

	xit = 0;
	blitY = 0;
	lastHliteY = hliteY = 0;
	do
	{
		int   key;

		textcolor(11);
		textbackground(1);
		BlitPhoneNumbers(blitY);

		AttriBar(16,lastHliteY+7,48,0x1b);
		lastHliteY = hliteY;
		AttriBar(16,hliteY+7,48,0x7f);
		key = _bios_keybrd(_NKEYBRD_READ)>>8;

		switch(key)
		{
         case 0x1c:           // ENTER
				index = blitY + hliteY;
            for (i=0;i<strlen(phonelist[index].number);i++)
               if ((phonelist[index].number[i] >= '0' &&
                   phonelist[index].number[i] <= '9') ||
                   phonelist[index].number[i] == '#' ||
                   phonelist[index].number[i] == '*' ||
                   phonelist[index].number[i] == ',')
               {
                  char num[2];
                  num[0] = phonelist[index].number[i];
                  num[1] = 0;
                  strcat(chosenPhoneNum,num);
               }

            xit = 1;
				break;

         case SC_ESC:         // ESC
            xit = 1;
            break;

			case 0x48:           // UP
				if (hliteY > 5 ||
               (!blitY && hliteY))
               hliteY--;
            else
            if (blitY)
					blitY--;
				break;

			case 0x50:           // DOWN
				if (hliteY > 4 && blitY+10<numInList)
					blitY++;
				else
				if (hliteY < 9 &&
					((blitY+10 == numInList) ||
					(hliteY < numInList - 1)))
					hliteY++;

				break;

			case 0x49:           // PGUP
				blitY -= 9;
				if (blitY < 0)
					hliteY = blitY = 0;

				break;

			case 0x51:           // PGDN
				blitY += 9;
				if (blitY+10 > numInList)
				{
					blitY = numInList - 10;
					hliteY = 9;
				}
				if (numInList < 10)
				{
					blitY = 0;
					hliteY = numInList - 1;
				}
				break;
		}

	} while(!xit);

	RestoreScreen();
	return chosenPhoneNum[0];
}

#ifndef DOOM2
//
// Epsiode radio buttons
//
radio_t mepi[]=
{
	{28,3,1},
	{28,4,2},
	{28,5,3}
};
radiogroup_t mepig=
{	&mepi[0],3,&minfo.episode,15,1	};

//
// Skill level radio buttons
//
radio_t mskill[]=
{
	{28,7,2},
	{28,8,3},
	{28,9,4},
	{28,10,5}
};
radiogroup_t mskillg=
{	&mskill[0],4,&minfo.skillLevel,15,1	};

//
// Mode radio buttons
//
radio_t mdeath[]=
{
	{28,12,0},
	{28,13,1}
};
radiogroup_t mdeathg=
{	&mdeath[0],2,&minfo.deathMatch,15,1	};

//
// Com port radio buttons
//
radio_t mcomport[]=
{
	{28,15,1},
	{28,16,2},
	{42,15,3},
	{42,16,4}
};
radiogroup_t mcomportg=
{	&mcomport[0],4,&minfo.comport,15,1	};

//
// Connection-type radio buttons
//
radio_t mcomtype[]=
{
	{28,18,0},
	{28,19,1},
	{28,20,2}
};
radiogroup_t mcomtypeg=
{	&mcomtype[0],3,&minfo.comtype,15,1	};
#else
//
// Skill level radio buttons
//
radio_t mskill[]=
{
	{28,5,2},
	{28,6,3},
	{28,7,4},
	{28,8,5},
};
radiogroup_t mskillg=
{	&mskill[0],4,&minfo.skillLevel,15,1	};

//
// Mode radio buttons
//
radio_t mdeath[]=
{
	{28,10,0},
	{28,11,1}
};
radiogroup_t mdeathg=
{	&mdeath[0],2,&minfo.deathMatch,15,1	};

//
// Com port radio buttons
//
radio_t mcomport[]=
{
	{28,13,1},
	{28,14,2},
	{42,13,3},
	{42,14,4}
};
radiogroup_t mcomportg=
{	&mcomport[0],4,&minfo.comport,15,1	};

//
// Connection-type radio buttons
//
radio_t mcomtype[]=
{
	{28,16,0},
	{28,17,1},
	{28,18,2}
};
radiogroup_t mcomtypeg=
{	&mcomtype[0],3,&minfo.comtype,15,1	};
#endif

//
// Menu info
//
enum
{
#ifndef DOOM2
	MOD_EPISODE0, MOD_EPISODE1, MOD_EPISODE2,
#endif
	MOD_SKILL1, MOD_SKILL2, MOD_SKILL3, MOD_SKILL4,
	MOD_DEATHNO, MOD_DEATHYES,
	MOD_COM1, MOD_COM2, MOD_COM3, MOD_COM4,
	MOD_CONN0, MOD_CONN1, MOD_CONN2,
	MOD_MAX
};
item_t cmodemitems[]=
{
#ifndef DOOM2
	{MOD_EPISODE0,	31,3,21,	-1,-1},
	{MOD_EPISODE1,	31,4,21,	-1,-1},
	{MOD_EPISODE2,	31,5,21,	-1,-1},

	{MOD_SKILL1,	31,7,21,	-1,-1},
	{MOD_SKILL2,	31,8,21,	-1,-1},
	{MOD_SKILL3,	31,9,21,	-1,-1},
	{MOD_SKILL4,	31,10,21,	-1,-1},

	{MOD_DEATHNO,	31,12,20,-1,-1},
	{MOD_DEATHYES,	31,13,20,-1,-1},

	{MOD_COM1,		31,15,4,	-1,		MOD_COM3},
	{MOD_COM2,		31,16,4,	-1,		MOD_COM4,0,MOD_CONN0},
	{MOD_COM3,		45,15,4,MOD_COM1,	-1,MOD_DEATHYES},
	{MOD_COM4,		45,16,4,MOD_COM2,	-1},

	{MOD_CONN0,		31,18,20,	-1,-1,MOD_COM2},
	{MOD_CONN1,		31,19,20,	-1,-1},
	{MOD_CONN2,		31,20,7,		-1,-1}
#else
	{MOD_SKILL1,	31,5,20,	-1,-1},
	{MOD_SKILL2,	31,6,20,	-1,-1},
	{MOD_SKILL3,	31,7,20,	-1,-1},
	{MOD_SKILL4,	31,8,20,	-1,-1},

	{MOD_DEATHNO,	31,10,20,-1,-1},
	{MOD_DEATHYES,	31,11,20,-1,-1},

	{MOD_COM1,		31,13,4,	-1,		MOD_COM3},
	{MOD_COM2,		31,14,4,	-1,		MOD_COM4,0,MOD_CONN0},
	{MOD_COM3,		45,13,4,MOD_COM1,	-1,MOD_DEATHYES},
	{MOD_COM4,		45,14,4,MOD_COM2,	-1},

	{MOD_CONN0,		31,16,20,	-1,-1,MOD_COM2},
	{MOD_CONN1,		31,17,20,	-1,-1},
	{MOD_CONN2,		31,18,7,		-1,-1}
#endif
};
menu_t cmodemmenu=
{
	&cmodemitems[0],
#ifndef DOOM2
	MOD_EPISODE0,
#else
	MOD_SKILL1,
#endif
	MOD_MAX,
	0x7f
};

int ModemConfig(void)
{
	short field;
	short	key;
	int   rval = 0;
	char  *args[MAXARGS];
	int   argcount;
	char  string[MAXARGS*2][12];
	int   i;
	int   level;
#ifndef DOOM2
	item_t phonenum = {2,39,20,13,-1,-1};
#else
	item_t phonenum = {2,39,18,13,-1,-1};
#endif
	char	tempstring[16];


	minfo = modeminfo;       // save copy of modeminfo

	SaveScreen();
	DrawPup(&cmodem);

	//
	// Set defaults
	//
	minfo.skillLevel = 3;
	DrawRadios(&mskillg);

	minfo.episode = 1;
	#ifndef DOOM2
	DrawRadios(&mepig);
	#endif

	minfo.deathMatch = 1;
	DrawRadios(&mdeathg);

	minfo.comport = comport;
	DrawRadios(&mcomportg);

	minfo.comtype = 1;
	DrawRadios(&mcomtypeg);

	strcpy(minfo.phonenum,"");

	while(1)
	{
		SetupMenu(&cmodemmenu);
		field = GetMenuInput();
		key = menukey;
		switch ( key )
		{
			case KEY_ESC:
				rval = -1;
				goto func_exit;

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
					DrawRadios(&mcomtypeg);
					gotoxy(1,25);
				 }
				 break;

			//
			// WARP
			//
			case KEY_F1:
				{
					level = WarpTime();
					if (!level)
						continue;

					modeminfo = minfo;

					M_SaveDefaults();
					RestoreScreen();

					argcount = 1;

					args[0] = "sersetup.exe ";

					if (cdrom)
						args[argcount++] = "-cdrom";

					args[argcount++] = "-skill";
					sprintf(string[argcount],"%d",modeminfo.skillLevel);
					args[argcount] = string[argcount];
					argcount++;

					if (!level)
					{
						#ifndef DOOM2
						 args[argcount++] = "-episode";
						 sprintf(string[argcount],"%d",modeminfo.episode);
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
						argcount++;
						#else
						sprintf(string[argcount],"%d",level>>8);
						args[argcount] = string[argcount];
						argcount++;
						sprintf(string[argcount],"%d",level&0x0f);
                  args[argcount] = string[argcount];
						argcount++;
                  #endif
               }

               if (modeminfo.deathMatch)
                  args[argcount++] = "-deathmatch";
            
					if (nomonsters)
                  args[argcount++] = "-nomonsters";

               if (respawn)
                  args[argcount++] = "-respawn";

					if (deathmatch2 && modeminfo.deathMatch)
                  args[argcount++] = "-altdeath";

               sprintf(string[argcount],"-com%d",modeminfo.comport);
               args[argcount] = string[argcount];
               argcount++;

               //
               // IF # IS ENTERED, CALL #
               //
					if (minfo.phonenum[0])
                  modeminfo.comtype = 2;

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

				modeminfo = minfo;

				M_SaveDefaults();
				RestoreScreen();

				argcount = 1;

				args[0] = "sersetup.exe ";

					if (cdrom)
						args[argcount++] = "-cdrom";

				args[argcount++] = "-skill";
				sprintf(string[argcount],"%d",modeminfo.skillLevel);
				args[argcount] = string[argcount];
				argcount++;

				if (modeminfo.deathMatch)
					args[argcount++] = "-deathmatch";

				#ifndef DOOM2
				args[argcount++] = "-episode";
				sprintf(string[argcount],"%d",modeminfo.episode);
				args[argcount] = string[argcount];
				argcount++;
				#endif

				sprintf(string[argcount],"-com%d",modeminfo.comport);
				args[argcount] = string[argcount];
				argcount++;

				//
				// IF # IS ENTERED, CALL #
				//
				if (minfo.phonenum[0])
					modeminfo.comtype = 2;

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
				switch ( field )
				{
					#ifndef DOOM2
					//
					// Episode
					//
					case MOD_EPISODE0:
					case MOD_EPISODE1:
					case MOD_EPISODE2:
						minfo.episode = field - MOD_EPISODE0 + 1;
						break;
					#endif

					//
					// Skill level
					//
					case MOD_SKILL1:
					case MOD_SKILL2:
					case MOD_SKILL3:
					case MOD_SKILL4:
						minfo.skillLevel = field - MOD_SKILL1 + 2;
						break;

					//
					// Deathmatch
					//
					case MOD_DEATHNO:
					case MOD_DEATHYES:
						minfo.deathMatch = field - MOD_DEATHNO;
						break;

					//
					// COM port
					//
					case MOD_COM1:
					case MOD_COM2:
					case MOD_COM3:
					case MOD_COM4:
						comport = minfo.comport = field - MOD_COM1 + 1;
						break;

					//
					// Connection type
					//
					case MOD_CONN0:
					case MOD_CONN1:
						minfo.comtype = field - MOD_CONN0;
						break;
					case MOD_CONN2:
						minfo.comtype = 2;
						key = EditLine(&phonenum,tempstring,12);
						if (key == KEY_ENTER)
							strcpy(minfo.phonenum,tempstring);
						gotoxy(1,25);
						break;

					default:
						break;
				}
				#ifndef DOOM2
				DrawRadios(&mepig);
				#endif
				DrawRadios(&mskillg);
				DrawRadios(&mdeathg);
				DrawRadios(&mcomportg);
				DrawRadios(&mcomtypeg);
				break;
		}
	}

	func_exit:

	RestoreScreen();
	return ( rval );
}

