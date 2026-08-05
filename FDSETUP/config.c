//
// Configure controllers
//
#include <dos.h>
#include <conio.h>
#include <bios.h>

#include "main.h"

void Pos(item_t *item)
{
	gotoxy(item->x + 1, item->y + 1);
}

void Clear(item_t *item)
{
	int i;

	Pos(item);
	for (i = 0; i < item->w; i++)
		cprintf(" ");
	gotoxy(1, 25);
}

//
//	Get keyboard scan code
//
int GetScanCode(void)
{
	volatile unsigned short rval;

	while (kbhit())
		getch();

	SaveScreen();
	DrawPup(&askpres);

	while (1)
	{
		rval = _bios_keybrd(_KEYBRD_SHIFTSTATUS);

		if (rval & 0x0004)
		{
			rval = SC_CTRL;
			break;
		}
		else if (rval & 0x0008)
		{
			rval = SC_ALT;
			break;
		}
		else if ((rval & 0x0001) || (rval & 0x0002))
		{
			rval = SC_RIGHT_SHIFT;
			break;
		}
		else
		{
			rval = _bios_keybrd(_KEYBRD_READY);
			rval = rval >> 8;

#ifndef STRIFE
			if (rval == SC_ENTER)
				rval = 0;
			if (rval == SC_BACKSPACE)
				rval = 0;
#endif
			if (rval)
				break;
		}
	}

	RestoreScreen();
	while (kbhit())
		getch();

	return (rval);
}

enum
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	USE,
	FIRE,
	SPEED,
	STRAFE,
	STRAFE_LEFT,
	STRAFE_RIGHT,
	MAXKEYS
};
item_t idkeyselitems[] =
	{
		{FORWARD, 47, 5, 8, -1, -1},
		{BACKWARD, 47, 6, 8, -1, -1},
		{LEFT, 47, 7, 8, -1, -1},
		{RIGHT, 47, 8, 8, -1, -1},

		{USE, 47, 12, 8, -1, -1},
		{FIRE, 47, 13, 8, -1, -1},
		{SPEED, 47, 14, 8, -1, -1},
		{STRAFE, 47, 15, 8, -1, -1},
		{STRAFE_LEFT, 47, 16, 8, -1, -1},
		{STRAFE_RIGHT, 47, 17, 8, -1, -1},
};
menu_t idkeyselmenu =
	{
		&idkeyselitems[0],
		FORWARD,
		MAXKEYS,
		0x7f};

