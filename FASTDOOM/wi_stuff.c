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

#include <string.h>
#include <stdio.h>
#include "options.h"
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
#define NUMEPISODES 4
#define NUMMAPS 9
#define NUMCMAPS 32

// in tics
// U #define PAUSELEN		(TICRATE*2)
// U #define SCORESTEP		100
// U #define ANIMPERIOD		32
// pixel distance from "(YOU)" to "PLAYER N"
// U #define STARDIST		10
// U #define WK 1

// GLOBAL LOCATIONS
#define WI_TITLEY 2
#define WI_SPACINGY 33

// SINGPLE-PLAYER STUFF
#define SP_STATSX 50
#define SP_STATSY 50

#define SP_TIMEX 16
#define SP_TIMEY (200 - 32)

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

// Name graphics of each level (centered)
static patch_t **lnames;

//
// CODE
//

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA)
char bgname[9];
#endif

byte *screen1;

void WI_slamBackground(void)
{
#if defined(MODE_T4025)
	V_DrawPatchDirectText4025(0, 0, W_CacheLumpName(bgname, PU_CACHE));
#endif
#if defined(MODE_T4050)
	V_DrawPatchDirectText4050(0, 0, W_CacheLumpName(bgname, PU_CACHE));
#endif
#if defined(MODE_T8025)
	V_DrawPatchDirectText8025(0, 0, W_CacheLumpName(bgname, PU_CACHE));
#endif
#if defined(MODE_MDA)
	V_DrawPatchDirectTextMDA(0, 0, W_CacheLumpName(bgname, PU_CACHE));
#endif
#if defined(MODE_T8043)
	V_DrawPatchDirectText8043(0, 0, W_CacheLumpName(bgname, PU_CACHE));
#endif
#if defined(MODE_T8050)
	V_DrawPatchDirectText8050(0, 0, W_CacheLumpName(bgname, PU_CACHE));
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
	CopyDWords(screen1, screen0, (SCREENWIDTH * SCREENHEIGHT) / 4);
	V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);
#endif
#if defined(USE_BACKBUFFER)
	CopyDWords(screen1, backbuffer, (SCREENWIDTH * SCREENHEIGHT) / 4);
#endif
}

// Draws "<Levelname> Finished!"
void WI_drawLF(void)
{
	int y = WI_TITLEY;

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA)
	char *titlecurrent;
	char *titlenext;
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA)
	if (gamemode == commercial)
	{
		if (gamemission == pack_plut)
		{
			titlecurrent = mapnamesp[gamemap - 1];
			titlenext = mapnamesp[gamemap];
		}
		else if (gamemission == pack_tnt)
		{
			titlecurrent = mapnamest[gamemap - 1];
			titlenext = mapnamest[gamemap];
		}
		else
		{
			titlecurrent = mapnames2[gamemap - 1];
			titlenext = mapnames2[gamemap];
		}
	}
	else
	{
		titlecurrent = mapnames[(gameepisode - 1) * 9 + gamemap - 1];
		titlenext = mapnames[(gameepisode - 1) * 9 + gamemap];
	}
#endif

	// draw <LevelName>

#if defined(MODE_T4025) || defined(MODE_T4050)
	V_WriteTextDirect((320 - lnames[wbs->last]->width) / 16, y / 8, titlecurrent);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	V_WriteTextDirect((320 - lnames[wbs->last]->width) / 8, y / 8, titlecurrent);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	V_WriteTextDirect((320 - lnames[wbs->last]->width) / 8, y / 4, titlecurrent);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
  	V_DrawPatchModeCentered((320 - lnames[wbs->last]->width) / 2, y, lnames[wbs->last]);
#endif
#if defined(MODE_Y_HALF)
	V_DrawPatchModeCentered((320 - lnames[wbs->last]->width) / 2, y / 2, lnames[wbs->last]);
#endif
	// draw "Finished!"
	y += (5 * lnames[wbs->last]->height) / 4;

