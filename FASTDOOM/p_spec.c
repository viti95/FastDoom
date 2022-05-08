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
//	Implements special effects:
//	Texture animation, height or lighting changes
//	 according to adjacent sectors, respective
//	 utility functions, etc.
//	Line Tag handling. Line and Sector triggers.
//

#include <string.h>

#include <stdlib.h>

#include "doomdef.h"
#include "doomstat.h"

#include "i_random.h"

#include "i_system.h"
#include "z_zone.h"
#include "m_misc.h"
#include "w_wad.h"

#include "r_local.h"
#include "p_local.h"

#include "g_game.h"

#include "s_sound.h"

// State.
#include "r_state.h"

// Data.
#include "sounds.h"

//
// Animating textures and planes
// There is another anim_t used in wi_stuff, unrelated.
//
typedef struct
{
	byte istexture;
	int picnum;
	int basepic;
	int numpics;
	int lastpic;	// basepics + numpics
	int currentpic; // store last animation pic
} anim_t;

//
//      source animation definition
//
typedef struct
{
	byte istexture; // if false, it is a flat
	char endname[9];
	char startname[9];
	int speed;
} animdef_t;

#define MAXANIMS 32

extern anim_t anims[MAXANIMS];
extern anim_t *lastanim;

//
// P_InitPicAnims
//

// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to
//  be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in
//  the WAD file.
//

anim_t anims[MAXANIMS];
anim_t *lastanim;

//
//      Animating line specials
//
#define MAXLINEANIMS 64

extern short numlinespecials;
extern line_t *linespeciallist[MAXLINEANIMS];

void P_InitPicAnims(void)
{
	const animdef_t animdefs[] =
		{
			{0, "NUKAGE3", "NUKAGE1"},
			{0, "FWATER4", "FWATER1"},
			{0, "SWATER4", "SWATER1"},
			{0, "LAVA4", "LAVA1"},
			{0, "BLOOD3", "BLOOD1"},

			// DOOM II flat animations.
			{0, "RROCK08", "RROCK05"},
			{0, "SLIME04", "SLIME01"},
			{0, "SLIME08", "SLIME05"},
			{0, "SLIME12", "SLIME09"},

			{1, "BLODGR4", "BLODGR1"},
			{1, "SLADRIP3", "SLADRIP1"},

			{1, "BLODRIP4", "BLODRIP1"},
			{1, "FIREWALL", "FIREWALA"},
			{1, "GSTFONT3", "GSTFONT1"},
			{1, "FIRELAVA", "FIRELAV3"},
			{1, "FIREMAG3", "FIREMAG1"},
			{1, "FIREBLU2", "FIREBLU1"},
			{1, "ROCKRED3", "ROCKRED1"},

			{1, "BFALL4", "BFALL1"},
			{1, "SFALL4", "SFALL1"},
			{1, "WFALL4", "WFALL1"},
			{1, "DBRAIN4", "DBRAIN1"},

			{2}};

	int i;

	//	Init animation
	lastanim = anims;
	for (i = 0; animdefs[i].istexture != 2; i++)
	{
		if (animdefs[i].istexture)
		{
			// different episode ?
			if (R_TextureNumForName((char *)animdefs[i].startname) == -1)
				continue;

			lastanim->picnum = R_TextureNumForName((char *)animdefs[i].endname);
			lastanim->basepic = R_TextureNumForName((char *)animdefs[i].startname);
		}
		else
		{
			if (W_GetNumForName((char *)animdefs[i].startname) == -1)
				continue;

			lastanim->picnum = R_FlatNumForName((char *)animdefs[i].endname);
			lastanim->basepic = R_FlatNumForName((char *)animdefs[i].startname);
		}

		lastanim->istexture = animdefs[i].istexture;
		lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

		lastanim->lastpic = lastanim->numpics + lastanim->basepic;
		lastanim->currentpic = lastanim->basepic;

		lastanim++;
	}
}

//
// UTILITIES
//

