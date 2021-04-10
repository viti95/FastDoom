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
//	Game completion, final screen animation.
//

#include <string.h>
#include <stdio.h>

#include "std_func.h"

// Functions.
#include "i_system.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"
#include "r_state.h"

#include "vmode.h"

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
int finalestage;

int finalecount;

#define TEXTSPEED 3
#define TEXTWAIT 250

char *finaletext;
char *finaleflat;

void F_StartCast(void);
void F_CastTicker(void);
byte F_CastResponder(event_t *ev);
void F_CastDrawer(void);
void F_CastDrawerText(void);

int F_GetFileSize(char *filename)
{
	int result;
	FILE *fp;

	fp = fopen(filename, "r");

	if (fp == NULL)
		return -1;

	fseek(fp, 0L, SEEK_END);
	result = ftell(fp);
	fclose(fp);

	return result;
}

int F_ReadTextFile(char *dest, char *filename, int size)
{
	FILE *fp;

	fp = fopen(filename, "r");
	if (fp == NULL)
		return -1;

	fread(dest, size, 1, fp);
	fclose(fp);
	dest[size] = '\0';

	return 0;
}

void F_LoadFinaleText(char *filename)
{
	int size;

	size = F_GetFileSize(filename);

	finaletext = (char *)Z_MallocUnowned(size + 1, PU_CACHE);
	F_ReadTextFile(finaletext, filename, size);
}

//
// F_StartFinale
//
void F_StartFinale(void)
{
	int finalemusic;

	gameaction = ga_nothing;
	gamestate = GS_FINALE;
	viewactive = 0;
	automapactive = 0;

	if (gamemode == commercial)
	{
		if (gamemission == pack_plut)
		{
			switch (gamemap)
			{
			case 6:
				finaleflat = "SLIME16";
				F_LoadFinaleText("INTER\\P1.TXT");
				break;
			case 11:
				finaleflat = "RROCK14";
				F_LoadFinaleText("INTER\\P2.TXT");
				break;
			case 20:
				finaleflat = "RROCK07";
				F_LoadFinaleText("INTER\\P3.TXT");
				break;
			case 30:
				finaleflat = "RROCK17";
				F_LoadFinaleText("INTER\\P4.TXT");
				break;
			case 15:
				finaleflat = "RROCK13";
				F_LoadFinaleText("INTER\\P5.TXT");
				break;
			case 31:
				finaleflat = "RROCK19";
				F_LoadFinaleText("INTER\\P6.TXT");
				break;
			default:
				// Ouch.
				break;
			}
		}
		else if (gamemission == pack_tnt)
		{
			switch (gamemap)
			{
			case 6:
				finaleflat = "SLIME16";
				F_LoadFinaleText("INTER\\T1.TXT");
				break;
			case 11:
				finaleflat = "RROCK14";
				F_LoadFinaleText("INTER\\T2.TXT");
				break;
			case 20:
				finaleflat = "RROCK07";
				F_LoadFinaleText("INTER\\T3.TXT");
				break;
			case 30:
				finaleflat = "RROCK17";
				F_LoadFinaleText("INTER\\T4.TXT");
				break;
			case 15:
				finaleflat = "RROCK13";
				F_LoadFinaleText("INTER\\T5.TXT");
				break;
			case 31:
				finaleflat = "RROCK19";
				F_LoadFinaleText("INTER\\T6.TXT");
				break;
			default:
				// Ouch.
				break;
			}
		}
		else
		{
			// DOOM II and missions packs with E1, M34
			switch (gamemap)
			{
			case 6:
				finaleflat = "SLIME16";
				F_LoadFinaleText("INTER\\C1.TXT");
				break;
			case 11:
				finaleflat = "RROCK14";
				F_LoadFinaleText("INTER\\C2.TXT");
				break;
			case 20:
				finaleflat = "RROCK07";
				F_LoadFinaleText("INTER\\C3.TXT");
				break;
			case 30:
				finaleflat = "RROCK17";
				F_LoadFinaleText("INTER\\C4.TXT");
				break;
			case 15:
				finaleflat = "RROCK13";
				F_LoadFinaleText("INTER\\C5.TXT");
				break;
			case 31:
				finaleflat = "RROCK19";
				F_LoadFinaleText("INTER\\C6.TXT");
				break;
			default:
				// Ouch.
				break;
			}
		}
		finalemusic = mus_read_m;
	}
	else
	{
		// DOOM 1 - E1, E3 or E4, but each nine missions
		switch (gameepisode)
		{
		case 1:
			finaleflat = "FLOOR4_8";
			F_LoadFinaleText("INTER\\E1.TXT");
			break;
		case 2:
			finaleflat = "SFLR6_1";
			F_LoadFinaleText("INTER\\E2.TXT");
			break;
		case 3:
			finaleflat = "MFLR8_4";
			F_LoadFinaleText("INTER\\E3.TXT");
			break;
		case 4:
			finaleflat = "MFLR8_3";
			F_LoadFinaleText("INTER\\E4.TXT");
			break;
		default:
			// Ouch.
			break;
		}
		finalemusic = mus_victor;
	}

	S_ChangeMusic(finalemusic, true);
	finalestage = 0;
	finalecount = 0;
}