#if defined(MODE_T4025) || defined(MODE_T4050)
	V_WriteTextDirect((320 - finished->width) / 16, y / 8, "FINISHED");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	V_WriteTextDirect((320 - finished->width) / 8, y / 8, "FINISHED");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	V_WriteTextDirect((320 - finished->width) / 8, y / 4, "FINISHED");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
  	V_DrawPatchModeCentered((320 - finished->width) / 2, y, finished);
#endif
#if defined(MODE_Y_HALF)
  	V_DrawPatchModeCentered((320 - finished->width) / 2, y / 2, finished);
#endif
}

// Draws "Entering <LevelName>"
void WI_drawEL(void)
{
	int y = WI_TITLEY;
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA)
	char *titlecurrent;
	char *titlenext;
#endif

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA)
	if (gamemode == commercial)
	{
		if (gamemission == pack_plut)
		{
			titlecurrent = mapnamesp[gamemap - 1];
			titlenext = mapnamesp[gamemap];
		}
		else if (gamemission == pack_tnt)
		{
			titlecurrent = mapnamest[gamemap - 1];
			titlenext = mapnamest[gamemap];
		}
		else
		{
			titlecurrent = mapnames2[gamemap - 1];
			titlenext = mapnames2[gamemap];
		}
	}
	else
	{
		titlecurrent = mapnames[(gameepisode - 1) * 9 + gamemap - 1];
		titlenext = mapnames[(gameepisode - 1) * 9 + gamemap];
	}
#endif

// draw "Entering"
#if defined(MODE_T4025) || defined(MODE_T4050)
	V_WriteTextDirect((320 - entering->width) / 16, y / 8, "ENTERING");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	V_WriteTextDirect((320 - entering->width) / 8, y / 8, "ENTERING");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	V_WriteTextDirect((320 - entering->width) / 8, y / 4, "ENTERING");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
  	V_DrawPatchModeCentered((320 - entering->width) / 2, y, entering);
#endif
#if defined(MODE_Y_HALF)
  	V_DrawPatchModeCentered((320 - entering->width) / 2, y / 2, entering);
#endif

	// draw level
	y += (5 * lnames[wbs->next]->height) / 4;

#if defined(MODE_T4025) || defined(MODE_T4050)
	V_WriteTextDirect((320 - lnames[wbs->next]->width) / 16, y / 8, titlenext);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	V_WriteTextDirect((320 - lnames[wbs->next]->width) / 8, y / 8, titlenext);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	V_WriteTextDirect((320 - lnames[wbs->next]->width) / 8, y / 4, titlenext);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
  	V_DrawPatchModeCentered((320 - lnames[wbs->next]->width) / 2, y, lnames[wbs->next]);
#endif
#if defined(MODE_Y_HALF)
  	V_DrawPatchModeCentered((320 - lnames[wbs->next]->width) / 2, y / 2, lnames[wbs->next]);
#endif
}

#if defined(MODE_X) || defined(MODE_Y) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void WI_drawOnLnode(int n, patch_t *c[])
{

	int i;
	int left;
	int top;
	int right;
	int bottom;
	byte fits = 0;

	i = 0;
	do
	{
		left = lnodes[wbs->epsd][n].x - c[i]->leftoffset;
		top = lnodes[wbs->epsd][n].y - c[i]->topoffset;
		right = left + c[i]->width;
		bottom = top + c[i]->height;

		if (left >= 0 && right < 320 && top >= 0 && bottom < SCALED_SCREENHEIGHT)
		{
			fits = 1;
		}
		else
		{
			i++;
		}
	} while (!fits && i != 2);

	if (fits && i < 2)
	{
		V_DrawPatchModeCentered(lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y, c[i]);
	}
}
#endif

