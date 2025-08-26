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
// DESCRIPTION:  the automap code
//

#include <stdlib.h>
#include <stdio.h>

#include "z_zone.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"

#include "dutils.h"
#include "i_system.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "dstrings.h"

#include "am_map.h"

#include "doomstat.h"

#include "options.h"

#include "m_menu.h"

#if !defined(TEXT_MODE) && !defined(MODE_MDA)

// For use if I do walls with outsides/insides
#define REDS (256 - 5 * 16)
#define REDRANGE 16
#define BLUES (256 - 4 * 16 + 8)
#define BLUERANGE 8
#define GREENS (7 * 16)
#define GREENRANGE 16
#define GRAYS (6 * 16)
#define GRAYSRANGE 16
#define BROWNS (4 * 16)
#define BROWNRANGE 16
#define YELLOWS (256 - 32 + 7)
#define YELLOWRANGE 1
#define BLACK 0
#define WHITE (256 - 47)
#define TRANSPARENTPIXEL 251
#define TRANSPARENTPIXELDWORD 0xFBFBFBFB

// Automap colors
#define BACKGROUND BLACK
#define YOURCOLORS WHITE
#define YOURRANGE 0
#define WALLCOLORS REDS
#define WALLRANGE REDRANGE
#define TSWALLCOLORS GRAYS
#define TSWALLRANGE GRAYSRANGE
#define FDWALLCOLORS BROWNS
#define FDWALLRANGE BROWNRANGE
#define CDWALLCOLORS YELLOWS
#define CDWALLRANGE YELLOWRANGE
#define THINGCOLORS GREENS
#define THINGRANGE GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE WALLRANGE
#define GRIDCOLORS (GRAYS + GRAYSRANGE / 2)
#define GRIDRANGE 0
#define XHAIRCOLORS GRAYS

// drawing stuff
#define FB 0

#define AM_PANDOWNKEY KEY_DOWNARROW
#define AM_PANUPKEY KEY_UPARROW
#define AM_PANRIGHTKEY KEY_RIGHTARROW
#define AM_PANLEFTKEY KEY_LEFTARROW
#define AM_ZOOMINKEY '='
#define AM_ZOOMOUTKEY '-'
#define AM_STARTKEY KEY_TAB
#define AM_ENDKEY KEY_TAB
#define AM_GOBIGKEY '0'
#define AM_FOLLOWKEY 'f'
#define AM_MARKKEY 'm'
#define AM_CLEARMARKKEY 'c'
#define AM_TRANSPARENTMAP 't'

#define AM_NUMMARKPOINTS 10

// scale on entry
#define INITSCALEMTOF (.2 * FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC 4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN ((int)(1.02 * FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT ((int)(FRACUNIT / 1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x) << 16), scale_ftom)
#define MTOF(x) (FixedMul((x), scale_mtof) >> 16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x) (MTOF((x)-m_x))
#define CYMTOF(y) ((automapheight - MTOF((y)-m_y)))

typedef struct
{
	int x, y;
} fpoint_t;

typedef struct
{
	fpoint_t a, b;
} fline_t;

typedef struct
{
	fixed_t x, y;
} mpoint_t;

typedef struct
{
	mpoint_t a, b;
} mline_t;

typedef struct
{
	fixed_t slp, islp;
} islope_t;

//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8 * PLAYERRADIUS) / 7)
const mline_t player_arrow[] = {
	{{-R + R / 8, 0}, {R, 0}},	  // -----
	{{R, 0}, {R - R / 2, R / 4}}, // ----->
	{{R, 0}, {R - R / 2, -R / 4}},
	{{-R + R / 8, 0}, {-R - R / 8, R / 4}}, // >---->
	{{-R + R / 8, 0}, {-R - R / 8, -R / 4}},
	{{-R + 3 * R / 8, 0}, {-R + R / 8, R / 4}}, // >>--->
	{{-R + 3 * R / 8, 0}, {-R + R / 8, -R / 4}}};
