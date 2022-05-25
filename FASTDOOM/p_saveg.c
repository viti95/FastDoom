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
//	Archiving: SaveGame I/O.
//

#include <string.h>

#include "i_system.h"
#include "z_zone.h"
#include "p_local.h"

// State.
#include "doomstat.h"
#include "r_state.h"

byte *save_p;

// Pads save_p to a 4-byte boundary
//  so that the load/save works on SGI&Gecko.
#define PADSAVEP() save_p += (4 - ((int)save_p & 3)) & 3

//
// P_ArchivePlayers
//
void P_ArchivePlayers(void)
{
	int j;
	player_t *dest;

	PADSAVEP();

	dest = (player_t *)save_p;
	CopyBytes(&players, dest, sizeof(player_t));
	//memcpy(dest, &players, sizeof(player_t));
	save_p += sizeof(player_t);
	for (j = 0; j < NUMPSPRITES; j++)
	{
		if (dest->psprites[j].state)
		{
			dest->psprites[j].state = (state_t *)(dest->psprites[j].state - states);
		}
	}
}

//
// P_UnArchivePlayers
//
void P_UnArchivePlayers(void)
{
	int j;

	PADSAVEP();

	CopyBytes(save_p, &players, sizeof(player_t));
	//memcpy(&players, save_p, sizeof(player_t));
	save_p += sizeof(player_t);

	// will be set when unarc thinker
	players_mo = NULL;
	players_mo = NULL;
	players.message = NULL;
	players.attacker = NULL;

	for (j = 0; j < NUMPSPRITES; j++)
	{
		if (players.psprites[j].state)
		{
			players.psprites[j].state = &states[(int)players.psprites[j].state];
		}
	}
}

//
// P_ArchiveWorld
//
void P_ArchiveWorld(void)
{
	int i;
	int j;
	sector_t *sec;
	line_t *li;
	side_t *si;
	short *put;

	put = (short *)save_p;

	// do sectors
	for (i = 0, sec = sectors; i < numsectors; i++, sec++)
	{
		*put++ = sec->floorheight >> FRACBITS;
		*put++ = sec->ceilingheight >> FRACBITS;
		*put++ = sec->floorpic;
		*put++ = sec->ceilingpic;
		*put++ = sec->lightlevel;
		*put++ = sec->special; // needed?
		*put++ = sec->tag;	   // needed?
	}

	// do lines
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		*put++ = li->flags;
		*put++ = li->special;
		*put++ = li->tag;
		for (j = 0; j < 2; j++)
		{
			if (li->sidenum[j] == -1)
				continue;

			si = &sides[li->sidenum[j]];

			*put++ = si->textureoffset >> FRACBITS;
			*put++ = si->rowoffset >> FRACBITS;
			*put++ = si->toptexture;
			*put++ = si->bottomtexture;
			*put++ = si->midtexture;
		}
	}

	save_p = (byte *)put;
}

//
// P_UnArchiveWorld
//
void P_UnArchiveWorld(void)
{
	int i;
	int j;
	sector_t *sec;
	line_t *li;
	side_t *si;
	short *get;

	get = (short *)save_p;

	// do sectors
	for (i = 0, sec = sectors; i < numsectors; i++, sec++)
	{
		sec->floorheight = *get++ << FRACBITS;
		sec->ceilingheight = *get++ << FRACBITS;
		sec->floorpic = *get++;
		sec->ceilingpic = *get++;
		sec->lightlevel = *get++;
		sec->special = *get++; // needed?
		sec->tag = *get++;	   // needed?
		sec->specialdata = 0;
		sec->soundtarget = 0;
	}

	// do lines
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		li->flags = *get++;
		li->special = *get++;
		li->tag = *get++;
		for (j = 0; j < 2; j++)
		{
			if (li->sidenum[j] == -1)
				continue;
			si = &sides[li->sidenum[j]];
			si->textureoffset = *get++ << FRACBITS;
			si->rowoffset = *get++ << FRACBITS;
			si->toptexture = *get++;
			si->bottomtexture = *get++;
			si->midtexture = *get++;
		}
	}
	save_p = (byte *)get;
}