#if defined(MODE_Y_HALF)
void WI_drawOnLnode(int n, patch_t *c[])
{

	int i;
	int left;
	int top;
	int right;
	int bottom;
	byte fits = 0;

	i = 0;
	do
	{
		left = lnodes[wbs->epsd][n].x - c[i]->leftoffset;
		top = (lnodes[wbs->epsd][n].y - c[i]->topoffset) / 2;
		right = left + c[i]->width;
		bottom = top + (c[i]->height / 2);

		if (left >= 0 && right < 320 && top >= 0 && bottom < SCALED_SCREENHEIGHT)
		{
			fits = 1;
		}
		else
		{
			i++;
		}
	} while (!fits && i != 2);

	if (fits && i < 2)
	{
		V_DrawPatchModeCentered(lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y / 2, c[i]);
	}
}
#endif

void WI_initAnimatedBack(void)
{
	int i;
	anim_t *a;

	if (gamemode == commercial)
		return;

	if (wbs->epsd > 2)
		return;

	for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
	{
		a = &anims[wbs->epsd][i];

		// init variables
		a->ctr = -1;

		// specify the next time to draw it
		if (a->type == ANIM_ALWAYS)
			a->nexttic = bcnt + 1 + (M_Random % a->period);
		else if (a->type == ANIM_LEVEL)
			a->nexttic = bcnt + 1;
	}
}

void WI_updateAnimatedBack(void)
{
	int i;
	anim_t *a;

	if (gamemode == commercial)
		return;

	if (wbs->epsd > 2)
		return;

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

			case ANIM_LEVEL:
				// gawd-awful hack for level anims
				if (!(state == StatCount && i == 7) && wbs->next == a->data1)
				{
					a->ctr++;
					a->ctr -= a->ctr == a->nanims;
					a->nexttic = bcnt + a->period;
				}
				break;
			}
		}
	}
}

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void WI_drawAnimatedBack(void)
{
	int i;
	anim_t *a;

	if (gamemode == commercial)
		return;

	if (wbs->epsd > 2)
		return;

	for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
	{
		a = &anims[wbs->epsd][i];

		if (a->ctr >= 0)
		{
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
      V_DrawPatchModeCentered(a->loc.x, a->loc.y, a->p[a->ctr]);
#endif
#if defined(MODE_Y_HALF)
      V_DrawPatchModeCentered(a->loc.x, a->loc.y / 2, a->p[a->ctr]);
#endif
		}
	}
}
#endif

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//

int WI_drawNumTwoDigits(int x, int y, int n)
{

#if defined(MODE_T4025) || defined(MODE_T4050)
	int fontwidth = num[0]->width;
	char strnum[4];

	sprintf(strnum, "%i", n);
	V_WriteTextDirect(x / 8, y / 8, strnum);

	x -= 4 * fontwidth;
	return x;
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	int fontwidth = num[0]->width;
	char strnum[4];

	sprintf(strnum, "%i", n);
	V_WriteTextDirect(x / 4, y / 8, strnum);

	x -= 2 * fontwidth;
	return x;
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	int fontwidth = num[0]->width;
	char strnum[4];

	sprintf(strnum, "%i", n);
	V_WriteTextDirect(x / 4, y / 4, strnum);

	x -= 2 * fontwidth;
	return x;
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
	int original;
	int fontwidth = num[0]->width;

	// if non-number, do not draw it
	if (n == 1994)
		return 0;

	original = n;
	n = Div10(n);
	x -= fontwidth;
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
	V_DrawPatchModeCentered(x, y, num[original - Mul10(n)]);
#endif
#if defined(MODE_Y_HALF)
	V_DrawPatchModeCentered(x, y / 2, num[original - Mul10(n)]);
#endif

	original = n;
	n = Div10(n);
	x -= fontwidth;
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
	V_DrawPatchModeCentered(x, y, num[original - Mul10(n)]);
#endif
#if defined(MODE_Y_HALF)
	V_DrawPatchModeCentered(x, y / 2, num[original - Mul10(n)]);
#endif

	return x;
#endif
}

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
int WI_drawNum(int x, int y, int n)
{

	int fontwidth = num[0]->width;

	// if non-number, do not draw it
	if (n == 1994)
		return 0;

	// draw the new number
	do
	{
		int original = n;

		n = Div10(n);
		x -= fontwidth;
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
		V_DrawPatchModeCentered(x, y, num[original - Mul10(n)]);
#endif
#if defined(MODE_Y_HALF)
		V_DrawPatchModeCentered(x, y / 2, num[original - Mul10(n)]);
#endif
	} while (n);

	return x;
}
#endif