#undef R
#define NUMPLYRLINES (sizeof(player_arrow) / sizeof(mline_t))

#define R (FRACUNIT)
const mline_t thintriangle_guy[] = {
	{{-.5 * R, -.7 * R}, {R, 0}},
	{{R, 0}, {-.5 * R, .7 * R}},
	{{-.5 * R, .7 * R}, {-.5 * R, -.7 * R}}};
#undef R
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy) / sizeof(mline_t))

static int cheating = 0;
static int grid = 0;
int transparentmap = 0;

byte automapactive = 0;

static mpoint_t m_paninc;	 // how far the window pans each tic (map coords)
static fixed_t mtof_zoommul; // how far the window zooms in each tic (map coords)
static fixed_t ftom_zoommul; // how far the window zooms in each tic (fb coords)

static fixed_t m_x, m_y;   // LL x,y where the window is on the map (map coords)
static fixed_t m_x2, m_y2; // UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static fixed_t m_w;
static fixed_t m_h;

// based on level size
static fixed_t min_x;
static fixed_t min_y;
static fixed_t max_x;
static fixed_t max_y;

static fixed_t max_w; // max_x-min_x,
static fixed_t max_h; // max_y-min_y

// based on player size

static fixed_t min_scale_mtof; // used to tell when to stop zooming out
#define MAXSCALEMTOF 344064	   // used to tell when to stop zooming in

// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = INITSCALEMTOF;
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static byte followplayer = 1; // specifies whether to follow the player around

static unsigned char cheat_amap_seq[] = {'i', 'd', 'd', 't', 0xff};
static cheatseq_t cheat_amap = {cheat_amap_seq, 0};

static byte stopped = true;

extern byte viewactive;

#ifdef SUPPORTS_HERCULES_AUTOMAP
byte *automapbuffer1;
byte *automapbuffer2;
byte *automapbuffer;
#endif

//
//
//
void AM_activateNewScale(void)
{
	m_x += m_w / 2;
	m_y += m_h / 2;
	m_w = FTOM(SCREENWIDTH);
	m_h = FTOM(automapheight);
	m_x -= m_w / 2;
	m_y -= m_h / 2;
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}

//
//
//
void AM_saveScaleAndLoc(void)
{
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;
}

//
//
//
void AM_restoreScaleAndLoc(void)
{

	m_w = old_m_w;
	m_h = old_m_h;
	if (!followplayer)
	{
		m_x = old_m_x;
		m_y = old_m_y;
	}
	else
	{
		m_x = players_mo->x - m_w / 2;
		m_y = players_mo->y - m_h / 2;
	}
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;

	// Change the scaling multipliers
	scale_mtof = FixedDiv(SCREENWIDTH << FRACBITS, m_w);
	scale_ftom = (4 >= scale_mtof) ? (65536 ^ scale_mtof >> 31) ^ MAXINT : FixedDiv65536(scale_mtof);
}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries(void)
{
	int i;
	fixed_t a;
	fixed_t b;

	min_x = min_y = MAXINT;
	max_x = max_y = -MAXINT;

	for (i = 0; i < numvertexes; i++)
	{
		if (min_x > vertexes[i].x)
			min_x = vertexes[i].x;
		else if (max_x < vertexes[i].x)
			max_x = vertexes[i].x;

		if (min_y > vertexes[i].y)
			min_y = vertexes[i].y;
		else if (max_y < vertexes[i].y)
			max_y = vertexes[i].y;
	}

	max_w = max_x - min_x;
	max_h = max_y - min_y;

	a = FixedDiv(SCREENWIDTH << FRACBITS, max_w);
	b = FixedDiv((automapheight) << FRACBITS, max_h);

	min_scale_mtof = a < b ? a : b;
}