//
// Thinkers
//
typedef enum
{
	tc_end,
	tc_mobj

} thinkerclass_t;

//
// P_ArchiveThinkers
//
void P_ArchiveThinkers(void)
{
	thinker_t *th;
	mobj_t *mobj;

	// save off the current thinkers
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_MobjThinker || th->function.acp1 == (actionf_p1)P_MobjBrainlessThinker || th->function.acp1 == (actionf_p1)P_MobjTicklessThinker)
		{
			*save_p++ = tc_mobj;
			PADSAVEP();
			mobj = (mobj_t *)save_p;
			CopyBytes(th, mobj, sizeof(*mobj));
			//memcpy(mobj, th, sizeof(*mobj));
			save_p += sizeof(*mobj);
			mobj->state = (state_t *)(mobj->state - states);

			if (mobj->player)
				mobj->player = (player_t *)(mobj->player);
			continue;
		}
	}

	// add a terminating marker
	*save_p++ = tc_end;
}

//
// P_UnArchiveThinkers
//
void P_UnArchiveThinkers(void)
{
	byte tclass;
	thinker_t *currentthinker;
	thinker_t *next;
	mobj_t *mobj;

	// remove all the current thinkers
	currentthinker = thinkercap.next;
	while (currentthinker != &thinkercap)
	{
		next = currentthinker->next;

		if (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker || currentthinker->function.acp1 == (actionf_p1)P_MobjBrainlessThinker || currentthinker->function.acp1 == (actionf_p1)P_MobjTicklessThinker)
			P_RemoveMobj((mobj_t *)currentthinker);
		else
			Z_Free(currentthinker);

		currentthinker = next;
	}
	P_InitThinkers();

	// read in saved thinkers
	while (1)
	{
		tclass = *save_p++;
		switch (tclass)
		{
		case tc_end:
			return; // end of list

		case tc_mobj:
			PADSAVEP();
			mobj = Z_MallocUnowned(sizeof(*mobj), PU_LEVEL);
			CopyBytes(save_p, mobj, sizeof(*mobj));
			//memcpy(mobj, save_p, sizeof(*mobj));
			save_p += sizeof(*mobj);
			mobj->state = &states[(int)mobj->state];
			mobj->target = NULL;
			if (mobj->player)
			{
				mobj->player = &players;
				mobj->player->mo = mobj;
				players_mo = mobj;
			}
			P_SetThingPosition(mobj);
			mobj->info = &mobjinfo[mobj->type];
			mobj->floorz = mobj->subsector->sector->floorheight;
			mobj->ceilingz = mobj->subsector->sector->ceilingheight;
			mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;

			thinkercap.prev->next = &mobj->thinker;
    		mobj->thinker.next = &thinkercap;
    		mobj->thinker.prev = thinkercap.prev;
			thinkercap.prev = &mobj->thinker;

			break;

		default:
			I_Error("Unknown tclass %i in savegame", tclass);
		}
	}
}

//
// P_ArchiveSpecials
//
#define tc_ceiling 0
#define tc_door 1
#define tc_floor 2
#define tc_plat 3
#define tc_flash 4
#define tc_strobe 5
#define tc_glow 6
#define tc_endspecials 7