void WI_drawPercent(int x, int y, int p)
{
#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA)
	char strnum[4];
#endif

	if (p < 0)
		return;

#if defined(MODE_T4025) || defined(MODE_T4050)
	sprintf(strnum, "%i%%", p);
	V_WriteTextDirect(x / 4, y / 8 - 1, strnum);
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	sprintf(strnum, "%i%%", p);
	V_WriteTextDirect(x / 2, y / 8 - 1, strnum);
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	sprintf(strnum, "%i%%", p);
	V_WriteTextDirect(x / 2, y / 4 - 1, strnum);
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
  	V_DrawPatchModeCentered(x, y, percent);
	WI_drawNum(x, y, p);
#endif
#if defined(MODE_Y_HALF)
  	V_DrawPatchModeCentered(x, y / 2, percent);
	WI_drawNum(x, y, p);
#endif
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
			// VITI95: OPTIMIZE
			n = (t / div) % 60;
			x = WI_drawNumTwoDigits(x, y, n) - colon->width;
			div *= 60;

			// draw
			if (div == 60 || t / div)
			{
#if defined(MODE_T4025) || defined(MODE_T4050)
				V_WriteTextDirect(x / 8, y / 8, ":");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
				V_WriteTextDirect(x / 4, y / 8, ":");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
				V_WriteTextDirect(x / 4, y / 4, ":");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
				V_DrawPatchModeCentered(x, y, colon);
#endif
#if defined(MODE_Y_HALF)
				V_DrawPatchModeCentered(x, y / 2, colon);
#endif
			}
		} while (t / div);
	}
	else
	{
		// "sucks"
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
		V_DrawPatchModeCentered(x - sucks->width, y, sucks);
#endif
#if defined(MODE_Y_HALF)
		V_DrawPatchModeCentered(x - sucks->width, y / 2, sucks);
#endif
	}
}

void WI_initNoState(void)
{
	state = NoState;
	acceleratestage = 0;
	cnt = 10;
}

void WI_unloadData(void)
{
	int i;
	int j;

	for (i = 0; i < 10; i++)
		Z_ChangeTag(num[i], PU_CACHE);

	if (gamemode == commercial)
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

		if (wbs->epsd < 3)
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
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
	Z_Free(screen1);
#endif

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
}

void WI_updateNoState(void)
{

	WI_updateAnimatedBack();

	if (!--cnt)
	{
		WI_unloadData();
		G_WorldDone();
	}
}

