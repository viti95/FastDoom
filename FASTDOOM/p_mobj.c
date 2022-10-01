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
//	Moving object handling. Spawn functions.
//

#include <string.h>
#include "options.h"
#include "i_random.h"

#include "i_system.h"
#include "z_zone.h"
#include "m_misc.h"

#include "doomdef.h"
#include "p_local.h"
#include "sounds.h"

#include "st_stuff.h"
#include "hu_stuff.h"

#include "s_sound.h"

#include "doomstat.h"

void G_PlayerReborn();
void P_SpawnMapThing(mapthing_t *mthing);

//
// P_SetMobjState
// Returns true if the mobj is still present.
//

byte P_NotSetMobjState(mobj_t *mobj, unsigned short state)
{
    state_t *st;

    do
    {
        if (state == S_NULL)
        {
            mobj->state = (state_t *)S_NULL;
            P_RemoveMobj(mobj);
            return 1;
        }

        st = &states[state];
        mobj->state = st;
        mobj->tics = st->tics;
        mobj->sprite = st->sprite;
        mobj->frame = st->frame;

        // Modified handling.
        // Call action functions when the state is set
        if (st->action.acp1)
            st->action.acp1(mobj);

        state = st->nextstate;
    } while (!mobj->tics);

    return 0;
}

//
// P_ExplodeMissile
//
void P_ExplodeMissile(mobj_t *mo)
{
    mo->momx = mo->momy = mo->momz = 0;

    P_NotSetMobjState(mo, mobjinfo[mo->type].deathstate);

    mo->tics -= P_Random & 3;

    if (mo->tics < 1)
        mo->tics = 1;

    mo->flags &= ~MF_MISSILE;

    if (mo->info->deathsound)
        S_StartSound(mo, mo->info->deathsound);
}

//
// P_XYMovement
//
#define STOPSPEED 0x1000
#define FRICTION 0xe800 // (2^15 + 2^14 + 2^13 + 2^11) or (29/2^5)

void P_XYMovement(mobj_t *mo)
{
    fixed_t ptryx;
    fixed_t ptryy;
    player_t *player;
    fixed_t xmove;
    fixed_t ymove;

    if (!mo->momx && !mo->momy)
    {
        if (mo->flags & MF_SKULLFLY)
        {
            // the skull slammed into something
            mo->flags &= ~MF_SKULLFLY;
            mo->momx = mo->momy = mo->momz = 0;

            P_NotSetMobjState(mo, mo->info->spawnstate);
        }
        return;
    }

    player = mo->player;

    // BETWEEN -MAXMOVE AND MAXMOVE
    if (mo->momx > MAXMOVE)
        mo->momx = MAXMOVE;
    else if (mo->momx < -MAXMOVE)
        mo->momx = -MAXMOVE;

    // BETWEEN -MAXMOVE AND MAXMOVE
    if (mo->momy > MAXMOVE)
        mo->momy = MAXMOVE;
    else if (mo->momy < -MAXMOVE)
        mo->momy = -MAXMOVE;

    xmove = mo->momx;
    ymove = mo->momy;

    do
    {
        if (xmove > MAXMOVE / 2 || ymove > MAXMOVE / 2)
        {
            ptryx = mo->x + xmove / 2;
            ptryy = mo->y + ymove / 2;
            xmove >>= 1;
            ymove >>= 1;
        }
        else
        {
            ptryx = mo->x + xmove;
            ptryy = mo->y + ymove;
            xmove = ymove = 0;
        }

        if (P_TryMove(mo, ptryx, ptryy))
        {
            // blocked move
            if (mo->player)
            { // try to slide along it
                P_SlideMove(mo);
            }
            else if (mo->flags & MF_MISSILE)
            {
                // explode a missile
                if (ceilingline &&
                    ceilingline->backsector &&
                    ceilingline->backsector->ceilingpic == skyflatnum)
                {
                    // Hack to prevent missiles exploding
                    // against the sky.
                    // Does not handle sky floors.
                    P_RemoveMobj(mo);
                    return;
                }
                P_ExplodeMissile(mo);
            }
            else
                mo->momx = mo->momy = 0;
        }
    } while (xmove || ymove);

    if (((mo->flags & (MF_MISSILE | MF_SKULLFLY)) || mo->z > mo->floorz) || ((mo->flags & MF_CORPSE) && ((mo->momx > FRACUNIT / 4 || mo->momx < -FRACUNIT / 4 || mo->momy > FRACUNIT / 4 || mo->momy < -FRACUNIT / 4) && mo->floorz != mo->subsector->sector->floorheight)))
        return; // no friction for missiles ever, no friction when airborne, do not stop sliding if halfway off a step with some momentum

    if (mo->momx > -STOPSPEED && mo->momx < STOPSPEED && mo->momy > -STOPSPEED && mo->momy < STOPSPEED && (!player || (player->cmd.forwardmove == 0 && player->cmd.sidemove == 0)))
    {
        // if in a walking frame, stop moving
        if (player && (unsigned)((player->mo->state - states) - S_PLAY_RUN1) < 4)
            P_NotSetMobjState(player->mo, S_PLAY);

        mo->momx = 0;
        mo->momy = 0;
    }
    else
    {
        //mo->momx = FixedMul(mo->momx, FRICTION);
        //mo->momy = FixedMul(mo->momy, FRICTION);

        mo->momx = (mo->momx * 29) >> 5;
        mo->momy = (mo->momy * 29) >> 5;
    }
}

