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
//	Weapon sprite animation, weapon objects.
//	Action functions for weapons.
//

#include <string.h>
#include "options.h"
#include "i_random.h"

#include "doomdef.h"
#include "d_event.h"

#include "m_misc.h"
#include "p_local.h"
#include "s_sound.h"

// State.
#include "doomstat.h"

// Data.
#include "sounds.h"

#include "p_pspr.h"

#define LOWERSPEED FRACUNIT * 6
#define RAISESPEED FRACUNIT * 6

#define WEAPONBOTTOM 128 * FRACUNIT
#define WEAPONTOP 32 * FRACUNIT

// plasma cells for a bfg attack
#define BFGCELLS 40

//
// PSPRITE ACTIONS for waepons.
// This struct controls the weapon animations.
//
// Each entry is:
//   ammo/amunition type
//  upstate
//  downstate
// readystate
// atkstate, i.e. attack/fire/hit frame
// flashstate, muzzle flash
//
const weaponinfo_t weaponinfo[NUMWEAPONS] =
    {
        {// fist
         am_noammo,
         S_PUNCHUP,
         S_PUNCHDOWN,
         S_PUNCH,
         S_PUNCH1,
         S_NULL},
        {// pistol
         am_clip,
         S_PISTOLUP,
         S_PISTOLDOWN,
         S_PISTOL,
         S_PISTOL1,
         S_PISTOLFLASH},
        {// shotgun
         am_shell,
         S_SGUNUP,
         S_SGUNDOWN,
         S_SGUN,
         S_SGUN1,
         S_SGUNFLASH1},
        {// chaingun
         am_clip,
         S_CHAINUP,
         S_CHAINDOWN,
         S_CHAIN,
         S_CHAIN1,
         S_CHAINFLASH1},
        {// missile launcher
         am_misl,
         S_MISSILEUP,
         S_MISSILEDOWN,
         S_MISSILE,
         S_MISSILE1,
         S_MISSILEFLASH1},
        {// plasma rifle
         am_cell,
         S_PLASMAUP,
         S_PLASMADOWN,
         S_PLASMA,
         S_PLASMA1,
         S_PLASMAFLASH1},
        {// bfg 9000
         am_cell,
         S_BFGUP,
         S_BFGDOWN,
         S_BFG,
         S_BFG1,
         S_BFGFLASH1},
        {// chainsaw
         am_noammo,
         S_SAWUP,
         S_SAWDOWN,
         S_SAW,
         S_SAW1,
         S_NULL},
        {// super shotgun
         am_shell,
         S_DSGUNUP,
         S_DSGUNDOWN,
         S_DSGUN,
         S_DSGUN1,
         S_DSGUNFLASH1},
};

//
// P_SetPsprite
//
void P_SetPsprite(byte position, unsigned short stnum)
{
    pspdef_t *psp;
    state_t *state;

    psp = &players.psprites[position];

    do
    {
        if (!stnum)
        {
            // object removed itself
            psp->state = NULL;
            break;
        }

        state = &states[stnum];
        psp->state = state;
        psp->tics = state->tics; // could be 0

        // Call action routine.
        // Modified handling.
        if (state->action.acp2)
        {
            state->action.acp2(NULL, psp);
            if (!psp->state)
                break;
        }

        stnum = psp->state->nextstate;

    } while (!psp->tics);
    // an initial state of 0 could cycle through
}

//
// P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//
void P_BringUpWeapon(void)
{
    unsigned short newstate;

    if (players.pendingweapon == wp_nochange)
        players.pendingweapon = players.readyweapon;

    if (players.pendingweapon == wp_chainsaw)
        S_StartSound(players_mo, sfx_sawup);

    newstate = weaponinfo[players.pendingweapon].upstate;

    players.pendingweapon = wp_nochange;
    players.psprites[ps_weapon].sy = WEAPONBOTTOM;

    P_SetPsprite(ps_weapon, newstate);
}

