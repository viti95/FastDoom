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
//	Handling interactions (i.e., collisions).
//

#include <string.h>

// Data.
#include "doomdef.h"
#include "dstrings.h"
#include "sounds.h"
#include "options.h"
#include "doomstat.h"

#include "i_random.h"

#include "m_misc.h"
#include "i_system.h"

#include "am_map.h"

#include "p_local.h"

#include "s_sound.h"

#include "p_inter.h"

#define BONUSADD 6

// a weapon is found with two clip loads,
// a big item has five clip loads
int maxammo[NUMAMMO] = {200, 50, 300, 50};

//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

byte P_GiveAmmo(ammotype_t ammo, int num)
{
	const int clipammo[NUMAMMO] = {10, 4, 20, 1};
	int oldammo;

	if (ammo == am_noammo || players.ammo[ammo] == players.maxammo[ammo])
		return 0;

	if (num)
		num *= clipammo[ammo];
	else
		num = clipammo[ammo] / 2;

	// give double ammo in trainer mode,
	// you'll need in nightmare
	num <<= gameskill == sk_baby || gameskill == sk_nightmare;

	oldammo = players.ammo[ammo];
	players.ammo[ammo] += num;
	if (players.ammo[ammo] > players.maxammo[ammo])
		players.ammo[ammo] = players.maxammo[ammo];

	// If non zero ammo,
	// don't change up weapons,
	// player was lower on purpose.
	if (oldammo)
		return 1;

	// We were down to zero,
	// so select a new weapon.
	// Preferences are not user selectable.
	switch (ammo)
	{
	case am_clip:
		if (players.readyweapon == wp_fist)
		{
			if (players.weaponowned[wp_chaingun])
				players.pendingweapon = wp_chaingun;
			else
				players.pendingweapon = wp_pistol;
		}
		break;

	case am_shell:
		if ((players.readyweapon == wp_fist || players.readyweapon == wp_pistol) && players.weaponowned[wp_shotgun])
		{
			players.pendingweapon = wp_shotgun;
		}
		break;

	case am_cell:
		if ((players.readyweapon == wp_fist || players.readyweapon == wp_pistol) && players.weaponowned[wp_plasma])
		{
			players.pendingweapon = wp_plasma;
		}
		break;

	case am_misl:
		if (players.readyweapon == wp_fist && players.weaponowned[wp_missile])
		{
			players.pendingweapon = wp_missile;
		}
	}

	return 1;
}

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
byte P_GiveWeapon(weapontype_t weapon, boolean dropped)
{
	byte gaveammo;
	byte gaveweapon;

	if (weaponinfo[weapon].ammo != am_noammo)
	{
		// give one clip with a dropped weapon,
		// two clips with a found weapon
		if (dropped)
			gaveammo = P_GiveAmmo(weaponinfo[weapon].ammo, 1);
		else
			gaveammo = P_GiveAmmo(weaponinfo[weapon].ammo, 2);
	}
	else
		gaveammo = 0;

	if (players.weaponowned[weapon])
		gaveweapon = 0;
	else
	{
		gaveweapon = 1;
		players.weaponowned[weapon] = true;
		players.pendingweapon = weapon;
	}

	return (gaveweapon || gaveammo);
}

//
// P_GiveBody
// Returns false if the body isn't needed at all
//
byte P_NotGiveBody(int num)
{
	if (players.health >= MAXHEALTH)
		return 1;

	players.health += num;
	if (players.health > MAXHEALTH)
		players.health = MAXHEALTH;
	players_mo->health = players.health;

	return 0;
}

//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
byte P_NotGiveArmor(int armortype)
{
	int hits;

	hits = Mul100(armortype);
	if (players.armorpoints >= hits)
		return 1; // don't pick up

	players.armortype = armortype;
	players.armorpoints = hits;

	return 0;
}

//
// P_GiveCard
//
void P_GiveCard(card_t card)
{
	if (players.cards[card])
		return;

	players.bonuscount = BONUSADD;
	players.cards[card] = 1;
}