//
// getSide()
// Will return a side_t*
//  given the number of the current sector,
//  the line number, and the side (0/1) that you want.
//
side_t *
getSide(int currentSector,
		int line,
		int side)
{
	return &sides[(sectors[currentSector].lines[line])->sidenum[side]];
}

//
// getSector()
// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.
//
sector_t *
getSector(int currentSector,
		  int line,
		  int side)
{
	return sides[(sectors[currentSector].lines[line])->sidenum[side]].sector;
}

//
// twoSided()
// Given the sector number and the line number,
//  it will tell you whether the line is two-sided or not.
//
int twoSided(int sector,
			 int line)
{
	return (sectors[sector].lines[line])->flags & ML_TWOSIDED;
}

//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
sector_t *
getNextSector(line_t *line,
			  sector_t *sec)
{
	if (!(line->flags & ML_TWOSIDED))
		return NULL;

	if (line->frontsector == sec)
		return line->backsector;

	return line->frontsector;
}

//
// P_FindLowestFloorSurrounding()
// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t P_FindLowestFloorSurrounding(sector_t *sec)
{
	short i;
	line_t *check;
	sector_t *other;
	fixed_t floor = sec->floorheight;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->floorheight < floor)
			floor = other->floorheight;
	}
	return floor;
}

//
// P_FindHighestFloorSurrounding()
// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t P_FindHighestFloorSurrounding(sector_t *sec)
{
	short i;
	line_t *check;
	sector_t *other;
	fixed_t floor = -500 * FRACUNIT;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->floorheight > floor)
			floor = other->floorheight;
	}
	return floor;
}

//
// P_FindNextHighestFloor
//
fixed_t P_FindNextHighestFloor(sector_t *sec, int currentheight)
{
	sector_t *other;
	short i;

	for (i = 0; i < sec->linecount; i++)
		if ((other = getNextSector(sec->lines[i], sec)) &&
			other->floorheight > currentheight)
		{
			int height = other->floorheight;
			while (++i < sec->linecount)
				if ((other = getNextSector(sec->lines[i], sec)) &&
					other->floorheight < height &&
					other->floorheight > currentheight)
					height = other->floorheight;
			return height;
		}
	return currentheight;
}

//
// FIND LOWEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t
P_FindLowestCeilingSurrounding(sector_t *sec)
{
	short i;
	line_t *check;
	sector_t *other;
	fixed_t height = MAXINT;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->ceilingheight < height)
			height = other->ceilingheight;
	}
	return height;
}

//
// FIND HIGHEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t P_FindHighestCeilingSurrounding(sector_t *sec)
{
	short i;
	line_t *check;
	sector_t *other;
	fixed_t height = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->ceilingheight > height)
			height = other->ceilingheight;
	}
	return height;
}

//
// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//

// Find the next sector with the same tag as a linedef.
// Rewritten by Lee Killough to use chained hashing to improve speed

int P_FindSectorFromLineTag(line_t *line, int start)
{
	start = start >= 0 ? sectors[start].nexttag : sectors[(unsigned)line->tag % (unsigned)numsectors].firsttag;
	while (start >= 0 && sectors[start].tag != line->tag)
		start = sectors[start].nexttag;
	return start;
}

//
// Find minimum light from an adjacent sector
//
int P_FindMinSurroundingLight(sector_t *sector,
							  int max)
{
	short i;
	int min;
	line_t *line;
	sector_t *check;

	min = max;
	for (i = 0; i < sector->linecount; i++)
	{
		line = sector->lines[i];
		check = getNextSector(line, sector);

		if (!check)
			continue;

		if (check->lightlevel < min)
			min = check->lightlevel;
	}
	return min;
}

//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//

