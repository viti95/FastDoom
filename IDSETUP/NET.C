//
// Enter network config info
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

#ifndef DOOM2
#define	NET_EPY	3			// Y-coord for Episode group
#define	NET_NPY	7			// Number of players
#define	NET_SKY  11			// Skill level
#define	NET_DMY	17			// Deathmatch
#define	NET_STY  20			// Socket value
#else
#define	NET_NPY	5
#define	NET_SKY  9
#define	NET_DMY	15
#define	NET_STY  18
#endif

#ifndef DOOM2
// # of players radio group
radio_t netepi[]=
{
	{28,NET_EPY,1},
	{28,NET_EPY+1,2},
	{28,NET_EPY+2,3}
};
radiogroup_t netepig={&netepi[0],3,&info.episode,15,1};
#endif

// # of players radio group
radio_t netp[]=
{
	{28,NET_NPY,2},
	{28,NET_NPY+1,3},
	{28,NET_NPY+2,4}
};
radiogroup_t netpg={&netp[0],3,&info.numberOfPlayers,15,1};

// SKILL radio group
radio_t netskill[]=
{
	{28,NET_SKY,1},
	{28,NET_SKY+1,2},
	{28,NET_SKY+2,3},
	{28,NET_SKY+3,4},
	{28,NET_SKY+4,5}
};
radiogroup_t netskillg={&netskill[0],5,&info.skillLevel,15,1};

// DEATHMATCH radio group
radio_t netdeath[]=
{
	{28,NET_DMY,0},
	{28,NET_DMY+1,1}
};
radiogroup_t netdeathg={&netdeath[0],2,&info.deathMatch,15,1};

enum
{
	#ifndef DOOM2
	NET_EPISODE0, NET_EPISODE1, NET_EPISODE2,
	#endif
	NET_P2, NET_P3, NET_P4,
	NET_SKILL1, NET_SKILL2, NET_SKILL3, NET_SKILL4, NET_SKILL5,
	NET_DEATHNO, NET_DEATHYES,
	NET_SOCKET,
	NET_MAX
};
item_t netwkitems[]=
{
#ifndef DOOM2
	{NET_EPISODE0,	31,NET_EPY,21,		-1,-1},
	{NET_EPISODE1,	31,NET_EPY+1,21,	-1,-1},
	{NET_EPISODE2,	31,NET_EPY+2,21,	-1,-1},
#endif
	{NET_P2,			31,NET_NPY,21,		-1,-1},
	{NET_P3,			31,NET_NPY+1,21,	-1,-1},
	{NET_P4,			31,NET_NPY+2,21,	-1,-1},

	{NET_SKILL1,	31,NET_SKY,21,		-1,-1},
	{NET_SKILL2,	31,NET_SKY+1,21,	-1,-1},
	{NET_SKILL3,	31,NET_SKY+2,21,	-1,-1},
	{NET_SKILL4,	31,NET_SKY+3,21,	-1,-1},
	{NET_SKILL5,	31,NET_SKY+4,21,	-1,-1},

	{NET_DEATHNO,	31,NET_DMY,20,		-1,-1},
	{NET_DEATHYES,	31,NET_DMY+1,20,	-1,-1},

	{NET_SOCKET,	28,NET_STY,12,		-1,-1},
};
menu_t netwkmenu=
{
	&netwkitems[0],
	#ifndef DOOM2
	NET_EPISODE0,
	#else
	NET_P2,
	#endif
	NET_MAX,
	0x7f
};
