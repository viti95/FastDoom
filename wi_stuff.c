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
//	Intermission screens.
//

#include <stdio.h>

#include "i_random.h"

#include "z_zone.h"

#include "m_misc.h"

#include "i_system.h"

#include "w_wad.h"

#include "g_game.h"

#include "r_local.h"
#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

// Needs access to LFB.
#include "v_video.h"

#include "wi_stuff.h"

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//

//
// Different vetween registered DOOM (1994) and
//  Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II), which had 34 maps
//  in one episode. So there.
#if (EXE_VERSION < EXE_VERSION_ULTIMATE)
#define NUMEPISODES 3
#else
#define NUMEPISODES 4
#endif
#define NUMMAPS 9

// in tics
//U #define PAUSELEN		(TICRATE*2)
//U #define SCORESTEP		100
//U #define ANIMPERIOD		32
// pixel distance from "(YOU)" to "PLAYER N"
//U #define STARDIST		10
//U #define WK 1

// GLOBAL LOCATIONS
#define WI_TITLEY 2
#define WI_SPACINGY 33

// SINGPLE-PLAYER STUFF
#define SP_STATSX 50
#define SP_STATSY 50

#define SP_TIMEX 16
#define SP_TIMEY (SCREENHEIGHT - 32)

typedef enum
{
	ANIM_ALWAYS,
	ANIM_RANDOM,
	ANIM_LEVEL

} animenum_t;

typedef struct
{
	int x;
	int y;

} point_t;

//
// Animation.
// There is another anim_t used in p_spec.
//
typedef struct
{
	animenum_t type;

	// period in tics between animations
	int period;

	// number of animation frames
	int nanims;

	// location of animation
	point_t loc;

	// ALWAYS: n/a,
	// RANDOM: period deviation (<256),
	// LEVEL: level
	int data1;

	// ALWAYS: n/a,
	// RANDOM: random base period,
	// LEVEL: n/a
	int data2;

	// actual graphics for frames of animations
	patch_t *p[3];

	// following must be initialized to zero before use!

	// next value of bcnt (used in conjunction with period)
	int nexttic;

	// last drawn animation frame
	int lastdrawn;

	// next frame number to animate
	int ctr;

	// used by RANDOM and LEVEL when animating
	int state;

} anim_t;

static point_t lnodes[NUMEPISODES][NUMMAPS] =
	{
		// Episode 0 World Map
		{
			{185, 164}, // location of level 0 (CJ)
			{148, 143}, // location of level 1 (CJ)
			{69, 122},	// location of level 2 (CJ)
			{209, 102}, // location of level 3 (CJ)
			{116, 89},	// location of level 4 (CJ)
			{166, 55},	// location of level 5 (CJ)
			{71, 56},	// location of level 6 (CJ)
			{135, 29},	// location of level 7 (CJ)
			{71, 24}	// location of level 8 (CJ)
		},

		// Episode 1 World Map should go here
		{
			{254, 25},	// location of level 0 (CJ)
			{97, 50},	// location of level 1 (CJ)
			{188, 64},	// location of level 2 (CJ)
			{128, 78},	// location of level 3 (CJ)
			{214, 92},	// location of level 4 (CJ)
			{133, 130}, // location of level 5 (CJ)
			{208, 136}, // location of level 6 (CJ)
			{148, 140}, // location of level 7 (CJ)
			{235, 158}	// location of level 8 (CJ)
		},

		// Episode 2 World Map should go here
		{
			{156, 168}, // location of level 0 (CJ)
			{48, 154},	// location of level 1 (CJ)
			{174, 95},	// location of level 2 (CJ)
			{265, 75},	// location of level 3 (CJ)
			{130, 48},	// location of level 4 (CJ)
			{279, 23},	// location of level 5 (CJ)
			{198, 48},	// location of level 6 (CJ)
			{140, 25},	// location of level 7 (CJ)
			{281, 136}	// location of level 8 (CJ)
		}

};