//
// P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with a non 0 special.
//
void P_CrossSpecialLine(int linenum, byte side, mobj_t *thing)
{
	line_t *line;
	int ok;

	line = &lines[linenum];

	//	Triggers that other things can activate
	if (!thing->player)
	{
		// Things that should NOT trigger specials...
		switch (thing->type)
		{
		case MT_ROCKET:
		case MT_PLASMA:
		case MT_BFG:
		case MT_TROOPSHOT:
		case MT_HEADSHOT:
		case MT_BRUISERSHOT:
			return;
			break;

		default:
			break;
		}

		ok = 0;
		switch (line->special)
		{
		case 39:  // TELEPORT TRIGGER
		case 97:  // TELEPORT RETRIGGER
		case 125: // TELEPORT MONSTERONLY TRIGGER
		case 126: // TELEPORT MONSTERONLY RETRIGGER
		case 4:	  // RAISE DOOR
		case 10:  // PLAT DOWN-WAIT-UP-STAY TRIGGER
		case 88:  // PLAT DOWN-WAIT-UP-STAY RETRIGGER
			ok = 1;
			break;
		}
		if (!ok)
			return;
	}

	// Note: could use some const's here.
	switch (line->special)
	{
		// TRIGGERS.
		// All from here to RETRIGGERS.
	case 2:
		// Open Door
		EV_DoDoor(line, open);
		line->special = 0;
		break;

	case 3:
		// Close Door
		EV_DoDoor(line, close);
		line->special = 0;
		break;

	case 4:
		// Raise Door
		EV_DoDoor(line, normal);
		line->special = 0;
		break;

	case 5:
		// Raise Floor
		EV_DoFloor(line, raiseFloor);
		line->special = 0;
		break;

	case 6:
		// Fast Ceiling Crush & Raise
		EV_DoCeiling(line, fastCrushAndRaise);
		line->special = 0;
		break;

	case 8:
		// Build Stairs
		EV_BuildStairs(line, build8);
		line->special = 0;
		break;

	case 10:
		// PlatDownWaitUp
		EV_DoPlat(line, downWaitUpStay, 0);
		line->special = 0;
		break;

	case 12:
		// Light Turn On - brightest near
		EV_LightTurnOn(line, 0);
		line->special = 0;
		break;

	case 13:
		// Light Turn On 255
		EV_LightTurnOn(line, 255);
		line->special = 0;
		break;

	case 16:
		// Close Door 30
		EV_DoDoor(line, close30ThenOpen);
		line->special = 0;
		break;

	case 17:
		// Start Light Strobing
		EV_StartLightStrobing(line);
		line->special = 0;
		break;

	case 19:
		// Lower Floor
		EV_DoFloor(line, lowerFloor);
		line->special = 0;
		break;

	case 22:
		// Raise floor to nearest height and change texture
		EV_DoPlat(line, raiseToNearestAndChange, 0);
		line->special = 0;
		break;

	case 25:
		// Ceiling Crush and Raise
		EV_DoCeiling(line, crushAndRaise);
		line->special = 0;
		break;

	case 30:
		// Raise floor to shortest texture height
		//  on either side of lines.
		EV_DoFloor(line, raiseToTexture);
		line->special = 0;
		break;

	case 35:
		// Lights Very Dark
		EV_LightTurnOn(line, 35);
		line->special = 0;
		break;

	case 36:
		// Lower Floor (TURBO)
		EV_DoFloor(line, turboLower);
		line->special = 0;
		break;

	case 37:
		// LowerAndChange
		EV_DoFloor(line, lowerAndChange);
		line->special = 0;
		break;

	case 38:
		// Lower Floor To Lowest
		EV_DoFloor(line, lowerFloorToLowest);
		line->special = 0;
		break;

	case 39:
		// TELEPORT!
		EV_Teleport(line, side, thing);
		line->special = 0;
		break;

	case 40:
		// RaiseCeilingLowerFloor
		EV_DoCeiling(line, raiseToHighest);
		EV_DoFloor(line, lowerFloorToLowest);
		line->special = 0;
		break;

	case 44:
		// Ceiling Crush
		EV_DoCeiling(line, lowerAndCrush);
		line->special = 0;
		break;

	case 52:
		// EXIT!
		G_ExitLevel();
		break;

	case 53:
		// Perpetual Platform Raise
		EV_DoPlat(line, perpetualRaise, 0);
		line->special = 0;
		break;

	case 54:
		// Platform Stop
		EV_StopPlat(line);
		line->special = 0;
		break;

	case 56:
		// Raise Floor Crush
		EV_DoFloor(line, raiseFloorCrush);
		line->special = 0;
		break;

	case 57:
		// Ceiling Crush Stop
		EV_CeilingCrushStop(line);
		line->special = 0;
		break;

	case 58:
		// Raise Floor 24
		EV_DoFloor(line, raiseFloor24);
		line->special = 0;
		break;

	case 59:
		// Raise Floor 24 And Change
		EV_DoFloor(line, raiseFloor24AndChange);
		line->special = 0;
		break;

	case 104:
		// Turn lights off in sector(tag)
		EV_TurnTagLightsOff(line);
		line->special = 0;
		break;

	case 108:
		// Blazing Door Raise (faster than TURBO!)
		EV_DoDoor(line, blazeRaise);
		line->special = 0;
		break;

	case 109:
		// Blazing Door Open (faster than TURBO!)
		EV_DoDoor(line, blazeOpen);
		line->special = 0;
		break;

	case 100:
		// Build Stairs Turbo 16
		EV_BuildStairs(line, turbo16);
		line->special = 0;
		break;

	case 110:
		// Blazing Door Close (faster than TURBO!)
		EV_DoDoor(line, blazeClose);
		line->special = 0;
		break;

	case 119:
		// Raise floor to nearest surr. floor
		EV_DoFloor(line, raiseFloorToNearest);
		line->special = 0;
		break;

	case 121:
		// Blazing PlatDownWaitUpStay
		EV_DoPlat(line, blazeDWUS, 0);
		line->special = 0;
		break;

	case 124:
		// Secret EXIT
		G_SecretExitLevel();
		break;

	case 125:
		// TELEPORT MonsterONLY
		if (!thing->player)
		{
			EV_Teleport(line, side, thing);
			line->special = 0;
		}
		break;

	case 130:
		// Raise Floor Turbo
		EV_DoFloor(line, raiseFloorTurbo);
		line->special = 0;
		break;

	case 141:
		// Silent Ceiling Crush & Raise
		EV_DoCeiling(line, silentCrushAndRaise);
		line->special = 0;
		break;

		// RETRIGGERS.  All from here till end.
	case 72:
		// Ceiling Crush
		EV_DoCeiling(line, lowerAndCrush);
		break;

	case 73:
		// Ceiling Crush and Raise
		EV_DoCeiling(line, crushAndRaise);
		break;

	case 74:
		// Ceiling Crush Stop
		EV_CeilingCrushStop(line);
		break;

	case 75:
		// Close Door
		EV_DoDoor(line, close);
		break;

	case 76:
		// Close Door 30
		EV_DoDoor(line, close30ThenOpen);
		break;

	case 77:
		// Fast Ceiling Crush & Raise
		EV_DoCeiling(line, fastCrushAndRaise);
		break;

	case 79:
		// Lights Very Dark
		EV_LightTurnOn(line, 35);
		break;

	case 80:
		// Light Turn On - brightest near
		EV_LightTurnOn(line, 0);
		break;

	case 81:
		// Light Turn On 255
		EV_LightTurnOn(line, 255);
		break;

	case 82:
		// Lower Floor To Lowest
		EV_DoFloor(line, lowerFloorToLowest);
		break;

	case 83:
		// Lower Floor
		EV_DoFloor(line, lowerFloor);
		break;

	case 84:
		// LowerAndChange
		EV_DoFloor(line, lowerAndChange);
		break;

	case 86:
		// Open Door
		EV_DoDoor(line, open);
		break;

	case 87:
		// Perpetual Platform Raise
		EV_DoPlat(line, perpetualRaise, 0);
		break;

	case 88:
		// PlatDownWaitUp
		EV_DoPlat(line, downWaitUpStay, 0);
		break;

	case 89:
		// Platform Stop
		EV_StopPlat(line);
		break;

	case 90:
		// Raise Door
		EV_DoDoor(line, normal);
		break;

	case 91:
		// Raise Floor
		EV_DoFloor(line, raiseFloor);
		break;

	case 92:
		// Raise Floor 24
		EV_DoFloor(line, raiseFloor24);
		break;

	case 93:
		// Raise Floor 24 And Change
		EV_DoFloor(line, raiseFloor24AndChange);
		break;

	case 94:
		// Raise Floor Crush
		EV_DoFloor(line, raiseFloorCrush);
		break;

	case 95:
		// Raise floor to nearest height
		// and change texture.
		EV_DoPlat(line, raiseToNearestAndChange, 0);
		break;

	case 96:
		// Raise floor to shortest texture height
		// on either side of lines.
		EV_DoFloor(line, raiseToTexture);
		break;

	case 97:
		// TELEPORT!
		EV_Teleport(line, side, thing);
		break;

	case 98:
		// Lower Floor (TURBO)
		EV_DoFloor(line, turboLower);
		break;

	case 105:
		// Blazing Door Raise (faster than TURBO!)
		EV_DoDoor(line, blazeRaise);
		break;

	case 106:
		// Blazing Door Open (faster than TURBO!)
		EV_DoDoor(line, blazeOpen);
		break;

	case 107:
		// Blazing Door Close (faster than TURBO!)
		EV_DoDoor(line, blazeClose);
		break;

	case 120:
		// Blazing PlatDownWaitUpStay.
		EV_DoPlat(line, blazeDWUS, 0);
		break;

	case 126:
		// TELEPORT MonsterONLY.
		if (!thing->player)
			EV_Teleport(line, side, thing);
		break;

	case 128:
		// Raise To Nearest Floor
		EV_DoFloor(line, raiseFloorToNearest);
		break;

	case 129:
		// Raise Floor Turbo
		EV_DoFloor(line, raiseFloorTurbo);
		break;
	}
}