//
// Things to handle:
//
// T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
// T_VerticalDoor, (vldoor_t: sector_t * swizzle),
// T_MoveFloor, (floormove_t: sector_t * swizzle),
// T_LightFlash, (lightflash_t: sector_t * swizzle),
// T_StrobeFlash, (strobe_t: sector_t *),
// T_Glow, (glow_t: sector_t *),
// T_PlatRaise, (plat_t: sector_t *), - active list
//
void P_ArchiveSpecials(void)
{
	thinker_t *th;
	ceiling_t *ceiling;
	vldoor_t *door;
	floormove_t *floor;
	plat_t *plat;
	lightflash_t *flash;
	strobe_t *strobe;
	glow_t *glow;
	int i;

	// save off the current thinkers
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acv == (actionf_v)NULL)
		{
			for (i = 0; i < MAXCEILINGS; i++)
				if (activeceilings[i] == (ceiling_t *)th)
					break;

			if (i < MAXCEILINGS)
			{
				*save_p++ = tc_ceiling;
				PADSAVEP();
				ceiling = (ceiling_t *)save_p;
				CopyBytes(th, ceiling, sizeof(*ceiling));				
				//memcpy(ceiling, th, sizeof(*ceiling));
				save_p += sizeof(*ceiling);
				ceiling->sector = (sector_t *)(ceiling->sector - sectors);
			}
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
		{
			*save_p++ = tc_ceiling;
			PADSAVEP();
			ceiling = (ceiling_t *)save_p;
			CopyBytes(th, ceiling, sizeof(*ceiling));
			//memcpy(ceiling, th, sizeof(*ceiling));
			save_p += sizeof(*ceiling);
			ceiling->sector = (sector_t *)(ceiling->sector - sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_VerticalDoor)
		{
			*save_p++ = tc_door;
			PADSAVEP();
			door = (vldoor_t *)save_p;
			CopyBytes(th, door, sizeof(*door));
			//memcpy(door, th, sizeof(*door));
			save_p += sizeof(*door);
			door->sector = (sector_t *)(door->sector - sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_MoveFloor)
		{
			*save_p++ = tc_floor;
			PADSAVEP();
			floor = (floormove_t *)save_p;
			CopyBytes(th, floor, sizeof(*floor));
			//memcpy(floor, th, sizeof(*floor));
			save_p += sizeof(*floor);
			floor->sector = (sector_t *)(floor->sector - sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_PlatRaise)
		{
			*save_p++ = tc_plat;
			PADSAVEP();
			plat = (plat_t *)save_p;
			CopyBytes(th, plat, sizeof(*plat));
			//memcpy(plat, th, sizeof(*plat));
			save_p += sizeof(*plat);
			plat->sector = (sector_t *)(plat->sector - sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_LightFlash)
		{
			*save_p++ = tc_flash;
			PADSAVEP();
			flash = (lightflash_t *)save_p;
			CopyBytes(th, flash, sizeof(*flash));
			//memcpy(flash, th, sizeof(*flash));
			save_p += sizeof(*flash);
			flash->sector = (sector_t *)(flash->sector - sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
		{
			*save_p++ = tc_strobe;
			PADSAVEP();
			strobe = (strobe_t *)save_p;
			CopyBytes(th, strobe, sizeof(*strobe));
			//memcpy(strobe, th, sizeof(*strobe));
			save_p += sizeof(*strobe);
			strobe->sector = (sector_t *)(strobe->sector - sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_Glow)
		{
			*save_p++ = tc_glow;
			PADSAVEP();
			glow = (glow_t *)save_p;
			CopyBytes(th, glow, sizeof(*glow));
			//memcpy(glow, th, sizeof(*glow));
			save_p += sizeof(*glow);
			glow->sector = (sector_t *)(glow->sector - sectors);
			continue;
		}
	}

	// add a terminating marker
	*save_p++ = tc_endspecials;
}

//
// P_UnArchiveSpecials
//
void P_UnArchiveSpecials(void)
{
	byte tclass;
	ceiling_t *ceiling;
	vldoor_t *door;
	floormove_t *floor;
	plat_t *plat;
	lightflash_t *flash;
	strobe_t *strobe;
	glow_t *glow;

	// read in saved thinkers
	while (1)
	{
		tclass = *save_p++;
		switch (tclass)
		{
		case tc_endspecials:
			return; // end of list

		case tc_ceiling:
			PADSAVEP();
			ceiling = Z_MallocUnowned(sizeof(*ceiling), PU_LEVEL);
			CopyBytes(save_p, ceiling, sizeof(*ceiling));
			//memcpy(ceiling, save_p, sizeof(*ceiling));
			save_p += sizeof(*ceiling);
			ceiling->sector = &sectors[(int)ceiling->sector];
			ceiling->sector->specialdata = ceiling;

			if (ceiling->thinker.function.acp1)
				ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;

			thinkercap.prev->next = &ceiling->thinker;
    		ceiling->thinker.next = &thinkercap;
    		ceiling->thinker.prev = thinkercap.prev;
			thinkercap.prev = &ceiling->thinker;

			P_AddActiveCeiling(ceiling);
			break;

		case tc_door:
			PADSAVEP();
			door = Z_MallocUnowned(sizeof(*door), PU_LEVEL);
			CopyBytes(save_p, door, sizeof(*door));
			//memcpy(door, save_p, sizeof(*door));
			save_p += sizeof(*door);
			door->sector = &sectors[(int)door->sector];
			door->sector->specialdata = door;
			door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
			
			thinkercap.prev->next = &door->thinker;
    		door->thinker.next = &thinkercap;
    		door->thinker.prev = thinkercap.prev;
			thinkercap.prev = &door->thinker;
			
			break;

		case tc_floor:
			PADSAVEP();
			floor = Z_MallocUnowned(sizeof(*floor), PU_LEVEL);
			CopyBytes(save_p, floor, sizeof(*floor));
			//memcpy(floor, save_p, sizeof(*floor));
			save_p += sizeof(*floor);
			floor->sector = &sectors[(int)floor->sector];
			floor->sector->specialdata = floor;
			floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
			
			thinkercap.prev->next = &floor->thinker;
    		floor->thinker.next = &thinkercap;
    		floor->thinker.prev = thinkercap.prev;
			thinkercap.prev = &floor->thinker;
			
			break;

		case tc_plat:
			PADSAVEP();
			plat = Z_MallocUnowned(sizeof(*plat), PU_LEVEL);
			CopyBytes(save_p, plat, sizeof(*plat));
			//memcpy(plat, save_p, sizeof(*plat));
			save_p += sizeof(*plat);
			plat->sector = &sectors[(int)plat->sector];
			plat->sector->specialdata = plat;

			if (plat->thinker.function.acp1)
				plat->thinker.function.acp1 = (actionf_p1)T_PlatRaise;

			thinkercap.prev->next = &plat->thinker;
    		plat->thinker.next = &thinkercap;
    		plat->thinker.prev = thinkercap.prev;
			thinkercap.prev = &plat->thinker;
			
			P_AddActivePlat(plat);
			break;

		case tc_flash:
			PADSAVEP();
			flash = Z_MallocUnowned(sizeof(*flash), PU_LEVEL);
			CopyBytes(save_p, flash, sizeof(*flash));
			//memcpy(flash, save_p, sizeof(*flash));
			save_p += sizeof(*flash);
			flash->sector = &sectors[(int)flash->sector];
			flash->thinker.function.acp1 = (actionf_p1)T_LightFlash;

			thinkercap.prev->next = &flash->thinker;
    		flash->thinker.next = &thinkercap;
    		flash->thinker.prev = thinkercap.prev;
			thinkercap.prev = &flash->thinker;

			break;

		case tc_strobe:
			PADSAVEP();
			strobe = Z_MallocUnowned(sizeof(*strobe), PU_LEVEL);
			CopyBytes(save_p, strobe, sizeof(*strobe));
			//memcpy(strobe, save_p, sizeof(*strobe));
			save_p += sizeof(*strobe);
			strobe->sector = &sectors[(int)strobe->sector];
			strobe->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;

			thinkercap.prev->next = &strobe->thinker;
    		strobe->thinker.next = &thinkercap;
    		strobe->thinker.prev = thinkercap.prev;
			thinkercap.prev = &strobe->thinker;

			break;

		case tc_glow:
			PADSAVEP();
			glow = Z_MallocUnowned(sizeof(*glow), PU_LEVEL);
			CopyBytes(save_p, glow, sizeof(*glow));
			//memcpy(glow, save_p, sizeof(*glow));
			save_p += sizeof(*glow);
			glow->sector = &sectors[(int)glow->sector];
			glow->thinker.function.acp1 = (actionf_p1)T_Glow;

			thinkercap.prev->next = &glow->thinker;
    		glow->thinker.next = &thinkercap;
    		glow->thinker.prev = thinkercap.prev;
			thinkercap.prev = &glow->thinker;

			break;

		default:
			I_Error("P_UnarchiveSpecials:Unknown tclass %i "
					"in savegame",
					tclass);
		}
	}
}
