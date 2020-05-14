//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 2016-2017 Alexey Khokholov (Nuke.YKT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  DOOM Network game communication and protocol,
//  all OS independend parts.
//

#include "m_menu.h"
#include "i_system.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

#define NCMD_EXIT 0x80000000
#define NCMD_RETRANSMIT 0x40000000
#define NCMD_SETUP 0x20000000
#define NCMD_KILL 0x10000000 // kill game

doomcom_t *doomcom;
doomdata_t *netbuffer; // points inside doomcom

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// nettics[] has the maketics for all players
//
// a gametic cannot be run until nettics[] > gametic for all players
//
#define RESENDCOUNT 10
#define PL_DRONE 0x80 // bit flag in doomdata->player

ticcmd_t localcmds[BACKUPTICS];

ticcmd_t netcmds[MAXPLAYERS][BACKUPTICS];
int nettics[MAXNETNODES];
boolean nodeingame[MAXNETNODES];   // set false as nodes leave game
boolean remoteresend[MAXNETNODES]; // set when local needs tics
int resendto[MAXNETNODES];		   // set when remote needs tics
int resendcount[MAXNETNODES];

int nodeforplayer[MAXPLAYERS];

int maketic;
int lastnettic;
int skiptics;
int ticdup;
int maxsend; // BACKUPTICS/(2*ticdup)-1

void D_ProcessEvents(void);
void G_BuildTiccmd(ticcmd_t *cmd);
void D_DoAdvanceDemo(void);

boolean reboundpacket;
doomdata_t reboundstore;

//
//
//
int NetbufferSize(void)
{
	return (int)&(((doomdata_t *)0)->cmds[netbuffer->numtics]);
}

//
//
//
int ExpandTics(int low)
{
	int delta;

	delta = low - (maketic & 0xff);

	if (delta >= -64 && delta <= 64)
		return (maketic & ~0xff) + low;
	if (delta > 64)
		return (maketic & ~0xff) - 256 + low;
	if (delta < -64)
		return (maketic & ~0xff) + 256 + low;

	return 0;
}

//
// HSendPacket
//
void HSendPacket(int node,
				 int flags)
{
	if (!node)
	{
		reboundstore = *netbuffer;
		reboundpacket = true;
		return;
	}

	if (demoplayback)
		return;

	I_Error("Tried to transmit to another node");

	doomcom->command = CMD_SEND;
	doomcom->remotenode = node;
	doomcom->datalength = NetbufferSize();

	I_NetCmd();
}

//
// HGetPacket
// Returns false if no packet is waiting
//
boolean HGetPacket(void)
{
	if (reboundpacket)
	{
		*netbuffer = reboundstore;
		doomcom->remotenode = 0;
		reboundpacket = false;
		return true;
	}

	return false;

	if (demoplayback)
		return false;

	doomcom->command = CMD_GET;
	I_NetCmd();

	if (doomcom->remotenode == -1)
		return false;

	if (doomcom->datalength != NetbufferSize())
	{
		if (debugfile)
			fprintf(debugfile, "bad packet length %i\n", doomcom->datalength);
		return false;
	}

	return true;
}

//
// GetPackets
//
char exitmsg[80];

void GetPackets(void)
{
	int netconsole;
	int netnode;
	ticcmd_t *src, *dest;
	int realend;
	int realstart;

	while (HGetPacket())
	{
		netconsole = netbuffer->player & ~PL_DRONE;
		netnode = doomcom->remotenode;

		// to save bytes, only the low byte of tic numbers are sent
		// Figure out what the rest of the bytes are
		realstart = ExpandTics(netbuffer->starttic);
		realend = (realstart + netbuffer->numtics);


		nodeforplayer[netconsole] = netnode;

		// check for out of order / duplicated packet
		if (realend == nettics[netnode])
			continue;

		if (realend < nettics[netnode])
		{
			continue;
		}

		// check for a missed packet
		if (realstart > nettics[netnode])
		{
			// stop processing until the other system resends the missed tics
			remoteresend[netnode] = true;
			continue;
		}

		// update command store from the packet
		{
			int start;

			remoteresend[netnode] = false;

			start = nettics[netnode] - realstart;
			src = &netbuffer->cmds[start];

			while (nettics[netnode] < realend)
			{
				dest = &netcmds[netconsole][nettics[netnode] % BACKUPTICS];
				nettics[netnode]++;
				*dest = *src;
				src++;
			}
		}
	}
}

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
int gametime;