//
// P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//
byte P_CheckAmmo(void)
{
    ammotype_t ammo;
    int count;

    ammo = weaponinfo[players.readyweapon].ammo;

    // Minimal amount for one shot varies.
    if (players.readyweapon == wp_bfg)
        count = BFGCELLS;
    else if (players.readyweapon == wp_supershotgun)
        count = 2; // Double barrel.
    else
        count = 1; // Regular.

    // Some do not need ammunition anyway.
    // Return if current ammunition sufficient.
    if (ammo == am_noammo || players.ammo[ammo] >= count)
        return 1;

    // Out of ammo, pick a weapon to change to.
    // Preferences are set here.
    do
    {
        if (players.weaponowned[wp_plasma] && players.ammo[am_cell] && (gamemode != shareware))
        {
            players.pendingweapon = wp_plasma;
        }
        else if (players.weaponowned[wp_supershotgun] && players.ammo[am_shell] > 2 && (gamemode == commercial))
        {
            players.pendingweapon = wp_supershotgun;
        }
        else if (players.weaponowned[wp_chaingun] && players.ammo[am_clip])
        {
            players.pendingweapon = wp_chaingun;
        }
        else if (players.weaponowned[wp_shotgun] && players.ammo[am_shell])
        {
            players.pendingweapon = wp_shotgun;
        }
        else if (players.ammo[am_clip])
        {
            players.pendingweapon = wp_pistol;
        }
        else if (players.weaponowned[wp_chainsaw])
        {
            players.pendingweapon = wp_chainsaw;
        }
        else if (players.weaponowned[wp_missile] && players.ammo[am_misl])
        {
            players.pendingweapon = wp_missile;
        }
        else if (players.weaponowned[wp_bfg] && players.ammo[am_cell] > 40 && (gamemode != shareware))
        {
            players.pendingweapon = wp_bfg;
        }
        else
        {
            // If everything fails.
            players.pendingweapon = wp_fist;
        }

    } while (players.pendingweapon == wp_nochange);

    // Now set appropriate weapon overlay.
    P_SetPsprite(ps_weapon, weaponinfo[players.readyweapon].downstate);

    return 0;
}

//
// P_FireWeapon.
//
void P_FireWeapon(void)
{
    unsigned short newstate;

    if (!P_CheckAmmo())
        return;

    P_NotSetMobjState(players_mo, S_PLAY_ATK1);
    newstate = weaponinfo[players.readyweapon].atkstate;
    P_SetPsprite(ps_weapon, newstate);
    P_NoiseAlert();
}

//
// P_DropWeapon
// Player died, so put the weapon away.
//
void P_DropWeapon(void)
{
    P_SetPsprite(ps_weapon, weaponinfo[players.readyweapon].downstate);
}

//
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//
void A_WeaponReady(player_t *player,
                   pspdef_t *psp)
{
    unsigned short newstate;
    int angle;

    // get out of attack state
    if (players_mo->state == &states[S_PLAY_ATK1] || players_mo->state == &states[S_PLAY_ATK2])
    {
        P_NotSetMobjState(players_mo, S_PLAY);
    }

    if (players.readyweapon == wp_chainsaw && psp->state == &states[S_SAW])
    {
        S_StartSound(players_mo, sfx_sawidl);
    }

    // check for change
    //  if player is dead, put the weapon away
    if (players.pendingweapon != wp_nochange || !players.health)
    {
        // change weapon
        //  (pending weapon should allready be validated)
        newstate = weaponinfo[players.readyweapon].downstate;
        P_SetPsprite(ps_weapon, newstate);
        return;
    }

    // check for fire
    //  the missile launcher and bfg do not auto fire
    if (players.cmd.buttons & BT_ATTACK)
    {
        if (!players.attackdown || (players.readyweapon != wp_missile && players.readyweapon != wp_bfg))
        {
            players.attackdown = true;
            P_FireWeapon();
            return;
        }
    }
    else
        players.attackdown = false;

    // bob the weapon based on movement speed
    angle = (128 * leveltime) & FINEMASK;
    psp->sx = FRACUNIT + FixedMul(players.bob, finecosine[angle]);
    angle &= FINEANGLES / 2 - 1;
    psp->sy = WEAPONTOP + FixedMul(players.bob, finesine[angle]);
}