//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
static anim_t epsd0animinfo[] =
	{
		{ANIM_ALWAYS, TICRATE / 3, 3, {224, 104}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {184, 160}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {112, 136}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {72, 112}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {88, 96}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {64, 48}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {192, 40}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {136, 16}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {80, 16}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {64, 24}}};

static anim_t epsd1animinfo[] =
	{
		{ANIM_LEVEL, TICRATE / 3, 1, {128, 136}, 1},
		{ANIM_LEVEL, TICRATE / 3, 1, {128, 136}, 2},
		{ANIM_LEVEL, TICRATE / 3, 1, {128, 136}, 3},
		{ANIM_LEVEL, TICRATE / 3, 1, {128, 136}, 4},
		{ANIM_LEVEL, TICRATE / 3, 1, {128, 136}, 5},
		{ANIM_LEVEL, TICRATE / 3, 1, {128, 136}, 6},
		{ANIM_LEVEL, TICRATE / 3, 1, {128, 136}, 7},
		{ANIM_LEVEL, TICRATE / 3, 3, {192, 144}, 8},
		{ANIM_LEVEL, TICRATE / 3, 1, {128, 136}, 8}};

static anim_t epsd2animinfo[] =
	{
		{ANIM_ALWAYS, TICRATE / 3, 3, {104, 168}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {40, 136}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {160, 96}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {104, 80}},
		{ANIM_ALWAYS, TICRATE / 3, 3, {120, 32}},
		{ANIM_ALWAYS, TICRATE / 4, 3, {40, 0}}};

static int NUMANIMS[NUMEPISODES] =
	{
		sizeof(epsd0animinfo) / sizeof(anim_t),
		sizeof(epsd1animinfo) / sizeof(anim_t),
		sizeof(epsd2animinfo) / sizeof(anim_t)};

static anim_t *anims[NUMEPISODES] =
	{
		epsd0animinfo,
		epsd1animinfo,
		epsd2animinfo};

//
// GENERAL DATA
//

//
// Locally used stuff.
//
#define FB 0

// States for single-player
#define SP_KILLS 0
#define SP_ITEMS 2
#define SP_SECRET 4
#define SP_TIME 8
#define SP_PAR ST_TIME

#define SP_PAUSE 1

// in seconds
#define SHOWNEXTLOCDELAY 4

// used to accelerate or skip a stage
static int acceleratestage;

// specifies current state
static stateenum_t state;

// contains information passed into intermission
static wbstartstruct_t *wbs;

static wbplayerstruct_t plrs; // wbs->plyr[]

// used for general timing
static int cnt;

// used for timing of background animation
static int bcnt;

// signals to refresh everything for one frame
static int firstrefresh;

static int cnt_kills;
static int cnt_items;
static int cnt_secret;
static int cnt_time;
static int cnt_par;
static int cnt_pause;

// # of commercial levels
static int NUMCMAPS;

//
//	GRAPHICS
//

// background (map of levels).
static patch_t *bg;

// You Are Here graphic
static patch_t *yah[2];

// splat
static patch_t *splat;

// %, : graphics
static patch_t *percent;
static patch_t *colon;

// 0-9 graphic
static patch_t *num[10];

// "Finished!" graphics
static patch_t *finished;

// "Entering" graphic
static patch_t *entering;

// "secret"
static patch_t *sp_secret;

// "Kills", "Scrt", "Items"
static patch_t *kills;
static patch_t *items;

// Time sucks.
static patch_t *time;
static patch_t *par;
static patch_t *sucks;

// "Total", your face, your dead face
static patch_t *total;
static patch_t *star;
static patch_t *bstar;

// Name graphics of each level (centered)
static patch_t **lnames;

//
// CODE
//

void WI_slamBackground(void)
{
	memcpy(screens[0], screens[1], SCREENWIDTH * SCREENHEIGHT);
	V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);
}

