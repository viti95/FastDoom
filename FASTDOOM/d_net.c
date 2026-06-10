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

int maketic = 0;

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
	int delta;
	// check time
	nowtime = ticcount;
	newtics = nowtime - gametime;
	gametime = nowtime;

	if (newtics <= 0) {
		return;
	}

	if (uncappedFPS) {
		if (newtics > 1) {
			newtics = 1;
		}
	
		delta = maketic - gametic;
		if (delta < 0) {
			maketic = gametic;
		}
	} 
	
	//I_Printf("maketics: %d, gametic: %d, newtics: %d, delta: %d\n", maketic, gametic, newtics, delta);

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
	while (maketic < gametic + counts)
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
       static int last_ts = 0;
       unsigned int new_ts = ticcount_hr;
       int gametics_elapsed, time_elapsed_since_last_process;
       if (last_ts == 0) {
           last_ts = new_ts;
       }
       gametics_elapsed = (new_ts >> 4) - (last_ts >> 4);
       if (gametics_elapsed < 0) {
           gametics_elapsed = 0;
       }
       time_elapsed_since_last_process = new_ts - last_ts;
       if (time_elapsed_since_last_process <= 0) {
           goto render_frame;
       }
       last_ts = new_ts;

       if (frametime_hrticks > 16) {
           if (interpolation_weight != 0x10000) {
               interpolation_weight += 0x1000;
               if (interpolation_weight >= 0x10000) {
                   interpolation_weight = 0x10000;
               }
           }
       } else {
           int current_time_in_gametic = new_ts & 0xf;
           int next_time_in_gametic = current_time_in_gametic + frametime_hrticks;
           if (next_time_in_gametic > 16) {
               gametics_elapsed++;
               gametime--;
               maketic--;
               next_time_in_gametic -= 16;
               last_ts += 16 - current_time_in_gametic;
           }
           interpolation_weight = (next_time_in_gametic) << 12;
           ASSERT(interpolation_weight <= 0x10000 && interpolation_weight >= 0);
       }

   render_frame:
       ticks_ran = gametics_elapsed;

       // FIX: Call NetUpdate ONCE before the loop to refill the tic buffer.
       // Do NOT call NetUpdate inside the loop.
       NetUpdate();

       // Run the tics that are already available
       while (gametics_elapsed > 0 && maketic > gametic) {
           D_SetupInterpolation();
           RunTick();
           gametics_elapsed--;
       }

       {
           int last_framets = ticcount_hr;
           int new_framets;
           D_Display();
           new_framets = ticcount_hr;
           frametime_hrticks = new_framets - last_framets;
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
		NetUpdate();
		if (counts > 0) {
			// IF we ran any tics, we need to break to handle sound etc
			return;
		}
	}
}

void TryRunTics(void)
{
	if (highResTimer && !singletics) {
		TryRunTicsUncapped();
	} else {
		TryRunTicsCapped();
	}
}
