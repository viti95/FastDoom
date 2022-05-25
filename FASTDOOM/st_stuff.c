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
//	Status bar code.
//	Does the face/direction indicator animatin.
//	Does palette indicators as well (red pain/berserk, bright pickup)
//

#include <stdio.h>

#include "i_random.h"

#include "i_system.h"
#include "z_zone.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"

#include "g_game.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"

#include "p_local.h"
#include "p_inter.h"

#include "am_map.h"
#include "dutils.h"

#include "s_sound.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "st_lib.h"

#include "m_menu.h"

//
// STATUS BAR DATA
//

// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS 1
#define STARTBONUSPALS 9
#define NUMREDPALS 8
#define NUMBONUSPALS 4
// Radiation suit, green shift.
#define RADIATIONPAL 13

// N/256*100% probability
//  that the normal face state will change
#define ST_FACEPROBABILITY 96

// Location of status bar
#define ST_X 0
#define ST_X2 104

#define ST_FX 143
#define ST_FY 169

// Should be set to patch width
//  for tall numbers later on
#define ST_TALLNUMWIDTH (tallnum[0]->width)

// Number of status faces.
#define ST_NUMPAINFACES 5
#define ST_NUMSTRAIGHTFACES 3
#define ST_NUMTURNFACES 2
#define ST_NUMSPECIALFACES 3