// Draws "<Levelname> Finished!"
void WI_drawLF(void)
{
	int y = WI_TITLEY;

	// draw <LevelName>
	V_DrawPatch((SCREENWIDTH - SHORT(lnames[wbs->last]->width)) / 2,
				y, FB, lnames[wbs->last]);

	// draw "Finished!"
	y += (5 * SHORT(lnames[wbs->last]->height)) / 4;

	V_DrawPatch((SCREENWIDTH - SHORT(finished->width)) / 2,
				y, FB, finished);
}

// Draws "Entering <LevelName>"
void WI_drawEL(void)
{
	int y = WI_TITLEY;

	// draw "Entering"
	V_DrawPatch((SCREENWIDTH - SHORT(entering->width)) / 2,
				y, FB, entering);

	// draw level
	y += (5 * SHORT(lnames[wbs->next]->height)) / 4;

	V_DrawPatch((SCREENWIDTH - SHORT(lnames[wbs->next]->width)) / 2,
				y, FB, lnames[wbs->next]);
}

void WI_drawOnLnode(int n,
					patch_t *c[])
{

	int i;
	int left;
	int top;
	int right;
	int bottom;
	boolean fits = false;

	i = 0;
	do
	{
		left = lnodes[wbs->epsd][n].x - SHORT(c[i]->leftoffset);
		top = lnodes[wbs->epsd][n].y - SHORT(c[i]->topoffset);
		right = left + SHORT(c[i]->width);
		bottom = top + SHORT(c[i]->height);

		if (left >= 0 && right < SCREENWIDTH && top >= 0 && bottom < SCREENHEIGHT)
		{
			fits = true;
		}
		else
		{
			i++;
		}
	} while (!fits && i != 2);

	if (fits && i < 2)
	{
		V_DrawPatch(lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y,
					FB, c[i]);
	}
}

void WI_initAnimatedBack(void)
{
	int i;
	anim_t *a;

	if (commercial)
		return;

#if (EXE_VERSION >= EXE_VERSION_ULTIMATE)
	if (wbs->epsd > 2)
		return;
#endif

	for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
	{
		a = &anims[wbs->epsd][i];

		// init variables
		a->ctr = -1;

		// specify the next time to draw it
		if (a->type == ANIM_ALWAYS)
			a->nexttic = bcnt + 1 + (M_Random % a->period);
		else if (a->type == ANIM_RANDOM)
			a->nexttic = bcnt + 1 + a->data2 + (M_Random % a->data1);
		else if (a->type == ANIM_LEVEL)
			a->nexttic = bcnt + 1;
	}
}

void WI_updateAnimatedBack(void)
{
	int i;
	anim_t *a;

	if (commercial)
		return;

#if (EXE_VERSION >= EXE_VERSION_ULTIMATE)
	if (wbs->epsd > 2)
		return;
#endif

	for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
	{
		a = &anims[wbs->epsd][i];

		if (bcnt == a->nexttic)
		{
			switch (a->type)
			{
			case ANIM_ALWAYS:
				if (++a->ctr >= a->nanims)
					a->ctr = 0;
				a->nexttic = bcnt + a->period;
				break;

			case ANIM_RANDOM:
				a->ctr++;
				if (a->ctr == a->nanims)
				{
					a->ctr = -1;
					a->nexttic = bcnt + a->data2 + (M_Random % a->data1);
				}
				else
					a->nexttic = bcnt + a->period;
				break;

			case ANIM_LEVEL:
				// gawd-awful hack for level anims
				if (!(state == StatCount && i == 7) && wbs->next == a->data1)
				{
					a->ctr++;
					if (a->ctr == a->nanims)
						a->ctr--;
					a->nexttic = bcnt + a->period;
				}
				break;
			}
		}
	}
}