//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//
void A_ReFire(player_t *player,
              pspdef_t *psp)
{

    // check for fire
    //  (if a weaponchange is pending, let it go through instead)
    if ((players.cmd.buttons & BT_ATTACK) && players.pendingweapon == wp_nochange && players.health)
    {
        players.refire++;
        P_FireWeapon();
    }
    else
    {
        players.refire = 0;
        P_CheckAmmo();
    }
}

void A_CheckReload(player_t *player,
                   pspdef_t *psp)
{
    P_CheckAmmo();
}

//
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//
void A_Lower(player_t *player,
             pspdef_t *psp)
{
    psp->sy += LOWERSPEED;

    // Is already down.
    if (psp->sy < WEAPONBOTTOM)
        return;

    // Player is dead.
    if (players.playerstate == PST_DEAD)
    {
        psp->sy = WEAPONBOTTOM;

        // don't bring weapon back up
        return;
    }

    // The old weapon has been lowered off the screen,
    // so change the weapon and start raising it
    if (!players.health)
    {
        // Player is dead, so keep the weapon off screen.
        P_SetPsprite(ps_weapon, S_NULL);
        return;
    }

    players.readyweapon = players.pendingweapon;

    P_BringUpWeapon();
}

//
// A_Raise
//
void A_Raise(player_t *player,
             pspdef_t *psp)
{
    unsigned short newstate;

    psp->sy -= RAISESPEED;

    if (psp->sy > WEAPONTOP)
        return;

    psp->sy = WEAPONTOP;

    // The weapon has been raised all the way,
    //  so change to the ready state.
    newstate = weaponinfo[players.readyweapon].readystate;

    P_SetPsprite(ps_weapon, newstate);
}

//
// A_GunFlash
//
void A_GunFlash(player_t *player,
                pspdef_t *psp)
{
    P_NotSetMobjState(players_mo, S_PLAY_ATK2);
    P_SetPsprite(ps_flash, weaponinfo[players.readyweapon].flashstate);
}

//
// WEAPON ATTACKS
//

//
// A_Punch
//
void A_Punch(player_t *player,
             pspdef_t *psp)
{
    angle_t angle;
    int damage;
    int slope;

    damage = P_Random_Mul2_Mod10_Plus1;

    if (players.powers[pw_strength])
        damage = Mul10(damage);

    angle = players_mo->angle;
    angle += (P_Random_Minus_P_Random) << 18;
    prndindex += 2;
    slope = P_AimLineAttack(players_mo, angle, MELEERANGE);
    P_LineAttack(players_mo, angle, MELEERANGE, slope, damage);

    // turn to face target
    if (linetarget)
    {
        S_StartSound(players_mo, sfx_punch);
        players_mo->prevangle = players_mo->angle;
        players_mo->angle = R_PointToAngle2(players_mo->x,
                                            players_mo->y,
                                            linetarget->x,
                                            linetarget->y);
    }
}

//
// A_Saw
//
void A_Saw(player_t *player,
           pspdef_t *psp)
{
    angle_t angle;
    int damage;
    int slope;

    damage = P_Random_Mul2_Mod10_Plus1;
    angle = players_mo->angle;
    angle += (P_Random_Minus_P_Random) << 18;
    prndindex += 2;

    // use meleerange + 1 se the puff doesn't skip the flash
    slope = P_AimLineAttack(players_mo, angle, MELEERANGE + 1);
    P_LineAttack(players_mo, angle, MELEERANGE + 1, slope, damage);

    if (!linetarget)
    {
        S_StartSound(players_mo, sfx_sawful);
        return;
    }
    S_StartSound(players_mo, sfx_sawhit);
    players_mo->prevangle = players_mo->angle;
    // turn to face target
    angle = R_PointToAngle2(players_mo->x, players_mo->y,
                            linetarget->x, linetarget->y);
    if (angle - players_mo->angle > ANG180)
    {
        if (angle - players_mo->angle < -ANG90 / 20)
            players_mo->angle = angle + ANG90 / 21;
        else
            players_mo->angle -= ANG90 / 20;
    }
    else
    {
        if (angle - players_mo->angle > ANG90 / 20)
            players_mo->angle = angle - ANG90 / 21;
        else
            players_mo->angle += ANG90 / 20;
    }
    players_mo->flags |= MF_JUSTATTACKED;
}