void NetUpdate(void)
{
	int nowtime;
	int newtics;
	int i, j;
	int realstart;
	int gameticdiv;

	// check time
	nowtime = I_GetTime() / ticdup;
	newtics = nowtime - gametime;
	gametime = nowtime;

	if (newtics <= 0) // nothing new to update
		goto listen;

	if (skiptics <= newtics)
	{
		newtics -= skiptics;
		skiptics = 0;
	}
	else
	{
		skiptics -= newtics;
		newtics = 0;
	}

	netbuffer->player = consoleplayer;

	// build new ticcmds for console player
	gameticdiv = gametic / ticdup;
	for (i = 0; i < newtics; i++)
	{
		I_StartTic();
		D_ProcessEvents();
		if (maketic - gameticdiv >= BACKUPTICS / 2 - 1)
			break; // can't hold any more

		G_BuildTiccmd(&localcmds[maketic % BACKUPTICS]);
		maketic++;
	}

	if (singletics)
		return; // singletic update is syncronous

	// send the packet to the other nodes
	for (i = 0; i < doomcom->numnodes; i++)
		if (nodeingame[i])
		{
			netbuffer->starttic = realstart = resendto[i];
			netbuffer->numtics = maketic - realstart;
			if (netbuffer->numtics > BACKUPTICS)
				I_Error("NetUpdate: netbuffer->numtics > BACKUPTICS");

			resendto[i] = maketic - doomcom->extratics;

			for (j = 0; j < netbuffer->numtics; j++)
				netbuffer->cmds[j] =
					localcmds[(realstart + j) % BACKUPTICS];

			if (remoteresend[i])
			{
				netbuffer->retransmitfrom = nettics[i];
				HSendPacket(i, NCMD_RETRANSMIT);
			}
			else
			{
				netbuffer->retransmitfrom = 0;
				HSendPacket(i, 0);
			}
		}

	// listen for other packets
listen:
	GetPackets();
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
void D_CheckNetGame(void)
{
	int i;

	for (i = 0; i < MAXNETNODES; i++)
	{
		nodeingame[i] = false;
		nettics[i] = 0;
		remoteresend[i] = false; // set when local needs tics
		resendto[i] = 0;		 // which tic to start sending
	}

	// I_InitNetwork sets doomcom and netgame
	I_InitNetwork();
	if (doomcom->id != DOOMCOM_ID)
		I_Error("Doomcom buffer invalid!");

	netbuffer = &doomcom->data;
	consoleplayer = displayplayer = doomcom->consoleplayer;

	printf("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
		   startskill, deathmatch, startmap, startepisode);

	// read values out of doomcom
	ticdup = doomcom->ticdup;
	maxsend = BACKUPTICS / (2 * ticdup) - 1;
	if (maxsend < 1)
		maxsend = 1;

	for (i = 0; i < doomcom->numplayers; i++)
		playeringame[i] = true;
	for (i = 0; i < doomcom->numnodes; i++)
		nodeingame[i] = true;

	printf("player %i of %i (%i nodes)\n",
		   consoleplayer + 1, doomcom->numplayers, doomcom->numnodes);
}

//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame(void)
{
	int i, j;

	if (debugfile)
		fclose(debugfile);

	if (!netgame || !usergame || consoleplayer == -1 || demoplayback)
		return;

	// send a bunch of packets for security
	netbuffer->player = consoleplayer;
	netbuffer->numtics = 0;
	for (i = 0; i < 4; i++)
	{
		for (j = 1; j < doomcom->numnodes; j++)
			if (nodeingame[j])
				HSendPacket(j, NCMD_EXIT);
		I_WaitVBL(1);
	}
}

//
// TryRunTics
//
int frametics[4];
int frameon;
int frameskip[4];
int oldnettics;

extern boolean advancedemo;

void TryRunTics(void)
{
	int i;
	int lowtic;
	int entertic;
	static int oldentertics;
	int realtics;
	int availabletics;
	int counts;
	int numplaying;

	// get real tics
	entertic = I_GetTime() / ticdup;
	realtics = entertic - oldentertics;
	oldentertics = entertic;

	// get available tics
	NetUpdate();

	lowtic = MAXINT;
	numplaying = 0;
	for (i = 0; i < doomcom->numnodes; i++)
	{
		if (nodeingame[i])
		{
			numplaying++;
			if (nettics[i] < lowtic)
				lowtic = nettics[i];
		}
	}
	availabletics = lowtic - gametic / ticdup;

	// decide how many tics to run
	if (realtics < availabletics - 1)
		counts = realtics + 1;
	else if (realtics < availabletics)
		counts = realtics;
	else
		counts = availabletics;

	if (counts < 1)
		counts = 1;

	frameon++;

	if (debugfile)
		fprintf(debugfile,
				"=======real: %i  avail: %i  game: %i\n",
				realtics, availabletics, counts);

	if (!demoplayback)
	{
		// ideally nettics[0] should be 1 - 3 tics above lowtic
		// if we are consistantly slower, speed up time
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				break;
		if (consoleplayer == i)
		{
			// the key player does not adapt
		}
		else
		{
			if (nettics[0] <= nettics[nodeforplayer[i]])
			{
				gametime--;
				// printf ("-");
			}
			frameskip[frameon & 3] = (oldnettics > nettics[nodeforplayer[i]]);
			oldnettics = nettics[0];
			if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3])
			{
				skiptics = 1;
				// printf ("+");
			}
		}
	} // demoplayback

	// wait for new tics if needed
	while (lowtic < gametic / ticdup + counts)
	{
		NetUpdate();
		lowtic = MAXINT;

		for (i = 0; i < doomcom->numnodes; i++)
			if (nodeingame[i] && nettics[i] < lowtic)
				lowtic = nettics[i];

		// don't stay in here forever -- give the menu a chance to work
		if (I_GetTime() / ticdup - entertic >= 20)
		{
			M_Ticker();
			return;
		}
	}

	// run the count * ticdup dics
	while (counts--)
	{
		for (i = 0; i < ticdup; i++)
		{
			if (advancedemo)
				D_DoAdvanceDemo();
			M_Ticker();
			G_Ticker();
			gametic++;

			// modify command for duplicated tics
			if (i != ticdup - 1)
			{
				ticcmd_t *cmd;
				int buf;
				int j;

				buf = (gametic / ticdup) % BACKUPTICS;
				for (j = 0; j < MAXPLAYERS; j++)
				{
					cmd = &netcmds[j][buf];
					cmd->chatchar = 0;
					if (cmd->buttons & BT_SPECIAL)
						cmd->buttons = 0;
				}
			}
		}
		NetUpdate(); // check for new console commands
	}
}