//
// P_ShootSpecialLine - IMPACT SPECIALS
// Called when a thing shoots a special line.
//
void P_ShootSpecialLine(mobj_t *thing,
						line_t *line)
{
	int ok;

	//	Impacts that other things can activate.
	if (!thing->player)
	{
		ok = 0;
		switch (line->special)
		{
		case 46:
			// OPEN DOOR IMPACT
			ok = 1;
			break;
		}
		if (!ok)
			return;
	}

	switch (line->special)
	{
	case 24:
		// RAISE FLOOR
		EV_DoFloor(line, raiseFloor);
		P_ChangeSwitchTexture(line, 0);
		break;

	case 46:
		// OPEN DOOR
		EV_DoDoor(line, open);
		P_ChangeSwitchTexture(line, 1);
		break;

	case 47:
		// RAISE FLOOR NEAR AND CHANGE
		EV_DoPlat(line, raiseToNearestAndChange, 0);
		P_ChangeSwitchTexture(line, 0);
		break;
	}
}

void P_InitTagLists(void)
{
	register int i;

	for (i = numsectors; --i >= 0;) // Initially make all slots empty.
		sectors[i].firsttag = -1;

	for (i = numsectors; --i >= 0;)								 // Proceed from last to first sector
	{															 // so that lower sectors appear first
		int j = (unsigned)sectors[i].tag % (unsigned)numsectors; // Hash func
		sectors[i].nexttag = sectors[j].firsttag;				 // Prepend sector to chain
		sectors[j].firsttag = i;
	}
}