byte F_Responder(event_t *event)
{
	if (finalestage == 2)
		return F_CastResponder(event);

	return 0;
}

//
// F_Ticker
//
void F_Ticker(void)
{
	int i;

	// check for skipping
	if ((gamemode == commercial) && (finalecount > 50))
	{
		// go on to the next level
		if (players.cmd.buttons)
		{
			if (gamemap == 30)
				F_StartCast();
			else
				gameaction = ga_worlddone;
		}
	}

	// advance animation
	finalecount++;

	if (finalestage == 2)
	{
		F_CastTicker();
		return;
	}

	if (gamemode == commercial)
		return;

	if (!finalestage && finalecount > strlen(finaletext) * TEXTSPEED + TEXTWAIT)
	{
		finalecount = 0;
		finalestage = 1;
		wipegamestate = -1; // force a wipe
		if (gameepisode == 3)
			S_ChangeMusic(mus_bunny, false);
	}
}

//
// F_TextWrite
//

#include "hu_stuff.h"
extern patch_t *hu_font[HU_FONTSIZE];

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void F_TextWrite(void)
{
	byte *src;
	byte *dest;

	int x, y, w;
	int count;
	char *ch;
	int c;
	int cx;
	int cy;

	// erase the entire screen to a tiled background
	src = W_CacheLumpName(finaleflat, PU_CACHE);
	dest = screen0;

	for (y = 0; y < SCREENHEIGHT; y++)
	{
		for (x = 0; x < SCREENWIDTH / 64; x++)
		{
			CopyDWords(src + ((y & 63) << 6), dest, 16);
			//memcpy(dest, src + ((y & 63) << 6), 64);
			dest += 64;
		}
	}

	V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);

	// draw some of the text onto the screen
	cx = 10;
	cy = 10;
	ch = finaletext;

	count = (finalecount - 10) / 3;
	if (count < 0)
		count = 0;
	for (; count; count--)
	{
		c = *ch++;
		if (!c)
			break;
		if (c == '\n')
		{
			cx = 10;
			cy += 11;
			continue;
		}

		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c > HU_FONTSIZE)
		{
			cx += 4;
			continue;
		}

		w = hu_font[c]->width;
		if (cx + w > SCREENWIDTH)
			break;
		V_DrawPatchScreen0(cx, cy, hu_font[c]);
		cx += w;
	}
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25) || (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
void F_TextWriteText(void)
{
	int x, y, w;
	int count;
	char *ch;
	int c;
	int cx;
	int cy;

	// erase the entire screen to a tiled background

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
	SetWords(textdestscreen, 0, 80 * 25);
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
	SetWords(textdestscreen, 0, 80 * 50);
#endif

	// draw some of the text onto the screen
	cx = 1;
	cy = 1;
	ch = finaletext;

	count = Div3(finalecount - 10);
	if (count < 0)
		count = 0;
	for (; count; count--)
	{
		c = *ch++;
		if (!c)
			break;
		if (c == '\n')
		{
			cx = 1;
			cy += 1;
			continue;
		}

		if (c < 32)
		{
			cx += 1;
			continue;
		}

		if (cx + 1 > 80)
			break;

		V_WriteCharDirect(cx, cy, (unsigned char)c);
		cx += 1;
	}
}
#endif

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
typedef struct
{
	char *name;
	mobjtype_t type;
} castinfo_t;

castinfo_t castorder[] = {
	{CC_ZOMBIE, MT_POSSESSED},
	{CC_SHOTGUN, MT_SHOTGUY},
	{CC_HEAVY, MT_CHAINGUY},
	{CC_IMP, MT_TROOP},
	{CC_DEMON, MT_SERGEANT},
	{CC_LOST, MT_SKULL},
	{CC_CACO, MT_HEAD},
	{CC_HELL, MT_KNIGHT},
	{CC_BARON, MT_BRUISER},
	{CC_ARACH, MT_BABY},
	{CC_PAIN, MT_PAIN},
	{CC_REVEN, MT_UNDEAD},
	{CC_MANCU, MT_FATSO},
	{CC_ARCH, MT_VILE},
	{CC_SPIDER, MT_SPIDER},
	{CC_CYBER, MT_CYBORG},
	{CC_HERO, MT_PLAYER},
	{NULL, 0}};

int castnum;
int casttics;
state_t *caststate;
byte castdeath;
int castframes;
int castonmelee;
byte castattacking;

//
// F_StartCast
//
extern gamestate_t wipegamestate;

void F_StartCast(void)
{
	wipegamestate = -1; // force a screen wipe
	castnum = 0;
	caststate = &states[mobjinfo[castorder[castnum].type].seestate];
	casttics = caststate->tics;
	castdeath = 0;
	finalestage = 2;
	castframes = 0;
	castonmelee = 0;
	castattacking = 0;
	S_ChangeMusic(mus_evil, true);
}

//
// F_CastTicker
//
void F_CastTicker(void)
{
	int st;
	int sfx;

	if (--casttics > 0)
		return; // not time to change state yet

	if (caststate->tics == -1 || caststate->nextstate == S_NULL)
	{
		// switch from deathstate to next monster
		castnum++;
		castdeath = 0;
		if (castorder[castnum].name == NULL)
			castnum = 0;
		if (mobjinfo[castorder[castnum].type].seesound)
			S_StartSound(NULL, mobjinfo[castorder[castnum].type].seesound);
		caststate = &states[mobjinfo[castorder[castnum].type].seestate];
		castframes = 0;
	}
	else
	{
		// just advance to next state in animation
		if (caststate == &states[S_PLAY_ATK1])
			goto stopattack; // Oh, gross hack!
		st = caststate->nextstate;
		caststate = &states[st];
		castframes++;

		// sound hacks....
		switch (st)
		{
		case S_PLAY_ATK1:
			sfx = sfx_dshtgn;
			break;
		case S_POSS_ATK2:
			sfx = sfx_pistol;
			break;
		case S_SPOS_ATK2:
			sfx = sfx_shotgn;
			break;
		case S_VILE_ATK2:
			sfx = sfx_vilatk;
			break;
		case S_SKEL_FIST2:
			sfx = sfx_skeswg;
			break;
		case S_SKEL_FIST4:
			sfx = sfx_skepch;
			break;
		case S_SKEL_MISS2:
			sfx = sfx_skeatk;
			break;
		case S_FATT_ATK8:
		case S_FATT_ATK5:
		case S_FATT_ATK2:
			sfx = sfx_firsht;
			break;
		case S_CPOS_ATK2:
		case S_CPOS_ATK3:
		case S_CPOS_ATK4:
			sfx = sfx_shotgn;
			break;
		case S_TROO_ATK3:
			sfx = sfx_claw;
			break;
		case S_SARG_ATK2:
			sfx = sfx_sgtatk;
			break;
		case S_BOSS_ATK2:
		case S_BOS2_ATK2:
		case S_HEAD_ATK2:
			sfx = sfx_firsht;
			break;
		case S_SKULL_ATK2:
			sfx = sfx_sklatk;
			break;
		case S_SPID_ATK2:
		case S_SPID_ATK3:
			sfx = sfx_shotgn;
			break;
		case S_BSPI_ATK2:
			sfx = sfx_plasma;
			break;
		case S_CYBER_ATK2:
		case S_CYBER_ATK4:
		case S_CYBER_ATK6:
			sfx = sfx_rlaunc;
			break;
		case S_PAIN_ATK3:
			sfx = sfx_sklatk;
			break;
		default:
			sfx = 0;
			break;
		}

		if (sfx)
			S_StartSound(NULL, sfx);
	}

	if (castframes == 12)
	{
		// go into attack frame
		castattacking = 1;
		if (castonmelee)
			caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
		else
			caststate = &states[mobjinfo[castorder[castnum].type].missilestate];
		castonmelee ^= 1;
		if (caststate == &states[S_NULL])
		{
			if (castonmelee)
				caststate =
					&states[mobjinfo[castorder[castnum].type].meleestate];
			else
				caststate =
					&states[mobjinfo[castorder[castnum].type].missilestate];
		}
	}

	if (castattacking)
	{
		if (castframes == 24 || caststate == &states[mobjinfo[castorder[castnum].type].seestate])
		{
		stopattack:
			castattacking = 0;
			castframes = 0;
			caststate = &states[mobjinfo[castorder[castnum].type].seestate];
		}
	}

	casttics = caststate->tics;
	if (casttics == -1)
		casttics = 15;
}

//
// F_CastResponder
//

byte F_CastResponder(event_t *ev)
{
	if (ev->type != ev_keydown)
		return 0;

	if (castdeath)
		return 1; // already in dying frames

	// go into death frame
	castdeath = 1;
	caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
	casttics = caststate->tics;
	castframes = 0;
	castattacking = 0;
	if (mobjinfo[castorder[castnum].type].deathsound)
		S_StartSound(NULL, mobjinfo[castorder[castnum].type].deathsound);

	return 1;
}

void F_CastPrint(char *text)
{
	char *ch;
	int c;
	int cx;
	int w;
	int width;

	// find width
	ch = text;
	width = 0;

	while (ch)
	{
		c = *ch++;
		if (!c)
			break;
		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c > HU_FONTSIZE)
		{
			width += 4;
			continue;
		}

		w = hu_font[c]->width;
		width += w;
	}

	// draw it
	cx = 160 - width / 2;
	ch = text;
	while (ch)
	{
		c = *ch++;
		if (!c)
			break;
		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c > HU_FONTSIZE)
		{
			cx += 4;
			continue;
		}

		w = hu_font[c]->width;
		V_DrawPatchScreen0(cx, 180, hu_font[c]);
		cx += w;
	}
}

