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