//
// P_PlayerInSpecialSector
// Called every tic frame
//  that the player origin is in a special sector
//
void P_PlayerInSpecialSector(player_t *player)
{
	sector_t *sector;

	sector = player->mo->subsector->sector;

	// Falling, not all the way down yet?
	if (player->mo->z != sector->floorheight)
		return;

	// Has hitten ground.
	switch (sector->special)
	{
	case 5:
		// HELLSLIME DAMAGE
		if (!player->powers[pw_ironfeet])
			if (!(leveltime & 0x1f))
				P_DamageMobj(player->mo, NULL, NULL, 10);
		break;

	case 7:
		// NUKAGE DAMAGE
		if (!player->powers[pw_ironfeet])
			if (!(leveltime & 0x1f))
				P_DamageMobj(player->mo, NULL, NULL, 5);
		break;

	case 16:
		// SUPER HELLSLIME DAMAGE
	case 4:
		// STROBE HURT
		if (!player->powers[pw_ironfeet] || (P_Random < 5))
		{
			if (!(leveltime & 0x1f))
				P_DamageMobj(player->mo, NULL, NULL, 20);
		}
		break;

	case 9:
		// SECRET SECTOR
		player->secretcount++;
		sector->special = 0;
		break;

	case 11:
		// EXIT SUPER DAMAGE! (for E1M8 finale)
		player->cheats &= ~CF_GODMODE;

		if (!(leveltime & 0x1f))
			P_DamageMobj(player->mo, NULL, NULL, 20);

		if (player->health <= 10)
			G_ExitLevel();
		break;
	};
}