//
//
//
void AM_changeWindowLoc(void)
{
	if (m_paninc.x || m_paninc.y)
	{
		followplayer = 0;
		f_oldloc.x = MAXINT;
	}

	m_x += m_paninc.x;
	m_y += m_paninc.y;

	if (m_x > max_x - m_w / 2)
		m_x = max_x - m_w / 2;
	else if (m_x < min_x - m_w / 2)
		m_x = min_x - m_w / 2;

	if (m_y > max_y - m_h / 2)
		m_y = max_y - m_h / 2;
	else if (m_y < min_y - m_h / 2)
		m_y = min_y - m_h / 2;

	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}

//
//
//
void AM_initVariables(void)
{
	static event_t st_notify = {ev_keyup, AM_MSGENTERED};

	automapactive = 1;

	f_oldloc.x = MAXINT;

	m_paninc.x = m_paninc.y = 0;
	ftom_zoommul = FRACUNIT;
	mtof_zoommul = FRACUNIT;

	m_w = FTOM(SCREENWIDTH);
	m_h = FTOM(automapheight);

	m_x = players_mo->x - m_w / 2;
	m_y = players_mo->y - m_h / 2;
	AM_changeWindowLoc();

	// for saving & restoring
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;

	// inform the status bar of the change
	ST_Responder(&st_notify);
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit(void)
{
	AM_findMinMaxBoundaries();
	scale_mtof = FixedDiv(min_scale_mtof, (int)(0.7 * FRACUNIT));
	if (scale_mtof > MAXSCALEMTOF)
		scale_mtof = min_scale_mtof;
	scale_ftom = (4 >= scale_mtof) ? (65536 ^ scale_mtof >> 31) ^ MAXINT : FixedDiv65536(scale_mtof);
}

//
//
//
void AM_Stop(void)
{
	static event_t st_notify = {0, ev_keyup, AM_MSGEXITED};

	automapactive = 0;
	ST_Responder(&st_notify);
	stopped = 1;

#ifdef SUPPORTS_HERCULES_AUTOMAP
	if (HERCmap)
	{
		Z_Free(automapbuffer1);
		Z_Free(automapbuffer2);
	}
#endif
}

//
//
//
void AM_Start(void)
{
	static int lastlevel = -1, lastepisode = -1;

#ifdef SUPPORTS_HERCULES_AUTOMAP
	if (HERCmap)
	{
		// Init buffers
		automapbuffer1 = (byte *)Z_MallocUnowned(32768, PU_STATIC);
		automapbuffer2 = (byte *)Z_MallocUnowned(32768, PU_STATIC);
		SetDWords(automapbuffer1, 0, 8192);
		SetDWords(automapbuffer2, 0, 8192);
		automapbuffer = automapbuffer1;

		// Clear VRAM
		SetDWords((byte *)0xB0000, 0, 8192);
	}
#endif

	if (screenSize < 7)
		transparentmap = 0;

	if (!stopped)
		AM_Stop();
	stopped = 0;
	if (lastlevel != gamemap || lastepisode != gameepisode)
	{
		AM_LevelInit();
		lastlevel = gamemap;
		lastepisode = gameepisode;
	}
	AM_initVariables();
}

//
// set the window scale to the maximum size
//
void AM_minOutWindowScale(void)
{
	scale_mtof = min_scale_mtof;
	scale_ftom = (4 >= scale_mtof) ? (65536 ^ scale_mtof >> 31) ^ MAXINT : FixedDiv65536(scale_mtof);
	
	AM_activateNewScale();
}

//
// set the window scale to the minimum size
//
void AM_maxOutWindowScale(void)
{
	scale_mtof = MAXSCALEMTOF;
	// TODO: Optimize, this is an static value
	scale_ftom = (4 >= MAXSCALEMTOF) ? (65536 ^ MAXSCALEMTOF >> 31) ^ MAXINT : FixedDiv65536(MAXSCALEMTOF);

	AM_activateNewScale();
}

//
// Handle events (user inputs) in automap mode
//
byte AM_Responder(void)
{

	byte rc = 0;
	static byte bigstate = 0;

	if (!automapactive)
	{
		if (current_ev->type == ev_keydown && current_ev->data1 == AM_STARTKEY)
		{
			AM_Start();

#ifdef SUPPORTS_HERCULES_AUTOMAP
			viewactive = HERCmap;
#else
			viewactive = 0;
#endif

			rc = 1;
		}
	}
	else if (current_ev->type == ev_keydown)
	{

		rc = 1;
		switch (current_ev->data1)
		{
		case AM_PANRIGHTKEY: // pan right
			if (!followplayer)
				m_paninc.x = FTOM(F_PANINC);
			else
				rc = 0;
			break;
		case AM_PANLEFTKEY: // pan left
			if (!followplayer)
				m_paninc.x = -FTOM(F_PANINC);
			else
				rc = 0;
			break;
		case AM_PANUPKEY: // pan up
			if (!followplayer)
				m_paninc.y = FTOM(F_PANINC);
			else
				rc = 0;
			break;
		case AM_PANDOWNKEY: // pan down
			if (!followplayer)
				m_paninc.y = -FTOM(F_PANINC);
			else
				rc = 0;
			break;
		case AM_ZOOMOUTKEY: // zoom out
			mtof_zoommul = M_ZOOMOUT;
			ftom_zoommul = M_ZOOMIN;
			break;
		case AM_ZOOMINKEY: // zoom in
			mtof_zoommul = M_ZOOMIN;
			ftom_zoommul = M_ZOOMOUT;
			break;
		case AM_ENDKEY:
			bigstate = 0;
			viewactive = 1;
			AM_Stop();
			break;
		case AM_GOBIGKEY:
			bigstate = !bigstate;
			if (bigstate)
			{
				AM_saveScaleAndLoc();
				AM_minOutWindowScale();
			}
			else
				AM_restoreScaleAndLoc();
			break;
		case AM_FOLLOWKEY:
			followplayer = !followplayer;
			f_oldloc.x = MAXINT;
			players.message = followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF;
			break;
		case AM_TRANSPARENTMAP:
			if (automapRT && (screenSize >= 7)) 
			{
				transparentmap = !transparentmap;
				players.message = transparentmap ? AMSTR_TRANSON : AMSTR_TRANSOFF;
			} else {
				players.message = "TRANSPARENT AUTOMAP NOT AVAILABLE";
			}
			break;
		default:
			rc = 0;
		}
		if (gameskill != sk_nightmare && cht_CheckCheat(&cheat_amap))
		{
			rc = 0;

			cheating++;
			if (cheating == 3)
				cheating = 0;
		}
	}
	else if (current_ev->type == ev_keyup)
	{
		rc = 0;
		switch (current_ev->data1)
		{
		case AM_PANRIGHTKEY:
			if (!followplayer)
				m_paninc.x = 0;
			break;
		case AM_PANLEFTKEY:
			if (!followplayer)
				m_paninc.x = 0;
			break;
		case AM_PANUPKEY:
			if (!followplayer)
				m_paninc.y = 0;
			break;
		case AM_PANDOWNKEY:
			if (!followplayer)
				m_paninc.y = 0;
			break;
		case AM_ZOOMOUTKEY:
		case AM_ZOOMINKEY:
			mtof_zoommul = FRACUNIT;
			ftom_zoommul = FRACUNIT;
			break;
		}
	}

	return rc;
}

//
// Zooming
//
void AM_changeWindowScale(void)
{

	// Change the scaling multipliers
	scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
	scale_ftom = (4 >= scale_mtof) ? (65536 ^ scale_mtof >> 31) ^ MAXINT : FixedDiv65536(scale_mtof);

	if (scale_mtof < min_scale_mtof)
		AM_minOutWindowScale();
	else if (scale_mtof > MAXSCALEMTOF)
		AM_maxOutWindowScale();
	else
		AM_activateNewScale();
}

//
//
//
void AM_doFollowPlayer(void)
{
	if (f_oldloc.x != players_mo->x || f_oldloc.y != players_mo->y)
	{
    	// [JN] Prevent player arrow from jittering 
    	// by not using FTOM->MTOF conversion.
		m_x = players_mo->x - m_w / 2;
		m_y = players_mo->y - m_h / 2;
		m_x2 = m_x + m_w;
		m_y2 = m_y + m_h;
		f_oldloc.x = players_mo->x;
		f_oldloc.y = players_mo->y;
	}
}

//
// Updates on Game Tick
//
void AM_Ticker(void)
{

	if (!automapactive)
		return;

	if (followplayer)
		AM_doFollowPlayer();

	// Change the zoom if necessary
	if (ftom_zoommul != FRACUNIT)
		AM_changeWindowScale();

	// Change x,y location
	if (m_paninc.x || m_paninc.y)
		AM_changeWindowLoc();
}

//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
byte AM_clipMline(mline_t *ml, fline_t *fl)
{
	enum
	{
		LEFT = 1,
		RIGHT = 2,
		BOTTOM = 4,
		TOP = 8
	};

	register outcode1 = 0;
	register outcode2 = 0;
	register outside;

	fpoint_t tmp;
	int dx;
	int dy;

#define DOOUTCODE(oc, mx, my)         \
	(oc) = 0;                         \
	if ((my) < 0)                     \
		(oc) |= TOP;                  \
	else if ((my) >= (automapheight)) \
		(oc) |= BOTTOM;               \
	if ((mx) < 0)                     \
		(oc) |= LEFT;                 \
	else if ((mx) >= SCREENWIDTH)     \
		(oc) |= RIGHT;

	// do trivial rejects and outcodes
	if (ml->a.y > m_y2)
		outcode1 = TOP;
	else if (ml->a.y < m_y)
		outcode1 = BOTTOM;

	if (ml->b.y > m_y2)
		outcode2 = TOP;
	else if (ml->b.y < m_y)
		outcode2 = BOTTOM;

	if (outcode1 & outcode2)
		return 0; // trivially outside

	if (ml->a.x < m_x)
		outcode1 |= LEFT;
	else if (ml->a.x > m_x2)
		outcode1 |= RIGHT;

	if (ml->b.x < m_x)
		outcode2 |= LEFT;
	else if (ml->b.x > m_x2)
		outcode2 |= RIGHT;

	if (outcode1 & outcode2)
		return 0; // trivially outside

	// transform to frame-buffer coordinates.
	fl->a.x = CXMTOF(ml->a.x);
	fl->a.y = CYMTOF(ml->a.y);
	fl->b.x = CXMTOF(ml->b.x);
	fl->b.y = CYMTOF(ml->b.y);

	DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	DOOUTCODE(outcode2, fl->b.x, fl->b.y);

	if (outcode1 & outcode2)
		return 0;

	while (outcode1 | outcode2)
	{
		// may be partially inside box
		// find an outside point
		if (outcode1)
			outside = outcode1;
		else
			outside = outcode2;

		// clip to each side
		if (outside & TOP)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx * (fl->a.y)) / dy;
			tmp.y = 0;
		}
		else if (outside & BOTTOM)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx * (fl->a.y - (automapheight))) / dy;
			tmp.y = automapheight - 1;
		}
		else if (outside & RIGHT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy * (SCREENWIDTH - 1 - fl->a.x)) / dx;
			tmp.x = SCREENWIDTH - 1;
		}
		else if (outside & LEFT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy * (-fl->a.x)) / dx;
			tmp.x = 0;
		}

		if (outside == outcode1)
		{
			fl->a = tmp;
			DOOUTCODE(outcode1, fl->a.x, fl->a.y);
		}
		else
		{
			fl->b = tmp;
			DOOUTCODE(outcode2, fl->b.x, fl->b.y);
		}

		if (outcode1 & outcode2)
			return 0; // trivially outside
	}

	return 1;
}
#undef DOOUTCODE

