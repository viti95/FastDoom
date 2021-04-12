//
// Enter modem config info
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

//
// Choose who to call for modem play
//

phonelist_t phonelist[MAXPHNLIST];
int   numInList;
char  chosenPhoneNum[16];

//
// Choose who to call
//
enum {ADDNAME,ADDNUMBER,MAXADD};
item_t additems[]= {
	{ADDNAME,	17,12,28,-1,ADDNUMBER},
	{ADDNUMBER,	46,12,18,ADDNAME,-1}};
menu_t addmenu = {&additems[0],ADDNAME,MAXADD,0x7f};

#ifndef DOOM2
//
// Epsiode radio buttons
//
radio_t mepi[]=
{
	{28,3,1},
	{28,4,2},
	{28,5,3}
};
radiogroup_t mepig=
{	&mepi[0],3,&minfo.episode,15,1	};

//
// Skill level radio buttons
//
radio_t mskill[]=
{
	{28,7,2},
	{28,8,3},
	{28,9,4},
	{28,10,5}
};
radiogroup_t mskillg=
{	&mskill[0],4,&minfo.skillLevel,15,1	};

//
// Mode radio buttons
//
radio_t mdeath[]=
{
	{28,12,0},
	{28,13,1}
};
radiogroup_t mdeathg=
{	&mdeath[0],2,&minfo.deathMatch,15,1	};

//
// Com port radio buttons
//
radio_t mcomport[]=
{
	{28,15,1},
	{28,16,2},
	{42,15,3},
	{42,16,4}
};
radiogroup_t mcomportg=
{	&mcomport[0],4,&minfo.comport,15,1	};

//
// Connection-type radio buttons
//
radio_t mcomtype[]=
{
	{28,18,0},
	{28,19,1},
	{28,20,2}
};
radiogroup_t mcomtypeg=
{	&mcomtype[0],3,&minfo.comtype,15,1	};
#else
//
// Skill level radio buttons
//
radio_t mskill[]=
{
	{28,5,2},
	{28,6,3},
	{28,7,4},
	{28,8,5},
};
radiogroup_t mskillg=
{	&mskill[0],4,&minfo.skillLevel,15,1	};

//
// Mode radio buttons
//
radio_t mdeath[]=
{
	{28,10,0},
	{28,11,1}
};
radiogroup_t mdeathg=
{	&mdeath[0],2,&minfo.deathMatch,15,1	};

//
// Com port radio buttons
//
radio_t mcomport[]=
{
	{28,13,1},
	{28,14,2},
	{42,13,3},
	{42,14,4}
};
radiogroup_t mcomportg=
{	&mcomport[0],4,&minfo.comport,15,1	};

//
// Connection-type radio buttons
//
radio_t mcomtype[]=
{
	{28,16,0},
	{28,17,1},
	{28,18,2}
};
radiogroup_t mcomtypeg=
{	&mcomtype[0],3,&minfo.comtype,15,1	};
#endif

//
// Menu info
//
enum
{
#ifndef DOOM2
	MOD_EPISODE0, MOD_EPISODE1, MOD_EPISODE2,
#endif
	MOD_SKILL1, MOD_SKILL2, MOD_SKILL3, MOD_SKILL4,
	MOD_DEATHNO, MOD_DEATHYES,
	MOD_COM1, MOD_COM2, MOD_COM3, MOD_COM4,
	MOD_CONN0, MOD_CONN1, MOD_CONN2,
	MOD_MAX
};
item_t cmodemitems[]=
{
#ifndef DOOM2
	{MOD_EPISODE0,	31,3,21,	-1,-1},
	{MOD_EPISODE1,	31,4,21,	-1,-1},
	{MOD_EPISODE2,	31,5,21,	-1,-1},

	{MOD_SKILL1,	31,7,21,	-1,-1},
	{MOD_SKILL2,	31,8,21,	-1,-1},
	{MOD_SKILL3,	31,9,21,	-1,-1},
	{MOD_SKILL4,	31,10,21,	-1,-1},

	{MOD_DEATHNO,	31,12,20,-1,-1},
	{MOD_DEATHYES,	31,13,20,-1,-1},

	{MOD_COM1,		31,15,4,	-1,		MOD_COM3},
	{MOD_COM2,		31,16,4,	-1,		MOD_COM4,0,MOD_CONN0},
	{MOD_COM3,		45,15,4,MOD_COM1,	-1,MOD_DEATHYES},
	{MOD_COM4,		45,16,4,MOD_COM2,	-1},

	{MOD_CONN0,		31,18,20,	-1,-1,MOD_COM2},
	{MOD_CONN1,		31,19,20,	-1,-1},
	{MOD_CONN2,		31,20,7,		-1,-1}
#else
	{MOD_SKILL1,	31,5,20,	-1,-1},
	{MOD_SKILL2,	31,6,20,	-1,-1},
	{MOD_SKILL3,	31,7,20,	-1,-1},
	{MOD_SKILL4,	31,8,20,	-1,-1},

	{MOD_DEATHNO,	31,10,20,-1,-1},
	{MOD_DEATHYES,	31,11,20,-1,-1},

	{MOD_COM1,		31,13,4,	-1,		MOD_COM3},
	{MOD_COM2,		31,14,4,	-1,		MOD_COM4,0,MOD_CONN0},
	{MOD_COM3,		45,13,4,MOD_COM1,	-1,MOD_DEATHYES},
	{MOD_COM4,		45,14,4,MOD_COM2,	-1},

	{MOD_CONN0,		31,16,20,	-1,-1,MOD_COM2},
	{MOD_CONN1,		31,17,20,	-1,-1},
	{MOD_CONN2,		31,18,7,		-1,-1}
#endif
};
menu_t cmodemmenu=
{
	&cmodemitems[0],
#ifndef DOOM2
	MOD_EPISODE0,
#else
	MOD_SKILL1,
#endif
	MOD_MAX,
	0x7f
};
