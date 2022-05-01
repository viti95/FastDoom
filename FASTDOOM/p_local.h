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
//	Play functions, animation, global header.
//

#ifndef __P_LOCAL__
#define __P_LOCAL__

#ifndef __R_LOCAL__
#include "r_local.h"
#endif

#define FLOATSPEED (FRACUNIT * 4)

#define MAXHEALTH 100
#define VIEWHEIGHT (41 * FRACUNIT)

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS 128
#define MAPBLOCKSIZE (MAPBLOCKUNITS * FRACUNIT)
#define MAPBLOCKSHIFT (FRACBITS + 7)
#define MAPBMASK (MAPBLOCKSIZE - 1)
#define MAPBTOFRAC (MAPBLOCKSHIFT - FRACBITS)

// player radius for movement checking
#define PLAYERRADIUS 16 * FRACUNIT

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS 32 * FRACUNIT

#define GRAVITY FRACUNIT
#define MAXMOVE (30 * FRACUNIT)

#define USERANGE (64 * FRACUNIT)
#define MELEERANGE (64 * FRACUNIT)
#define MISSILERANGE (32 * 64 * FRACUNIT)
#define HALFMISSILERANGE (16 * 64 * FRACUNIT)

// follow a player exlusively for 3 seconds
#define BASETHRESHOLD 100

//
// P_TICK
//

// both the head and tail of the thinker list
extern thinker_t thinkercap;

void P_InitThinkers(void);

//
// P_PSPR
//
void P_SetupPsprites(player_t *curplayer);
void P_MovePsprites(player_t *curplayer);
void P_DropWeapon(player_t *player);

//
// P_USER
//
void P_PlayerThink(player_t *player);

//
// P_MOBJ
//
#define ONFLOORZ MININT
#define ONCEILINGZ MAXINT

// Time interval for item respawning.
#define ITEMQUESIZE 128

extern int iquehead;
extern int iquetail;

mobj_t * P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, byte type);

void P_RemoveMobj(mobj_t *th);
byte P_NotSetMobjState(mobj_t *mobj, unsigned short state);
void P_MobjThinker(mobj_t *mobj);
void P_MobjBrainlessThinker(mobj_t *mobj);
void P_MobjTicklessThinker(mobj_t *mobj);

void P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z);
void P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, int damage);
mobj_t *P_SpawnMissile(mobj_t *source, mobj_t *dest, byte type);
void P_SpawnPlayerMissile(mobj_t *source, byte type);

//
// P_ENEMY
//
void P_NoiseAlert(mobj_t *target, mobj_t *emmiter);

//
// P_MAPUTL
//
typedef struct
{
    fixed_t x;
    fixed_t y;
    fixed_t dx;
    fixed_t dy;

} divline_t;

typedef struct
{
    fixed_t frac; // along trace line
    byte isaline;
    union {
        mobj_t *thing;
        line_t *line;
    } d;
} intercept_t;

#define MAXINTERCEPTS 128

extern intercept_t intercepts[MAXINTERCEPTS];
extern intercept_t *intercept_p;

typedef byte (*traverser_t)(intercept_t *in);

fixed_t P_AproxDistance(fixed_t dx, fixed_t dy);
byte P_PointOnLineSide(fixed_t x, fixed_t y, line_t *line);
byte P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t *line);
fixed_t P_InterceptVector(divline_t *v2, divline_t *v1);
byte P_BoxOnLineSide(fixed_t *tmbox, line_t *ld);

extern fixed_t opentop;
extern fixed_t openbottom;
extern fixed_t openrange;
extern fixed_t lowfloor;

void P_LineOpening(line_t *linedef);

byte P_NotBlockLinesIterator(int x, int y, byte (*func)(line_t *));
byte P_NotBlockThingsIterator(int x, int y, byte (*func)(mobj_t *));

#define PT_ADDLINES 1
#define PT_ADDTHINGS 2

extern divline_t trace;

void P_PathTraverseLI(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
void P_PathTraverseLITH(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
void P_TraverseIntercepts(traverser_t func);

void P_UnsetThingPosition(mobj_t *thing);
void P_SetThingPosition(mobj_t *thing);

//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern byte floatok;
extern fixed_t tmfloorz;
extern fixed_t tmceilingz;

extern line_t *ceilingline;

byte P_NotCheckPosition(mobj_t *thing, fixed_t x, fixed_t y);
byte P_TryMove(mobj_t *thing, fixed_t x, fixed_t y);
byte P_TeleportMove(mobj_t *thing, fixed_t x, fixed_t y);
void P_SlideMove(mobj_t *mo);
byte P_CheckSight(mobj_t *t1, mobj_t *t2);
void P_UseLines(player_t *player);

byte P_ChangeSector(sector_t *sector, byte crunch);

extern mobj_t *linetarget; // who got hit (or NULL)

fixed_t
P_AimLineAttack(mobj_t *t1,
                angle_t angle,
                fixed_t distance);

void P_LineAttack(mobj_t *t1,
                  angle_t angle,
                  fixed_t distance,
                  fixed_t slope,
                  int damage);

void P_RadiusAttack(mobj_t *spot,
                    mobj_t *source,
                    int damage);

//
// P_SETUP
//
extern byte *rejectmatrix;  // for fast sight rejection
extern short *blockmaplump; // offsets in blockmap are from here
extern short *blockmap;
extern int bmapwidth;
extern int bmapheight; // in mapblocks
extern fixed_t bmaporgx;
extern fixed_t bmaporgy;    // origin of block map
extern mobj_t **blocklinks; // for thing chains

//
// P_INTER
//
extern int maxammo[NUMAMMO];

void P_TouchSpecialThing(mobj_t *special,
                         mobj_t *toucher);

void P_DamageMobj(mobj_t *target,
                  mobj_t *inflictor,
                  mobj_t *source,
                  int damage);

//
// P_SPEC
//
#include "p_spec.h"

#endif // __P_LOCAL__