void WI_drawAnimatedBack(void)
{
	int i;
	anim_t *a;

	if (commercial)
		return;

#if (EXE_VERSION >= EXE_VERSION_ULTIMATE)
	if (wbs->epsd > 2)
		return;
#endif

	for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
	{
		a = &anims[wbs->epsd][i];

		if (a->ctr >= 0)
			V_DrawPatch(a->loc.x, a->loc.y, FB, a->p[a->ctr]);
	}
}

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//

int WI_drawNum(int x,
			   int y,
			   int n,
			   int digits)
{

	int fontwidth = SHORT(num[0]->width);
	int temp;

	if (digits < 0)
	{
		if (!n)
		{
			// make variable-length zeros 1 digit long
			digits = 1;
		}
		else
		{
			// figure out # of digits in #
			digits = 0;
			temp = n;

			while (temp)
			{
				temp /= 10;
				digits++;
			}
		}
	}

	// if non-number, do not draw it
	if (n == 1994)
		return 0;

	// draw the new number
	while (digits--)
	{
		x -= fontwidth;
		V_DrawPatch(x, y, FB, num[n % 10]);
		n /= 10;
	}

	return x;
}

void WI_drawPercent(int x,
					int y,
					int p)
{
	if (p < 0)
		return;

	V_DrawPatch(x, y, FB, percent);
	WI_drawNum(x, y, p, -1);
}

//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
void WI_drawTime(int x,
				 int y,
				 int t)
{

	int div;
	int n;

	if (t < 0)
		return;

	if (t <= 61 * 59)
	{
		div = 1;

		do
		{
			n = (t / div) % 60;
			x = WI_drawNum(x, y, n, 2) - SHORT(colon->width);
			div *= 60;

			// draw
			if (div == 60 || t / div)
				V_DrawPatch(x, y, FB, colon);

		} while (t / div);
	}
	else
	{
		// "sucks"
		V_DrawPatch(x - SHORT(sucks->width), y, FB, sucks);
	}
}

void WI_End(void)
{
	void WI_unloadData(void);
	WI_unloadData();
}

void WI_initNoState(void)
{
	state = NoState;
	acceleratestage = 0;
	cnt = 10;
}

void WI_updateNoState(void)
{

	WI_updateAnimatedBack();

	if (!--cnt)
	{
		WI_End();
		G_WorldDone();
	}
}

static boolean snl_pointeron = false;

void WI_initShowNextLoc(void)
{
	state = ShowNextLoc;
	acceleratestage = 0;
	cnt = SHOWNEXTLOCDELAY * TICRATE;

	WI_initAnimatedBack();
}

void WI_updateShowNextLoc(void)
{
	WI_updateAnimatedBack();

	if (!--cnt || acceleratestage)
		WI_initNoState();
	else
		snl_pointeron = (cnt & 31) < 20;
}

void WI_drawShowNextLoc(void)
{

	int i;
	int last;

	WI_slamBackground();

	// draw animated background
	WI_drawAnimatedBack();

	if (!commercial)
	{
#if (EXE_VERSION >= EXE_VERSION_ULTIMATE)
		if (wbs->epsd > 2)
		{
			WI_drawEL();
			return;
		}
#endif

		last = (wbs->last == 8) ? wbs->next - 1 : wbs->last;

		// draw a splat on taken cities.
		for (i = 0; i <= last; i++)
			WI_drawOnLnode(i, &splat);

		// splat the secret level?
		if (wbs->didsecret)
			WI_drawOnLnode(8, &splat);

		// draw flashing ptr
		if (snl_pointeron)
			WI_drawOnLnode(wbs->next, yah);
	}

	// draws which level you are entering..
	if ((!commercial) || wbs->next != 30)
		WI_drawEL();
}

void WI_drawNoState(void)
{
	snl_pointeron = true;
	WI_drawShowNextLoc();
}

static int ng_state;

static int sp_state;