//
// P_ZMovement
//
void P_ZMovement(mobj_t *mo)
{
    fixed_t dist;
    fixed_t delta;

    // check for smooth step up
    if (mo->player && mo->z < mo->floorz)
    {
        mo->player->viewheight -= mo->floorz - mo->z;
        mo->player->deltaviewheight = (VIEWHEIGHT - mo->player->viewheight) >> 3;
    }

    // adjust height
    mo->z += mo->momz;

    if (mo->flags & MF_FLOAT && mo->target)
    {
        // float down towards target if too close
        if (!(mo->flags & MF_SKULLFLY) && !(mo->flags & MF_INFLOAT))
        {
            dist = P_AproxDistance(mo->x - mo->target->x,
                                   mo->y - mo->target->y);

            delta = (mo->target->z + (mo->height >> 1)) - mo->z;

            if (delta < 0 && dist < -(delta * 3))
                mo->z -= FLOATSPEED;
            else if (delta > 0 && dist < (delta * 3))
                mo->z += FLOATSPEED;
        }
    }

    // clip movement
    if (mo->z <= mo->floorz)
    {
        // hit the floor

        if (complevel >= COMPLEVEL_ULTIMATE_DOOM)
        {
            if (mo->flags & MF_SKULLFLY)
            {
                // the skull slammed into something
                mo->momz = -mo->momz;
            }
        }

        if (mo->momz < 0)
        {
            if (mo->player && mo->momz < -GRAVITY * 8)
            {
                // Squat down.
                // Decrease viewheight for a moment
                // after hitting the ground (hard),
                // and utter appropriate sound.
                mo->player->deltaviewheight = mo->momz >> 3;
                S_StartSound(mo, sfx_oof);
            }
            mo->momz = 0;
        }
        mo->z = mo->floorz;

        if (complevel < COMPLEVEL_ULTIMATE_DOOM)
        {
            if (mo->flags & MF_SKULLFLY)
            {
                // the skull slammed into something
                mo->momz = -mo->momz;
            }
        }

        if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
        {
            P_ExplodeMissile(mo);
            return;
        }
    }
    else if (!(mo->flags & MF_NOGRAVITY))
    {
        if (mo->momz == 0)
            mo->momz = -GRAVITY * 2;
        else
            mo->momz -= GRAVITY;
    }

    if (mo->z > mo->ceilingz - mo->height)
    {
        // hit the ceiling
        mo->z = mo->ceilingz - mo->height;
        
        if (mo->momz > 0)
            mo->momz = 0;
        
        // OPTIMIZE NEGATE
        if (mo->flags & MF_SKULLFLY)
        { // the skull slammed into something
            mo->momz = -mo->momz;
        }

        if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
        {
            P_ExplodeMissile(mo);
            return;
        }
    }
}