//
// Classic Bresenham w/ whatever optimizations needed for speed
//
#ifdef SUPPORTS_HERCULES_AUTOMAP
void AM_drawFlineHercules(fline_t *fl)
{
	register int x;
	register int y;
	register int dx;
	register int dy;
	register int sx;
	register int sy;
	register int ax;
	register int ay;
	register int d;

#define PUTDOTH(xx, yy) automapbuffer[(0x2000 * ((yy) % 4)) + (80 * ((yy) / 4)) + ((xx) / 8)] = automapbuffer[(0x2000 * ((yy) % 4)) + (80 * ((yy) / 4)) + ((xx) / 8)] | (0x80 >> ((xx) % 8))
#define PUTDOT2H(xx, yy) automapbuffer[(0x2000 * ((yy) % 4)) + (80 * ((yy) / 4)) + ((xx) / 8)] = automapbuffer[(0x2000 * ((yy) % 4)) + (80 * ((yy) / 4)) + ((xx) / 8)] | (0xC0 >> ((xx) % 8))

	dx = fl->b.x - fl->a.x;

	if (dx < 0)
	{
		ax = 2 * -dx;
		sx = -1;
	}
	else
	{
		ax = 2 * dx;
		sx = 1;
	}

	dy = fl->b.y - fl->a.y;

	if (dy < 0)
	{
		ay = 2 * -dy;
		sy = -1;
	}
	else
	{
		ay = 2 * dy;
		sy = 1;
	}

	x = fl->a.x;
	y = fl->a.y;

	if (ax > ay)
	{
		d = ay - ax / 2;
		while (1)
		{
			if ((7 - ((x) % 8)) > 0)
			{
				// We can optimize write 2 pixels on the same scanline
				PUTDOT2H((2 * x), (2 * y));
				PUTDOT2H((2 * x), (2 * y) + 1);
			}
			else
			{
				PUTDOTH((2 * x), (2 * y));
				PUTDOTH((2 * x) + 1, (2 * y));
				PUTDOTH((2 * x), (2 * y) + 1);
				PUTDOTH((2 * x) + 1, (2 * y) + 1);
			}

			if (x == fl->b.x)
				return;
			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - ay / 2;
		while (1)
		{
			if ((7 - ((x) % 8)) > 0)
			{
				// We can optimize write 2 pixels on the same scanline
				PUTDOT2H((2 * x), (2 * y));
				PUTDOT2H((2 * x), (2 * y) + 1);
			}
			else
			{
				PUTDOTH((2 * x), (2 * y));
				PUTDOTH((2 * x) + 1, (2 * y));
				PUTDOTH((2 * x), (2 * y) + 1);
				PUTDOTH((2 * x) + 1, (2 * y) + 1);
			}

			if (y == fl->b.y)
				return;
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}
#endif

void AM_drawFline(fline_t *fl, int color)
{
	register int x;
	register int y;
	register int dx;
	register int dy;
	register int sx;
	register int sy;
	register int ax;
	register int ay;
	register int d;

#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
#define PUTDOT(xx, yy, cc) screen0[MulScreenWidth(yy) + (xx)] = (cc)
#endif

#if defined(USE_BACKBUFFER)
#define PUTDOT(xx, yy, cc) backbuffer[MulScreenWidth(yy) + (xx)] = (cc)
#endif

	dx = fl->b.x - fl->a.x;

	if (dx < 0)
	{
		ax = 2 * -dx;
		sx = -1;
	}
	else
	{
		ax = 2 * dx;
		sx = 1;
	}

	dy = fl->b.y - fl->a.y;

	if (dy < 0)
	{
		ay = 2 * -dy;
		sy = -1;
	}
	else
	{
		ay = 2 * dy;
		sy = 1;
	}

	x = fl->a.x;
	y = fl->a.y;

	if (ax > ay)
	{
		d = ay - ax / 2;
		while (1)
		{
			PUTDOT(x, y, color);

			if (x == fl->b.x)
				return;
			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - ay / 2;
		while (1)
		{
			PUTDOT(x, y, color);

			if (y == fl->b.y)
				return;
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}

//
// Clip lines, draw visible part sof lines.
//
void AM_drawMline(mline_t *ml,
				  int color)
{
	static fline_t fl;

#ifdef SUPPORTS_HERCULES_AUTOMAP
	if (AM_clipMline(ml, &fl))
		if (HERCmap)
		{
			AM_drawFlineHercules(&fl); // draws it on frame buffer using fb coords
		}
		else
		{
			AM_drawFline(&fl, color); // draws it on frame buffer using fb coords
		}
#else
	if (AM_clipMline(ml, &fl))
		AM_drawFline(&fl, color); // draws it on frame buffer using fb coords
#endif
}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
void AM_drawWalls(void)
{
	int i;
	static mline_t l;

	for (i = 0; i < numlines; i++)
	{
		l.a.x = lines[i].v1->x;
		l.a.y = lines[i].v1->y;
		l.b.x = lines[i].v2->x;
		l.b.y = lines[i].v2->y;
		if (cheating || (lines[i].flags & ML_MAPPED))
		{
			if ((lines[i].flags & ML_DONTDRAW) && !cheating)
				continue;
			if (!lines[i].backsector)
			{
				AM_drawMline(&l, WALLCOLORS);
			}
			else
			{
				if (lines[i].special == 39)
				{ // teleporters
					AM_drawMline(&l, WALLCOLORS + WALLRANGE / 2);
				}
				else if (lines[i].flags & ML_SECRET) // secret door
				{
					if (cheating)
						AM_drawMline(&l, SECRETWALLCOLORS);
					else
						AM_drawMline(&l, WALLCOLORS);
				}
				else if (lines[i].backsector->floorheight != lines[i].frontsector->floorheight)
				{
					AM_drawMline(&l, FDWALLCOLORS); // floor level change
				}
				else if (lines[i].backsector->ceilingheight != lines[i].frontsector->ceilingheight)
				{
					AM_drawMline(&l, CDWALLCOLORS); // ceiling level change
				}
				else if (cheating)
				{
					AM_drawMline(&l, TSWALLCOLORS);
				}
			}
		}
		else if (players.powers[pw_allmap])
		{
			if (!(lines[i].flags & ML_DONTDRAW))
				AM_drawMline(&l, GRAYS + 3);
		}
	}
}

//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void AM_rotate(fixed_t *x,
			   fixed_t *y,
			   angle_t a)
{
	fixed_t tmpx;
	fixed_t fc;
	fixed_t fs;

	a >>= ANGLETOFINESHIFT;
	fc = finecosine[a];
	fs = finesine[a];

	tmpx = FixedMul(*x, fc) - FixedMul(*y, fs);

	*y = FixedMul(*x, fs) + FixedMul(*y, fc);

	*x = tmpx;
}

void AM_drawLineCharacter(mline_t *lineguy,
						  int lineguylines,
						  fixed_t scale,
						  angle_t angle,
						  int color,
						  fixed_t x,
						  fixed_t y)
{
	int i;
	mline_t l;

	for (i = 0; i < lineguylines; i++)
	{
		l.a.x = lineguy[i].a.x;
		l.a.y = lineguy[i].a.y;

		if (scale)
		{
			l.a.x = FixedMul(scale, l.a.x);
			l.a.y = FixedMul(scale, l.a.y);
		}

		if (angle)
			AM_rotate(&l.a.x, &l.a.y, angle);

		l.a.x += x;
		l.a.y += y;

		l.b.x = lineguy[i].b.x;
		l.b.y = lineguy[i].b.y;

		if (scale)
		{
			l.b.x = FixedMul(scale, l.b.x);
			l.b.y = FixedMul(scale, l.b.y);
		}

		if (angle)
			AM_rotate(&l.b.x, &l.b.y, angle);

		l.b.x += x;
		l.b.y += y;

		AM_drawMline(&l, color);
	}
}

void AM_drawPlayers(void)
{
	AM_drawLineCharacter(player_arrow, NUMPLYRLINES, 0, players_mo->angle, WHITE, players_mo->x, players_mo->y);
}

void AM_drawThings(int colors,
				   int colorrange)
{
	int i;
	mobj_t *t;

	for (i = 0; i < numsectors; i++)
	{
		t = sectors[i].thinglist;
		while (t)
		{
			AM_drawLineCharacter(thintriangle_guy, NUMTHINTRIANGLEGUYLINES,
								 16 << FRACBITS, t->angle, colors, t->x, t->y);
			t = t->snext;
		}
	}
}

void AM_Drawer(void)
{
	int i;

#ifdef SUPPORTS_HERCULES_AUTOMAP
	byte *herc = (byte *)0xB0000;
#endif

	if (!automapactive)
		return;

#if defined(USE_BACKBUFFER)
	updatestate |= I_FULLSCRN;
#endif

#ifdef SUPPORTS_HERCULES_AUTOMAP
	if (HERCmap)
	{

		// Change automapbuffer
		if (automapbuffer == automapbuffer1)
		{
			automapbuffer = automapbuffer2;
		}
		else
		{
			automapbuffer = automapbuffer1;
		}

		SetDWords(automapbuffer, 0, 8192);
	}
	else
	{
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
		if (transparentmap)
		{
			SetDWords(screen0, TRANSPARENTPIXELDWORD, MulScreenWidthQuarter(automapheight)); // Clear automap frame buffer
		}
		else
		{
			SetDWords(screen0, BACKGROUND, MulScreenWidthQuarter(automapheight)); // Clear automap frame buffer
		}
#endif
#if defined(USE_BACKBUFFER)
		if (!transparentmap)
			SetDWords(backbuffer, BACKGROUND, MulScreenWidthQuarter(automapheight)); // Clear automap frame buffer
#endif
	}
#else
#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
	if (transparentmap)
	{
		SetDWords(screen0, TRANSPARENTPIXELDWORD, MulScreenWidthQuarter(automapheight)); // Clear automap frame buffer
	}
	else
	{
		SetDWords(screen0, BACKGROUND, MulScreenWidthQuarter(automapheight)); // Clear automap frame buffer
	}
#endif
#if defined(USE_BACKBUFFER)
	if (!transparentmap)
		SetDWords(backbuffer, BACKGROUND, MulScreenWidthQuarter(automapheight)); // Clear automap frame buffer
#endif
#endif

	AM_drawWalls();
	AM_drawPlayers();
	if (cheating == 2)
		AM_drawThings(THINGCOLORS, THINGRANGE);

#ifdef SUPPORTS_HERCULES_AUTOMAP

	if (automapbuffer == automapbuffer1)
	{
		for (i = 0; i < 32768; i += 4)
		{
			if (automapbuffer[i] != automapbuffer2[i])
				herc[i] = automapbuffer[i];

			if (automapbuffer[i + 1] != automapbuffer2[i + 1])
				herc[i + 1] = automapbuffer[i + 1];

			if (automapbuffer[i + 2] != automapbuffer2[i + 2])
				herc[i + 2] = automapbuffer[i + 2];

			if (automapbuffer[i + 3] != automapbuffer2[i + 3])
				herc[i + 3] = automapbuffer[i + 3];
		}
	}
	else
	{
		for (i = 0; i < 32768; i += 4)
		{
			if (automapbuffer[i] != automapbuffer1[i])
				herc[i] = automapbuffer[i];

			if (automapbuffer[i + 1] != automapbuffer1[i + 1])
				herc[i + 1] = automapbuffer[i + 1];

			if (automapbuffer[i + 2] != automapbuffer1[i + 2])
				herc[i + 2] = automapbuffer[i + 2];

			if (automapbuffer[i + 3] != automapbuffer1[i + 3])
				herc[i + 3] = automapbuffer[i + 3];
		}
	}

#endif

#ifdef SUPPORTS_HERCULES_AUTOMAP
	#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
		if(!HERCmap)
			V_MarkRect(0, 0, SCREENWIDTH, automapheight);
	#endif
#else
	#if defined(MODE_X) || defined(MODE_Y) || defined(MODE_Y_HALF) || defined(MODE_VBE2_DIRECT)
		V_MarkRect(0, 0, SCREENWIDTH, automapheight);
	#endif
#endif
}

#endif
