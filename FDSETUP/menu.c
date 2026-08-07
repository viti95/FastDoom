#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <dos.h>

#include "main.h"

item_t *current;
menu_t *currentmenu;
int curitem;	  // current item #
char buffer[160]; // save the entire screen line!
short menukey;	  // globally set after GetMenuInput()

//
// Make a sound!
//
void Sound(int freq, int dly)
{
	sound(freq);
	delay(dly);
	nosound();
}

//
// Invert the menu item
//
void Invert(item_t *item)
{
	char far *screen;
	int i;

	if (mono)
	{
		movedata(0xb000, item->y * 160, FP_SEG(&buffer), FP_OFF(&buffer), 160);
		screen = MK_FP(0xb000, item->y * 160 + item->x * 2);
	}
	else
	{
		movedata(0xb800, item->y * 160, FP_SEG(&buffer), FP_OFF(&buffer), 160);
		screen = MK_FP(0xb800, item->y * 160 + item->x * 2);
	}

	for (i = 0; i < item->w; i++)
	{
		if (mono)
			*(screen + 1) = 0x70;
		else
			*(screen + 1) = 0x7f;
		screen += 2;
	}
}

//
//	Restore the screen line (uninvert)
//
void UnInvert(item_t *item)
{
	if (mono)
		movedata(FP_SEG(&buffer), FP_OFF(&buffer), 0xb000, item->y * 160, 160);
	else
		movedata(FP_SEG(&buffer), FP_OFF(&buffer), 0xb800, item->y * 160, 160);
}

//
//	Set "current" to first menu item
//
void SetupMenu(menu_t *menu)
{
	currentmenu = menu;
	current = menu->items;
	current += menu->startitem;
	curitem = menu->startitem;
	Invert(current);
}

//
//	Get menu input for current menu
// Exit:	-1 = ESC was pressed, xx = item id
//
int GetMenuInput(void)
{
	char c;
	while (1)
	{
		c = getch();
		//		gotoxy(1,2);
		//		printf("char:%x  ",c);
		switch (c)
		{
		case 0x48: // UP
			if (!curitem)
				break;
			UnInvert(current);
			if (current->up)
			{
				curitem = current->up;
				current = currentmenu->items + curitem;
			}
			else
			{
				curitem--;
				current--;
			}
			Invert(current);
			Sound(50, 10);
			break;

		case 0x50: // DOWN
			if (curitem == currentmenu->maxitems - 1)
				break;
			UnInvert(current);
			if (current->down)
			{
				curitem = current->down;
				current = currentmenu->items + curitem;
			}
			else
			{
				curitem++;
				current++;
			}
			Invert(current);
			Sound(50, 10);
			break;

		case 0x4b: // LEFT
			if (current->left != -1)
			{
				UnInvert(current);
				curitem = current->left;
				current = currentmenu->items + curitem;
				Invert(current);
				Sound(50, 10);
			}
			break;

		case 0x4d: // RIGHT
			if (current->right != -1)
			{
				UnInvert(current);
				curitem = current->right;
				current = currentmenu->items + curitem;
				Invert(current);
				Sound(50, 10);
			}
			break;

		case 0x44: // F10
		case 0x3b: // F1
		case 0x3c: // F2
			UnInvert(current);
			menukey = c;
			currentmenu->startitem = curitem;
			Sound(50, 10);
			return current->id;

		case 0x0d: // ENTER
			UnInvert(current);
			menukey = c;
			currentmenu->startitem = curitem;
			Sound(2000, 10);
			return current->id;

		case 0x1b: // ESC
			UnInvert(current);
			menukey = c;
			Sound(3000, 10);
			return -1;
		}
	}
}