void WI_initStats(void)
{
	state = StatCount;
	acceleratestage = 0;
	sp_state = 1;
	cnt_kills = cnt_items = cnt_secret = -1;
	cnt_time = cnt_par = -1;
	cnt_pause = TICRATE;

	WI_initAnimatedBack();
}

void WI_updateStats(void)
{

	WI_updateAnimatedBack();

	if (acceleratestage && sp_state != 10)
	{
		acceleratestage = 0;
		cnt_kills = (plrs.skills * 100) / wbs->maxkills;
		cnt_items = (plrs.sitems * 100) / wbs->maxitems;
		cnt_secret = (plrs.ssecret * 100) / wbs->maxsecret;
		cnt_time = plrs.stime / TICRATE;
		cnt_par = wbs->partime / TICRATE;
		S_StartSound(0, sfx_barexp);
		sp_state = 10;
	}

	if (sp_state == 2)
	{
		cnt_kills += 2;

		if (!(bcnt & 3))
			S_StartSound(0, sfx_pistol);

		if (cnt_kills >= (plrs.skills * 100) / wbs->maxkills)
		{
			cnt_kills = (plrs.skills * 100) / wbs->maxkills;
			S_StartSound(0, sfx_barexp);
			sp_state++;
		}
	}
	else if (sp_state == 4)
	{
		cnt_items += 2;

		if (!(bcnt & 3))
			S_StartSound(0, sfx_pistol);

		if (cnt_items >= (plrs.sitems * 100) / wbs->maxitems)
		{
			cnt_items = (plrs.sitems * 100) / wbs->maxitems;
			S_StartSound(0, sfx_barexp);
			sp_state++;
		}
	}
	else if (sp_state == 6)
	{
		cnt_secret += 2;

		if (!(bcnt & 3))
			S_StartSound(0, sfx_pistol);

		if (cnt_secret >= (plrs.ssecret * 100) / wbs->maxsecret)
		{
			cnt_secret = (plrs.ssecret * 100) / wbs->maxsecret;
			S_StartSound(0, sfx_barexp);
			sp_state++;
		}
	}

	else if (sp_state == 8)
	{
		if (!(bcnt & 3))
			S_StartSound(0, sfx_pistol);

		cnt_time += 3;

		if (cnt_time >= plrs.stime / TICRATE)
			cnt_time = plrs.stime / TICRATE;

		cnt_par += 3;

		if (cnt_par >= wbs->partime / TICRATE)
		{
			cnt_par = wbs->partime / TICRATE;

			if (cnt_time >= plrs.stime / TICRATE)
			{
				S_StartSound(0, sfx_barexp);
				sp_state++;
			}
		}
	}
	else if (sp_state == 10)
	{
		if (acceleratestage)
		{
			S_StartSound(0, sfx_sgcock);

			if (commercial)
				WI_initNoState();
			else
				WI_initShowNextLoc();
		}
	}
	else if (sp_state & 1)
	{
		if (!--cnt_pause)
		{
			sp_state++;
			cnt_pause = TICRATE;
		}
	}
}

void WI_drawStats(void)
{
	// line height
	int lh;

	lh = (3 * SHORT(num[0]->height)) / 2;

	WI_slamBackground();

	// draw animated background
	WI_drawAnimatedBack();

	WI_drawLF();

	V_DrawPatch(SP_STATSX, SP_STATSY, FB, kills);
	WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY, cnt_kills);

	V_DrawPatch(SP_STATSX, SP_STATSY + lh, FB, items);
	WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY + lh, cnt_items);

	V_DrawPatch(SP_STATSX, SP_STATSY + 2 * lh, FB, sp_secret);
	WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY + 2 * lh, cnt_secret);

	V_DrawPatch(SP_TIMEX, SP_TIMEY, FB, time);
	WI_drawTime(SCREENWIDTH / 2 - SP_TIMEX, SP_TIMEY, cnt_time);

#if (EXE_VERSION >= EXE_VERSION_ULTIMATE)
	if (wbs->epsd < 3)
