#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <bios.h>

#include "main.h"

//
// Choose which type of modem to use
//
modem_t  modems[MAXMODEMS];
int      numModems;

//
// Choose which type of netplay
//
enum {GT_IPX,GT_MODEM,GT_SERIAL,GT_CHOOSE,GT_MACROS,GT_MAX};
item_t netplayitems[]=
{
	{GT_IPX,		26,9,27,		-1,-1},
	{GT_MODEM,	26,10,27,	-1,-1},
	{GT_SERIAL,	26,11,27,	-1,-1},

	{GT_CHOOSE,	26,13,27,	-1,-1},
	{GT_MACROS,	26,14,27,	-1,-1}
};
menu_t netplaymenu=
{
	&netplayitems[0],
	GT_IPX,
	GT_MAX,
	0x7f
};