//
// P_NightmareRespawn
//
void P_NightmareRespawn(mobj_t *mobj)
{
    fixed_t x;
    fixed_t y;
    fixed_t z;
    subsector_t *ss;
    mobj_t *mo;
    mapthing_t *mthing;

    x = mobj->spawnpoint.x << FRACBITS;
    y = mobj->spawnpoint.y << FRACBITS;

    // somthing is occupying it's position?
    if (P_NotCheckPosition(mobj, x, y))
        return; // no respwan

    // spawn a teleport fog at old spot
    // because of removal of the body?
    mo = P_SpawnMobj(mobj->x,
                     mobj->y,
                     mobj->subsector->sector->floorheight, MT_TFOG);
    // initiate teleport sound
    S_StartSound(mo, sfx_telept);

    // spawn a teleport fog at the new spot
    ss = R_PointInSubsector(x, y);

    mo = P_SpawnMobj(x, y, ss->sector->floorheight, MT_TFOG);

    S_StartSound(mo, sfx_telept);

    // spawn the new monster
    mthing = &mobj->spawnpoint;

    // spawn it
    if (mobj->info->flags & MF_SPAWNCEILING)
        z = ONCEILINGZ;
    else
        z = ONFLOORZ;

    // inherit attributes from deceased one
    mo = P_SpawnMobj(x, y, z, mobj->type);
    mo->spawnpoint = mobj->spawnpoint;

    // VITI95: OPTIMIZE
    mo->angle = ANG45 * (mthing->angle / 45);

    if (mthing->options & MTF_AMBUSH)
        mo->flags |= MF_AMBUSH;

    mo->reactiontime = 18;

    // remove the old monster,
    P_RemoveMobj(mobj);
}

//
// P_MobjThinker
//
void P_MobjThinker(mobj_t *mobj)
{
    // momentum movement
    if (mobj->momx || mobj->momy || (mobj->flags & MF_SKULLFLY))
    {
        P_XYMovement(mobj);

        // FIXME: decent NOP/NULL/Nil function pointer please.
        if (mobj->thinker.function.acv == (actionf_v)(-1))
            return; // mobj was removed
    }
    if ((mobj->z != mobj->floorz) || mobj->momz)
    {
        P_ZMovement(mobj);

        // FIXME: decent NOP/NULL/Nil function pointer please.
        if (mobj->thinker.function.acv == (actionf_v)(-1))
            return; // mobj was removed
    }

    // cycle through states,
    // calling action functions at transitions
    if (mobj->tics != -1)
    {
        mobj->tics--;

        // you can cycle through multiple states in a tic
        if (!mobj->tics && P_NotSetMobjState(mobj, mobj->state->nextstate))
            return; // freed itself
    }
    else
    {
        // check for nightmare respawn
        if (!(mobj->flags & MF_COUNTKILL) || !respawnmonsters)
            return;

        mobj->movecount++;

        if (mobj->movecount < 12 * 35 || (leveltime & 31) || P_Random > 4)
            return;

        P_NightmareRespawn(mobj);
    }
}

//Thinker function for stuff that doesn't need to do anything
//interesting.
//Just cycles through the states. Allows sprite animation to work.
void P_MobjBrainlessThinker(mobj_t *mobj)
{
    // cycle through states,
    // calling action functions at transitions

    if (mobj->tics != -1)
    {
        mobj->tics--;

        // you can cycle through multiple states in a tic

        if (!mobj->tics)
            P_NotSetMobjState(mobj, mobj->state->nextstate);
    }
}

void P_MobjTicklessThinker(mobj_t *mobj)
{
    // DO NOTHING
}

//
// P_SpawnMobj
//
mobj_t *
P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, byte type)
{
    mobj_t *mobj;
    state_t *st;
    mobjinfo_t *info;

    mobj = Z_MallocUnowned(sizeof(*mobj), PU_LEVEL);
    memset(mobj, 0, sizeof(*mobj));
    info = &mobjinfo[type];

    mobj->type = type;
    mobj->info = info;
    mobj->x = x;
    mobj->y = y;
    mobj->radius = info->radius;
    mobj->height = info->height;
    mobj->flags = info->flags;
    mobj->health = info->spawnhealth;

    if (gameskill != sk_nightmare)
        mobj->reactiontime = info->reactiontime;

    mobj->lastlook = P_Random & (MAXPLAYERS - 1); // MAXPLAYERS=4

    // do not set the state with P_SetMobjState,
    // because action routines can not be called yet
    st = &states[info->spawnstate];

    mobj->state = st;
    mobj->tics = st->tics;
    mobj->sprite = st->sprite;
    mobj->frame = st->frame;

    // set subsector and/or block links
    P_SetThingPosition(mobj);

    mobj->floorz = mobj->subsector->sector->floorheight;
    mobj->ceilingz = mobj->subsector->sector->ceilingheight;

    if (z == ONFLOORZ)
        mobj->z = mobj->floorz;
    else if (z == ONCEILINGZ)
        mobj->z = mobj->ceilingz - mobj->info->height;
    else
        mobj->z = z;

    if (mobj->type < MT_MISC0)
    {
        mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;
    }
    else
    {
        if (mobj->tics != -1)
            mobj->thinker.function.acp1 = (actionf_p1)P_MobjBrainlessThinker;
        else
            mobj->thinker.function.acp1 = (actionf_p1)P_MobjTicklessThinker;
    }

    thinkercap.prev->next = &mobj->thinker;
    mobj->thinker.next = &thinkercap;
    mobj->thinker.prev = thinkercap.prev;
    thinkercap.prev = &mobj->thinker;

    return mobj;
}