//
// P_GivePower
//
byte P_NotGivePower(int power)
{
	switch (power)
	{
	case pw_invulnerability:
		players.powers[power] = INVULNTICS;
		return 0;
	case pw_invisibility:
		players.powers[power] = INVISTICS;
		players_mo->flags |= MF_SHADOW;
		return 0;
	case pw_infrared:
		players.powers[power] = INFRATICS;
		return 0;
	case pw_ironfeet:
		players.powers[power] = IRONTICS;
		return 0;
	case pw_strength:
		P_NotGiveBody(100);
		players.powers[power] = 1;
		return 0;
	}

	if (players.powers[power])
		return 1; // already got it

	players.powers[power] = 1;
	return 0;
}

//
// P_TouchSpecialThing
//
void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher)
{
	//player_t *player;
	int i;
	fixed_t delta;
	int sound;

	if (toucher->health <= 0)
		return;

	delta = special->z - toucher->z;

	if (delta > toucher->height || delta < -8 * FRACUNIT)
		return;

	sound = sfx_itemup;
	//player = toucher->player;

	// Identify by sprite.
	switch (special->sprite)
	{
		// armor
	case SPR_ARM1:
		if (P_NotGiveArmor(1))
			return;
		players.message = GOTARMOR;
		break;

	case SPR_ARM2:
		if (P_NotGiveArmor(2))
			return;
		players.message = GOTMEGA;
		break;

		// bonus items
	case SPR_BON1:
		players.health++; // can go over 100%
		if (players.health > 200)
			players.health = 200;
		players_mo->health = players.health;
		players.message = GOTHTHBONUS;
		break;

	case SPR_BON2:
		players.armorpoints++; // can go over 100%
		if (players.armorpoints > 200)
			players.armorpoints = 200;
		if (!players.armortype)
			players.armortype = 1;
		players.message = GOTARMBONUS;
		break;

	case SPR_SOUL:
		players.health += 100;
		if (players.health > 200)
			players.health = 200;
		players_mo->health = players.health;
		players.message = GOTSUPER;
		sound = sfx_getpow;
		break;

	case SPR_MEGA:
		if (gamemode != commercial)
			return;
		players.health = 200;
		players_mo->health = players.health;
		P_NotGiveArmor(2);
		players.message = GOTMSPHERE;
		sound = sfx_getpow;
		break;

		// cards
		// leave cards for everyone
	case SPR_BKEY:
		if (!players.cards[it_bluecard])
			players.message = GOTBLUECARD;
		P_GiveCard(it_bluecard);
		break;

	case SPR_YKEY:
		if (!players.cards[it_yellowcard])
			players.message = GOTYELWCARD;
		P_GiveCard(it_yellowcard);
		break;

	case SPR_RKEY:
		if (!players.cards[it_redcard])
			players.message = GOTREDCARD;
		P_GiveCard(it_redcard);
		break;

	case SPR_BSKU:
		if (!players.cards[it_blueskull])
			players.message = GOTBLUESKUL;
		P_GiveCard(it_blueskull);
		break;

	case SPR_YSKU:
		if (!players.cards[it_yellowskull])
			players.message = GOTYELWSKUL;
		P_GiveCard(it_yellowskull);
		break;

	case SPR_RSKU:
		if (!players.cards[it_redskull])
			players.message = GOTREDSKULL;
		P_GiveCard(it_redskull);
		break;

		// medikits, heals
	case SPR_STIM:
		if (P_NotGiveBody(10))
			return;
		players.message = GOTSTIM;
		break;

	case SPR_MEDI:
		if (P_NotGiveBody(25))
			return;
		players.message = GOTMEDIKIT;
		break;

		// power ups
	case SPR_PINV:
		if (P_NotGivePower(pw_invulnerability))
			return;
		players.message = GOTINVUL;
		sound = sfx_getpow;
		break;

	case SPR_PSTR:
		if (P_NotGivePower(pw_strength))
			return;
		players.message = GOTBERSERK;
		if (players.readyweapon != wp_fist)
			players.pendingweapon = wp_fist;
		sound = sfx_getpow;
		break;

	case SPR_PINS:
		if (P_NotGivePower(pw_invisibility))
			return;
		players.message = GOTINVIS;
		sound = sfx_getpow;
		break;

	case SPR_SUIT:
		if (P_NotGivePower(pw_ironfeet))
			return;
		players.message = GOTSUIT;
		sound = sfx_getpow;
		break;

	case SPR_PMAP:
		if (P_NotGivePower(pw_allmap))
			return;
		players.message = GOTMAP;
		sound = sfx_getpow;
		break;

	case SPR_PVIS:
		if (P_NotGivePower(pw_infrared))
			return;
		players.message = GOTVISOR;
		sound = sfx_getpow;
		break;

		// ammo
	case SPR_CLIP:
		if (special->flags & MF_DROPPED)
		{
			if (!P_GiveAmmo(am_clip, 0))
				return;
		}
		else
		{
			if (!P_GiveAmmo(am_clip, 1))
				return;
		}
		players.message = GOTCLIP;
		break;

	case SPR_AMMO:
		if (!P_GiveAmmo(am_clip, 5))
			return;
		players.message = GOTCLIPBOX;
		break;

	case SPR_ROCK:
		if (!P_GiveAmmo(am_misl, 1))
			return;
		players.message = GOTROCKET;
		break;

	case SPR_BROK:
		if (!P_GiveAmmo(am_misl, 5))
			return;
		players.message = GOTROCKBOX;
		break;

	case SPR_CELL:
		if (!P_GiveAmmo(am_cell, 1))
			return;
		players.message = GOTCELL;
		break;

	case SPR_CELP:
		if (!P_GiveAmmo(am_cell, 5))
			return;
		players.message = GOTCELLBOX;
		break;

	case SPR_SHEL:
		if (!P_GiveAmmo(am_shell, 1))
			return;
		players.message = GOTSHELLS;
		break;

	case SPR_SBOX:
		if (!P_GiveAmmo(am_shell, 5))
			return;
		players.message = GOTSHELLBOX;
		break;

	case SPR_BPAK:
		if (!players.backpack)
		{
			players.maxammo[0] *= 2;
			players.maxammo[1] *= 2;
			players.maxammo[2] *= 2;
			players.maxammo[3] *= 2;
			players.backpack = 1;
		}
		P_GiveAmmo(0, 1);
		P_GiveAmmo(1, 1);
		P_GiveAmmo(2, 1);
		P_GiveAmmo(3, 1);
		players.message = GOTBACKPACK;
		break;

		// weapons
	case SPR_BFUG:
		if (!P_GiveWeapon(wp_bfg, false))
			return;
		players.message = GOTBFG9000;
		sound = sfx_wpnup;
		break;

	case SPR_MGUN:
		if (!P_GiveWeapon(wp_chaingun, special->flags & MF_DROPPED))
			return;
		players.message = GOTCHAINGUN;
		sound = sfx_wpnup;
		break;

	case SPR_CSAW:
		if (!P_GiveWeapon(wp_chainsaw, false))
			return;
		players.message = GOTCHAINSAW;
		sound = sfx_wpnup;
		break;

	case SPR_LAUN:
		if (!P_GiveWeapon(wp_missile, false))
			return;
		players.message = GOTLAUNCHER;
		sound = sfx_wpnup;
		break;

	case SPR_PLAS:
		if (!P_GiveWeapon(wp_plasma, false))
			return;
		players.message = GOTPLASMA;
		sound = sfx_wpnup;
		break;

	case SPR_SHOT:
		if (!P_GiveWeapon(wp_shotgun, special->flags & MF_DROPPED))
			return;
		players.message = GOTSHOTGUN;
		sound = sfx_wpnup;
		break;

	case SPR_SGN2:
		if (!P_GiveWeapon(wp_supershotgun, special->flags & MF_DROPPED))
			return;
		players.message = GOTSHOTGUN2;
		sound = sfx_wpnup;
		break;
	}

	players.itemcount += (special->flags & MF_COUNTITEM) != 0;

	P_RemoveMobj(special);
	players.bonuscount += BONUSADD;
	S_StartSound(NULL, sound);
}

