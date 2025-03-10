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

#define FASTDOOMHELP                                \
    "fastdoom " FDOOMVERSION " by ViTi95\n\n"       \
    "thanks to all contributors:\n\n"               \
    "JNechaevsky    RamonUnch    PickledDog\n"      \
    "jsmolina    deat322    FrenkelS    rasz_pl\n"  \
    "AXDOOMER    FavoritoHJS    Tronix286\n"        \
    "DeathEgg    CarlosTex    noop0x90\n"           \
    "FreddoUK    maximilien-noal    appiah4\n"      \
    "ghderty    Redneckerz    tigrouind\n"          \
    "SpitFire-666    Optimus6128    viciious\n"     \
    "Shogun38    cher-nov    neuralnetworkz\n"      \
    "Ethaniel-404    zokum-no    DiSCATTe\n"        \
    "Doug Johnson    crazii     Mikolaj Feliks\n"   \
    "bnied    javiergutierrezchamorro\n\n"          \
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

//
//	HU_stuff.C
//
#define HUSTR_E1M1 "E1M1: Hangar"
#define HUSTR_E1M2 "E1M2: Nuclear Plant"
#define HUSTR_E1M3 "E1M3: Toxin Refinery"
#define HUSTR_E1M4 "E1M4: Command Control"
#define HUSTR_E1M5 "E1M5: Phobos Lab"
#define HUSTR_E1M6 "E1M6: Central Processing"
#define HUSTR_E1M7 "E1M7: Computer Station"
#define HUSTR_E1M8 "E1M8: Phobos Anomaly"
#define HUSTR_E1M9 "E1M9: Military Base"

#define HUSTR_E2M1 "E2M1: Deimos Anomaly"
#define HUSTR_E2M2 "E2M2: Containment Area"
#define HUSTR_E2M3 "E2M3: Refinery"
#define HUSTR_E2M4 "E2M4: Deimos Lab"
#define HUSTR_E2M5 "E2M5: Command Center"
#define HUSTR_E2M6 "E2M6: Halls of the Damned"
#define HUSTR_E2M7 "E2M7: Spawning Vats"
#define HUSTR_E2M8 "E2M8: Tower of Babel"
#define HUSTR_E2M9 "E2M9: Fortress of Mystery"

#define HUSTR_E3M1 "E3M1: Hell Keep"
#define HUSTR_E3M2 "E3M2: Slough of Despair"
#define HUSTR_E3M3 "E3M3: Pandemonium"
#define HUSTR_E3M4 "E3M4: House of Pain"
#define HUSTR_E3M5 "E3M5: Unholy Cathedral"
#define HUSTR_E3M6 "E3M6: Mt. Erebus"
#define HUSTR_E3M7 "E3M7: Limbo"
#define HUSTR_E3M8 "E3M8: Dis"
#define HUSTR_E3M9 "E3M9: Warrens"

#define HUSTR_E4M1 "E4M1: Hell Beneath"
#define HUSTR_E4M2 "E4M2: Perfect Hatred"
#define HUSTR_E4M3 "E4M3: Sever The Wicked"
#define HUSTR_E4M4 "E4M4: Unruly Evil"
#define HUSTR_E4M5 "E4M5: They Will Repent"
#define HUSTR_E4M6 "E4M6: Against Thee Wickedly"
#define HUSTR_E4M7 "E4M7: And Hell Followed"
#define HUSTR_E4M8 "E4M8: Unto The Cruel"
#define HUSTR_E4M9 "E4M9: Fear"

#define HUSTR_1 "level 1: entryway"
#define HUSTR_2 "level 2: underhalls"
#define HUSTR_3 "level 3: the gantlet"
#define HUSTR_4 "level 4: the focus"
#define HUSTR_5 "level 5: the waste tunnels"
#define HUSTR_6 "level 6: the crusher"
#define HUSTR_7 "level 7: dead simple"
#define HUSTR_8 "level 8: tricks and traps"
#define HUSTR_9 "level 9: the pit"
#define HUSTR_10 "level 10: refueling base"
#define HUSTR_11 "level 11: 'o' of destruction!"