//
// P_UpdateSpecials
// Animate planes, scroll walls, etc.
//

void P_UpdateSpecials(void)
{
	anim_t *anim;
	int pic;
	int i;
	line_t *line;

	if (!(gametic & 7)) // Optimization from Jaguar Doom port
		//	ANIMATE FLATS AND TEXTURES GLOBALLY
		for (anim = anims; anim < lastanim; anim++)
		{
			++anim->currentpic;

			if (anim->currentpic >= anim->lastpic)
				anim->currentpic = anim->basepic;

			for (i = anim->basepic; i < anim->lastpic; i++)
			{
				if (anim->istexture)
					texturetranslation[i] = anim->currentpic;
				else
					flattranslation[i] = anim->currentpic;
			}
		}

	//	ANIMATE LINE SPECIALS
	for (i = 0; i < numlinespecials; i++)
	{
		line = linespeciallist[i];
		switch (line->special)
		{
		case 48:
			// EFFECT FIRSTCOL SCROLL +
			sides[line->sidenum[0]].textureoffset += FRACUNIT;
			break;
		}
	}

	//	DO BUTTONS
	for (i = 0; i < MAXBUTTONS; i++)
		if (buttonlist[i].btimer)
		{
			buttonlist[i].btimer--;
			if (!buttonlist[i].btimer)
			{
				switch (buttonlist[i].where)
				{
				case top:
					sides[buttonlist[i].line->sidenum[0]].toptexture =
						buttonlist[i].btexture;
					break;

				case middle:
					sides[buttonlist[i].line->sidenum[0]].midtexture =
						buttonlist[i].btexture;
					break;

				case bottom:
					sides[buttonlist[i].line->sidenum[0]].bottomtexture =
						buttonlist[i].btexture;
					break;
				}
				S_StartSound((mobj_t *)&buttonlist[i].soundorg, sfx_swtchn);
				memset(&buttonlist[i], 0, sizeof(button_t));
			}
		}
}