//
// A_FireMissile
//
void A_FireMissile(player_t *player,
                   pspdef_t *psp)
{
    players.ammo[weaponinfo[players.readyweapon].ammo]--;
    P_SpawnPlayerMissile(MT_ROCKET);
}

//
// A_FireBFG
//
void A_FireBFG(player_t *player,
               pspdef_t *psp)
{
    players.ammo[weaponinfo[players.readyweapon].ammo] -= BFGCELLS;
    P_SpawnPlayerMissile(MT_BFG);
}

//
// A_FirePlasma
//
void A_FirePlasma(player_t *player,
                  pspdef_t *psp)
{
    players.ammo[weaponinfo[players.readyweapon].ammo]--;

    P_SetPsprite(ps_flash, weaponinfo[players.readyweapon].flashstate + (P_Random_And1));

    P_SpawnPlayerMissile(MT_PLASMA);
}

//
// P_BulletSlope
// Sets a slope so a near miss is at aproximately
// the height of the intended target
//
fixed_t bulletslope;

void P_BulletSlope(void)
{
    angle_t an;

    // see which target is to be aimed at
    an = players_mo->angle;
    bulletslope = P_AimLineAttack(players_mo, an, HALFMISSILERANGE);

    if (!linetarget)
    {
        an += 1 << 26;
        bulletslope = P_AimLineAttack(players_mo, an, HALFMISSILERANGE);
        if (!linetarget)
        {
            an -= 2 << 26;
            bulletslope = P_AimLineAttack(players_mo, an, HALFMISSILERANGE);
        }
    }
}

//
// P_GunShot
//
void P_GunShot(boolean accurate)
{
    angle_t angle;
    int damage;

    damage = P_Random_Mul5_Mod3_Plus1;
    angle = players_mo->angle;

    if (!accurate)
    {
        angle += (P_Random_Minus_P_Random) << 18;
        prndindex += 2;
    }
        

    P_LineAttack(players_mo, angle, MISSILERANGE, bulletslope, damage);
}

//
// A_FirePistol
//
void A_FirePistol(player_t *player,
                  pspdef_t *psp)
{
    S_StartSound(players_mo, sfx_pistol);

    P_NotSetMobjState(players_mo, S_PLAY_ATK2);
    players.ammo[weaponinfo[players.readyweapon].ammo]--;

    P_SetPsprite(ps_flash, weaponinfo[players.readyweapon].flashstate);

    P_BulletSlope();
    P_GunShot(!players.refire);
}

//
// A_FireShotgun
//
void A_FireShotgun(player_t *player,
                   pspdef_t *psp)
{
    S_StartSound(players_mo, sfx_shotgn);
    P_NotSetMobjState(players_mo, S_PLAY_ATK2);

    players.ammo[weaponinfo[players.readyweapon].ammo]--;

    P_SetPsprite(ps_flash, weaponinfo[players.readyweapon].flashstate);

    P_BulletSlope();

    P_GunShot(false);
    P_GunShot(false);
    P_GunShot(false);
    P_GunShot(false);
    P_GunShot(false);
    P_GunShot(false);
    P_GunShot(false);
}