#define HUSTR_12 "level 12: the factory"
#define HUSTR_13 "level 13: downtown"
#define HUSTR_14 "level 14: the inmost dens"
#define HUSTR_15 "level 15: industrial zone"
#define HUSTR_16 "level 16: suburbs"
#define HUSTR_17 "level 17: tenements"
#define HUSTR_18 "level 18: the courtyard"
#define HUSTR_19 "level 19: the citadel"
#define HUSTR_20 "level 20: gotcha!"

#define HUSTR_21 "level 21: nirvana"
#define HUSTR_22 "level 22: the catacombs"
#define HUSTR_23 "level 23: barrels o' fun"
#define HUSTR_24 "level 24: the chasm"
#define HUSTR_25 "level 25: bloodfalls"
#define HUSTR_26 "level 26: the abandoned mines"
#define HUSTR_27 "level 27: monster condo"
#define HUSTR_28 "level 28: the spirit world"
#define HUSTR_29 "level 29: the living end"
#define HUSTR_30 "level 30: icon of sin"

#define HUSTR_31 "level 31: wolfenstein"
#define HUSTR_32 "level 32: grosse"

#define PHUSTR_1 "level 1: congo"
#define PHUSTR_2 "level 2: well of souls"
#define PHUSTR_3 "level 3: aztec"
#define PHUSTR_4 "level 4: caged"
#define PHUSTR_5 "level 5: ghost town"
#define PHUSTR_6 "level 6: baron's lair"
#define PHUSTR_7 "level 7: caughtyard"
#define PHUSTR_8 "level 8: realm"
#define PHUSTR_9 "level 9: abattoire"
#define PHUSTR_10 "level 10: onslaught"
#define PHUSTR_11 "level 11: hunted"

#define PHUSTR_12 "level 12: speed"
#define PHUSTR_13 "level 13: the crypt"
#define PHUSTR_14 "level 14: genesis"
#define PHUSTR_15 "level 15: the twilight"
#define PHUSTR_16 "level 16: the omen"
#define PHUSTR_17 "level 17: compound"
#define PHUSTR_18 "level 18: neurosphere"
#define PHUSTR_19 "level 19: nme"
#define PHUSTR_20 "level 20: the death domain"

#define PHUSTR_21 "level 21: slayer"
#define PHUSTR_22 "level 22: impossible mission"
#define PHUSTR_23 "level 23: tombstone"
#define PHUSTR_24 "level 24: the final frontier"
#define PHUSTR_25 "level 25: the temple of darkness"
#define PHUSTR_26 "level 26: bunker"
#define PHUSTR_27 "level 27: anti-christ"
#define PHUSTR_28 "level 28: the sewers"
#define PHUSTR_29 "level 29: odyssey of noises"
#define PHUSTR_30 "level 30: the gateway of hell"

#define PHUSTR_31 "level 31: cyberden"
#define PHUSTR_32 "level 32: go 2 it"

#define THUSTR_1 "level 1: system control"
#define THUSTR_2 "level 2: human bbq"
#define THUSTR_3 "level 3: power control"
#define THUSTR_4 "level 4: wormhole"
#define THUSTR_5 "level 5: hanger"
#define THUSTR_6 "level 6: open season"
#define THUSTR_7 "level 7: prison"
#define THUSTR_8 "level 8: metal"
#define THUSTR_9 "level 9: stronghold"
#define THUSTR_10 "level 10: redemption"
#define THUSTR_11 "level 11: storage facility"

#define THUSTR_12 "level 12: crater"
#define THUSTR_13 "level 13: nukage processing"
#define THUSTR_14 "level 14: steel works"
#define THUSTR_15 "level 15: dead zone"
#define THUSTR_16 "level 16: deepest reaches"
#define THUSTR_17 "level 17: processing area"
#define THUSTR_18 "level 18: mill"
#define THUSTR_19 "level 19: shipping/respawning"
#define THUSTR_20 "level 20: central processing"

#define THUSTR_21 "level 21: administration center"
#define THUSTR_22 "level 22: habitat"
#define THUSTR_23 "level 23: lunar mining project"
#define THUSTR_24 "level 24: quarry"
#define THUSTR_25 "level 25: baron's den"
#define THUSTR_26 "level 26: ballistyx"
#define THUSTR_27 "level 27: mount pain"
#define THUSTR_28 "level 28: heck"
#define THUSTR_29 "level 29: river styx"
#define THUSTR_30 "level 30: last call"

#define THUSTR_31 "level 31: pharaoh"
#define THUSTR_32 "level 32: caribbean"

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