void IDConfigKeyboard(void)
{
	short field;
	short key;
	CONTS turk;
	int rval;

	SaveScreen();
	DrawPup(&idkeysel);

	turk = curk;

	textbackground(1);
	textcolor(15);
	Clear(&idkeyselitems[FORWARD]);
	Pos(&idkeyselitems[FORWARD]);
	cprintf("%s", keydesc[turk.up]);
	Clear(&idkeyselitems[BACKWARD]);
	Pos(&idkeyselitems[BACKWARD]);
	cprintf("%s", keydesc[turk.down]);
	Clear(&idkeyselitems[LEFT]);
	Pos(&idkeyselitems[LEFT]);
	cprintf("%s", keydesc[turk.left]);
	Clear(&idkeyselitems[RIGHT]);
	Pos(&idkeyselitems[RIGHT]);
	cprintf("%s", keydesc[turk.right]);
	Clear(&idkeyselitems[USE]);
	Pos(&idkeyselitems[USE]);
	cprintf("%s", keydesc[turk.use]);
	Clear(&idkeyselitems[FIRE]);
	Pos(&idkeyselitems[FIRE]);
	cprintf("%s", keydesc[turk.fire]);
	Clear(&idkeyselitems[SPEED]);
	Pos(&idkeyselitems[SPEED]);
	cprintf("%s", keydesc[turk.key2]);
	Clear(&idkeyselitems[STRAFE]);
	Pos(&idkeyselitems[STRAFE]);
	cprintf("%s", keydesc[turk.key1]);
	Clear(&idkeyselitems[STRAFE_LEFT]);
	Pos(&idkeyselitems[STRAFE_LEFT]);
	cprintf("%s", keydesc[turk.key3]);
	Clear(&idkeyselitems[STRAFE_RIGHT]);
	Pos(&idkeyselitems[STRAFE_RIGHT]);
	cprintf("%s", keydesc[turk.key4]);

	gotoxy(1, 25);

	while (1)
	{
		SetupMenu(&idkeyselmenu);
		field = GetMenuInput();
		key = menukey;
		switch (key)
		{
		case KEY_ESC:
			goto func_exit;

		case KEY_F10:
			curk = turk;
			goto func_exit;

		case KEY_ENTER:
			switch (field)
			{
			case FORWARD:
				rval = GetScanCode();
				if (rval)
				{
					turk.up = rval;
					Clear(&idkeyselitems[FORWARD]);
					Pos(&idkeyselitems[FORWARD]);
					cprintf("%s", keydesc[turk.up]);
				}
				break;

			case BACKWARD:
				rval = GetScanCode();
				if (rval)
				{
					turk.down = rval;
					Clear(&idkeyselitems[BACKWARD]);
					Pos(&idkeyselitems[BACKWARD]);
					cprintf("%s", keydesc[turk.down]);
				}
				break;

			case LEFT:
				rval = GetScanCode();
				if (rval)
				{
					turk.left = rval;
					Clear(&idkeyselitems[LEFT]);
					Pos(&idkeyselitems[LEFT]);
					cprintf("%s", keydesc[turk.left]);
				}
				break;

			case RIGHT:
				rval = GetScanCode();
				if (rval)
				{
					turk.right = rval;
					Clear(&idkeyselitems[RIGHT]);
					Pos(&idkeyselitems[RIGHT]);
					cprintf("%s", keydesc[turk.right]);
				}
				break;

			case USE:
				rval = GetScanCode();
				if (rval)
				{
					turk.use = rval;
					Clear(&idkeyselitems[USE]);
					Pos(&idkeyselitems[USE]);
					cprintf("%s", keydesc[turk.use]);
				}
				break;

			case FIRE:
				rval = GetScanCode();
				if (rval)
				{
					turk.fire = rval;
					Clear(&idkeyselitems[FIRE]);
					Pos(&idkeyselitems[FIRE]);
					cprintf("%s", keydesc[turk.fire]);
				}
				break;

			case SPEED:
				rval = GetScanCode();
				if (rval)
				{
					turk.key2 = rval;
					Clear(&idkeyselitems[SPEED]);
					Pos(&idkeyselitems[SPEED]);
					cprintf("%s", keydesc[turk.key2]);
				}
				break;

			case STRAFE:
				rval = GetScanCode();
				if (rval)
				{
					turk.key1 = rval;
					Clear(&idkeyselitems[STRAFE]);
					Pos(&idkeyselitems[STRAFE]);
					cprintf("%s", keydesc[turk.key1]);
				}
				break;

			case STRAFE_LEFT:
				rval = GetScanCode();
				if (rval)
				{
					turk.key3 = rval;
					Clear(&idkeyselitems[STRAFE_LEFT]);
					Pos(&idkeyselitems[STRAFE_LEFT]);
					cprintf("%s", keydesc[turk.key3]);
				}
				break;

			case STRAFE_RIGHT:
				rval = GetScanCode();
				if (rval)
				{
					turk.key4 = rval;
					Clear(&idkeyselitems[STRAFE_RIGHT]);
					Pos(&idkeyselitems[STRAFE_RIGHT]);
					cprintf("%s", keydesc[turk.key4]);
				}
				break;
			}
			gotoxy(1, 25);
			break;
		}
	}

func_exit:

	RestoreScreen();
	return;
}

//
//	Configure joystick buttons
//
enum
{
	J_FIRE,
	J_FORWARD,
	J_USE,
	J_STRAFE,
	J_MAX
};
item_t idjoyselitems[] =
	{
		{J_FIRE, 42, 10, 9, -1, -1},
		{J_FORWARD, 42, 11, 9, -1, -1},
		{J_USE, 42, 12, 9, -1, -1},
		{J_STRAFE, 42, 13, 9, -1, -1}};
menu_t idjoyselmenu =
	{
		&idjoyselitems[0],
		J_FIRE,
		J_MAX,
		0x7f};

//
// Configure mouse buttons
//
enum
{
	M_FIRE,
	M_FORWARD,
	M_STRAFE,
	M_MAX
};
item_t idmouselitems[] =
	{
		{M_FIRE, 44, 9, 13, -1, -1},
		{M_FORWARD, 44, 10, 13, -1, -1},
		{M_STRAFE, 44, 11, 13, -1, -1}};
menu_t idmouselmenu =
	{
		&idmouselitems[0],
		M_FIRE,
		M_MAX,
		0x7f};

//
//	Get mouse button
//
int GetMouseButton(void)
{
	int rval = -1;
	union REGS r;

	SaveScreen();
	DrawPup(&mousentr);

	while (1)
	{
		r.x.ax = 3;
		int86(0x33, &r, &r);

		if (r.x.bx & 1)
			rval = 0;
		else if (r.x.bx & 2)
			rval = 1;
		else if (r.x.bx & 4)
			rval = 2;

		if (rval != -1)
			break;

		if ((_bios_keybrd(_KEYBRD_READY) >> 8) == SC_ESC)
			break;
	}

	RestoreScreen();
	while (kbhit())
		getch();

	return (rval);
}

