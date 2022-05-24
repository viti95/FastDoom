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
//	Player related stuff.
//	Bobbing POV/weapon, movement.
//	Pending weapon.
//

#include "doomdef.h"
#include "d_event.h"

#include "p_local.h"

#include "doomstat.h"

// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP 32

//
// Movement.
//

// 16 pixels of bob
#define MAXBOB 0x100000

byte onground;

//
// P_Thrust
// Moves the given origin along a given angle.
//
void P_Thrust(angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	players.mo->momx += FixedMul(move, finecosine[angle]);
	players.mo->momy += FixedMul(move, finesine[angle]);
}

//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight(void)
{
	int angle;
	fixed_t bob;

	// Regular movement bobbing
	// (needs to be calculated for gun swing
	// even if not on ground)
	// OPTIMIZE: tablify angle
	// Note: a LUT allows for effects
	//  like a ramp with low health.

	players.bob = FixedMul(players.mo->momx, players.mo->momx) + FixedMul(players.mo->momy, players.mo->momy);
	players.bob >>= 2;
	if (players.bob > MAXBOB)
		players.bob = MAXBOB;

	if (!onground)
	{
		players.viewz = players.mo->z + VIEWHEIGHT;

		if (players.viewz > players.mo->ceilingz - 4 * FRACUNIT)
			players.viewz = players.mo->ceilingz - 4 * FRACUNIT;

		players.viewz = players.mo->z + players.viewheight;
		return;
	}

	angle = Mul409(leveltime) & FINEMASK;
	bob = FixedMul(players.bob / 2, finesine[angle]);

	// move viewheight
	if (players.playerstate == PST_LIVE)
	{
		players.viewheight += players.deltaviewheight;

		if (players.viewheight > VIEWHEIGHT)
		{
			players.viewheight = VIEWHEIGHT;
			players.deltaviewheight = 0;
		}
		else if (players.viewheight < VIEWHEIGHT / 2)
		{
			players.viewheight = VIEWHEIGHT / 2;
			if (players.deltaviewheight <= 0)
				players.deltaviewheight = 1;
		}

		if (players.deltaviewheight)
		{
			players.deltaviewheight += FRACUNIT / 4;
			if (!players.deltaviewheight)
				players.deltaviewheight = 1;
		}
	}
	players.viewz = players.mo->z + players.viewheight + bob;

	if (players.viewz > players.mo->ceilingz - 4 * FRACUNIT)
		players.viewz = players.mo->ceilingz - 4 * FRACUNIT;
}

//
// P_MovePlayer
//
void P_MovePlayer(void)
{
	ticcmd_t *cmd;

	cmd = &players.cmd;

	players.mo->angle += (cmd->angleturn << 16);

	// Do not let the player control movement
	//  if not onground.
	onground = (players.mo->z <= players.mo->floorz);

	if (onground)
	{
		if (cmd->forwardmove)
			P_Thrust(players.mo->angle, cmd->forwardmove * 2048);

		if (cmd->sidemove)
			P_Thrust(players.mo->angle - ANG90, cmd->sidemove * 2048);
	}

	if ((cmd->forwardmove || cmd->sidemove) && players.mo->state == &states[S_PLAY])
	{
		P_NotSetMobjState(players.mo, S_PLAY_RUN1);
	}
}

//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
#define ANG5 (ANG90 / 18)

void P_DeathThink(void)
{
	angle_t angle;
	angle_t delta;

	P_MovePsprites();

	// fall to the ground
	if (players.viewheight > 6 * FRACUNIT)
		players.viewheight -= FRACUNIT;
	else if (players.viewheight < 6 * FRACUNIT)
		players.viewheight = 6 * FRACUNIT;

	players.deltaviewheight = 0;
	onground = (players.mo->z <= players.mo->floorz);
	P_CalcHeight();

	if (players.attacker && players.attacker != players.mo)
	{
		angle = R_PointToAngle2(players.mo->x,
								players.mo->y,
								players.attacker->x,
								players.attacker->y);

		delta = angle - players.mo->angle;

		if (delta < ANG5 || delta > (unsigned)-ANG5)
		{
			// Looking at killer,
			//  so fade damage flash down.
			players.mo->angle = angle;
			players.damagecount -= players.damagecount != 0;
		}
		else if (delta < ANG180)
			players.mo->angle += ANG5;
		else
			players.mo->angle -= ANG5;
	}
	else
		players.damagecount -= players.damagecount != 0;

	if (players.cmd.buttons & BT_USE)
		players.playerstate = PST_REBORN;
}