//
// KillMobj
//
void P_KillMobj(mobj_t *target)
{
	byte item;
	mobj_t *mo;

	target->flags &= ~(MF_SHOOTABLE | MF_FLOAT | MF_SKULLFLY);

	if (target->type != MT_SKULL)
		target->flags &= ~MF_NOGRAVITY;

	target->flags |= MF_CORPSE | MF_DROPOFF;
	target->height >>= 2;

	players.killcount += (target->flags & MF_COUNTKILL) != 0;
	
	if (target->player)
	{
		target->flags &= ~MF_SOLID;
		target->player->playerstate = PST_DEAD;
		P_DropWeapon();

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
		if (target->player == &players && automapactive)
		{
			// don't die in auto map,
			// switch view prior to dying
			AM_Stop();
		}
#endif
	}

	if (target->health < -target->info->spawnhealth && target->info->xdeathstate)
	{
		P_NotSetMobjState(target, target->info->xdeathstate);
	}
	else
		P_NotSetMobjState(target, target->info->deathstate);

	target->tics -= P_Random_And3;

	if (target->tics < 1)
		target->tics = 1;

	// Drop stuff.
	// This determines the kind of object spawned
	// during the death frame of a thing.
	switch (target->type)
	{
	case MT_WOLFSS:
	case MT_POSSESSED:
		item = MT_CLIP;
		break;

	case MT_SHOTGUY:
		item = MT_SHOTGUN;
		break;

	case MT_CHAINGUY:
		item = MT_CHAINGUN;
		break;

	default:
		return;
	}

	mo = P_SpawnMobj(target->x, target->y, ONFLOORZ, item);
	mo->flags |= MF_DROPPED; // special versions of items
}