void IDConfigMouse(void)
{
	short key;
	short field;
	int rval;
	CONTS turk;
	char mousebuts[3][20] = {
		"LEFT BUTTON",
		"RIGHT BUTTON",
		"MID BUTTON"};
	int fire;
	int frwd;
	int strf;

	SaveScreen();
	DrawPup(&idmousel);
	turk = curk;

	fire = turk.mouse[ID_FIRE];
	frwd = turk.mouse[ID_FORWARD];
	strf = turk.mouse[ID_STRAFE];

	textbackground(1);
	textcolor(15);
	Clear(&idmouselitems[M_FIRE]);
	Pos(&idmouselitems[M_FIRE]);
	if (fire >= 0)
		cprintf("%s", mousebuts[fire]);

	Clear(&idmouselitems[M_FORWARD]);
	Pos(&idmouselitems[M_FORWARD]);
	if (frwd >= 0)
		cprintf("%s", mousebuts[frwd]);

	Clear(&idmouselitems[M_STRAFE]);
	Pos(&idmouselitems[M_STRAFE]);
	if (strf >= 0)
		cprintf("%s", mousebuts[strf]);
	gotoxy(1, 25);

	while (1)
	{
		SetupMenu(&idmouselmenu);
		field = GetMenuInput();
		key = menukey;
		switch (key)
		{
		case KEY_ESC:
			goto func_exit;

		case KEY_F10:
			curk = turk;
			goto func_exit;

		case KEY_ENTER:
			switch (field)
			{
			case M_FIRE:
				rval = GetMouseButton();
				if (rval != -1)
				{
					turk.mouse[ID_FIRE] = rval;
					Clear(&idmouselitems[M_FIRE]);
					Pos(&idmouselitems[M_FIRE]);
					cprintf("%s", mousebuts[rval]);
					if (turk.mouse[ID_STRAFE] == rval)
					{
						turk.mouse[ID_STRAFE] = -1;
						Clear(&idmouselitems[M_STRAFE]);
					}
					if (turk.mouse[ID_FORWARD] == rval)
					{
						turk.mouse[ID_FORWARD] = -1;
						Clear(&idmouselitems[M_FORWARD]);
					}
				}
				break;

			case M_FORWARD:
				rval = GetMouseButton();
				if (rval != -1)
				{
					turk.mouse[ID_FORWARD] = rval;
					Clear(&idmouselitems[M_FORWARD]);
					Pos(&idmouselitems[M_FORWARD]);
					cprintf("%s", mousebuts[rval]);

					if (turk.mouse[ID_STRAFE] == rval)
					{
						turk.mouse[ID_STRAFE] = -1;
						Clear(&idmouselitems[M_STRAFE]);
					}
					if (turk.mouse[ID_FIRE] == rval)
					{
						turk.mouse[ID_FIRE] = -1;
						Clear(&idmouselitems[M_FIRE]);
					}
				}
				break;

			case M_STRAFE:
				rval = GetMouseButton();
				if (rval != -1)
				{
					turk.mouse[ID_STRAFE] = rval;
					Clear(&idmouselitems[M_STRAFE]);
					Pos(&idmouselitems[M_STRAFE]);
					cprintf("%s", mousebuts[rval]);

					if (turk.mouse[ID_FORWARD] == rval)
					{
						turk.mouse[ID_FORWARD] = -1;
						Clear(&idmouselitems[M_FORWARD]);
					}
					if (turk.mouse[ID_FIRE] == rval)
					{
						turk.mouse[ID_FIRE] = -1;
						Clear(&idmouselitems[M_FIRE]);
					}
				}
				break;
			}
			gotoxy(1, 25);
			break;
		}
	}

func_exit:

	RestoreScreen();
	return;
}

//
// Choose which controller to configure!
//
enum
{
	CFG_KEY,
	CFG_MOUSE,
	CFG_MAX
};
item_t conselitems[] =
	{
		{CFG_KEY, 31, 11, 14, -1, -1},
		{CFG_MOUSE, 31, 12, 14, -1, -1},
};
menu_t conselmenu =
	{
		&conselitems[0],
		CFG_KEY,
		CFG_MAX,
		0x7f};

void ConfigControl(void)
{
	short key;
	short field;

	SaveScreen();
	DrawPup(&consel);

	while (1)
	{
		SetupMenu(&conselmenu);
		field = GetMenuInput();
		key = menukey;

		if (key == KEY_ESC)
			break;

		if (key != KEY_ENTER && key != KEY_F10)
			continue;

		switch (field)
		{
		default:
		case CFG_KEY:
			IDConfigKeyboard();
			goto func_exit;

		case CFG_MOUSE:
			if (!mousepresent)
			{
				ErrorWindow(&mouspres);
				break;
			}
			IDConfigMouse();
			goto func_exit;
		}
	}

func_exit:

	RestoreScreen();
	return;
}