//
// P_PlayerThink
//
void P_PlayerThink(void)
{
	ticcmd_t *cmd;
	weapontype_t newweapon;

	// fixme: do this in the cheat code
	if (players.cheats & CF_NOCLIP)
		players.mo->flags |= MF_NOCLIP;
	else
		players.mo->flags &= ~MF_NOCLIP;

	// chain saw run forward
	cmd = &players.cmd;
	if (players.mo->flags & MF_JUSTATTACKED)
	{
		cmd->angleturn = 0;
		cmd->forwardmove = 0xc800 / 512;
		cmd->sidemove = 0;
		players.mo->flags &= ~MF_JUSTATTACKED;
	}

	if (players.playerstate == PST_DEAD)
	{
		P_DeathThink();
		return;
	}

	// Move around.
	// Reactiontime is used to prevent movement
	//  for a bit after a teleport.
	if (players.mo->reactiontime)
		players.mo->reactiontime--;
	else
		P_MovePlayer();

	P_CalcHeight();

	if (players.mo->subsector->sector->special)
		P_PlayerInSpecialSector();

	// Check for weapon change.

	// A special event has no other buttons.
	if (cmd->buttons & BT_SPECIAL)
		cmd->buttons = 0;

	if (cmd->buttons & BT_CHANGE)
	{
		// The actual changing of the weapon is done
		//  when the weapon psprite can do it
		//  (read: not in the middle of an attack).
		newweapon = (cmd->buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT;

		if (newweapon == wp_fist && players.weaponowned[wp_chainsaw] && !(players.readyweapon == wp_chainsaw && players.powers[pw_strength]))
		{
			newweapon = wp_chainsaw;
		}

		if (gamemode == commercial && newweapon == wp_shotgun && players.weaponowned[wp_supershotgun] && players.readyweapon != wp_supershotgun)
		{
			newweapon = wp_supershotgun;
		}

		if (players.weaponowned[newweapon] && newweapon != players.readyweapon)
		{
			// Do not go to plasma or BFG in shareware,
			//  even if cheated.
			if ((newweapon != wp_plasma && newweapon != wp_bfg) || gamemode != shareware)
			{
				players.pendingweapon = newweapon;
			}
		}
	}

	// check for use
	if (cmd->buttons & BT_USE)
	{
		if (!players.usedown)
		{
			P_UseLines();
			players.usedown = true;
		}
	}
	else
		players.usedown = false;

	// cycle psprites
	P_MovePsprites();

	// Counters, time dependend power ups.

	// Strength counts up to diminish fade.

	players.powers[pw_strength] += players.powers[pw_strength] != 0;
	players.powers[pw_invulnerability] -= players.powers[pw_invulnerability] != 0;

	if (players.powers[pw_invisibility])
		if (!--players.powers[pw_invisibility])
			players.mo->flags &= ~MF_SHADOW;

	players.powers[pw_infrared] -= players.powers[pw_infrared] != 0;
	players.powers[pw_ironfeet] -= players.powers[pw_ironfeet] != 0;
	players.damagecount -= players.damagecount != 0;
	players.bonuscount -= players.bonuscount != 0;

	// Handling colormaps.
	if (players.powers[pw_invulnerability])
	{
		if (players.powers[pw_invulnerability] > 4 * 32 || (players.powers[pw_invulnerability] & 8))
			players.fixedcolormap = INVERSECOLORMAP;
		else
			players.fixedcolormap = 0;
	}
	else if (players.powers[pw_infrared])
	{
		players.fixedcolormap = players.powers[pw_infrared] > 4 * 32 || (players.powers[pw_infrared] & 8); // almost full bright
	}
	else
		players.fixedcolormap = 0;
}