//
// Special Stuff that can not be categorized
//
int EV_DoDonut(line_t *line)
{
	sector_t *s1;
	sector_t *s2;
	sector_t *s3;
	int secnum;
	int rtn;
	short i;
	floormove_t *floor;

	secnum = -1;
	rtn = 0;
	while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
	{
		s1 = &sectors[secnum];

		// ALREADY MOVING?  IF SO, KEEP GOING...
		if (s1->specialdata)
			continue;

		rtn = 1;
		s2 = getNextSector(s1->lines[0], s1);
		for (i = 0; i < s2->linecount; i++)
		{
			if ((!s2->lines[i]->flags & ML_TWOSIDED) ||
				(s2->lines[i]->backsector == s1))
				continue;
			s3 = s2->lines[i]->backsector;

			//	Spawn rising slime
			floor = Z_MallocUnowned(sizeof(*floor), PU_LEVSPEC);

			thinkercap.prev->next = &floor->thinker;
			floor->thinker.next = &thinkercap;
			floor->thinker.prev = thinkercap.prev;
			thinkercap.prev = &floor->thinker;

			s2->specialdata = floor;
			floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
			floor->type = donutRaise;
			floor->crush = 0;
			floor->direction = 1;
			floor->sector = s2;
			floor->speed = FLOORSPEED / 2;
			floor->texture = s3->floorpic;
			floor->newspecial = 0;
			floor->floordestheight = s3->floorheight;

			//	Spawn lowering donut-hole
			floor = Z_MallocUnowned(sizeof(*floor), PU_LEVSPEC);

			thinkercap.prev->next = &floor->thinker;
			floor->thinker.next = &thinkercap;
			floor->thinker.prev = thinkercap.prev;
			thinkercap.prev = &floor->thinker;

			s1->specialdata = floor;
			floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
			floor->type = lowerFloor;
			floor->crush = 0;
			floor->direction = -1;
			floor->sector = s1;
			floor->speed = FLOORSPEED / 2;
			floor->floordestheight = s3->floorheight;
			break;
		}
	}
	return rtn;
}

//
// SPECIAL SPAWNING
//

//
// P_SpawnSpecials
// After the map has been loaded, scan for specials
//  that spawn thinkers
//
short numlinespecials;
line_t *linespeciallist[MAXLINEANIMS];

// Parses command line parameters.
void P_SpawnSpecials(void)
{
	sector_t *sector;
	int i;
	int episode;

	episode = 1;
	if (W_GetNumForName("TEXTURE2") >= 0)
		episode = 2;

	//	Init special SECTORs.
	sector = sectors;
	for (i = 0; i < numsectors; i++, sector++)
	{
		switch (sector->special)
		{
		case 0:
			continue;
		case 1:
			// FLICKERING LIGHTS
			P_SpawnLightFlash(sector);
			break;

		case 2:
			// STROBE FAST
			P_SpawnStrobeFlash(sector, FASTDARK, 0);
			break;

		case 3:
			// STROBE SLOW
			P_SpawnStrobeFlash(sector, SLOWDARK, 0);
			break;

		case 4:
			// STROBE FAST/DEATH SLIME
			P_SpawnStrobeFlash(sector, FASTDARK, 0);
			sector->special = 4;
			break;

		case 8:
			// GLOWING LIGHT
			P_SpawnGlowingLight(sector);
			break;
		case 9:
			// SECRET SECTOR
			totalsecret++;
			break;

		case 10:
			// DOOR CLOSE IN 30 SECONDS
			P_SpawnDoorCloseIn30(sector);
			break;

		case 12:
			// SYNC STROBE SLOW
			P_SpawnStrobeFlash(sector, SLOWDARK, 1);
			break;

		case 13:
			// SYNC STROBE FAST
			P_SpawnStrobeFlash(sector, FASTDARK, 1);
			break;

		case 14:
			// DOOR RAISE IN 5 MINUTES
			P_SpawnDoorRaiseIn5Mins(sector, i);
			break;

		case 17:
			P_SpawnFireFlicker(sector);
			break;
		}
	}

	//	Init line EFFECTs
	numlinespecials = 0;
	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
		case 48:
			// EFFECT FIRSTCOL SCROLL+
			linespeciallist[numlinespecials] = &lines[i];
			numlinespecials++;
			break;
		}
	}

	//	Init other misc stuff
	for (i = 0; i < MAXCEILINGS; i++)
		activeceilings[i] = NULL;

	for (i = 0; i < MAXPLATS; i++)
		activeplats[i] = NULL;

	for (i = 0; i < MAXBUTTONS; i++)
		memset(&buttonlist[i], 0, sizeof(button_t));

	P_InitTagLists();
}
