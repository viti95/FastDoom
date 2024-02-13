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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DOOM Network game communication and protocol,
//	all OS independend parts.
//

#include "m_menu.h"
#include "i_system.h"
#include "g_game.h"
#include "r_main.h"
#include "i_debug.h"
#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "options.h"

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
//
#define PL_DRONE 0x80 // bit flag in doomdata->player

ticcmd_t localcmds[BACKUPTICS];

int maketic;
int skiptics;

void D_ProcessEvents(void);
void G_BuildTiccmd(ticcmd_t *cmd);
void D_DoAdvanceDemo(void);

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
	int i;

	// check time
	nowtime = ticcount;
	newtics = nowtime - gametime;
	gametime = nowtime;

	if (newtics <= 0) // nothing new to update
		return;

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

	// build new ticcmds for console player
	for (i = 0; i < newtics; i++)
	{
		I_StartTic();
		D_ProcessEvents();
		if (maketic - gametic >= BACKUPTICS / 2 - 1)
			break; // can't hold any more

		G_BuildTiccmd(&localcmds[maketic & (BACKUPTICS-1)]);
		maketic++;
	}

	if (singletics)
		return; // singletic update is syncronous
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
void D_CheckNetGame(void)
{
	maketic = 0;
}


extern byte advancedemo;

void RunTick(void)
{
		if (advancedemo)
			D_DoAdvanceDemo();
		M_Ticker();
		G_Ticker();
		gametic++;
		NetUpdate(); // check for new console commands
}

void RunTickUncapped(void)
{
		// TODO (dougvj) I am not sure how to handle the events here, it seems like
		// doing it asynchronously like the TryRunTics causes buffering issues
		// May just have to tell people to not use uncapped mode for low
		// performance systems
		I_StartTic();
		D_ProcessEvents();
		G_BuildTiccmd(&localcmds[maketic & (BACKUPTICS-1)]);
		if (advancedemo)
			D_DoAdvanceDemo();
		M_Ticker();
		D_SetupInterpolation();
		G_Ticker();
		gametic++;
		maketic = gametic + 1;
}

//
// TryRunTics
//


void TryRunTicsCapped(void)
{
	int i;
	int entertic;
	static int oldentertics;
	int realtics;
	int availabletics;
	int counts;

	// get real tics
	entertic = ticcount;
	realtics = entertic - oldentertics;
	oldentertics = entertic;

	// get available tics
	NetUpdate();

	availabletics = maketic - gametic;

	// decide how many tics to run
	if (realtics + 1 < availabletics)
		counts = realtics + 1;
	else if (realtics < availabletics)
		counts = realtics;
	else
		counts = availabletics;

	if (counts < 1)
		counts = 1;

	// wait for new tics if needed
	while (maketic < gametic + counts || uncappedFPS)
	{
		NetUpdate();

		// don't stay in here forever -- give the menu a chance to work
		if (ticcount - entertic >= 20)
		{
			M_Ticker();
			return;
		}
	}
	// run the count dics
	while (counts--)
	{
		RunTick();
	}
}

// Returns the number of tics that ran. Renders interpolated frames between
// ticks if our rendering is fast enough
int ProcessInterpolationAndTics(void) {
	int ticks_ran = 0;
	// First get the time between the last run and the current run
	static int last_ts = 0;
	static int frametime = 0;
	unsigned int new_ts = ticcount_hr;
	// See how many gametics should have elapsed (quantized to 16) ticks,
	// since our hr timer is 1/560 and /16 is 1/35
	int gametics_elapsed = (new_ts >> 4) - (last_ts >> 4);
	int time_elapsed_since_last_process = new_ts - last_ts;
	//I_Printf("last_ts: %d\n", last_ts);
	//I_Printf("new_ts: %d\n", new_ts);
	//I_Printf("time_elapsed_since_last_process: %d\n", time_elapsed_since_last_process);
	if (time_elapsed_since_last_process <= 0) {
		// Don't bother calculating anything new just rerender the
		// previous frame. If we are hitting this then we are > 560 fps
		goto render_frame;
	}
	last_ts = new_ts;
	// If any gametics have elapsed since our last run let's process them.
	if (gametics_elapsed > 0) {
		ticks_ran = gametics_elapsed;
		while (gametics_elapsed--) {
			RunTickUncapped();
		}
	}
	if (frametime > 16) {
		// We are too slow to really interpolate frames
		if (interpolationweight != 0x10000) {
			// We need to reset the interpolation weight
			// to 0x10000 (1.0) but let's not do it immediately
			// becasue that creates a visual artifact
			// We should tell people that if they are near 35 fps in performance,
			// they should just run in vsync or capped mode
			interpolationweight += 0x1000;
			if (interpolationweight >= 0x10000) {
				interpolationweight = 0x10000;
			}
		}
		goto render_frame;
	} else {
		// Now let's process the interpolation weight
		// First figure out where we are in the current gametic
		// We take the lower 4 bits at 0 as the idealized gametic
		// dispatch, so that the lower 4 bits correspond to between
		// gametics
		int current_time_in_gametic = new_ts & 0xf;
		// Where will we be in the next frame Use our last frametime
		// to guesstimate
		int next_time_in_gametic = current_time_in_gametic + frametime;
		// If we are going to be in the next gametic, then we will have to
		// run the next gametic to do the next interpolation
		if (next_time_in_gametic > 16) {
			ticks_ran += 1;
			RunTickUncapped();
			// Adjust the last_ts to the beggining of the next
			// gametic so that we account for running this one early
			// when calculating gametics_elapsed
			last_ts += 16 - current_time_in_gametic;
			// Now move the next_time_in_gametic back to the current gametic
			// since it's been processed
			next_time_in_gametic -= 16;
		}
		// OK we are safely in the current gametic, convert our time to a
		// fixed point number used by the interpolation routine
		interpolationweight = (next_time_in_gametic) << 12;
		ASSERT(interpolationweight <= 0x10000 && interpolationweight >= 0);
	}
render_frame:
	{
		// Render the frame, recording the frametime for the
		// interpolation of the next frame
		int last_framets = ticcount_hr;
		int new_framets;
		D_Display();
		new_framets = ticcount_hr;
		frametime = new_framets - last_framets;
	}
	return ticks_ran;
}

// Like the above but we use an HR timer and D_Display to manage the frame
// and tic rate
void TryRunTicsUncapped(void)
{
	int counts;
	while(1) {
		counts = ProcessInterpolationAndTics();
		if (counts > 0) {
			// IF we ran any tics, we need to break to handle sound etc
			break;
		}
	}
}

void TryRunTics(void)
{
	if (uncappedFPS && !singletics) {
		TryRunTicsUncapped();
	} else {
		TryRunTicsCapped();
	}
}