//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
void P_DamageMobj(mobj_t *target,
				  mobj_t *inflictor,
				  mobj_t *source,
				  int damage)
{
	unsigned ang;
	int saved;
	player_t *player;
	fixed_t thrust;
	int temp;

	if (!(target->flags & MF_SHOOTABLE) || target->health <= 0)
		return; // shouldn't happen...

	if (target->flags & MF_SKULLFLY)
	{
		target->momx = target->momy = target->momz = 0;
	}

	player = target->player;
	damage >>= player && gameskill == sk_baby; // take half damage in trainer mode

	// Some close combat weapons should not
	// inflict thrust and push the victim out of reach,
	// thus kick away unless using the chainsaw.
	if (inflictor && !(target->flags & MF_NOCLIP) && (!source || !source->player || source->player->readyweapon != wp_chainsaw))
	{
		ang = R_PointToAngle2(inflictor->x,
							  inflictor->y,
							  target->x,
							  target->y);

		// VITI95: OPTIMIZE
		thrust = Mul819200(damage) / target->info->mass;

		// make fall forwards sometimes
		if (damage < 40 && damage > target->health && target->z - inflictor->z > 64 * FRACUNIT && (P_Random_And1))
		{
			ang += ANG180;
			thrust *= 4;
		}

		ang >>= ANGLETOFINESHIFT;
		target->momx += FixedMul(thrust, finecosine[ang]);
		target->momy += FixedMul(thrust, finesine[ang]);
	}

	// player specific
	if (player)
	{
		// end of game hell hack
		if (target->subsector->sector->special == 11 && damage >= target->health)
		{
			damage = target->health - 1;
		}

		// Below certain threshold,
		// ignore damage in GOD mode, or with INVUL power.
		if (damage < 1000 && ((player->cheats & CF_GODMODE) || player->powers[pw_invulnerability]))
		{
			return;
		}

		if (player->armortype)
		{
			if (player->armortype == 1)
				saved = Div3(damage);
			else
				saved = damage / 2;

			if (player->armorpoints <= saved)
			{
				// armor is used up
				saved = player->armorpoints;
				player->armortype = 0;
			}
			player->armorpoints -= saved;
			damage -= saved;
		}
		player->health -= damage; // mirror mobj health here for Dave
		if (player->health < 0)
			player->health = 0;

		player->attacker = source;
		player->damagecount += damage; // add damage after armor / invuln

		if (player->damagecount > 100)
			player->damagecount = 100; // teleport stomp does 10k points...
	}

	// do the damage
	target->health -= damage;
	if (target->health <= 0)
	{
		P_KillMobj(target);
		return;
	}

	if ((P_Random < target->info->painchance) && !(target->flags & MF_SKULLFLY))
	{
		target->flags |= MF_JUSTHIT; // fight back!
		P_NotSetMobjState(target, target->info->painstate);
	}

	target->reactiontime = 0; // we're awake now...

	if ((!target->threshold || target->type == MT_VILE) && source && source != target && source->type != MT_VILE)
	{
		// if not intent on another player,
		// chase after this one
		target->target = source;
		target->threshold = BASETHRESHOLD;
		if (target->state == &states[target->info->spawnstate] && target->info->seestate != S_NULL)
			P_NotSetMobjState(target, target->info->seestate);
	}
}