#endif
	{
		V_DrawPatch(SCREENWIDTH / 2 + SP_TIMEX, SP_TIMEY, FB, par);
		WI_drawTime(SCREENWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);
	}
}

void WI_checkForAccelerate(void)
{
	int i;
	player_t *player;

	player = &players;

	if (player->cmd.buttons & BT_ATTACK)
	{
		if (!player->attackdown)
			acceleratestage = 1;
		player->attackdown = true;
	}
	else
		player->attackdown = false;
	if (player->cmd.buttons & BT_USE)
	{
		if (!player->usedown)
			acceleratestage = 1;
		player->usedown = true;
	}
	else
		player->usedown = false;
}

// Updates stuff each tick
void WI_Ticker(void)
{
	// counter for general background animation
	bcnt++;

	if (bcnt == 1)
	{
		// intermission music
		if (commercial)
			S_ChangeMusic(mus_dm2int, true);
		else
			S_ChangeMusic(mus_inter, true);
	}

	WI_checkForAccelerate();

	switch (state)
	{
	case StatCount:
		WI_updateStats();
		break;

	case ShowNextLoc:
		WI_updateShowNextLoc();
		break;

	case NoState:
		WI_updateNoState();
		break;
	}
}

void WI_loadData(void)
{
	int i;
	int j;
	char name[9];
	anim_t *a;

	if (commercial)
		strcpy(name, "INTERPIC");
	else
		sprintf(name, "WIMAP%d", wbs->epsd);

#if (EXE_VERSION >= EXE_VERSION_ULTIMATE)
	if (wbs->epsd == 3)
		strcpy(name, "INTERPIC");
#endif

	// background
	bg = W_CacheLumpName(name, PU_CACHE);
	V_DrawPatch(0, 0, 1, bg);

	if (commercial)
	{
		NUMCMAPS = 32;
		lnames = (patch_t **)Z_Malloc(sizeof(patch_t *) * NUMCMAPS,
									  PU_STATIC, 0);
		for (i = 0; i < NUMCMAPS; i++)
		{
			sprintf(name, "CWILV%2.2d", i);
			lnames[i] = W_CacheLumpName(name, PU_STATIC);
		}
	}
	else
	{
		lnames = (patch_t **)Z_Malloc(sizeof(patch_t *) * NUMMAPS,
									  PU_STATIC, 0);
		for (i = 0; i < NUMMAPS; i++)
		{
			sprintf(name, "WILV%d%d", wbs->epsd, i);
			lnames[i] = W_CacheLumpName(name, PU_STATIC);
		}

		// you are here
		yah[0] = W_CacheLumpName("WIURH0", PU_STATIC);

		// you are here (alt.)
		yah[1] = W_CacheLumpName("WIURH1", PU_STATIC);

		// splat
		splat = W_CacheLumpName("WISPLAT", PU_STATIC);

#if (EXE_VERSION >= EXE_VERSION_ULTIMATE)
		if (wbs->epsd < 3)
#endif
		{
			for (j = 0; j < NUMANIMS[wbs->epsd]; j++)
			{
				a = &anims[wbs->epsd][j];
				for (i = 0; i < a->nanims; i++)
				{
					// MONDO HACK!
					if (wbs->epsd != 1 || j != 8)
					{
						// animations
						sprintf(name, "WIA%d%.2d%.2d", wbs->epsd, j, i);
						a->p[i] = W_CacheLumpName(name, PU_STATIC);
					}
					else
					{
						// HACK ALERT!
						a->p[i] = anims[1][4].p[i];
					}
				}
			}
		}
	}

	for (i = 0; i < 10; i++)
	{
		// numbers 0-9
		sprintf(name, "WINUM%d", i);
		num[i] = W_CacheLumpName(name, PU_STATIC);
	}

	// percent sign
	percent = W_CacheLumpName("WIPCNT", PU_STATIC);

	// "finished"
	finished = W_CacheLumpName("WIF", PU_STATIC);

	// "entering"
	entering = W_CacheLumpName("WIENTER", PU_STATIC);

	// "kills"
	kills = W_CacheLumpName("WIOSTK", PU_STATIC);

	// "secret"
	sp_secret = W_CacheLumpName("WISCRT2", PU_STATIC);

	// Yuck.
	items = W_CacheLumpName("WIOSTI", PU_STATIC);

	// ":"
	colon = W_CacheLumpName("WICOLON", PU_STATIC);

	// "time"
	time = W_CacheLumpName("WITIME", PU_STATIC);

	// "sucks"
	sucks = W_CacheLumpName("WISUCKS", PU_STATIC);

	// "par"
	par = W_CacheLumpName("WIPAR", PU_STATIC);

	// "total"
	total = W_CacheLumpName("WIMSTT", PU_STATIC);

	// your face
	star = W_CacheLumpName("STFST01", PU_STATIC);

	// dead face
	bstar = W_CacheLumpName("STFDEAD0", PU_STATIC);
}