//
// F_CastDrawer
//
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void F_CastDrawer(void)
{
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	int lump;
	byte flip;
	patch_t *patch;

	// erase the entire screen to a background
	V_DrawPatchScreen0(0, 0, W_CacheLumpName("BOSSBACK", PU_CACHE));

	F_CastPrint(castorder[castnum].name);

	// draw the current frame in the middle of the screen
	sprdef = &sprites[caststate->sprite];
	sprframe = &sprdef->spriteframes[caststate->frame & FF_FRAMEMASK];
	lump = sprframe->lump[0];
	flip = sprframe->flip[0];

	patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);
	if (flip)
		V_DrawPatchFlippedScreen0(160, 170, patch);
	else
		V_DrawPatchScreen0(160, 170, patch);
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25) || (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
void F_CastDrawerText(void)
{
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	int lump;
	patch_t *patch;

// erase the entire screen to a background
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
	V_DrawPatchDirectText8025(0, 0, W_CacheLumpName("BOSSBACK", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
	V_DrawPatchDirectText8050(0, 0, W_CacheLumpName("BOSSBACK", PU_CACHE));
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
	V_WriteTextDirect(40 - strlen(castorder[castnum].name) / 2, 23, castorder[castnum].name);
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
	V_WriteTextDirect(40 - strlen(castorder[castnum].name) / 2, 48, castorder[castnum].name);
#endif

	// draw the current frame in the middle of the screen
	sprdef = &sprites[caststate->sprite];
	sprframe = &sprdef->spriteframes[caststate->frame & FF_FRAMEMASK];
	lump = sprframe->lump[0];

	patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
	V_DrawPatchDirectText8025(160, 170, patch);
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
	V_DrawPatchDirectText8050(160, 170, patch);
#endif
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void F_DrawPatchCol(int x, patch_t *patch, int col)
{
	column_t *column;
	byte *source;
	byte *dest;
	byte *desttop;
	int count;

	column = (column_t *)((byte *)patch + patch->columnofs[col]);
	desttop = screen0 + x;

	// step through the posts in a column
	while (column->topdelta != 0xff)
	{
		source = (byte *)column + 3;
		dest = desttop + Mul320(column->topdelta);
		count = column->length;

		while (count--)
		{
			*dest = *source++;
			dest += SCREENWIDTH;
		}
		column = (column_t *)((byte *)column + column->length + 4);
	}
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
void F_DrawPatchColText8025(int x, patch_t *patch, int col)
{
	column_t *column;
	byte *source;
	unsigned short *desttop;
	unsigned short *dest;
	int count;
	byte odd;
	unsigned short vmem;

	column = (column_t *)((byte *)patch + patch->columnofs[col]);
	desttop = textdestscreen + x / 4;

	// step through the posts in a column
	while (column->topdelta != 0xff)
	{
		source = (byte *)column + 3;
		odd = (column->topdelta / 4) % 2;
		dest = desttop + Mul80(column->topdelta / 8);
		count = column->length / 4;

		while (count--)
		{
			vmem = *dest;

			if (odd)
			{
				vmem = vmem & 0x0F00;
				*dest = vmem | lut16colors[*source] << 12 | 223;

				odd = 0;
				dest += 80;
			}
			else
			{
				vmem = vmem & 0xF000;
				*dest = vmem | lut16colors[*source] << 8 | 223;

				odd = 1;
			}

			source += 4;
		}
		column = (column_t *)((byte *)column + column->length + 4);
	}

	desttop += 1;
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
void F_DrawPatchColText8050(int x, patch_t *patch, int col)
{
	column_t *column;
	byte *source;
	unsigned short *desttop;
	unsigned short *dest;
	int count;
	unsigned short vmem;

	column = (column_t *)((byte *)patch + patch->columnofs[col]);
	desttop = textdestscreen + x / 4;

	// step through the posts in a column
	while (column->topdelta != 0xff)
	{
		source = (byte *)column + 3;
		dest = desttop + Mul80(column->topdelta / 4);
		count = column->length / 4;

		while (count--)
		{
			*dest = lut16colors[*source] << 8 | 219;
			source += 4;
			dest += 80;
		}
		column = (column_t *)((byte *)column + column->length + 4);
	}

	desttop += 1;
}
#endif

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25) || (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
void F_BunnyScrollText(void)
{
	int scrolled;
	int x;
	patch_t *p1;
	patch_t *p2;
	char name[10];
	int stage;
	static int laststage;

	p1 = W_CacheLumpName("PFUB2", PU_LEVEL);
	p2 = W_CacheLumpName("PFUB1", PU_LEVEL);

	scrolled = 320 - (finalecount - 230) / 2;

	if (scrolled > 320)
		scrolled = 320;
	else if (scrolled < 0)
		scrolled = 0;

	for (x = 0; x < SCREENWIDTH; x++)
	{
		if (x + scrolled < 320)
		{
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
			F_DrawPatchColText8025(x, p1, x + scrolled);
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
			F_DrawPatchColText8050(x, p1, x + scrolled);
#endif
		}
		else
		{
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
			F_DrawPatchColText8025(x, p2, x + scrolled - 320);
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
			F_DrawPatchColText8050(x, p2, x + scrolled - 320);
#endif
		}
	}

	if (finalecount < 1130)
		return;
	if (finalecount < 1180)
	{
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
		V_WriteTextDirect(37, 12, "THE END");
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
		V_WriteTextDirect(37, 25, "THE END");
#endif
		laststage = 0;
		return;
	}

	stage = (finalecount - 1180) / 5;
	if (stage > 6)
		stage = 6;
	if (stage > laststage)
	{
		S_StartSound(NULL, sfx_pistol);
		laststage = stage;
	}

#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
	V_WriteTextDirect(37, 12, "THE END");
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
	V_WriteTextDirect(37, 25, "THE END");
#endif
}
#endif

//
// F_BunnyScroll
//
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
void F_BunnyScroll(void)
{
	int scrolled;
	int x;
	patch_t *p1;
	patch_t *p2;
	char name[10];
	int stage;
	static int laststage;

	p1 = W_CacheLumpName("PFUB2", PU_LEVEL);
	p2 = W_CacheLumpName("PFUB1", PU_LEVEL);

	V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);

	scrolled = 320 - (finalecount - 230) / 2;

	if (scrolled > 320)
		scrolled = 320;
	else if (scrolled < 0)
		scrolled = 0;

	for (x = 0; x < SCREENWIDTH; x++)
	{
		if (x + scrolled < 320)
			F_DrawPatchCol(x, p1, x + scrolled);
		else
			F_DrawPatchCol(x, p2, x + scrolled - 320);
	}

	if (finalecount < 1130)
		return;
	if (finalecount < 1180)
	{
		V_DrawPatchScreen0((SCREENWIDTH - 13 * 8) / 2, (SCREENHEIGHT - 8 * 8) / 2, W_CacheLumpName("END0", PU_CACHE));
		laststage = 0;
		return;
	}

	stage = (finalecount - 1180) / 5;
	if (stage > 6)
		stage = 6;
	if (stage > laststage)
	{
		S_StartSound(NULL, sfx_pistol);
		laststage = stage;
	}

	sprintf(name, "END%i", stage);
	V_DrawPatchScreen0((SCREENWIDTH - 13 * 8) / 2, (SCREENHEIGHT - 8 * 8) / 2, W_CacheLumpName(name, PU_CACHE));
}
#endif

//
// F_Drawer
//
void F_Drawer(void)
{
	if (finalestage == 2)
	{
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
		F_CastDrawer();
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25) || (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
		F_CastDrawerText();
#endif
		return;
	}

	if (!finalestage)
	{
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25) || (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
		F_TextWriteText();
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
		F_TextWrite();
#endif
	}
	else
	{
		switch (gameepisode)
		{
		case 1:
			if (gamemode == shareware)
			{
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
				V_DrawPatchDirectText8025(0, 0, W_CacheLumpName("HELP2", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
				V_DrawPatchDirectText8050(0, 0, W_CacheLumpName("HELP2", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
				V_DrawPatchScreen0(0, 0, W_CacheLumpName("HELP2", PU_CACHE));
#endif
			}
			else
			{
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
				V_DrawPatchDirectText8025(0, 0, W_CacheLumpName("CREDIT", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
				V_DrawPatchDirectText8050(0, 0, W_CacheLumpName("CREDIT", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
				V_DrawPatchScreen0(0, 0, W_CacheLumpName("CREDIT", PU_CACHE));
#endif
			}

			break;
		case 2:
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
			V_DrawPatchDirectText8025(0, 0, W_CacheLumpName("VICTORY2", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
			V_DrawPatchDirectText8050(0, 0, W_CacheLumpName("VICTORY2", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
			V_DrawPatchScreen0(0, 0, W_CacheLumpName("VICTORY2", PU_CACHE));
#endif

			break;
		case 3:
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
			F_BunnyScroll();
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25) || (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
			F_BunnyScrollText();
#endif
			break;
		case 4:
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X25)
			V_DrawPatchDirectText8025(0, 0, W_CacheLumpName("ENDPIC", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_80X50)
			V_DrawPatchDirectText8050(0, 0, W_CacheLumpName("ENDPIC", PU_CACHE));
#endif
#if (EXE_VIDEOMODE == EXE_VIDEOMODE_Y)
			V_DrawPatchScreen0(0, 0, W_CacheLumpName("ENDPIC", PU_CACHE));
#endif

			break;
		}
	}
}