#define ST_FACESTRIDE \
	(ST_NUMSTRAIGHTFACES + ST_NUMTURNFACES + ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES 2

#define ST_NUMFACES \
	(ST_FACESTRIDE * ST_NUMPAINFACES + ST_NUMEXTRAFACES)

#define ST_TURNOFFSET (ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET (ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET (ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET (ST_EVILGRINOFFSET + 1)
#define ST_GODFACE (ST_NUMPAINFACES * ST_FACESTRIDE)
#define ST_DEADFACE (ST_GODFACE + 1)

#define ST_FACESX 143
#define ST_FACESY 168

#define ST_EVILGRINCOUNT (2 * TICRATE)
#define ST_STRAIGHTFACECOUNT (TICRATE / 2)
#define ST_TURNCOUNT (1 * TICRATE)
#define ST_OUCHCOUNT (1 * TICRATE)
#define ST_RAMPAGEDELAY (2 * TICRATE)

#define ST_MUCHPAIN 20

// Location and size of statistics,
//  justified according to widget type.
// Problem is, within which space? STbar? Screen?
// Note: this could be read in by a lump.
//       Problem is, is the stuff rendered
//       into a buffer,
//       or into the frame buffer?

// AMMO number pos.
#define ST_AMMOWIDTH 3
#define ST_AMMOX 44
#define ST_AMMOY 171

// HEALTH number pos.
#define ST_HEALTHWIDTH 3
#define ST_HEALTHX 90
#define ST_HEALTHY 171

// Weapon pos.
#define ST_ARMSX 111
#define ST_ARMSY 172
#define ST_ARMSBGX 104
#define ST_ARMSBGY 168
#define ST_ARMSXSPACE 12
#define ST_ARMSYSPACE 10

// ARMOR number pos.
#define ST_ARMORWIDTH 3
#define ST_ARMORX 221
#define ST_ARMORY 171

// Key icon positions.
#define ST_KEY0WIDTH 8
#define ST_KEY0HEIGHT 5
#define ST_KEY0X 239
#define ST_KEY0Y 171
#define ST_KEY1WIDTH ST_KEY0WIDTH
#define ST_KEY1X 239
#define ST_KEY1Y 181
#define ST_KEY2WIDTH ST_KEY0WIDTH
#define ST_KEY2X 239
#define ST_KEY2Y 191

// Ammunition counter.
#define ST_AMMO0WIDTH 3
#define ST_AMMO0HEIGHT 6
#define ST_AMMO0X 288
#define ST_AMMO0Y 173
#define ST_AMMO1WIDTH ST_AMMO0WIDTH
#define ST_AMMO1X 288
#define ST_AMMO1Y 179
#define ST_AMMO2WIDTH ST_AMMO0WIDTH
#define ST_AMMO2X 288
#define ST_AMMO2Y 191
#define ST_AMMO3WIDTH ST_AMMO0WIDTH
#define ST_AMMO3X 288
#define ST_AMMO3Y 185

// Indicate maximum ammunition.
// Only needed because backpack exists.
#define ST_MAXAMMO0WIDTH 3
#define ST_MAXAMMO0HEIGHT 5
#define ST_MAXAMMO0X 314
#define ST_MAXAMMO0Y 173
#define ST_MAXAMMO1WIDTH ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X 314
#define ST_MAXAMMO1Y 179
#define ST_MAXAMMO2WIDTH ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X 314
#define ST_MAXAMMO2Y 191
#define ST_MAXAMMO3WIDTH ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X 314
#define ST_MAXAMMO3Y 185

// pistol
#define ST_WEAPON0X 110
#define ST_WEAPON0Y 172

// shotgun
#define ST_WEAPON1X 122
#define ST_WEAPON1Y 172

// chain gun
#define ST_WEAPON2X 134
#define ST_WEAPON2Y 172

// missile launcher
#define ST_WEAPON3X 110
#define ST_WEAPON3Y 181

// plasma gun
#define ST_WEAPON4X 122
#define ST_WEAPON4Y 181

// bfg
#define ST_WEAPON5X 134
#define ST_WEAPON5Y 181

// WPNS title
#define ST_WPNSX 109
#define ST_WPNSY 191

// DETH title
#define ST_DETHX 109
#define ST_DETHY 191

//Incoming messages window location
#define ST_MSGTEXTX 0
#define ST_MSGTEXTY 0
// Dimensions given in characters.
#define ST_MSGWIDTH 52
// Or shall I say, in lines?
#define ST_MSGHEIGHT 1

#define ST_OUTTEXTX 0
#define ST_OUTTEXTY 6

// Width, in characters again.
#define ST_OUTWIDTH 52
// Height, in lines.
#define ST_OUTHEIGHT 1

#define ST_MAPTITLEY 0
#define ST_MAPHEIGHT 1

// ST_Start() has just been called
static byte st_firsttime;

// whether left-side main status bar is active
static byte st_statusbaron;

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
// main bar left
static patch_t *sbar;
#endif

// 0-9, tall numbers
static patch_t *tallnum[10];

// tall % sign
static patch_t *tallpercent;

// 0-9, short, yellow (,different!) numbers
static patch_t *shortnum[10];

// 3 key-cards, 3 skulls
static patch_t *keys[NUMCARDS];

// face status patches
static patch_t *faces[ST_NUMFACES];

// main bar right
static patch_t *armsbg;

// weapon ownership patches
static patch_t *arms[6][2];

// ready-weapon widget
static st_number_t w_ready;

// health widget
static st_percent_t w_health;

// arms background
static st_binicon_t w_armsbg;

// weapon ownership widgets
static st_multicon_t w_arms[6];

// face status widget
static st_multicon_t w_faces;

// keycard widgets
static st_multicon_t w_keyboxes[3];

// armor widget
static st_percent_t w_armor;

// ammo widgets
static st_number_t w_ammo[4];

// max ammo widgets
static st_number_t w_maxammo[4];

// used to use appopriately pained face
static int st_oldhealth = -1;

// used for evil grin
static boolean oldweaponsowned[NUMWEAPONS];

// count until face changes
static int st_facecount = 0;

// current face index, used by w_faces
static int st_faceindex = 0;

// holds key-type for each key box on bar
static int keyboxes[3];

// a random number per tick
static int st_randomnumber;

// Massive bunches of cheat shit
//  to keep it from being easy to figure them out.
// Yeah, right...
unsigned char cheat_mus_seq[] =
	{
		'i', 'd', 'm', 'u', 's', 1, 0, 0, 0xff};

unsigned char cheat_choppers_seq[] =
	{
		'i', 'd', 'c', 'h', 'o', 'p', 'p', 'e', 'r', 's', 0xff // idchoppers
};

unsigned char cheat_god_seq[] =
	{
		'i', 'd', 'd', 'q', 'd', 0xff // iddqd
};

unsigned char cheat_ammo_seq[] =
	{
		'i', 'd', 'k', 'f', 'a', 0xff // idkfa
};

unsigned char cheat_ammonokey_seq[] =
	{
		'i', 'd', 'f', 'a', 0xff // idfa
};

// Smashing Pumpkins Into Samml Piles Of Putried Debris.
unsigned char cheat_noclip_seq[] =
	{
		'i', 'd', 's', 'p', 'i', // idspispopd
		's', 'p', 'o', 'p', 'd', 0xff};

//
unsigned char cheat_commercial_noclip_seq[] =
	{
		'i', 'd', 'c', 'l', 'i', 'p', 0xff // idclip
};

unsigned char cheat_powerup_seq[7][10] =
	{
		{'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'v', 0xff}, // beholdv
		{'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 's', 0xff}, // beholds
		{'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'i', 0xff}, // beholdi
		{'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'r', 0xff}, // beholdr
		{'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'a', 0xff}, // beholda
		{'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'l', 0xff}, // beholdl
		{'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'd', 0xff}	 // beholdd
};

unsigned char cheat_clev_seq[] =
	{
		'i', 'd', 'c', 'l', 'e', 'v', 1, 0, 0, 0xff // idclev
};

// Now what?
cheatseq_t cheat_mus = {cheat_mus_seq, 0};
cheatseq_t cheat_god = {cheat_god_seq, 0};
cheatseq_t cheat_ammo = {cheat_ammo_seq, 0};
cheatseq_t cheat_ammonokey = {cheat_ammonokey_seq, 0};
cheatseq_t cheat_noclip = {cheat_noclip_seq, 0};
cheatseq_t cheat_commercial_noclip = {cheat_commercial_noclip_seq, 0};

cheatseq_t cheat_powerup[7] =
	{
		{cheat_powerup_seq[0], 0},
		{cheat_powerup_seq[1], 0},
		{cheat_powerup_seq[2], 0},
		{cheat_powerup_seq[3], 0},
		{cheat_powerup_seq[4], 0},
		{cheat_powerup_seq[5], 0},
		{cheat_powerup_seq[6], 0}};

cheatseq_t cheat_choppers = {cheat_choppers_seq, 0};
cheatseq_t cheat_clev = {cheat_clev_seq, 0};

//
// STATUS BAR CODE
//
void ST_Stop(void);

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void ST_refreshBackground(void)
{
	if (st_statusbaron)
	{
		if (simpleStatusBar)
		{
#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
			V_SetRect(ST_BACKGROUND_COLOR, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y, screen0);
#endif
#if defined(USE_BACKBUFFER)
			V_SetRect(ST_BACKGROUND_COLOR, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y, backbuffer);
#endif
		}
		else
		{
			V_DrawPatch(ST_X, 0, screen4, sbar);

#if defined(MODE_Y) || defined(MODE_VBE2_DIRECT)
			V_CopyRect(ST_X, 0, screen4, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y, screen0);
#endif
#if defined(USE_BACKBUFFER)
			V_CopyRect(ST_X, 0, screen4, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y, backbuffer);
#endif
		}

#if defined(USE_BACKBUFFER)
		updatestate |= I_STATBAR;
#endif
	}
}
#endif

// Respond to keyboard input events,
//  intercept cheats.
void ST_Responder(event_t *ev)
{
	int i;

	// Filter automap on/off.
	if (ev->type == ev_keyup && ((ev->data1 & 0xffff0000) == AM_MSGHEADER))
	{
		st_firsttime = ev->data1 == AM_MSGENTERED;
	}

	// if a user keypress...
	else if (ev->type == ev_keydown)
	{
		if (gameskill != sk_nightmare)
		{
			// 'dqd' cheat for toggleable god mode
			if (cht_CheckCheat(&cheat_god))
			{
				players.cheats ^= CF_GODMODE;
				if (players.cheats & CF_GODMODE)
				{
					if (players_mo)
						players_mo->health = 100;

					players.health = 100;
					players.message = STSTR_DQDON;
				}
				else
					players.message = STSTR_DQDOFF;
			}
			// 'fa' cheat for killer fucking arsenal
			else if (cht_CheckCheat(&cheat_ammonokey))
			{
				players.armorpoints = 200;
				players.armortype = 2;

				for (i = 0; i < NUMWEAPONS; i++)
					players.weaponowned[i] = true;

				for (i = 0; i < NUMAMMO; i++)
					players.ammo[i] = players.maxammo[i];

				players.message = STSTR_FAADDED;
			}
			// 'kfa' cheat for key full ammo
			else if (cht_CheckCheat(&cheat_ammo))
			{
				players.armorpoints = 200;
				players.armortype = 2;

				for (i = 0; i < NUMWEAPONS; i++)
					players.weaponowned[i] = true;

				for (i = 0; i < NUMAMMO; i++)
					players.ammo[i] = players.maxammo[i];

				for (i = 0; i < NUMCARDS; i++)
					players.cards[i] = true;

				players.message = STSTR_KFAADDED;
			}
			// 'mus' cheat for changing music
			else if (cht_CheckCheat(&cheat_mus))
			{
				char buf[3];
				int musnum;

				players.message = STSTR_MUS;
				cht_GetParam(&cheat_mus, buf);

				if (gamemode == commercial)
				{
					musnum = mus_runnin + Mul10(buf[0] - '0') + buf[1] - '0' - 1;

					if ((Mul10(buf[0] - '0') + buf[1] - '0') > 35)
						players.message = STSTR_NOMUS;
					else
						S_ChangeMusic(musnum, 1);
				}
				else
				{
					musnum = mus_e1m1 + (buf[0] - '1') * 9 + (buf[1] - '1');

					if (((buf[0] - '1') * 9 + buf[1] - '1') > 31)
						players.message = STSTR_NOMUS;
					else
						S_ChangeMusic(musnum, 1);
				}
			}
			else if (cht_CheckCheat(&cheat_noclip) || cht_CheckCheat(&cheat_commercial_noclip))
			{
				players.cheats ^= CF_NOCLIP;

				if (players.cheats & CF_NOCLIP)
					players.message = STSTR_NCON;
				else
					players.message = STSTR_NCOFF;
			}

			// 'behold?' power-up cheats
			for (i = 0; i < 6; i++)
			{
				if (cht_CheckCheat(&cheat_powerup[i]))
				{
					if (!players.powers[i])
						P_NotGivePower(i);
					else if (i != pw_strength)
						players.powers[i] = 1;
					else
						players.powers[i] = 0;

					players.message = STSTR_BEHOLDX;
				}
			}

			// 'behold' power-up menu
			if (cht_CheckCheat(&cheat_powerup[6]))
			{
				players.message = STSTR_BEHOLD;
			}
			// 'choppers' invulnerability & chainsaw
			else if (cht_CheckCheat(&cheat_choppers))
			{
				players.weaponowned[wp_chainsaw] = true;
				players.powers[pw_invulnerability] = true;
				players.message = STSTR_CHOPPERS;
			}
		}

		// 'clev' change-level cheat
		if (cht_CheckCheat(&cheat_clev))
		{
			char buf[3];
			int epsd;
			int map;

			cht_GetParam(&cheat_clev, buf);

			if (gamemode == commercial)
			{
				epsd = 0;
				map = 10 * (buf[0] - '0') + buf[1] - '0';
			}
			else
			{
				epsd = buf[0] - '0';
				map = buf[1] - '0';
			}

			// Ohmygod - this is not going to work.
			if ((gamemode == retail) && ((epsd > 4) || (map > 9)))
				return;

			if ((gamemode == registered) && ((epsd > 3) || (map > 9)))
				return;

			if ((gamemode == shareware) && ((epsd > 1) || (map > 9)))
				return;

			if ((gamemode == commercial) && ((epsd > 1) || (map > 32)))
				return;

			// So be it.
			players.message = STSTR_CLEV;
			G_DeferedInitNew(gameskill, epsd, map);
		}
	}
}

int ST_calcPainOffset(void)
{
	int health;
	static int lastcalc;
	static int oldhealth = -1;

	health = players.health > 100 ? 100 : players.health;

	if (health != oldhealth)
	{
		lastcalc = ST_FACESTRIDE * (Div101((100 - health) * ST_NUMPAINFACES));
		oldhealth = health;
	}
	return lastcalc;
}

//
// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
void ST_updateFaceWidget(void)
{
	int i;
	angle_t badguyangle;
	angle_t diffang;
	static int lastattackdown = -1;
	static int priority = 0;
	byte doevilgrin;
	int pos;

	//if (priority < 10)
	//{
	// dead
	if (!players.health)
	{
		priority = 9;
		st_faceindex = ST_DEADFACE;
		st_facecount = 1;
	}
	//}

	if (priority < 9 && players.bonuscount)
	{
		// picking up bonus
		doevilgrin = 0;

		for (i = 0; i < NUMWEAPONS; i++)
		{
			if (oldweaponsowned[i] != players.weaponowned[i])
			{
				doevilgrin = 1;
				oldweaponsowned[i] = players.weaponowned[i];
			}
		}
		if (doevilgrin)
		{
			// evil grin if just picked up weapon
			priority = 8;
			st_facecount = ST_EVILGRINCOUNT;
			st_faceindex = ST_calcPainOffset() + ST_EVILGRINOFFSET;
		}
	}

	if (priority < 8 && players.damagecount && players.attacker && players.attacker != players_mo)
	{
		// being attacked
		priority = 7;

		if (players.health - st_oldhealth > ST_MUCHPAIN)
		{
			st_facecount = ST_TURNCOUNT;
			st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
		}
		else
		{
			badguyangle = R_PointToAngle2(players_mo->x,
										  players_mo->y,
										  players.attacker->x,
										  players.attacker->y);

			if (badguyangle > players_mo->angle)
			{
				// whether right or left
				diffang = badguyangle - players_mo->angle;
				i = diffang > ANG180;
			}
			else
			{
				// whether left or right
				diffang = players_mo->angle - badguyangle;
				i = diffang <= ANG180;
			} // confusing, aint it?

			st_facecount = ST_TURNCOUNT;
			st_faceindex = ST_calcPainOffset();

			if (diffang < ANG45)
			{
				// head-on
				st_faceindex += ST_RAMPAGEOFFSET;
			}
			else if (i)
			{
				// turn face right
				st_faceindex += ST_TURNOFFSET;
			}
			else
			{
				// turn face left
				st_faceindex += ST_TURNOFFSET + 1;
			}
		}
	}

	if (priority < 7 && players.damagecount)
	{
		// getting hurt because of your own damn stupidity

		if (players.health - st_oldhealth > ST_MUCHPAIN)
		{
			priority = 7;
			st_facecount = ST_TURNCOUNT;
			st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
		}
		else
		{
			priority = 6;
			st_facecount = ST_TURNCOUNT;
			st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
		}
	}

	if (priority < 6)
	{
		// rapid firing
		if (players.attackdown)
		{
			if (lastattackdown == -1)
				lastattackdown = ST_RAMPAGEDELAY;
			else if (!--lastattackdown)
			{
				priority = 5;
				st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
				st_facecount = 1;
				lastattackdown = 1;
			}
		}
		else
			lastattackdown = -1;
	}

	if (priority < 5 && ((players.cheats & CF_GODMODE) || players.powers[pw_invulnerability]))
	{
		// invulnerability

		priority = 4;

		st_faceindex = ST_GODFACE;
		st_facecount = 1;
	}

	// look left or look right if the facecount has timed out
	if (!st_facecount)
	{
		pos = st_randomnumber & 3;
		if (pos == 3)
			pos = 0;
		st_faceindex = ST_calcPainOffset() + pos;
		st_facecount = ST_STRAIGHTFACECOUNT;
		priority = 0;
	}

	st_facecount--;
}

void ST_updateWidgets(void)
{
	static int largeammo = 1994; // means "n/a"
	int i;

	if (weaponinfo[players.readyweapon].ammo == am_noammo)
		w_ready.num = &largeammo;
	else
		w_ready.num = &players.ammo[weaponinfo[players.readyweapon].ammo];
	w_ready.data = players.readyweapon;

	// update keycard multiple widgets
	keyboxes[0] = players.cards[0] ? 0 : -1;
	if (players.cards[3])
		keyboxes[0] = 3;

	keyboxes[1] = players.cards[1] ? 1 : -1;
	if (players.cards[4])
		keyboxes[1] = 4;

	keyboxes[2] = players.cards[2] ? 2 : -1;
	if (players.cards[5])
		keyboxes[2] = 5;

	// refresh everything if this is him coming back to life
	ST_updateFaceWidget();
}

void ST_Ticker(void)
{
	st_randomnumber = M_Random;
	ST_updateWidgets();
	st_oldhealth = players.health;
}

static int st_palette = 0;

void ST_doPaletteStuff(void)
{

	int palette;
	byte *pal;
	int cnt;
	int bzc;

	cnt = players.damagecount;

	if (players.powers[pw_strength])
	{
		// slowly fade the berzerk out
		bzc = 12 - (players.powers[pw_strength] >> 6);

		if (bzc > cnt)
			cnt = bzc;
	}

	if (cnt)
	{
		palette = (cnt + 7) >> 3;

		if (palette > NUMREDPALS - 1)
			palette = NUMREDPALS - 1;

		palette += STARTREDPALS;
	}
	else if (players.bonuscount)
	{
		palette = (players.bonuscount + 7) >> 3;

		if (palette > NUMBONUSPALS - 1)
			palette = NUMBONUSPALS - 1;

		palette += STARTBONUSPALS;
	}
	else if (players.powers[pw_ironfeet] > 4 * 32 || players.powers[pw_ironfeet] & 8)
		palette = RADIATIONPAL;
	else
		palette = 0;

	if (palette != st_palette)
	{
		st_palette = palette;
		I_SetPalette(palette);
	}
}

#if defined(MODE_T4025) || defined(MODE_T4050)
void ST_DrawerText4025()
{
	if (w_health.n.on)
	{
		V_WriteTextColorDirect(1, 21, "HEALTH   %%", 7 << 8);
		STlib_drawNumText(&(w_health.n), 8, 21);
	}

	if (w_armor.n.on)
	{
		V_WriteTextColorDirect(1, 22, "ARMOR    %%", 7 << 8);
		STlib_drawNumText(&(w_armor.n), 8, 22);
	}

	if (w_ready.on)
	{
		V_WriteTextColorDirect(1, 23, "AMMO   ", 7 << 8);
		STlib_drawNumText(&(w_ready), 8, 23);
	}

	if (w_ammo[0].on)
	{
		V_WriteTextColorDirect(27, 20, "BULL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[0]), 32, 20);
		STlib_drawNumText(&(w_maxammo[0]), 36, 20);

		V_WriteTextColorDirect(27, 21, "SHEL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[1]), 32, 21);
		STlib_drawNumText(&(w_maxammo[1]), 36, 21);

		V_WriteTextColorDirect(27, 22, "RCKT    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[3]), 32, 22);
		STlib_drawNumText(&(w_maxammo[3]), 36, 22);

		V_WriteTextColorDirect(27, 23, "CELL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[2]), 32, 23);
		STlib_drawNumText(&(w_maxammo[2]), 36, 23);
	}

	if ((st_faceindex - 3) % 8 == 0)
	{
		// LOOK RIGHT
		V_WriteCharDirect(25, 20, 16);
		V_WriteCharDirect(25, 21, 16);
		V_WriteCharDirect(25, 22, 16);
		V_WriteCharDirect(25, 23, 16);
	}
	else
	{
		if ((st_faceindex - 4) % 8 == 0)
		{
			// LOOK LEFT
			V_WriteCharDirect(13, 20, 17);
			V_WriteCharDirect(13, 21, 17);
			V_WriteCharDirect(13, 22, 17);
			V_WriteCharDirect(13, 23, 17);
		}
	}

	if (w_keyboxes[0].on)
	{
		V_WriteTextColorDirect(1, 20, "KEYS   ", 7 << 8);

		switch (keyboxes[0])
		{
		case -1:
			V_WriteCharColorDirect(8, 20, 249, 7 << 8);
			break;
		case 0:
			V_WriteCharColorDirect(8, 20, 20, 1 << 8);
			break;
		case 3:
			V_WriteCharColorDirect(8, 20, 2, 1 << 8);
			break;
		}

		switch (keyboxes[1])
		{
		case -1:
			V_WriteCharColorDirect(9, 20, 249, 7 << 8);
			break;
		case 1:
			V_WriteCharColorDirect(9, 20, 20, 14 << 8);
			break;
		case 4:
			V_WriteCharColorDirect(9, 20, 2, 14 << 8);
			break;
		}

		switch (keyboxes[2])
		{
		case -1:
			V_WriteCharColorDirect(10, 20, 249, 7 << 8);
			break;
		case 2:
			V_WriteCharColorDirect(10, 20, 20, 4 << 8);
			break;
		case 5:
			V_WriteCharColorDirect(10, 20, 2, 4 << 8);
			break;
		}
	}
}
#endif

#if defined(MODE_T8025) || defined(MODE_MDA)
void ST_DrawerText8025()
{
	if (w_health.n.on)
	{
		V_WriteTextColorDirect(1, 21, "HEALTH   %%", 7 << 8);
		STlib_drawNumText(&(w_health.n), 8, 21);
	}

	if (w_armor.n.on)
	{
		V_WriteTextColorDirect(1, 22, "ARMOR    %%", 7 << 8);
		STlib_drawNumText(&(w_armor.n), 8, 22);
	}

	if (w_ready.on)
	{
		V_WriteTextColorDirect(1, 23, "AMMO   ", 7 << 8);
		STlib_drawNumText(&(w_ready), 8, 23);
	}

	if (w_ammo[0].on)
	{
		V_WriteTextColorDirect(67, 20, "BULL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[0]), 72, 20);
		STlib_drawNumText(&(w_maxammo[0]), 76, 20);

		V_WriteTextColorDirect(67, 21, "SHEL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[1]), 72, 21);
		STlib_drawNumText(&(w_maxammo[1]), 76, 21);

		V_WriteTextColorDirect(67, 22, "RCKT    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[3]), 72, 22);
		STlib_drawNumText(&(w_maxammo[3]), 76, 22);

		V_WriteTextColorDirect(67, 23, "CELL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[2]), 72, 23);
		STlib_drawNumText(&(w_maxammo[2]), 76, 23);
	}

	if ((st_faceindex - 3) % 8 == 0)
	{
		// LOOK RIGHT
		V_WriteCharDirect(65, 20, 16);
		V_WriteCharDirect(65, 21, 16);
		V_WriteCharDirect(65, 22, 16);
		V_WriteCharDirect(65, 23, 16);
	}
	else
	{
		if ((st_faceindex - 4) % 8 == 0)
		{
			// LOOK LEFT
			V_WriteCharDirect(13, 20, 17);
			V_WriteCharDirect(13, 21, 17);
			V_WriteCharDirect(13, 22, 17);
			V_WriteCharDirect(13, 23, 17);
		}
	}

	if (w_keyboxes[0].on)
	{
		V_WriteTextColorDirect(1, 20, "KEYS   ", 7 << 8);

		switch (keyboxes[0])
		{
		case -1:
			V_WriteCharColorDirect(8, 20, 249, 7 << 8);
			break;
		case 0:
			V_WriteCharColorDirect(8, 20, 20, 1 << 8);
			break;
		case 3:
			V_WriteCharColorDirect(8, 20, 2, 1 << 8);
			break;
		}

		switch (keyboxes[1])
		{
		case -1:
			V_WriteCharColorDirect(9, 20, 249, 7 << 8);
			break;
		case 1:
			V_WriteCharColorDirect(9, 20, 20, 14 << 8);
			break;
		case 4:
			V_WriteCharColorDirect(9, 20, 2, 14 << 8);
			break;
		}

		switch (keyboxes[2])
		{
		case -1:
			V_WriteCharColorDirect(10, 20, 249, 7 << 8);
			break;
		case 2:
			V_WriteCharColorDirect(10, 20, 20, 4 << 8);
			break;
		case 5:
			V_WriteCharColorDirect(10, 20, 2, 4 << 8);
			break;
		}
	}
}
#endif

#if defined(MODE_T8043) || defined(MODE_T8086)
void ST_DrawerText8043()
{
	if (w_health.n.on)
	{
		V_WriteTextColorDirect(1, 39, "HEALTH   %%", 7 << 8);
		STlib_drawNumText(&(w_health.n), 8, 39);
	}

	if (w_armor.n.on)
	{
		V_WriteTextColorDirect(1, 40, "ARMOR    %%", 7 << 8);
		STlib_drawNumText(&(w_armor.n), 8, 40);
	}

	if (w_ready.on)
	{
		V_WriteTextColorDirect(1, 41, "AMMO   ", 7 << 8);
		STlib_drawNumText(&(w_ready), 8, 41);
	}

	if (w_ammo[0].on)
	{
		V_WriteTextColorDirect(67, 38, "BULL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[0]), 72, 38);
		STlib_drawNumText(&(w_maxammo[0]), 76, 38);

		V_WriteTextColorDirect(67, 39, "SHEL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[1]), 72, 39);
		STlib_drawNumText(&(w_maxammo[1]), 76, 39);

		V_WriteTextColorDirect(67, 40, "RCKT    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[3]), 72, 40);
		STlib_drawNumText(&(w_maxammo[3]), 76, 40);

		V_WriteTextColorDirect(67, 41, "CELL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[2]), 72, 41);
		STlib_drawNumText(&(w_maxammo[2]), 76, 41);
	}

	if ((st_faceindex - 3) % 8 == 0)
	{
		// LOOK RIGHT
		V_WriteCharDirect(65, 38, 16);
		V_WriteCharDirect(65, 39, 16);
		V_WriteCharDirect(65, 40, 16);
		V_WriteCharDirect(65, 41, 16);
	}
	else
	{
		if ((st_faceindex - 4) % 8 == 0)
		{
			// LOOK LEFT
			V_WriteCharDirect(13, 38, 17);
			V_WriteCharDirect(13, 39, 17);
			V_WriteCharDirect(13, 40, 17);
			V_WriteCharDirect(13, 41, 17);
		}
	}

	if (w_keyboxes[0].on)
	{
		V_WriteTextColorDirect(1, 38, "KEYS   ", 7 << 8);

		switch (keyboxes[0])
		{
		case -1:
			V_WriteCharColorDirect(8, 38, 249, 7 << 8);
			break;
		case 0:
			V_WriteCharColorDirect(8, 38, 20, 1 << 8);
			break;
		case 3:
			V_WriteCharColorDirect(8, 38, 2, 1 << 8);
			break;
		}

		switch (keyboxes[1])
		{
		case -1:
			V_WriteCharColorDirect(9, 38, 249, 7 << 8);
			break;
		case 1:
			V_WriteCharColorDirect(9, 38, 20, 14 << 8);
			break;
		case 4:
			V_WriteCharColorDirect(9, 38, 2, 14 << 8);
			break;
		}

		switch (keyboxes[2])
		{
		case -1:
			V_WriteCharColorDirect(10, 38, 249, 7 << 8);
			break;
		case 2:
			V_WriteCharColorDirect(10, 38, 20, 4 << 8);
			break;
		case 5:
			V_WriteCharColorDirect(10, 38, 2, 4 << 8);
			break;
		}
	}
}
#endif

#if defined(MODE_T8050) || defined(MODE_T80100)
void ST_DrawerText8050()
{
	if (w_health.n.on)
	{
		V_WriteTextColorDirect(1, 46, "HEALTH   %%", 7 << 8);
		STlib_drawNumText(&(w_health.n), 8, 46);
	}

	if (w_armor.n.on)
	{
		V_WriteTextColorDirect(1, 47, "ARMOR    %%", 7 << 8);
		STlib_drawNumText(&(w_armor.n), 8, 47);
	}

	if (w_ready.on)
	{
		V_WriteTextColorDirect(1, 48, "AMMO   ", 7 << 8);
		STlib_drawNumText(&(w_ready), 8, 48);
	}

	if (w_ammo[0].on)
	{
		V_WriteTextColorDirect(67, 45, "BULL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[0]), 72, 45);
		STlib_drawNumText(&(w_maxammo[0]), 76, 45);

		V_WriteTextColorDirect(67, 46, "SHEL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[1]), 72, 46);
		STlib_drawNumText(&(w_maxammo[1]), 76, 46);

		V_WriteTextColorDirect(67, 47, "RCKT    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[3]), 72, 47);
		STlib_drawNumText(&(w_maxammo[3]), 76, 47);

		V_WriteTextColorDirect(67, 48, "CELL    /", 7 << 8);
		STlib_drawNumText(&(w_ammo[2]), 72, 48);
		STlib_drawNumText(&(w_maxammo[2]), 76, 48);
	}

	if ((st_faceindex - 3) % 8 == 0)
	{
		// LOOK RIGHT
		V_WriteCharDirect(65, 45, 16);
		V_WriteCharDirect(65, 46, 16);
		V_WriteCharDirect(65, 47, 16);
		V_WriteCharDirect(65, 48, 16);
	}
	else
	{
		if ((st_faceindex - 4) % 8 == 0)
		{
			// LOOK LEFT
			V_WriteCharDirect(13, 45, 17);
			V_WriteCharDirect(13, 46, 17);
			V_WriteCharDirect(13, 47, 17);
			V_WriteCharDirect(13, 48, 17);
		}
	}

	if (w_keyboxes[0].on)
	{
		V_WriteTextColorDirect(1, 45, "KEYS   ", 7 << 8);

		switch (keyboxes[0])
		{
		case -1:
			V_WriteCharColorDirect(8, 45, 249, 7 << 8);
			break;
		case 0:
			V_WriteCharColorDirect(8, 45, 20, 1 << 8);
			break;
		case 3:
			V_WriteCharColorDirect(8, 45, 2, 1 << 8);
			break;
		}

		switch (keyboxes[1])
		{
		case -1:
			V_WriteCharColorDirect(9, 45, 249, 7 << 8);
			break;
		case 1:
			V_WriteCharColorDirect(9, 45, 20, 14 << 8);
			break;
		case 4:
			V_WriteCharColorDirect(9, 45, 2, 14 << 8);
			break;
		}

		switch (keyboxes[2])
		{
		case -1:
			V_WriteCharColorDirect(10, 45, 249, 7 << 8);
			break;
		case 2:
			V_WriteCharColorDirect(10, 45, 20, 4 << 8);
			break;
		case 5:
			V_WriteCharColorDirect(10, 45, 2, 4 << 8);
			break;
		}
	}
}
#endif

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void ST_drawWidgets(byte refresh)
{
	int i;

	STlib_updateNum(&w_ready, refresh);

	STlib_updateNum(&w_ammo[0], refresh);
	STlib_updateNum(&w_maxammo[0], refresh);

	STlib_updateNum(&w_ammo[1], refresh);
	STlib_updateNum(&w_maxammo[1], refresh);

	STlib_updateNum(&w_ammo[2], refresh);
	STlib_updateNum(&w_maxammo[2], refresh);

	STlib_updateNum(&w_ammo[3], refresh);
	STlib_updateNum(&w_maxammo[3], refresh);

	STlib_updatePercent(&w_health, refresh);
	STlib_updatePercent(&w_armor, refresh);

	STlib_updateBinIcon(&w_armsbg, refresh);

	STlib_updateMultIcon(&w_arms[0], refresh);
	STlib_updateMultIcon(&w_arms[1], refresh);
	STlib_updateMultIcon(&w_arms[2], refresh);
	STlib_updateMultIcon(&w_arms[3], refresh);
	STlib_updateMultIcon(&w_arms[4], refresh);
	STlib_updateMultIcon(&w_arms[5], refresh);

	STlib_updateMultIcon(&w_faces, refresh);

	STlib_updateMultIcon(&w_keyboxes[0], refresh);
	STlib_updateMultIcon(&w_keyboxes[1], refresh);
	STlib_updateMultIcon(&w_keyboxes[2], refresh);
}
#endif

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void ST_Drawer(byte screenblocks, byte refresh)
{

	st_statusbaron = (screenblocks < 11) || automapactive;

	// Do red-/gold-shifts from damage/items
	ST_doPaletteStuff();

	st_firsttime = st_firsttime || refresh;

	if (screenblocks >= 11)
		return;

	// If just after ST_Start(), refresh all
	if (st_firsttime)
	{
		st_firsttime = 0;

		// draw status bar background to off-screen buff
		ST_refreshBackground();

		// and refresh all widgets
		ST_drawWidgets(1);
	}
	// Otherwise, update as little as possible
	else
	{
		ST_drawWidgets(0);
	}
}

void ST_DrawerMini()
{
	// Do red-/gold-shifts from damage/items
	ST_doPaletteStuff();

	STlib_updateNum_Direct(&w_ready);

	STlib_updateNum_Direct(&(w_health.n));
	STlib_updateNum_Direct(&(w_armor.n));

	STlib_updateMultIcon_Direct(&w_faces);

	STlib_updateMultIcon_Direct(&w_keyboxes[0]);
	STlib_updateMultIcon_Direct(&w_keyboxes[1]);
	STlib_updateMultIcon_Direct(&w_keyboxes[2]);
}
#endif

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void ST_loadGraphics(void)
{

	int i;
	int j;
	int facenum;

	char namebuf[9];

	// Load the numbers, tall and short
	for (i = 0; i < 10; i++)
	{
		sprintf(namebuf, "STTNUM%d", i);
		tallnum[i] = (patch_t *)W_CacheLumpName(namebuf, PU_STATIC);

		sprintf(namebuf, "STYSNUM%d", i);
		shortnum[i] = (patch_t *)W_CacheLumpName(namebuf, PU_STATIC);
	}

	// Load percent key.
	//Note: why not load STMINUS here, too?
	tallpercent = (patch_t *)W_CacheLumpName("STTPRCNT", PU_STATIC);

	// key cards
	for (i = 0; i < NUMCARDS; i++)
	{
		sprintf(namebuf, "STKEYS%d", i);
		keys[i] = (patch_t *)W_CacheLumpName(namebuf, PU_STATIC);
	}

	// arms background
	armsbg = (patch_t *)W_CacheLumpName("STARMS", PU_STATIC);

	// arms ownership widgets
	for (i = 0; i < 6; i++)
	{
		sprintf(namebuf, "STGNUM%d", i + 2);

		// gray #
		arms[i][0] = (patch_t *)W_CacheLumpName(namebuf, PU_STATIC);

		// yellow #
		arms[i][1] = shortnum[i + 2];
	}

	// status bar background bits
	sbar = (patch_t *)W_CacheLumpName("STBAR", PU_STATIC);

	// face states
	facenum = 0;
	for (i = 0; i < ST_NUMPAINFACES; i++)
	{
		for (j = 0; j < ST_NUMSTRAIGHTFACES; j++)
		{
			sprintf(namebuf, "STFST%d%d", i, j);
			faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
		}
		sprintf(namebuf, "STFTR%d0", i); // turn right
		faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
		sprintf(namebuf, "STFTL%d0", i); // turn left
		faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
		sprintf(namebuf, "STFOUCH%d", i); // ouch!
		faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
		sprintf(namebuf, "STFEVL%d", i); // evil grin ;)
		faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
		sprintf(namebuf, "STFKILL%d", i); // pissed off
		faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
	}
	faces[facenum++] = W_CacheLumpName("STFGOD0", PU_STATIC);
	faces[facenum++] = W_CacheLumpName("STFDEAD0", PU_STATIC);
}
#endif

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void ST_loadData(void)
{
	ST_loadGraphics();
}
#endif

void ST_unloadGraphics(void)
{

	int i;

	// unload the numbers, tall and short
	for (i = 0; i < 10; i++)
	{
		Z_ChangeTag(tallnum[i], PU_CACHE);
		Z_ChangeTag(shortnum[i], PU_CACHE);
	}
	// unload tall percent
	Z_ChangeTag(tallpercent, PU_CACHE);

	// unload arms background
	Z_ChangeTag(armsbg, PU_CACHE);

	// unload gray #'s
	for (i = 0; i < 6; i++)
		Z_ChangeTag(arms[i][0], PU_CACHE);

	// unload the key cards
	for (i = 0; i < NUMCARDS; i++)
		Z_ChangeTag(keys[i], PU_CACHE);

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
	Z_ChangeTag(sbar, PU_CACHE);
#endif

	for (i = 0; i < ST_NUMFACES; i++)
		Z_ChangeTag(faces[i], PU_CACHE);

	// Note: nobody ain't seen no unloading
	//   of stminus yet. Dude.
}

void ST_initData(void)
{

	int i;

	st_firsttime = 1;

	st_statusbaron = 1;

	st_faceindex = 0;
	st_palette = -1;

	st_oldhealth = -1;

	for (i = 0; i < NUMWEAPONS; i++)
		oldweaponsowned[i] = players.weaponowned[i];

	for (i = 0; i < 3; i++)
		keyboxes[i] = -1;

	STlib_init();
}

void ST_createWidgets_mini(void)
{
	// ready weapon ammo
	STlib_initNum(&w_ready,
				  270,
				  170,
				  shortnum,
				  &players.ammo[weaponinfo[players.readyweapon].ammo],
				  &st_statusbaron);

	// the last weapon type
	w_ready.data = players.readyweapon;

	// health percentage
	STlib_initNum(&(w_health.n),
					  270,
					  180,
					  shortnum,
					  &players.health,
					  &st_statusbaron);

	// faces
	STlib_initMultIcon(&w_faces,
					   285,
					   165,
					   faces,
					   &st_faceindex,
					   &st_statusbaron);

	// armor percentage - should be colored later
	STlib_initNum(&(w_armor.n),
					  270,
					  190,
					  shortnum,
					  &players.armorpoints,
					  &st_statusbaron);

	// keyboxes 0-2
	STlib_initMultIcon(&w_keyboxes[0],
					   275,
					   169,
					   keys,
					   &keyboxes[0],
					   &st_statusbaron);

	STlib_initMultIcon(&w_keyboxes[1],
					   275,
					   179,
					   keys,
					   &keyboxes[1],
					   &st_statusbaron);

	STlib_initMultIcon(&w_keyboxes[2],
					   275,
					   189,
					   keys,
					   &keyboxes[2],
					   &st_statusbaron);

}

void ST_createWidgets(void)
{

	// ready weapon ammo
	STlib_initNum(&w_ready,
				  ST_AMMOX,
				  ST_AMMOY,
				  tallnum,
				  &players.ammo[weaponinfo[players.readyweapon].ammo],
				  &st_statusbaron);

	// the last weapon type
	w_ready.data = players.readyweapon;

	// health percentage
	STlib_initPercent(&w_health,
					  ST_HEALTHX,
					  ST_HEALTHY,
					  tallnum,
					  &players.health,
					  &st_statusbaron,
					  tallpercent);

	// arms background
	STlib_initBinIcon(&w_armsbg,
					  ST_ARMSBGX,
					  ST_ARMSBGY,
					  armsbg,
					  &st_statusbaron);

	// weapons owned
	STlib_initMultIcon(&w_arms[0],
					   ST_ARMSX,
					   ST_ARMSY,
					   arms[0], (int *)&players.weaponowned[1],
					   &st_statusbaron);

	STlib_initMultIcon(&w_arms[1],
					   ST_ARMSX + ST_ARMSXSPACE,
					   ST_ARMSY,
					   arms[1], (int *)&players.weaponowned[2],
					   &st_statusbaron);

	STlib_initMultIcon(&w_arms[2],
					   ST_ARMSX + 2 * ST_ARMSXSPACE,
					   ST_ARMSY,
					   arms[2], (int *)&players.weaponowned[3],
					   &st_statusbaron);

	STlib_initMultIcon(&w_arms[3],
					   ST_ARMSX,
					   ST_ARMSY + 10,
					   arms[3], (int *)&players.weaponowned[4],
					   &st_statusbaron);

	STlib_initMultIcon(&w_arms[4],
					   ST_ARMSX + ST_ARMSXSPACE,
					   ST_ARMSY + 10,
					   arms[4], (int *)&players.weaponowned[5],
					   &st_statusbaron);

	STlib_initMultIcon(&w_arms[5],
					   ST_ARMSX + 2 * ST_ARMSXSPACE,
					   ST_ARMSY + 10,
					   arms[5], (int *)&players.weaponowned[6],
					   &st_statusbaron);

	// faces
	STlib_initMultIcon(&w_faces,
					   ST_FACESX,
					   ST_FACESY,
					   faces,
					   &st_faceindex,
					   &st_statusbaron);

	// armor percentage - should be colored later
	STlib_initPercent(&w_armor,
					  ST_ARMORX,
					  ST_ARMORY,
					  tallnum,
					  &players.armorpoints,
					  &st_statusbaron, tallpercent);

	// keyboxes 0-2
	STlib_initMultIcon(&w_keyboxes[0],
					   ST_KEY0X,
					   ST_KEY0Y,
					   keys,
					   &keyboxes[0],
					   &st_statusbaron);

	STlib_initMultIcon(&w_keyboxes[1],
					   ST_KEY1X,
					   ST_KEY1Y,
					   keys,
					   &keyboxes[1],
					   &st_statusbaron);

	STlib_initMultIcon(&w_keyboxes[2],
					   ST_KEY2X,
					   ST_KEY2Y,
					   keys,
					   &keyboxes[2],
					   &st_statusbaron);

	// ammo count (all four kinds)
	STlib_initNum(&w_ammo[0],
				  ST_AMMO0X,
				  ST_AMMO0Y,
				  shortnum,
				  &players.ammo[0],
				  &st_statusbaron);

	STlib_initNum(&w_ammo[1],
				  ST_AMMO1X,
				  ST_AMMO1Y,
				  shortnum,
				  &players.ammo[1],
				  &st_statusbaron);

	STlib_initNum(&w_ammo[2],
				  ST_AMMO2X,
				  ST_AMMO2Y,
				  shortnum,
				  &players.ammo[2],
				  &st_statusbaron);

	STlib_initNum(&w_ammo[3],
				  ST_AMMO3X,
				  ST_AMMO3Y,
				  shortnum,
				  &players.ammo[3],
				  &st_statusbaron);

	// max ammo count (all four kinds)
	STlib_initNum(&w_maxammo[0],
				  ST_MAXAMMO0X,
				  ST_MAXAMMO0Y,
				  shortnum,
				  &players.maxammo[0],
				  &st_statusbaron);

	STlib_initNum(&w_maxammo[1],
				  ST_MAXAMMO1X,
				  ST_MAXAMMO1Y,
				  shortnum,
				  &players.maxammo[1],
				  &st_statusbaron);

	STlib_initNum(&w_maxammo[2],
				  ST_MAXAMMO2X,
				  ST_MAXAMMO2Y,
				  shortnum,
				  &players.maxammo[2],
				  &st_statusbaron);

	STlib_initNum(&w_maxammo[3],
				  ST_MAXAMMO3X,
				  ST_MAXAMMO3Y,
				  shortnum,
				  &players.maxammo[3],
				  &st_statusbaron);
}

static byte st_stopped = 1;

void ST_Start(void)
{

	if (!st_stopped)
		ST_Stop();

	ST_initData();

	if (screenblocks == 11){
		ST_createWidgets_mini();
	}else{
		ST_createWidgets();
	}

	st_stopped = 0;
}

void ST_Stop(void)
{
	if (st_stopped)
		return;

	I_SetPalette(0);

	st_stopped = 1;
}

#if defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void ST_Init(void)
{
	ST_loadData();
}
#endif
