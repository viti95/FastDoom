#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <bios.h>

#include "main.h"

//
// Choose which type of modem to use
//
modem_t  modems[MAXMODEMS];
int      numModems;

//
// Draw a bar of attributes
//
void AttriBar(int x,int y,int length, char attr)
{
	int   i;
	char  far   *screen;

	screen = MK_FP(0xb800, y*160 + x*2 + 1);

	for (i = 0;i < length;i++)
	{
		*screen = attr;
		screen += 2;
	}
}


//
// Parse the MODOOM.STR file
//
int ParseMODOOM(void)
{
	int   i;
	int   ok;
	FILE  *fp;
	int   rv;
	int   found;


	fp = fopen("MODEM.STR","rt");
	if (fp == NULL)
	{
		gotoxy(2,25);
		cprintf("There's no MODEM.STR file!  Press a key.                      ");
		gotoxy(1,25);
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
		rv = fscanf(fp,"%[^\n]",&modems[i].name);
		if (!rv || rv == EOF)
			ok = 0;
      rv = fscanf(fp,"\n");
      if (rv == EOF)
         ok = 0;

      
      rv = fscanf(fp,"%[^\n]",&modems[i].init);
      if (!rv || rv == EOF)
         ok = 0;
		rv = fscanf(fp,"\n");
      if (rv == EOF)
         ok = 0;


      rv = fscanf(fp,"%[^\n]",&modems[i].hangup);
      if (!rv || rv == EOF)
			ok = 0;
      rv = fscanf(fp,"\n");
      if (rv == EOF)
			ok = 0;
      

      rv = fscanf(fp,"%[^\n]",&modems[i].baud);
      if (!rv || rv == EOF)
			ok = 0;
      rv = fscanf(fp,"\n\n");
		if (rv == EOF)
         ok = 0;
      
      
      i++;

   } while(ok);

   numModems = i;
   fclose(fp);


   //
   // Quick little shell sort
   //
   do
   {
		int   j;
      found = 0;

      for (i = 0; i < numModems-1; i++)
         for (j = i+1; j < numModems; j++)
            if (strcmp(modems[j].name,modems[i].name)<0)
            {
               modem_t  temp;

               temp = modems[j];
               modems[j] = modems[i];
               modems[i] = temp;
               found = 1;
            }
   } while(found);

   return 1;
}

//
// Blit the modem names to the screen
//
void BlitModemNames(int start)
{
	int   i;
	int   max;
	int   y;
	int   j;
	int   len;

	max = start + 10;
	if (max > numModems)
		max = numModems;


	y = 8;
	textbackground(1);
	textcolor(11);
	for (i = start; i < max; i++)
	{
		gotoxy(27,y++);
		cprintf("%s",modems[i].name);
		len = 27 - strlen(modems[i].name);
		for (j = 0; j < len; j++)
			cprintf(" ");
	}
	gotoxy(1,25);

}

void ChooseModem(void)
{
	int   xit;
	int   blitY;
	int   hliteY;
	int   lastHliteY;
	FILE  *fp;

	if (!ParseMODOOM())
		return;

	SaveScreen();
	DrawPup(&modemchs);

	textbackground(7);
	textcolor(0);

	xit = 0;
	blitY = 0;
	lastHliteY = hliteY = 0;
	do
	{
		int   key;

		BlitModemNames(blitY);

		textbackground(7);
		textcolor(0);
		gotoxy(70,25);
		cprintf("%s  ",modems[hliteY + blitY].baud);
		gotoxy(1,25);

		AttriBar(26,lastHliteY+7,27,0x1b);
		lastHliteY = hliteY;
		AttriBar(26,hliteY+7,27,0x7f);
		key = _bios_keybrd(_NKEYBRD_READ)>>8;

		switch(key)
		{
			case 0x1c:           // ENTER
				gotoxy(2,25);
				cprintf("Writing %s to MODEM.CFG...",modems[blitY + hliteY].name);
				gotoxy(1,25);

				fp = fopen("MODEM.CFG","w+t");
				if (fp == NULL)
				{
					gotoxy(2,25);
					cprintf("Error writing to MODEM.CFG. Press a key.");
					gotoxy(1,25);
					getch();
					break;
				}

				fprintf(fp,"%s\n%s\n%s\n%s\n\n",
						modems[blitY + hliteY].init,
						modems[blitY + hliteY].hangup,
						modems[blitY + hliteY].baud,
						modems[blitY + hliteY].name);

				fclose(fp);
				xit = 1;
				break;

			case SC_ESC:         // ESC
				xit = 1;
				break;

			case 0x48:           // UP
				if (hliteY > 5)
					hliteY--;
				else
				if (hliteY > 0 && blitY > 0)
					blitY--;
				else
				if (!blitY && hliteY)
					hliteY--;
				else
				if (!hliteY && blitY)
					blitY--;
				break;

			case 0x50:           // DOWN
				if (hliteY > 4 && blitY+10<numModems)
					blitY++;
				else
				if (hliteY < 9 &&
					((blitY+10 == numModems) ||
					(hliteY < numModems - 1)))
					hliteY++;
				break;

			case 0x49:           // PGUP
				blitY -= 9;
				if (blitY < 0)
					hliteY = blitY = 0;

				break;

			case 0x51:           // PGDN
				blitY += 9;
				if (blitY+10 > numModems)
            {
               blitY = numModems - 10;
               hliteY = 9;
            }
            if (numModems < 10)
            {
               blitY = 0;
               hliteY = numModems - 1;
            }
            break;
      }

   } while(!xit);
   
	RestoreScreen();
}


//
// Choose which type of netplay
//
enum {GT_IPX,GT_MODEM,GT_SERIAL,GT_CHOOSE,GT_MACROS,GT_MAX};
item_t netplayitems[]=
{
	{GT_IPX,		26,9,27,		-1,-1},
	{GT_MODEM,	26,10,27,	-1,-1},
	{GT_SERIAL,	26,11,27,	-1,-1},

	{GT_CHOOSE,	26,13,27,	-1,-1},
	{GT_MACROS,	26,14,27,	-1,-1}
};
menu_t netplaymenu=
{
	&netplayitems[0],
	GT_IPX,
	GT_MAX,
	0x7f
};

void ChooseNetplay(void)
{
	short		key;
	short    field;

	SaveScreen();
	DrawPup(&netplay);

	while(1)
	{
		SetupMenu(&netplaymenu);
		field = GetMenuInput();
		key = menukey;

		switch(key)
		{
			case KEY_ESC:
				RestoreScreen();
				return;

			case KEY_ENTER:
				if (field == GT_CHOOSE)
				{
					ChooseModem();
					continue;
				}

				switch(field)
				{
					case GT_IPX:
						NetworkConfig();
						break;
					case GT_MODEM:
						ModemConfig();
						break;
					case GT_SERIAL:
						SerialConfig();
						break;
					case GT_MACROS:
						MacroConfig();
						break;
				}
				RestoreScreen();
				return;
		}
	}
}