static byte snl_pointeron = 0;

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

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
void WI_drawShowNextLoc(void)
{

	int i;
	int last;

	WI_slamBackground();

	// draw animated background
	WI_drawAnimatedBack();

	if (gamemode != commercial)
	{
		if (wbs->epsd > 2)
		{
			WI_drawEL();
			return;
		}

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
	if ((gamemode != commercial) || wbs->next != 30)
		WI_drawEL();
}
#endif

void WI_drawNoState(void)
{
	snl_pointeron = 1;
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
	WI_drawShowNextLoc();
#endif
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

		// VITI95: OPTIMIZE
		cnt_kills = Mul100(plrs.skills) / wbs->maxkills;
		cnt_items = Mul100(plrs.sitems) / wbs->maxitems;
		cnt_secret = Mul100(plrs.ssecret) / wbs->maxsecret;
		cnt_time = Div35(plrs.stime);
		cnt_par = Div35(wbs->partime);
		S_StartSound(0, sfx_barexp);
		sp_state = 10;
	}

	if (sp_state == 2)
	{
		int optSkills;

		cnt_kills += 2;

		if (!(bcnt & 3))
			S_StartSound(0, sfx_pistol);

		optSkills = Mul100(plrs.skills) / wbs->maxkills;

		if (cnt_kills >= optSkills)
		{
			cnt_kills = optSkills;
			S_StartSound(0, sfx_barexp);
			sp_state++;
		}
	}
	else if (sp_state == 4)
	{
		int optSitems;

		cnt_items += 2;

		if (!(bcnt & 3))
			S_StartSound(0, sfx_pistol);

		optSitems = Mul100(plrs.sitems) / wbs->maxitems;

		if (cnt_items >= optSitems)
		{
			cnt_items = optSitems;
			S_StartSound(0, sfx_barexp);
			sp_state++;
		}
	}
	else if (sp_state == 6)
	{
		int optSsecret;

		cnt_secret += 2;

		if (!(bcnt & 3))
			S_StartSound(0, sfx_pistol);

		optSsecret = Mul100(plrs.ssecret) / wbs->maxsecret;

		if (cnt_secret >= optSsecret)
		{
			cnt_secret = optSsecret;
			S_StartSound(0, sfx_barexp);
			sp_state++;
		}
	}

	else if (sp_state == 8)
	{
		int optStime, optPartime;

		if (!(bcnt & 3))
			S_StartSound(0, sfx_pistol);

		cnt_time += 3;

		optStime = Div35(plrs.stime);
		if (cnt_time >= optStime)
			cnt_time = optStime;

		cnt_par += 3;

		optPartime = Div35(wbs->partime);
		if (cnt_par >= optPartime)
		{
			cnt_par = optPartime;

			if (cnt_time >= optStime)
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

			if (gamemode == commercial)
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

	lh = (3 * num[0]->height) / 2;

	WI_slamBackground();

	// draw animated background
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
	WI_drawAnimatedBack();
#endif

	WI_drawLF();

#if defined(MODE_T4025) || defined(MODE_T4050)
	V_WriteTextDirect(SP_STATSX / 4, SP_STATSY / 8, "KILLS:");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	V_WriteTextDirect(SP_STATSX / 2, SP_STATSY / 8, "KILLS:");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	V_WriteTextDirect(SP_STATSX / 2, SP_STATSY / 4, "KILLS:");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
	V_DrawPatchModeCentered(SP_STATSX, SP_STATSY, kills);
#endif
#if defined(MODE_Y_HALF)
	V_DrawPatchModeCentered(SP_STATSX, SP_STATSY / 2, kills);
#endif

	WI_drawPercent(320 - SP_STATSX, SP_STATSY, cnt_kills);

#if defined(MODE_T4025) || defined(MODE_T4050)
	V_WriteTextDirect(SP_STATSX / 4, (SP_STATSY + lh) / 8, "ITEMS:");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	V_WriteTextDirect(SP_STATSX / 2, (SP_STATSY + lh) / 8, "ITEMS:");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	V_WriteTextDirect(SP_STATSX / 2, (SP_STATSY + lh) / 4, "ITEMS:");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
	V_DrawPatchModeCentered(SP_STATSX, SP_STATSY + lh, items);
#endif
#if defined(MODE_Y_HALF)
	V_DrawPatchModeCentered(SP_STATSX, (SP_STATSY + lh) / 2, items);
#endif

	WI_drawPercent(320 - SP_STATSX, SP_STATSY + lh, cnt_items);

#if defined(MODE_T4025) || defined(MODE_T4050)
	V_WriteTextDirect(SP_STATSX / 4, (SP_STATSY + 2 * lh) / 8, "SECRET:");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	V_WriteTextDirect(SP_STATSX / 2, (SP_STATSY + 2 * lh) / 8, "SECRET:");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	V_WriteTextDirect(SP_STATSX / 2, (SP_STATSY + 2 * lh) / 4, "SECRET:");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
	V_DrawPatchModeCentered(SP_STATSX, SP_STATSY + 2 * lh, sp_secret);
#endif
#if defined(MODE_Y_HALF)
	V_DrawPatchModeCentered(SP_STATSX, (SP_STATSY + 2 * lh) / 2, sp_secret);
#endif
	WI_drawPercent(320 - SP_STATSX, SP_STATSY + 2 * lh, cnt_secret);

#if defined(MODE_T4025) || defined(MODE_T4050)
	V_WriteTextDirect(SP_TIMEX / 4, SP_TIMEY / 8, "TIME:");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
	V_WriteTextDirect(SP_TIMEX / 2, SP_TIMEY / 8, "TIME:");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
	V_WriteTextDirect(SP_TIMEX / 2, SP_TIMEY / 4, "TIME:");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
	V_DrawPatchModeCentered(SP_TIMEX, SP_TIMEY, time);
#endif
#if defined(MODE_Y_HALF)
	V_DrawPatchModeCentered(SP_TIMEX, SP_TIMEY / 2, time);
#endif
	WI_drawTime(160 - SP_TIMEX, SP_TIMEY, cnt_time);

	if (wbs->epsd < 3)
	{
#if defined(MODE_T4025) || defined(MODE_T4050)
		V_WriteTextDirect((160 + SP_TIMEX) / 8, SP_TIMEY / 8, "PAR:");
#endif
#if defined(MODE_T8025) || defined(MODE_MDA)
		V_WriteTextDirect((160 + SP_TIMEX) / 4, SP_TIMEY / 8, "PAR:");
#endif
#if defined(MODE_T8050) || defined(MODE_T8043)
		V_WriteTextDirect((160 + SP_TIMEX) / 4, SP_TIMEY / 4, "PAR:");
#endif
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_VBE2_DIRECT) || defined(USE_BACKBUFFER)
   		V_DrawPatchModeCentered(160 + SP_TIMEX, SP_TIMEY, par);
#endif
#if defined(MODE_Y_HALF)
   		V_DrawPatchModeCentered(160 + SP_TIMEX, SP_TIMEY / 2, par);
#endif
		WI_drawTime(320 - SP_TIMEX, SP_TIMEY, cnt_par);
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
		if (gamemode == commercial)
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
	anim_t *a;
	char name[9];

	if (gamemode == commercial)
	{
		strcpy(name, "INTERPIC");
	}
	else
	{
		sprintf(name, "WIMAP%d", wbs->epsd);
	}

	if (gamemode == retail)
	{
		if (wbs->epsd == 3)
		{
			strcpy(name, "INTERPIC");
		}
	}

#if defined(MODE_T8025) || defined(MODE_T8050) || defined(MODE_T8043) || defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_MDA)
	strcpy(bgname, name);
#endif

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
	screen1 = (byte *)Z_MallocUnowned(SCREENWIDTH * SCREENHEIGHT, PU_STATIC);

	// background
	bg = W_CacheLumpName(name, PU_CACHE);
#if CENTERING_OFFSET_X != 0 || CENTERING_OFFSET_Y != 0
  // Fill screen with black
  SetDWords(screen1, 0, SCREENWIDTH * SCREENHEIGHT / 4);
#endif
	V_DrawPatch(CENTERING_OFFSET_X, CENTERING_OFFSET_Y, screen1, bg);
#endif

	if (gamemode == commercial)
	{
		lnames = (patch_t **)Z_MallocUnowned(sizeof(patch_t *) * NUMCMAPS, PU_STATIC);
		for (i = 0; i < NUMCMAPS; i++)
		{
			sprintf(name, "CWILV%2.2d", i);
			lnames[i] = W_CacheLumpName(name, PU_STATIC);
		}
	}
	else
	{
		lnames = (patch_t **)Z_MallocUnowned(sizeof(patch_t *) * NUMMAPS, PU_STATIC);
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

		if (wbs->epsd < 3)
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
}

void WI_Drawer(void)
{
	switch (state)
	{
	case StatCount:
		WI_drawStats();
		break;
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(USE_BACKBUFFER) || defined(MODE_VBE2_DIRECT)
	case ShowNextLoc:
		WI_drawShowNextLoc();
		break;
#endif

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
