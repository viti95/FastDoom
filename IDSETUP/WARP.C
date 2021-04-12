//
// Warp to level
//
#include <dos.h>

#include "main.h"
#include "keys.h"

int   respawn;
int   nomonsters;
int   deathmatch2;

enum
{
	W_NOMON,
	W_DM2,
	W_RESPAWN,

	E1M1,
	E1M2,
	E1M3,
	E1M4,
	E1M5,
	E1M6,
	E1M7,
	E1M8,
	E1M9,
	E1M10,

	E2M1,
	E2M2,
	E2M3,
	E2M4,
	E2M5,
	E2M6,
	E2M7,
	E2M8,
	E2M9,
	E2M10,

	E3M1,
	E3M2,
	E3M3,
	E3M4,
	E3M5,
	E3M6,
	E3M7,
	E3M8,
	E3M9,
	E3M10,

	W_MAX
};

item_t cwarpitems[]=
{
	{W_NOMON,	35,5,15,	-1,-1},
	{W_DM2,		35,6,15,	-1,-1},
	{W_RESPAWN,	35,7,15,	-1,-1},
	{E1M1,		31,9,6,	-1,E2M1},
	{E1M2,		31,10,6,	-1,E2M2},
	{E1M3,		31,11,6,	-1,E2M3},
	{E1M4,		31,12,6,	-1,E2M4},
	{E1M5,		31,13,6,	-1,E2M5},
	{E1M6,		31,14,6,	-1,E2M6},
	{E1M7,		31,15,6,	-1,E2M7},
	{E1M8,		31,16,6,	-1,E2M8},
	{E1M9,		31,17,6,	-1,E2M9},
#ifdef DOOM2
	{E1M10,		31,18,6,	-1,E2M10},
#endif

	{E2M1,		38,9,6, E1M1,E3M1,W_RESPAWN},
	{E2M2,		38,10,6,E1M2,E3M2},
	{E2M3,		38,11,6,E1M3,E3M3},
	{E2M4,		38,12,6,E1M4,E3M4},
	{E2M5,		38,13,6,E1M5,E3M5},
	{E2M6,		38,14,6,E1M6,E3M6},
	{E2M7,		38,15,6,E1M7,E3M7},
	{E2M8,		38,16,6,	E1M8,E3M8},
	{E2M9,		38,17,6,	E1M9,E3M9},
#ifdef DOOM2
	{E2M10,		38,18,6,	E1M10,E3M10},
#endif

	{E3M1,		45,9,6,	E2M1,-1,W_RESPAWN},
	{E3M2,		45,10,6,	E2M2,-1},
	{E3M3,		45,11,6,	E2M3,-1},
	{E3M4,		45,12,6,	E2M4,-1},
	{E3M5,		45,13,6,	E2M5,-1},
	{E3M6,		45,14,6,	E2M6,-1},
	{E3M7,		45,15,6,	E2M7,-1},
	{E3M8,		45,16,6,	E2M8,-1},
	{E3M9,		45,17,6,	E2M9,-1},
#ifdef DOOM2
	{E3M10,		45,18,6,	E2M10,-1}
#endif
};

menu_t cwarpmenu=
{
	&cwarpitems[0],
	W_NOMON,
	W_MAX,
	0x7f
};

unsigned WarpTime(void)
{
	int   e;
	int   l;
	short	key;
	short field;
	int   exit;


	SaveScreen();
	DrawPup(&cwarp);

	SetMark(&cwarpitems[W_RESPAWN],respawn);
	SetMark(&cwarpitems[W_NOMON],nomonsters);
	SetMark(&cwarpitems[W_DM2],deathmatch2);

	exit = 0;

	while(1)
	{
		SetupMenu(&cwarpmenu);
		field = GetMenuInput();
		key = menukey;

		switch(key)
		{
			case KEY_ESC:
				RestoreScreen();
				return 0;

			case KEY_F10:
			case KEY_ENTER:

				switch(field)
				{
					case E1M1: e=1;l=1; exit=1; break;
					case E1M2: e=1;l=2; exit=1; break;
					case E1M3: e=1;l=3; exit=1; break;
					case E1M4: e=1;l=4; exit=1; break;
					case E1M5: e=1;l=5; exit=1; break;
					case E1M6: e=1;l=6; exit=1; break;
					case E1M7: e=1;l=7; exit=1; break;
					case E1M8: e=1;l=8; exit=1; break;
					case E1M9: e=1;l=9; exit=1; break;
					case E1M10: e=1;l=10; exit=1; break;

					case E2M1: e=2;l=1; exit=1; break;
					case E2M2: e=2;l=2; exit=1; break;
					case E2M3: e=2;l=3; exit=1; break;
					case E2M4: e=2;l=4; exit=1; break;
					case E2M5: e=2;l=5; exit=1; break;
					case E2M6: e=2;l=6; exit=1; break;
					case E2M7: e=2;l=7; exit=1; break;
					case E2M8: e=2;l=8; exit=1; break;
					case E2M9: e=2;l=9; exit=1; break;
					case E2M10: e=2;l=10; exit=1; break;

					case E3M1: e=3;l=1; exit=1; break;
					case E3M2: e=3;l=2; exit=1; break;
					case E3M3: e=3;l=3; exit=1; break;
					case E3M4: e=3;l=4; exit=1; break;
					case E3M5: e=3;l=5; exit=1; break;
					case E3M6: e=3;l=6; exit=1; break;
					case E3M7: e=3;l=7; exit=1; break;
					case E3M8: e=3;l=8; exit=1; break;
					case E3M9: e=3;l=9; exit=1; break;
					case E3M10: e=3;l=10; exit=1; break;

					case W_RESPAWN:
						respawn ^= 1;
						SetMark(&cwarpitems[W_RESPAWN],respawn);
						break;

					case W_NOMON:
						nomonsters ^= 1;
						SetMark(&cwarpitems[W_NOMON],nomonsters);
						break;
					case W_DM2:
						deathmatch2 ^= 1;
						SetMark(&cwarpitems[W_DM2],deathmatch2);
						break;
				}

				if (exit)
				{
					RestoreScreen();
					return ((e-1)*10+l);
				}
		 }
	}
}