//
// P_RemoveMobj
//
mapthing_t itemrespawnque[ITEMQUESIZE];
int itemrespawntime[ITEMQUESIZE];
int iquehead;
int iquetail;

void P_RemoveMobj(mobj_t *mobj)
{
    if ((mobj->flags & MF_SPECIAL) && !(mobj->flags & MF_DROPPED) && (mobj->type != MT_INV) && (mobj->type != MT_INS))
    {
        itemrespawnque[iquehead] = mobj->spawnpoint;
        itemrespawntime[iquehead] = leveltime;
        iquehead = (iquehead + 1) & (ITEMQUESIZE - 1);

        // lose one off the end?
        if (iquehead == iquetail)
            iquetail = (iquetail + 1) & (ITEMQUESIZE - 1);
    }

    // unlink from sector and block lists
    P_UnsetThingPosition(mobj);

    // stop any playing sound
    S_StopSound(mobj);

    // free block
    mobj->thinker.function.acv = (actionf_v)(-1);
}

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
//
void P_SpawnPlayer(mapthing_t *mthing)
{
    player_t *p;
    fixed_t x;
    fixed_t y;
    fixed_t z;

    mobj_t *mobj;

    p = &players;

    if (p->playerstate == PST_REBORN)
        G_PlayerReborn();

    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;
    z = ONFLOORZ;
    mobj = P_SpawnMobj(x, y, z, MT_PLAYER);

    // set color translations for player sprites
    if (mthing->type > 1)
        mobj->flags |= (mthing->type - 1) << MF_TRANSSHIFT;

    // VITI95: OPTIMIZE
    mobj->angle = ANG45 * (mthing->angle / 45);
    mobj->player = p;
    mobj->health = p->health;

    p->mo = mobj;
    players_mo = mobj;
    p->playerstate = PST_LIVE;
    p->refire = 0;
    p->message = NULL;
    p->damagecount = 0;
    p->bonuscount = 0;
    p->extralight = 0;
    p->fixedcolormap = 0;
    p->viewheight = VIEWHEIGHT;

    // setup gun psprite
    P_SetupPsprites();

    if (mthing->type - 1 == 0)
    {
        // wake up the status bar
        ST_Start();
        // wake up the heads up text
        HU_Start();
    }
}

//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
void P_SpawnMapThing(mapthing_t *mthing)
{
    int i;
    int bit;
    mobj_t *mobj;
    fixed_t x;
    fixed_t y;
    fixed_t z;

    // count deathmatch start positions
    if (mthing->type == 11 || mthing->type == 2 || mthing->type == 3 || mthing->type == 4)
    {
        return;
    }

    // check for players specially
    if (mthing->type == 1)
    {
        // save spots for respawning in network games
        P_SpawnPlayer(mthing);

        return;
    }

    // check for apropriate skill level
    if (mthing->options & 16)
        return;

    if (gameskill == sk_baby)
        bit = 1;
    else if (gameskill == sk_nightmare)
        bit = 4;
    else
        bit = 1 << (gameskill - 1);

    if (!(mthing->options & bit))
        return;

    // find which type to spawn
    for (i = 0; i < NUMMOBJTYPES; i++)
        if (mthing->type == mobjinfo[i].doomednum)
            break;

    // don't spawn any monsters if -nomonsters
    if (nomonsters && (i == MT_SKULL || (mobjinfo[i].flags & MF_COUNTKILL)))
    {
        return;
    }

    // spawn it
    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;

    if (mobjinfo[i].flags & MF_SPAWNCEILING)
        z = ONCEILINGZ;
    else
        z = ONFLOORZ;

    mobj = P_SpawnMobj(x, y, z, i);
    mobj->spawnpoint = *mthing;

    if (mobj->tics > 0)
        mobj->tics = 1 + (P_Random % mobj->tics);

    totalkills += (mobj->flags & MF_COUNTKILL) != 0;
    totalitems += (mobj->flags & MF_COUNTITEM) != 0;
    
    // VITI95: OPTIMIZE
    mobj->angle = ANG45 * (mthing->angle / 45);
    if (mthing->options & MTF_AMBUSH)
        mobj->flags |= MF_AMBUSH;
}

