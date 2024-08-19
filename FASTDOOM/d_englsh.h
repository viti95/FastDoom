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
//	Printed strings for translation.
//	English language support (default).
//

#ifndef __D_ENGLSH__
#define __D_ENGLSH__

#include "version.h"

//
//	Printed strings for translation
//

//
//	M_Menu.C
//
#define PRESSKEY "press a key."
#define PRESSYN "press y or n."
#define QUITMSG "are you sure you want to\nquit this great game?"
#define QSAVESPOT "you haven't picked a quicksave slot yet!\n\n" PRESSKEY
#define SAVEDEAD "you can't save if you aren't playing!\n\n" PRESSKEY
#define QSPROMPT "quicksave over your game named\n\n'%s'?\n\n" PRESSYN
#define QLPROMPT "do you want to quickload the game named\n\n'%s'?\n\n" PRESSYN

#define NIGHTMARE                      \
    "are you sure? this skill level\n" \
    "isn't even remotely fair.\n\n" PRESSYN

#define SWSTRING                                 \
    "this is the shareware version of doom.\n\n" \
    "you need to order the entire trilogy.\n\n" PRESSKEY

#define FASTDOOMHELP                            \
    "fastdoom " FDOOMVERSION " by ViTi95\n\n"             \
    "thanks to all contributors:\n\n"           \
    "JNechaevsky    RamonUnch    PickledDog\n"  \
    "jsmolina    deat322    FrenkelS\n"         \
    "AXDOOMER    FavoritoHJS    Tronix286\n"    \
    "DeathEgg    CarlosTex    noop0x90\n"       \
    "FreddoUK    maximilien-noal    appiah4\n"  \
    "ghderty    Redneckerz\n"                   \
    "SpitFire-666    Optimus6128    viciious\n" \
    "Shogun38    cher-nov    neuralnetworkz\n"  \
    "Ethaniel-404    zokum-no    rasz_pl\n"     \
    "Doug Johnson    crazii     Mikolaj Feliks\n" \
    "bnied    javiergutierrezchamorro\n\n"      \
    "and to the DoomWorld / Vogons communities\n\n" \
    "rip and tear, Bonepie"

#define MSGOFF "Messages OFF"
#define MSGON "Messages ON"
#define ENDGAME "are you sure you want to end the game?\n\n" PRESSYN

#define DOSY "(press y to quit to dos.)"

#define DETAILHI "High detail"
#define DETAILLO "Low detail"
#define DETAILPO "Potato detail"
#define EMPTYSTRING "empty slot"
#define AUTORUNON "Autorun enabled"
#define AUTORUNOFF "Autorun disabled"

//
//	P_inter.C
//
#define GOTARMOR "Picked up the armor."
#define GOTMEGA "Picked up the MegaArmor!"
#define GOTHTHBONUS "Picked up a health bonus."
#define GOTARMBONUS "Picked up an armor bonus."
#define GOTSTIM "Picked up a stimpack."
#define GOTMEDIKIT "Picked up a medikit."
#define GOTSUPER "Supercharge!"

#define GOTBLUECARD "Picked up a blue keycard."
#define GOTYELWCARD "Picked up a yellow keycard."
#define GOTREDCARD "Picked up a red keycard."
#define GOTBLUESKUL "Picked up a blue skull key."
#define GOTYELWSKUL "Picked up a yellow skull key."
#define GOTREDSKULL "Picked up a red skull key."

#define GOTINVUL "Invulnerability!"
#define GOTBERSERK "Berserk!"
#define GOTINVIS "Partial Invisibility"
#define GOTSUIT "Radiation Shielding Suit"
#define GOTMAP "Computer Area Map"
#define GOTVISOR "Light Amplification Visor"
#define GOTMSPHERE "MegaSphere!"

#define GOTCLIP "Picked up a clip."
#define GOTCLIPBOX "Picked up a box of bullets."
#define GOTROCKET "Picked up a rocket."
#define GOTROCKBOX "Picked up a box of rockets."
#define GOTCELL "Picked up an energy cell."
#define GOTCELLBOX "Picked up an energy cell pack."
#define GOTSHELLS "Picked up 4 shotgun shells."
#define GOTSHELLBOX "Picked up a box of shotgun shells."
#define GOTBACKPACK "Picked up a backpack full of ammo!"

#define GOTBFG9000 "You got the BFG9000! Oh, yes."
#define GOTCHAINGUN "You got the chaingun!"
#define GOTCHAINSAW "A chainsaw! Find some meat!"
#define GOTLAUNCHER "You got the rocket launcher!"
#define GOTPLASMA "You got the plasma gun!"
#define GOTSHOTGUN "You got the shotgun!"
#define GOTSHOTGUN2 "You got the super shotgun!"

//
// P_Doors.C
//
#define PD_BLUEO "You need a blue key to activate this object"
#define PD_REDO "You need a red key to activate this object"
#define PD_YELLOWO "You need a yellow key to activate this object"
#define PD_BLUEK "You need a blue key to open this door"
#define PD_REDK "You need a red key to open this door"
#define PD_YELLOWK "You need a yellow key to open this door"

//
//	G_game.C
//
#define GGSAVED "game saved."

// The following should NOT be changed unless it seems
// just AWFULLY necessary

#define HUSTR_PLRGREEN "Green: "

//
//	AM_map.C
//

#define AMSTR_FOLLOWON "Follow Mode ON"
#define AMSTR_FOLLOWOFF "Follow Mode OFF"

#define AMSTR_GRIDON "Grid ON"
#define AMSTR_GRIDOFF "Grid OFF"

#define AMSTR_TRANSON "Transparent Map ON"
#define AMSTR_TRANSOFF "Transparent Map OFF"

#define AMSTR_MARKEDSPOT "Marked Spot"
#define AMSTR_MARKSCLEARED "All Marks Cleared"

//
//	ST_stuff.C
//

#define STSTR_MUS "Music Change"
#define STSTR_NOMUS "IMPOSSIBLE SELECTION"
#define STSTR_DQDON "Degreelessness Mode On"
#define STSTR_DQDOFF "Degreelessness Mode Off"

#define STSTR_KFAADDED "Very Happy Ammo Added"
#define STSTR_FAADDED "Ammo (no keys) Added"

#define STSTR_NCON "No Clipping Mode ON"
#define STSTR_NCOFF "No Clipping Mode OFF"

#define STSTR_BEHOLD "inVuln, Str, Inviso, Rad, Allmap, or Lite-amp"
#define STSTR_BEHOLDX "Power-up Toggled"

#define STSTR_CHOPPERS "... doesn't suck - GM"
#define STSTR_CLEV "Changing Level..."

//
// Character cast strings F_FINALE.C
//
#define CC_ZOMBIE "ZOMBIEMAN"
#define CC_SHOTGUN "SHOTGUN GUY"
#define CC_HEAVY "HEAVY WEAPON DUDE"
#define CC_IMP "IMP"
#define CC_DEMON "DEMON"
#define CC_LOST "LOST SOUL"
#define CC_CACO "CACODEMON"
#define CC_HELL "HELL KNIGHT"
#define CC_BARON "BARON OF HELL"
#define CC_ARACH "ARACHNOTRON"
#define CC_PAIN "PAIN ELEMENTAL"
#define CC_REVEN "REVENANT"
#define CC_MANCU "MANCUBUS"
#define CC_ARCH "ARCH-VILE"
#define CC_SPIDER "THE SPIDER MASTERMIND"
#define CC_CYBER "THE CYBERDEMON"
#define CC_HERO "OUR HERO"

#endif