//
// A_FireShotgun2
//
void A_FireShotgun2(player_t *player,
                    pspdef_t *psp)
{
    int i;
    angle_t angle;
    int damage;

    S_StartSound(players_mo, sfx_dshtgn);
    P_NotSetMobjState(players_mo, S_PLAY_ATK2);

    players.ammo[weaponinfo[players.readyweapon].ammo] -= 2;

    P_SetPsprite(ps_flash, weaponinfo[players.readyweapon].flashstate);

    P_BulletSlope();

    for (i = 0; i < 20; i++)
    {
        int randomValue;

        damage = P_Random_Mul5_Mod3_Plus1;
        angle = players_mo->angle;
        angle += (P_Random_Minus_P_Random) << 19;
        prndindex += 2;

        randomValue = (P_Random_Minus_P_Random);
        prndindex += 2;

        P_LineAttack(players_mo, angle, MISSILERANGE, bulletslope + (randomValue << 5), damage);
    }
}

//
// A_FireCGun
//
void A_FireCGun(player_t *player,
                pspdef_t *psp)
{
    if (!players.ammo[weaponinfo[players.readyweapon].ammo])
        return;

    S_StartSound(players_mo, sfx_pistol);

    P_NotSetMobjState(players_mo, S_PLAY_ATK2);
    players.ammo[weaponinfo[players.readyweapon].ammo]--;

    P_SetPsprite(ps_flash, weaponinfo[players.readyweapon].flashstate + psp->state - &states[S_CHAIN1]);

    P_BulletSlope();

    P_GunShot(!players.refire);
}

//
// ?
//
void A_Light0(player_t *player, pspdef_t *psp)
{
    players.extralight = 0;
}

void A_Light1(player_t *player, pspdef_t *psp)
{
    players.extralight = 1;
}

void A_Light2(player_t *player, pspdef_t *psp)
{
    players.extralight = 2;
}

//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//
void A_BFGSpray(mobj_t *mo)
{
    int i;
    int j;
    int damage;
    angle_t an;

    an = mo->angle - ANG90 / 2;

    // offset angles from its attack angle
    for (i = 0; i < 40; i++)
    {
        // mo->target is the originator (player)
        //  of the missile
        P_AimLineAttack(mo->target, an, HALFMISSILERANGE);

        an += ANG90 / 40;

        if (!linetarget)
            continue;

        P_SpawnMobj(linetarget->x,
                    linetarget->y,
                    linetarget->z + (linetarget->height >> 2),
                    MT_EXTRABFG);

        damage = (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1) + (P_Random_And7_Plus1);

        P_DamageMobj(linetarget, mo->target, mo->target, damage);
    }
}

//
// A_BFGsound
//
void A_BFGsound(player_t *player,
                pspdef_t *psp)
{
    S_StartSound(players_mo, sfx_bfg);
}

//
// P_SetupPsprites
// Called at start of level for each player.
//
void P_SetupPsprites(void)
{
    int i;

    // remove all psprites
    players.psprites[ps_weapon].state = NULL;
    players.psprites[ps_flash].state = NULL;

    // spawn the gun
    players.pendingweapon = players.readyweapon;
    P_BringUpWeapon();
}

//
// P_MovePsprites
// Called every tic by player thinking routine.
//
void P_MovePsprites(void)
{
    int i;
    pspdef_t *psp;
    state_t *state;

    psp = &players.psprites[ps_weapon];

    // a null state means not active
    if ((state = psp->state))
    {
        // drop tic count and possibly change state

        // a -1 tic count never changes
        if (psp->tics != -1)
        {
            psp->tics--;
            if (!psp->tics)
                P_SetPsprite(ps_weapon, psp->state->nextstate);
        }
    }

    psp++;

    if ((state = psp->state))
    {
        // drop tic count and possibly change state

        // a -1 tic count never changes
        if (psp->tics != -1)
        {
            psp->tics--;
            if (!psp->tics)
                P_SetPsprite(ps_flash, psp->state->nextstate);
        }
    }

    players.psprites[ps_flash].sx = players.psprites[ps_weapon].sx;
    players.psprites[ps_flash].sy = players.psprites[ps_weapon].sy;
}
