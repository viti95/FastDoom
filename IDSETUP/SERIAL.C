//
// Enter serial config info
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

enum
{
#ifndef DOOM2
	SER_EPISODE0, SER_EPISODE1, SER_EPISODE2,
#endif
	SER_SKILL1, SER_SKILL2, SER_SKILL3, SER_SKILL4,
	SER_DEATHNO, SER_DEATHYES,
	SER_COM1, SER_COM2, SER_COM3, SER_COM4,
	SER_MAX
};
item_t cserialitems[]=
{
#ifndef DOOM2
	{SER_EPISODE0,	31,5,21,		-1,-1},
	{SER_EPISODE1,	31,6,21,		-1,-1},
	{SER_EPISODE2,	31,7,21,		-1,-1},

	{SER_SKILL1,	31,9,21,		-1,-1},
	{SER_SKILL2,	31,10,21,		-1,-1},
	{SER_SKILL3,	31,11,21,		-1,-1},
	{SER_SKILL4,	31,12,21,	-1,-1},

	{SER_DEATHNO,	31,14,20,	-1,-1},
	{SER_DEATHYES,	31,15,20,	-1,-1},

	{SER_COM1,		31,17,4,		-1,		SER_COM3},
	{SER_COM2,		31,18,4,		-1,		SER_COM4},
	{SER_COM3,		45,17,4,		SER_COM1,-1,SER_DEATHYES},
	{SER_COM4,		45,18,4,		SER_COM2,-1}
#else
	{SER_SKILL1,	31,7,20,		-1,-1},
	{SER_SKILL2,	31,8,20,		-1,-1},
	{SER_SKILL3,	31,9,20,		-1,-1},
	{SER_SKILL4,	31,10,20,	-1,-1},

	{SER_DEATHNO,	31,12,20,	-1,-1},
	{SER_DEATHYES,	31,13,20,	-1,-1},

	{SER_COM1,		31,15,4,		-1,		SER_COM3},
	{SER_COM2,		31,16,4,		-1,		SER_COM4},
	{SER_COM3,		45,15,4,		SER_COM1,-1,SER_DEATHYES},
	{SER_COM4,		45,16,4,		SER_COM2,-1}
#endif
};
menu_t cserialmenu=
{
	&cserialitems[0],
#ifndef DOOM2
	SER_EPISODE0,
#else
	SER_SKILL1,
#endif
	SER_MAX,
	0x7f
};

#ifndef DOOM2
// EPISODE radio buttons
radio_t sepi[]=
{
	{28,5,1},
	{28,6,2},
	{28,7,3}
};
radiogroup_t sepig={&sepi[0],3,&sinfo.episode,15,1};

// SKILL radio buttons
radio_t sskill[]=
{
	{28,9,2},
	{28,10,3},
	{28,11,4},
	{28,12,5}
};
radiogroup_t sskillg={&sskill[0],4,&sinfo.skillLevel,15,1};

// DEATHMATCH radio buttons
radio_t sdeath[]=
{
	{28,14,0},
	{28,15,1}
};
radiogroup_t sdeathg={&sdeath[0],2,&sinfo.deathMatch,15,1};

// COMPORT radio buttons
radio_t scom[]=
{
	{28,17,1},
	{28,18,2},
	{42,17,3},
	{42,18,4}
};
radiogroup_t scomg={&scom[0],4,&sinfo.comport,15,1};
#else
// SKILL radio buttons
radio_t sskill[]=
{
	{28,7,2},
	{28,8,3},
	{28,9,4},
	{28,10,5}
};
radiogroup_t sskillg={&sskill[0],4,&sinfo.skillLevel,15,1};

// DEATHMATCH radio buttons
radio_t sdeath[]=
{
	{28,12,0},
	{28,13,1}
};
radiogroup_t sdeathg={&sdeath[0],2,&sinfo.deathMatch,15,1};

// COMPORT radio buttons
radio_t scom[]=
{
	{28,15,1},
	{28,16,2},
	{42,15,3},
	{42,16,4}
};
radiogroup_t scomg={&scom[0],4,&sinfo.comport,15,1};
#endif
