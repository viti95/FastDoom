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
	WEAPON_PREV,
	WEAPON_NEXT,
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

		{WEAPON_PREV, 47, 18, 8, -1, -1},
		{WEAPON_NEXT, 47, 19, 8, -1, -1},
};
menu_t idkeyselmenu =
	{
		&idkeyselitems[0],
		FORWARD,
		MAXKEYS};

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
	Clear(&idkeyselitems[WEAPON_PREV]);
	Pos(&idkeyselitems[WEAPON_PREV]);
	cprintf("%s", keydesc[turk.key5]);
	Clear(&idkeyselitems[WEAPON_NEXT]);
	Pos(&idkeyselitems[WEAPON_NEXT]);
	cprintf("%s", keydesc[turk.key6]);

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

			case WEAPON_PREV:
				rval = GetScanCode();
				if (rval)
				{
					turk.key5 = rval;
					Clear(&idkeyselitems[WEAPON_PREV]);
					Pos(&idkeyselitems[WEAPON_PREV]);
					cprintf("%s", keydesc[turk.key5]);
				}
				break;

			case WEAPON_NEXT:
				rval = GetScanCode();
				if (rval)
				{
					turk.key6 = rval;
					Clear(&idkeyselitems[WEAPON_NEXT]);
					Pos(&idkeyselitems[WEAPON_NEXT]);
					cprintf("%s", keydesc[turk.key6]);
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

/*
 * Configure mouse buttons
 */
enum
{
	M_BUTTON1,
	M_BUTTON2,
	M_BUTTON3,
	M_MAX
};
item_t idmouselitems[] =
	{
		{M_BUTTON1, 38, 9, 13, -1, -1},
		{M_BUTTON2, 38, 10, 13, -1, -1},
		{M_BUTTON3, 38, 11, 13, -1, -1}};
menu_t idmouselmenu =
	{
		&idmouselitems[0],
		M_BUTTON1,
		M_MAX};

/* Action selection menu (item id and index = action value) */
item_t mouseactitems[] =
	{
		{MOUSE_ACT_NONE, 26, 9, 20, -1, -1},
		{MOUSE_ACT_FIRE, 26, 10, 20, -1, -1},
		{MOUSE_ACT_FORWARD, 26, 11, 20, -1, -1},
		{MOUSE_ACT_BACK, 26, 12, 20, -1, -1},
		{MOUSE_ACT_STRAFE, 26, 13, 20, -1, -1},
		{MOUSE_ACT_USE, 26, 14, 20, -1, -1},
		{MOUSE_ACT_SPEED, 26, 15, 20, -1, -1},
		{MOUSE_ACT_WEAPONPREV, 26, 16, 20, -1, -1},
		{MOUSE_ACT_WEAPONNEXT, 26, 17, 20, -1, -1}};
menu_t mouseactmenu =
	{
		&mouseactitems[0],
		0,
		MOUSE_ACT_MAX};

static char *mouseactnames[MOUSE_ACT_MAX] =
	{
		"None",
		"Fire Weapon",
		"Move Forward",
		"Move Backward",
		"Strafe On",
		"Use",
		"Speed On",
		"Previous Weapon",
		"Next Weapon"
	};

/*
 * Get mouse action
 */
int GetMouseAction(int startitem)
{
	short key;
	int field;

	if (startitem < 0 || startitem >= MOUSE_ACT_MAX)
		startitem = 0;

	mouseactmenu.startitem = startitem;

	SaveScreen();
	DrawPup(&mouseact);

	SetupMenu(&mouseactmenu);

	field = GetMenuInput();
	key = menukey;

	if (key == KEY_ESC)
		field = -1;

	RestoreScreen();
	while (kbhit())
		getch();

	return (field);
}

void IDConfigMouse(void)
{
	short key;
	short field;
	int rval;
	CONTS turk;

	SaveScreen();
	DrawPup(&idmousel);
	turk = curk;

	textbackground(1);
	textcolor(15);
	for (field = M_BUTTON1; field < M_MAX; field++)
	{
		Clear(&idmouselitems[field]);
		Pos(&idmouselitems[field]);
		if (turk.mouse[field] >= 0 && turk.mouse[field] < MOUSE_ACT_MAX)
			cprintf("%s", mouseactnames[turk.mouse[field]]);
	}
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
			if (field >= M_BUTTON1 && field < M_MAX)
			{
				rval = GetMouseAction(turk.mouse[field]);
				if (rval != -1)
				{
					turk.mouse[field] = rval;
					Clear(&idmouselitems[field]);
					Pos(&idmouselitems[field]);
					cprintf("%s", mouseactnames[rval]);
				}
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
		CFG_MAX};

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