void WI_unloadData(void)
{
	int i;
	int j;

	for (i = 0; i < 10; i++)
		Z_ChangeTag(num[i], PU_CACHE);

	if (commercial)
	{
		for (i = 0; i < NUMCMAPS; i++)
			Z_ChangeTag(lnames[i], PU_CACHE);
	}
	else
	{
		Z_ChangeTag(yah[0], PU_CACHE);
		Z_ChangeTag(yah[1], PU_CACHE);

		Z_ChangeTag(splat, PU_CACHE);

		for (i = 0; i < NUMMAPS; i++)
			Z_ChangeTag(lnames[i], PU_CACHE);
#if (EXE_VERSION >= EXE_VERSION_ULTIMATE)
		if (wbs->epsd < 3)
#endif
		{
			for (j = 0; j < NUMANIMS[wbs->epsd]; j++)
			{
				if (wbs->epsd != 1 || j != 8)
					for (i = 0; i < anims[wbs->epsd][j].nanims; i++)
						Z_ChangeTag(anims[wbs->epsd][j].p[i], PU_CACHE);
			}
		}
	}

	Z_Free(lnames);

	Z_ChangeTag(percent, PU_CACHE);
	Z_ChangeTag(colon, PU_CACHE);
	Z_ChangeTag(finished, PU_CACHE);
	Z_ChangeTag(entering, PU_CACHE);
	Z_ChangeTag(kills, PU_CACHE);
	Z_ChangeTag(sp_secret, PU_CACHE);
	Z_ChangeTag(items, PU_CACHE);
	Z_ChangeTag(time, PU_CACHE);
	Z_ChangeTag(sucks, PU_CACHE);
	Z_ChangeTag(par, PU_CACHE);

	Z_ChangeTag(total, PU_CACHE);
	//  Z_ChangeTag(star, PU_CACHE);
	//  Z_ChangeTag(bstar, PU_CACHE);

}

void WI_Drawer(void)
{
	switch (state)
	{
	case StatCount:
		WI_drawStats();
		break;

	case ShowNextLoc:
		WI_drawShowNextLoc();
		break;

	case NoState:
		WI_drawNoState();
		break;
	}
}

void WI_initVariables(wbstartstruct_t *wbstartstruct)
{
	wbs = wbstartstruct;
	acceleratestage = 0;
	cnt = bcnt = 0;
	firstrefresh = 1;
	plrs = wbs->plyr;

	if (!wbs->maxkills)
		wbs->maxkills = 1;

	if (!wbs->maxitems)
		wbs->maxitems = 1;

	if (!wbs->maxsecret)
		wbs->maxsecret = 1;
}

void WI_Start(wbstartstruct_t *wbstartstruct)
{

	WI_initVariables(wbstartstruct);
	WI_loadData();
	WI_initStats();
}
