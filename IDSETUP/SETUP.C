//
// SETUP PROGRAM
// (C) 1994 id Software, inc.
// by John Romero
// October 1-?
//

#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <mem.h>
#include <stdlib.h>

#include "main.h"		// #includes setup.h

//
//	Fatal error
//
char errorstring[80];
void Error(char *string)
{
	textbackground(0);
	textcolor(7);
	clrscr();
	printf("%s\n",string);
	exit(1);
}

void DrawRadios(radiogroup_t *rg)
{
	int	i;
	int	value;
	radio_t	*r;
	char	far *screen;
	byte	color;

	value = *(rg->master);
	color = (rg->bgcolor << 4)+rg->fgcolor;
	r = rg->radios;
	for (i = 0; i < rg->amount; i++)
	{
		screen = MK_FP(0xb800,(r->y*160)+(r->x*2));
		*(screen+1) = color;
		if (value == r->value)
			*screen = 7;
		else
			*screen = ' ';
		r++;
	}
}

//
// Save screens
//
static	int layer = 0;		// which screen layer we're at
char far screens[MAXLAYERS][4000];
void SaveScreen(void)
{
	movedata(0xb800,0,FP_SEG(&screens[layer]),FP_OFF(&screens[layer]),4000);
	layer++;
	if (layer > MAXLAYERS)
	{
		sprintf(errorstring,"More than %d layers!",layer);
		Error(errorstring);
	}
}

//
// Restore screens
//
void RestoreScreen(void)
{
	layer--;
	if (layer < 0)
		Error("Restored one layer too many!");
	movedata(FP_SEG(&screens[layer]),FP_OFF(&screens[layer]),0xb800,0,4000);
}

//
// Draw the dim 3-D edge of a window
//
void DrawDimEdge(pup_t far *pup)
{
	char	far *screen;
	int	i;

	if ((pup->x + pup->width + 1 > 79) ||
		(pup->y + pup->height > 24))
		return;

	for (i = pup->y + 1; i < pup->y + pup->height + 1; i++)
	{
		screen = MK_FP(0xb800,i*160 + (pup->x + pup->width)*2);
		*(screen+1) = 8;
		*(screen+3) = 8;
	}

	screen = MK_FP(0xb800,(pup->y + pup->height)*160 + (pup->x+2)*2);
	for (i = 0; i < pup->width; i++)
	{
		*(screen+1) = 8;
		screen += 2;
	}

}

//
// Draw LaughingDog .PUP files!
//
void DrawPup(pup_t far *pup)
{
	int	width;
	int	w;
	int	height;
	int	x;
	int	y;
	char	far *data;
	byte	code;
	char	far *screen;
	char	c;
	char	a;
	short	times;
	int	i;
	int	string;

	w = width = pup->width;
	height = pup->height;
	x = pup->x;
	y = pup->y;
	DrawDimEdge(pup);
	data = (char far *)(pup + 1);
	screen = MK_FP(0xb800,y*160 + x*2);
	string = normal;

	while(1)
	{
		code = *data++;
		switch(code)
		{
			case 0:		// String of chars w/same attribute
				if (string == stringdraw)
				{
					string = normal;
					break;
				}
				string = stringdraw;

				c = *data++;	// char
				a = *data++;	// attribute
				*screen++ = c;
				*screen++ = a;
				if (!--w)
				{
					if (!--height)
						return;	// finished!
					w = width;
					y++;
					screen = MK_FP(0xb800,y*160 + x*2);
				}
				break;

			case 0xff:	// Repeated byte
				c = *data++;
				a = *data++;
				times = (*data * 256) + *(data+1);
				data+=2;
				for (i = 0;i < times; i++)
				{
					*screen++ = c;
					*screen++ = a;
					if (!--w)
					{
						if (!--height)
							return;	// finished!
						w = width;
						y++;
						screen = MK_FP(0xb800,y*160 + x*2);
					}
				}
				break;

			default:		// char/attibute combo
				if (string != stringdraw)
					a = *data++;
				c = code;
				*screen++ = c;
				*screen++ = a;
				if (!--w)
				{
					if (!--height)
						return;	// finished!
					w = width;
					y++;
					screen = MK_FP(0xb800,y*160 + x*2);
				}
				break;
		}
	}
}

char **myargv;
int   myargc;

void main(int argc, char *argv[])
{
	myargv = argv;
	myargc = argc;

	#ifdef DEBUG
	ShowAllPups();
	#endif

	StartUp();
}