//
// GAME SPAWN FUNCTIONS
//

//
// P_SpawnPuff
//
extern fixed_t attackrange;

void P_SpawnPuff(fixed_t x,
                 fixed_t y,
                 fixed_t z)
{
    mobj_t *th;

    z += ((P_Random - P_Random) << 10);

    th = P_SpawnMobj(x, y, z, MT_PUFF);
    th->momz = FRACUNIT;
    th->tics -= P_Random & 3;

    if (th->tics < 1)
        th->tics = 1;

    // don't make punches spark on the wall
    if (attackrange == MELEERANGE)
        P_NotSetMobjState(th, S_PUFF3);
}

//
// P_SpawnBlood
//
void P_SpawnBlood(fixed_t x,
                  fixed_t y,
                  fixed_t z,
                  int damage)
{
    mobj_t *th;

    z += ((P_Random - P_Random) << 10);
    th = P_SpawnMobj(x, y, z, MT_BLOOD);
    th->momz = FRACUNIT * 2;
    th->tics -= P_Random & 3;

    if (th->tics < 1)
        th->tics = 1;

    if (damage <= 12 && damage >= 9)
        P_NotSetMobjState(th, S_BLOOD2);
    else if (damage < 9)
        P_NotSetMobjState(th, S_BLOOD3);
}

//
// P_CheckMissileSpawn
// Moves the missile forward a bit
//  and possibly explodes it right there.
//
void P_CheckMissileSpawn(mobj_t *th)
{
    th->tics -= P_Random & 3;
    if (th->tics < 1)
        th->tics = 1;

    // move a little forward so an angle can
    // be computed if it immediately explodes
    th->x += (th->momx >> 1);
    th->y += (th->momy >> 1);
    th->z += (th->momz >> 1);

    if (P_TryMove(th, th->x, th->y))
        P_ExplodeMissile(th);
}

//
// P_SpawnMissile
//
mobj_t *
P_SpawnMissile(mobj_t *source, mobj_t *dest, byte type)
{
    mobj_t *th;
    angle_t an;
    int dist;

    th = P_SpawnMobj(source->x,
                     source->y,
                     source->z + 4 * 8 * FRACUNIT, type);

    if (th->info->seesound)
        S_StartSound(th, th->info->seesound);

    th->target = source; // where it came from
    an = R_PointToAngle2(source->x, source->y, dest->x, dest->y);

    // fuzzy player
    if (dest->flags & MF_SHADOW)
        an += (P_Random - P_Random) << 20;

    th->angle = an;
    an >>= ANGLETOFINESHIFT;
    th->momx = FixedMul(th->info->speed, finecosine[an]);
    th->momy = FixedMul(th->info->speed, finesine[an]);

    dist = P_AproxDistance(dest->x - source->x, dest->y - source->y);

    // VITI95: OPTIMIZE
    dist /= th->info->speed;

    if (dist < 1)
    {
        th->momz = dest->z - source->z;
    }
    else
    {
        th->momz = (dest->z - source->z) / dist;
    }

    P_CheckMissileSpawn(th);

    return th;
}

//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
void P_SpawnPlayerMissile(byte type)
{
    mobj_t *th;
    angle_t an;

    fixed_t x;
    fixed_t y;
    fixed_t z;
    fixed_t slope;

    // see which target is to be aimed at
    an = players_mo->angle;
    slope = P_AimLineAttack(players_mo, an, HALFMISSILERANGE);

    if (!linetarget)
    {
        an += 1 << 26;
        slope = P_AimLineAttack(players_mo, an, HALFMISSILERANGE);

        if (!linetarget)
        {
            an -= 2 << 26;
            slope = P_AimLineAttack(players_mo, an, HALFMISSILERANGE);
        }

        if (!linetarget)
        {
            an = players_mo->angle;
            slope = 0;
        }
    }

    x = players_mo->x;
    y = players_mo->y;
    z = players_mo->z + 4 * 8 * FRACUNIT;

    th = P_SpawnMobj(x, y, z, type);

    if (th->info->seesound)
        S_StartSound(th, th->info->seesound);

    th->target = players_mo;
    th->angle = an;
    th->momx = FixedMul(th->info->speed, finecosine[an >> ANGLETOFINESHIFT]);
    th->momy = FixedMul(th->info->speed, finesine[an >> ANGLETOFINESHIFT]);
    th->momz = FixedMul(th->info->speed, slope);

    P_CheckMissileSpawn(th);
}
