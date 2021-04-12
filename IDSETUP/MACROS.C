//
// Choose which type of netplay
//
#include <dos.h>
#include <conio.h>
#include <string.h>

#include "main.h"

//
// Line input routine -- totally crude!
//
int	EditLine(item_t *item,char *string,int maxlen)
{
	char	c;
	int	len;

	textbackground(0);
	textcolor(15);
	Clear(item);
	Pos(item);
	cprintf("%s",string);
	while(1)
	{
		c = getch();
		switch(c)
		{
			case 8:				// BACKSPACE
			case 0x4b:			// LEFT ARROW
				len = strlen(string);
				if (!len)
				{
					sound(2500);
					delay(3);
					nosound();
					continue;
				}
				string[len-1] = 0;
				Clear(item);
				Pos(item);
				cprintf("%s",string);
			case 77:			// RIGHT ARROW
				sound(2500);
				delay(3);
				nosound();
				break;
			case KEY_ENTER:
			case KEY_ESC:
				return c;
			default:
				if (c < 0x20 || c > 0x7a)
				{
					sound(2500);
					delay(3);
					nosound();
					continue;
				}
				len = strlen(string);
				if (len+1 == maxlen)
				{
					sound(2500);
					delay(3);
					nosound();
					continue;
				}
				string[len] = c;
				string[len+1] = 0;
				Pos(item);
				cprintf("%s",string);
				break;
		}
	}
}

enum {MAC_MACRO0,MAC_MACRO1,MAC_MACRO2,MAC_MACRO3,MAC_MACRO4,MAC_MACRO5,
	MAC_MACRO6,MAC_MACRO7,MAC_MACRO8,MAC_MACRO9,MAC_MAX};
item_t macrositems[]=
{
	{MAC_MACRO0,	22,7,40,		-1,-1},
	{MAC_MACRO1,	22,8,40,		-1,-1},
	{MAC_MACRO2,	22,9,40,		-1,-1},
	{MAC_MACRO3,	22,10,40,	-1,-1},
	{MAC_MACRO4,	22,11,40,	-1,-1},
	{MAC_MACRO5,	22,12,40,	-1,-1},
	{MAC_MACRO6,	22,13,40,	-1,-1},
	{MAC_MACRO7,	22,14,40,	-1,-1},
	{MAC_MACRO8,	22,15,40,	-1,-1},
	{MAC_MACRO9,	22,16,40,	-1,-1}
};
menu_t macrosmenu=
{
	&macrositems[0],
	MAC_MACRO0,
	MAC_MAX,
	0x7f
};

void MacroConfig(void)
{
	short	key;
	short	field;
	int	i;
	char  string[40];

	SaveScreen();
	DrawPup(&macros);

	textcolor(15);
	textbackground(1);
	for (i = 0;i < MAC_MAX; i++)
	{
		Clear(&macrositems[i]);
		Pos(&macrositems[i]);
		cprintf("%s",&chatmacros[i][0]);
	}
	gotoxy(1,25);

	while(1)
	{
		SetupMenu(&macrosmenu);
		field = GetMenuInput();
		key = menukey;

		switch(key)
		{
			case KEY_ENTER:
				strcpy(string,chatmacros[field]);
				key = EditLine(&macrositems[field],string,40);
				if (key == KEY_ENTER)
					strcpy(chatmacros[field],string);
				textbackground(1);
				textcolor(15);
				Clear(&macrositems[field]);
				Pos(&macrositems[field]);
				cprintf("%s",chatmacros[field]);
				gotoxy(1,25);
				continue;

			case KEY_ESC:
				RestoreScreen();
				return;
		}
	}
};
