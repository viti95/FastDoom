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
//	All the clipping: columns, horizontal spans, sky columns.
//

#include <stdlib.h>

#include "i_system.h"
#include "options.h"
#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_data.h"
#include "w_wad.h"
#include "z_zone.h"

#include "std_func.h"
#include "i_debug.h"

#include <conio.h>

#include "sizeopt.h"

#define SC_INDEX 0x3C4

// OPTIMIZE: closed two sided lines as single sided

// True if any of the segs textures might be visible.
int segtextured;

// False if the back side is the same plane.
byte markfloor;
byte markceiling;

int maskedtexture;
int toptexture;
int bottomtexture;
int midtexture;

angle_t rw_normalangle;
// angle to line origin
int rw_angle1;

//
// regular wall
//
int rw_x;
int rw_stopx;
angle_t rw_centerangle;
fixed_t rw_offset;
fixed_t rw_distance;
fixed_t rw_scale;
fixed_t rw_scalestep;
fixed_t rw_midtexturemid;
fixed_t rw_toptexturemid;
fixed_t rw_bottomtexturemid;

int worldtop;
int worldbottom;
int worldhigh;
int worldlow;

fixed_t pixhigh;
fixed_t pixlow;
fixed_t pixhighstep;
fixed_t pixlowstep;

fixed_t topfrac;
fixed_t topstep;

fixed_t bottomfrac;
fixed_t bottomstep;

lighttable_t **walllights;

short *maskedtexturecol;

//
// R_RenderMaskedSegRange
//
void R_RenderMaskedSegRange(drawseg_t *ds,
							int x1,
							int x2)
{
	unsigned index;
	column_t *col;
	int lightnum;
	int texnum;

	int lump;
	int ofs;
	int tex;
	int column;

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	curline = ds->curline;
	frontsector = curline->frontsector;
	backsector = curline->backsector;
	texnum = texturetranslation[curline->sidedef->midtexture];

	lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT) + extralight;

	if (curline->v1->y == curline->v2->y)
		lightnum--;
	else if (curline->v1->x == curline->v2->x)
		lightnum++;

	// Lightnum between 0 and 15
	if (lightnum < 0)
		walllights = scalelight[0];
	else if (lightnum > LIGHTLEVELS - 1)
		walllights = scalelight[LIGHTLEVELS - 1];
	else
		walllights = scalelight[lightnum];

	maskedtexturecol = ds->maskedtexturecol;
	rw_scalestep = ds->scalestep;
	spryscale = ds->scale1 + (x1 - ds->x1) * rw_scalestep;
	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	// find positioning
	if (curline->linedef->dontPegBottom)
	{
		dc_texturemid = frontsector->floorheight > backsector->floorheight
							? frontsector->floorheight
							: backsector->floorheight;
		dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
	}
	else
	{
		dc_texturemid = frontsector->ceilingheight < backsector->ceilingheight
							? frontsector->ceilingheight
							: backsector->ceilingheight;
		dc_texturemid = dc_texturemid - viewz;
	}
	dc_texturemid += curline->sidedef->rowoffset;

	if (fixedcolormap){
		dc_colormap = fixedcolormap;
	}
		

	dc_x = x1;
	do
	{
		// calculate lighting
		if (maskedtexturecol[dc_x] != MAXSHORT)
		{
			int topscreen;
			int bottomscreen;
			fixed_t basetexturemid;

			int yl, yh;
			short mfc_x, mcc_x;

			if (!fixedcolormap)
			{
				index = spryscale >> LIGHTSCALESHIFT;
#if PIXEL_SCALING != 1
				index /= PIXEL_SCALING;
#endif

				if (index >= MAXLIGHTSCALE)
					index = MAXLIGHTSCALE - 1;

				dc_colormap = walllights[index];
			}

			sprtopscreen = centeryfrac - FixedMulEDX(spryscale, dc_texturemid);

			// VITI95: OPTIMIZE
			dc_iscale = 0xffffffffu / (unsigned)spryscale;

			// draw the texture

			tex = texnum;
			column = maskedtexturecol[dc_x];
			column &= texturewidthmask[tex];
			lump = texturecolumnlump[tex][column];
			ofs = texturecolumnofs[tex][column] - 3;

			if (lump > 0)
			{
				col = (column_t *)((byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs);
			}
			else
			{
				if (!texturecomposite[tex])
					R_GenerateComposite(tex);

				col = (column_t *)(texturecomposite[tex] + ofs);
			}

			basetexturemid = dc_texturemid;
			mfc_x = mfloorclip[dc_x];
			mcc_x = mceilingclip[dc_x];

			for (; col->topdelta != 0xff;)
			{
				// calculate unclipped screen coordinates
				//  for post
				topscreen = sprtopscreen + spryscale * col->topdelta;
				bottomscreen = topscreen + spryscale * col->length;

				yh = (bottomscreen - 1) >> FRACBITS;

				if (yh >= mfc_x)
					yh = mfc_x - 1;

				if (yh >= viewheight)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				yl = (topscreen + FRACUNIT - 1) >> FRACBITS;

				if (yl <= mcc_x)
					yl = mcc_x + 1;

				if (yl > yh)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				dc_source = (byte *)col + 3;
				dc_texturemid = basetexturemid - (col->topdelta << FRACBITS);

				dc_yh = yh;
				dc_yl = yl;

#if defined(MODE_CGA16) || defined(MODE_CVB)
				if (detailshift == DETAIL_HIGH)
				{
					if ((dc_x & 1) == 0)
						colfunc();
				}
				else
					colfunc();
#elif defined(MODE_CGA512)
				switch (detailshift)
				{
				case DETAIL_HIGH:
					if ((dc_x & 3) == 0)
						colfunc();
					break;
				case DETAIL_LOW:
					if ((dc_x & 1) == 0)
						colfunc();
					break;
				default:
					colfunc();
					break;
				}
#elif defined(MODE_MDA)
				if (dc_x == x1 || dc_x == x2)
					R_DrawLineColumnTextMDA();
				else
					R_DrawEmptyColumnTextMDA();
#else
				colfunc();
#endif

				col = (column_t *)((byte *)col + col->length + 4);
			}

			dc_texturemid = basetexturemid;

			maskedtexturecol[dc_x] = MAXSHORT;
		}
		spryscale += rw_scalestep;
		dc_x++;
	} while (dc_x <= x2);
}

void R_RenderMaskedSegRangeFlat(drawseg_t *ds,
							int x1,
							int x2)
{
	unsigned index;
	column_t *col;
	int lightnum;
	int texnum;

	int lump;
	int ofs;
	int tex;
	int column;

	byte *firstPixel;
	byte color;

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	curline = ds->curline;
	frontsector = curline->frontsector;
	backsector = curline->backsector;
	texnum = texturetranslation[curline->sidedef->midtexture];

	lump = texturecolumnlump[texnum][0];
	ofs = texturecolumnofs[texnum][0] - 3;

	if (lump > 0)
	{
		firstPixel = (byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs;
	}
	else
	{
		if (!texturecomposite[texnum])
			R_GenerateComposite(texnum);

		firstPixel = texturecomposite[texnum] + ofs;
	}

	color = *(firstPixel + 3);
	dc_color = color;

	lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT) + extralight;

	if (curline->v1->y == curline->v2->y)
		lightnum--;
	else if (curline->v1->x == curline->v2->x)
		lightnum++;

	// Lightnum between 0 and 15
	if (lightnum < 0)
		walllights = scalelight[0];
	else if (lightnum > LIGHTLEVELS - 1)
		walllights = scalelight[LIGHTLEVELS - 1];
	else
		walllights = scalelight[lightnum];

	maskedtexturecol = ds->maskedtexturecol;
	rw_scalestep = ds->scalestep;
	spryscale = ds->scale1 + (x1 - ds->x1) * rw_scalestep;
	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	// find positioning
	if (curline->linedef->dontPegBottom)
	{
		dc_texturemid = frontsector->floorheight > backsector->floorheight
							? frontsector->floorheight
							: backsector->floorheight;
		dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
	}
	else
	{
		dc_texturemid = frontsector->ceilingheight < backsector->ceilingheight
							? frontsector->ceilingheight
							: backsector->ceilingheight;
		dc_texturemid = dc_texturemid - viewz;
	}
	dc_texturemid += curline->sidedef->rowoffset;

	if (fixedcolormap){
		dc_colormap = fixedcolormap;
		dc_color = dc_colormap[color];
	}
		

	dc_x = x1;
	do
	{
		// calculate lighting
		if (maskedtexturecol[dc_x] != MAXSHORT)
		{
			int topscreen;
			int bottomscreen;
			fixed_t basetexturemid;

			int yl, yh;
			short mfc_x, mcc_x;

			if (!fixedcolormap)
			{
				index = spryscale >> LIGHTSCALESHIFT;
#if PIXEL_SCALING != 1
				index /= PIXEL_SCALING;
#endif

				if (index >= MAXLIGHTSCALE)
					index = MAXLIGHTSCALE - 1;

				dc_colormap = walllights[index];
				dc_color = dc_colormap[color];
			}

			sprtopscreen = centeryfrac - FixedMulEDX(spryscale, dc_texturemid);

			// draw the texture

			tex = texnum;
			column = maskedtexturecol[dc_x];
			column &= texturewidthmask[tex];
			lump = texturecolumnlump[tex][column];
			ofs = texturecolumnofs[tex][column] - 3;

			if (lump > 0)
			{
				col = (column_t *)((byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs);
			}
			else
			{
				if (!texturecomposite[tex])
					R_GenerateComposite(tex);

				col = (column_t *)(texturecomposite[tex] + ofs);
			}

			basetexturemid = dc_texturemid;
			mfc_x = mfloorclip[dc_x];
			mcc_x = mceilingclip[dc_x];

			for (; col->topdelta != 0xff;)
			{
				// calculate unclipped screen coordinates
				//  for post
				topscreen = sprtopscreen + spryscale * col->topdelta;
				bottomscreen = topscreen + spryscale * col->length;

				yh = (bottomscreen - 1) >> FRACBITS;

				if (yh >= mfc_x)
					yh = mfc_x - 1;

				if (yh >= viewheight)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				yl = (topscreen + FRACUNIT - 1) >> FRACBITS;

				if (yl <= mcc_x)
					yl = mcc_x + 1;

				if (yl > yh)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				dc_texturemid = basetexturemid - (col->topdelta << FRACBITS);

				dc_yh = yh;
				dc_yl = yl;

#if defined(MODE_CGA16) || defined(MODE_CVB)
				if (detailshift == DETAIL_HIGH)
				{
					if ((dc_x & 1) == 0)
						colfunc();
				}
				else
					colfunc();
#elif defined(MODE_CGA512)
				switch (detailshift)
				{
				case DETAIL_HIGH:
					if ((dc_x & 3) == 0)
						colfunc();
					break;
				case DETAIL_LOW:
					if ((dc_x & 1) == 0)
						colfunc();
					break;
				default:
					colfunc();
					break;
				}
#elif defined(MODE_MDA)
				if (dc_x == x1 || dc_x == x2)
					R_DrawLineColumnTextMDA();
				else
					R_DrawEmptyColumnTextMDA();
#else
				colfunc();
#endif

				col = (column_t *)((byte *)col + col->length + 4);
			}

			dc_texturemid = basetexturemid;

			maskedtexturecol[dc_x] = MAXSHORT;
		}
		spryscale += rw_scalestep;
		dc_x++;
	} while (dc_x <= x2);
}

void R_RenderMaskedSegRangeFlatter(drawseg_t *ds,
							int x1,
							int x2)
{
	unsigned index;
	column_t *col;
	int lightnum;
	int texnum;

	int lump;
	int ofs;
	int tex;
	int column;

	byte *firstPixel;
	byte color;

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	curline = ds->curline;
	frontsector = curline->frontsector;
	backsector = curline->backsector;
	texnum = texturetranslation[curline->sidedef->midtexture];

	lump = texturecolumnlump[texnum][0];
	ofs = texturecolumnofs[texnum][0] - 3;

	if (lump > 0)
	{
		firstPixel = (byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs;
	}
	else
	{
		if (!texturecomposite[texnum])
			R_GenerateComposite(texnum);

		firstPixel = texturecomposite[texnum] + ofs;
	}

	color = *(firstPixel + 3);
	dc_color = color;

	maskedtexturecol = ds->maskedtexturecol;
	rw_scalestep = ds->scalestep;
	spryscale = ds->scale1 + (x1 - ds->x1) * rw_scalestep;
	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	// find positioning
	if (curline->linedef->dontPegBottom)
	{
		dc_texturemid = frontsector->floorheight > backsector->floorheight
							? frontsector->floorheight
							: backsector->floorheight;
		dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
	}
	else
	{
		dc_texturemid = frontsector->ceilingheight < backsector->ceilingheight
							? frontsector->ceilingheight
							: backsector->ceilingheight;
		dc_texturemid = dc_texturemid - viewz;
	}
	dc_texturemid += curline->sidedef->rowoffset;

	dc_x = x1;
	do
	{
		// calculate lighting
		if (maskedtexturecol[dc_x] != MAXSHORT)
		{
			int topscreen;
			int bottomscreen;
			fixed_t basetexturemid;

			int yl, yh;
			short mfc_x, mcc_x;

			sprtopscreen = centeryfrac - FixedMulEDX(spryscale, dc_texturemid);

			// draw the texture

			tex = texnum;
			column = maskedtexturecol[dc_x];
			column &= texturewidthmask[tex];
			lump = texturecolumnlump[tex][column];
			ofs = texturecolumnofs[tex][column] - 3;

			if (lump > 0)
			{
				col = (column_t *)((byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs);
			}
			else
			{
				if (!texturecomposite[tex])
					R_GenerateComposite(tex);

				col = (column_t *)(texturecomposite[tex] + ofs);
			}

			basetexturemid = dc_texturemid;
			mfc_x = mfloorclip[dc_x];
			mcc_x = mceilingclip[dc_x];

			for (; col->topdelta != 0xff;)
			{
				// calculate unclipped screen coordinates
				//  for post
				topscreen = sprtopscreen + spryscale * col->topdelta;
				bottomscreen = topscreen + spryscale * col->length;

				yh = (bottomscreen - 1) >> FRACBITS;

				if (yh >= mfc_x)
					yh = mfc_x - 1;

				if (yh >= viewheight)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				yl = (topscreen + FRACUNIT - 1) >> FRACBITS;

				if (yl <= mcc_x)
					yl = mcc_x + 1;

				if (yl > yh)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				dc_texturemid = basetexturemid - (col->topdelta << FRACBITS);

				dc_yh = yh;
				dc_yl = yl;

#if defined(MODE_CGA16) || defined(MODE_CVB)
				if (detailshift == DETAIL_HIGH)
				{
					if ((dc_x & 1) == 0)
						colfunc();
				}
				else
					colfunc();
#elif defined(MODE_CGA512)
				switch (detailshift)
				{
				case DETAIL_HIGH:
					if ((dc_x & 3) == 0)
						colfunc();
					break;
				case DETAIL_LOW:
					if ((dc_x & 1) == 0)
						colfunc();
					break;
				default:
					colfunc();
					break;
				}
#elif defined(MODE_MDA)
				if (dc_x == x1 || dc_x == x2)
					R_DrawLineColumnTextMDA();
				else
					R_DrawEmptyColumnTextMDA();
#else
				colfunc();
#endif

				col = (column_t *)((byte *)col + col->length + 4);
			}

			dc_texturemid = basetexturemid;

			maskedtexturecol[dc_x] = MAXSHORT;
		}
		spryscale += rw_scalestep;
		dc_x++;
	} while (dc_x <= x2);
}

void R_RenderMaskedSegRange2(drawseg_t *ds)
{
	unsigned index;
	column_t *col;
	int lightnum;
	int texnum;

	int lump;
	int ofs;
	int tex;
	int column;

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	curline = ds->curline;
	frontsector = curline->frontsector;
	backsector = curline->backsector;
	texnum = texturetranslation[curline->sidedef->midtexture];

	lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT) + extralight;

	if (curline->v1->y == curline->v2->y)
		lightnum--;
	else if (curline->v1->x == curline->v2->x)
		lightnum++;

	// Lightnum between 0 and 15
	if (lightnum < 0)
		walllights = scalelight[0];
	else if (lightnum > LIGHTLEVELS - 1)
		walllights = scalelight[LIGHTLEVELS - 1];
	else
		walllights = scalelight[lightnum];

	maskedtexturecol = ds->maskedtexturecol;

	rw_scalestep = ds->scalestep;
	spryscale = ds->scale1;
	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	// find positioning
	if (curline->linedef->dontPegBottom)
	{
		dc_texturemid = frontsector->floorheight > backsector->floorheight
							? frontsector->floorheight
							: backsector->floorheight;
		dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
	}
	else
	{
		dc_texturemid = frontsector->ceilingheight < backsector->ceilingheight
							? frontsector->ceilingheight
							: backsector->ceilingheight;
		dc_texturemid = dc_texturemid - viewz;
	}
	dc_texturemid += curline->sidedef->rowoffset;

	if (fixedcolormap)
	{
		dc_colormap = fixedcolormap;
	}
		

	dc_x = ds->x1;
	do
	{
		// calculate lighting
		if (maskedtexturecol[dc_x] != MAXSHORT)
		{
			int topscreen;
			int bottomscreen;
			fixed_t basetexturemid;

			int yl, yh;
			short mfc_x, mcc_x;

			if (!fixedcolormap)
			{
				index = spryscale >> LIGHTSCALESHIFT;
#if PIXEL_SCALING != 1
				index /= PIXEL_SCALING;
#endif

				if (index >= MAXLIGHTSCALE)
					index = MAXLIGHTSCALE - 1;

				dc_colormap = walllights[index];
			}

			sprtopscreen = centeryfrac - FixedMulEDX(spryscale, dc_texturemid);

			// VITI95: OPTIMIZE
			dc_iscale = 0xffffffffu / (unsigned)spryscale;

			// draw the texture

			tex = texnum;
			column = maskedtexturecol[dc_x];
			column &= texturewidthmask[tex];
			lump = texturecolumnlump[tex][column];
			ofs = texturecolumnofs[tex][column] - 3;

			if (lump > 0)
			{
				col = (column_t *)((byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs);
			}
			else
			{
				if (!texturecomposite[tex])
					R_GenerateComposite(tex);

				col = (column_t *)(texturecomposite[tex] + ofs);
			}

			basetexturemid = dc_texturemid;
			mfc_x = mfloorclip[dc_x];
			mcc_x = mceilingclip[dc_x];

			for (; col->topdelta != 0xff;)
			{
				// calculate unclipped screen coordinates
				//  for post
				topscreen = sprtopscreen + spryscale * col->topdelta;
				bottomscreen = topscreen + spryscale * col->length;

				yh = (bottomscreen - 1) >> FRACBITS;

				if (yh >= mfc_x)
					yh = mfc_x - 1;

				if (yh >= viewheight)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				yl = (topscreen + FRACUNIT - 1) >> FRACBITS;

				if (yl <= mcc_x)
					yl = mcc_x + 1;

				if (yl > yh)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				dc_source = (byte *)col + 3;
				dc_texturemid = basetexturemid - (col->topdelta << FRACBITS);

				dc_yh = yh;
				dc_yl = yl;

#if defined(MODE_CGA16) || defined(MODE_CVB)
				if (detailshift == DETAIL_HIGH)
				{
					if ((dc_x & 1) == 0)
						colfunc();
				}
				else
					colfunc();
#elif defined(MODE_CGA512)
				switch (detailshift)
				{
				case DETAIL_HIGH:
					if ((dc_x & 3) == 0)
						colfunc();
					break;
				case DETAIL_LOW:
					if ((dc_x & 1) == 0)
						colfunc();
					break;
				default:
					colfunc();
					break;
				}
#elif defined(MODE_MDA)
				if (dc_x == ds->x1 || dc_x == ds->x2)
					R_DrawLineColumnTextMDA();
				else
					R_DrawEmptyColumnTextMDA();
#else
				colfunc();
#endif

				col = (column_t *)((byte *)col + col->length + 4);
			}

			dc_texturemid = basetexturemid;

			maskedtexturecol[dc_x] = MAXSHORT;
		}
		spryscale += rw_scalestep;
		dc_x++;
	} while (dc_x <= ds->x2);
}

void R_RenderMaskedSegRange2Flat(drawseg_t *ds)
{
	unsigned index;
	column_t *col;
	int lightnum;
	int texnum;

	int lump;
	int ofs;
	int tex;
	int column;

	byte *firstPixel;
	byte color;

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	curline = ds->curline;
	frontsector = curline->frontsector;
	backsector = curline->backsector;
	texnum = texturetranslation[curline->sidedef->midtexture];

	lump = texturecolumnlump[texnum][0];
	ofs = texturecolumnofs[texnum][0] - 3;

	if (lump > 0)
	{
		firstPixel = (byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs;
	}
	else
	{
		if (!texturecomposite[texnum])
			R_GenerateComposite(texnum);

		firstPixel = texturecomposite[texnum] + ofs;
	}

	color = *(firstPixel + 3);
	dc_color = color;

	lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT) + extralight;

	if (curline->v1->y == curline->v2->y)
		lightnum--;
	else if (curline->v1->x == curline->v2->x)
		lightnum++;

	// Lightnum between 0 and 15
	if (lightnum < 0)
		walllights = scalelight[0];
	else if (lightnum > LIGHTLEVELS - 1)
		walllights = scalelight[LIGHTLEVELS - 1];
	else
		walllights = scalelight[lightnum];

	maskedtexturecol = ds->maskedtexturecol;

	rw_scalestep = ds->scalestep;
	spryscale = ds->scale1;
	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	// find positioning
	if (curline->linedef->dontPegBottom)
	{
		dc_texturemid = frontsector->floorheight > backsector->floorheight
							? frontsector->floorheight
							: backsector->floorheight;
		dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
	}
	else
	{
		dc_texturemid = frontsector->ceilingheight < backsector->ceilingheight
							? frontsector->ceilingheight
							: backsector->ceilingheight;
		dc_texturemid = dc_texturemid - viewz;
	}
	dc_texturemid += curline->sidedef->rowoffset;

	if (fixedcolormap)
	{
		dc_colormap = fixedcolormap;
		dc_color = dc_colormap[color];
	}

	dc_x = ds->x1;
	do
	{
		// calculate lighting
		if (maskedtexturecol[dc_x] != MAXSHORT)
		{
			int topscreen;
			int bottomscreen;
			fixed_t basetexturemid;

			int yl, yh;
			short mfc_x, mcc_x;

			if (!fixedcolormap)
			{
				index = spryscale >> LIGHTSCALESHIFT;
#if PIXEL_SCALING != 1
				index /= PIXEL_SCALING;
#endif

				if (index >= MAXLIGHTSCALE)
					index = MAXLIGHTSCALE - 1;

				dc_colormap = walllights[index];
				dc_color = dc_colormap[color];
			}

			sprtopscreen = centeryfrac - FixedMulEDX(spryscale, dc_texturemid);

			// draw the texture

			tex = texnum;
			column = maskedtexturecol[dc_x];
			column &= texturewidthmask[tex];
			lump = texturecolumnlump[tex][column];
			ofs = texturecolumnofs[tex][column] - 3;

			if (lump > 0)
			{
				col = (column_t *)((byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs);
			}
			else
			{
				if (!texturecomposite[tex])
					R_GenerateComposite(tex);

				col = (column_t *)(texturecomposite[tex] + ofs);
			}

			basetexturemid = dc_texturemid;
			mfc_x = mfloorclip[dc_x];
			mcc_x = mceilingclip[dc_x];

			for (; col->topdelta != 0xff;)
			{
				// calculate unclipped screen coordinates
				//  for post
				topscreen = sprtopscreen + spryscale * col->topdelta;
				bottomscreen = topscreen + spryscale * col->length;

				yh = (bottomscreen - 1) >> FRACBITS;

				if (yh >= mfc_x)
					yh = mfc_x - 1;

				if (yh >= viewheight)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				yl = (topscreen + FRACUNIT - 1) >> FRACBITS;

				if (yl <= mcc_x)
					yl = mcc_x + 1;

				if (yl > yh)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				dc_texturemid = basetexturemid - (col->topdelta << FRACBITS);

				dc_yh = yh;
				dc_yl = yl;

#if defined(MODE_CGA16) || defined(MODE_CVB)
				if (detailshift == DETAIL_HIGH)
				{
					if ((dc_x & 1) == 0)
						colfunc();
				}
				else
					colfunc();
#elif defined(MODE_CGA512)
				switch (detailshift)
				{
				case DETAIL_HIGH:
					if ((dc_x & 3) == 0)
						colfunc();
					break;
				case DETAIL_LOW:
					if ((dc_x & 1) == 0)
						colfunc();
					break;
				default:
					colfunc();
					break;
				}
#elif defined(MODE_MDA)
				if (dc_x == ds->x1 || dc_x == ds->x2)
					R_DrawLineColumnTextMDA();
				else
					R_DrawEmptyColumnTextMDA();
#else
				colfunc();
#endif

				col = (column_t *)((byte *)col + col->length + 4);
			}

			dc_texturemid = basetexturemid;

			maskedtexturecol[dc_x] = MAXSHORT;
		}
		spryscale += rw_scalestep;
		dc_x++;
	} while (dc_x <= ds->x2);
}

void R_RenderMaskedSegRange2Flatter(drawseg_t *ds)
{
	unsigned index;
	column_t *col;
	int lightnum;
	int texnum;

	int lump;
	int ofs;
	int tex;
	int column;

	byte *firstPixel;
	byte color;

	// Calculate light table.
	// Use different light tables
	//   for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	curline = ds->curline;
	frontsector = curline->frontsector;
	backsector = curline->backsector;
	texnum = texturetranslation[curline->sidedef->midtexture];

	lump = texturecolumnlump[texnum][0];
	ofs = texturecolumnofs[texnum][0] - 3;

	if (lump > 0)
	{
		firstPixel = (byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs;
	}
	else
	{
		if (!texturecomposite[texnum])
			R_GenerateComposite(texnum);

		firstPixel = texturecomposite[texnum] + ofs;
	}

	color = *(firstPixel + 3);
	dc_color = color;

	maskedtexturecol = ds->maskedtexturecol;

	rw_scalestep = ds->scalestep;
	spryscale = ds->scale1;
	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	// find positioning
	if (curline->linedef->dontPegBottom)
	{
		dc_texturemid = frontsector->floorheight > backsector->floorheight
							? frontsector->floorheight
							: backsector->floorheight;
		dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
	}
	else
	{
		dc_texturemid = frontsector->ceilingheight < backsector->ceilingheight
							? frontsector->ceilingheight
							: backsector->ceilingheight;
		dc_texturemid = dc_texturemid - viewz;
	}
	dc_texturemid += curline->sidedef->rowoffset;

	dc_x = ds->x1;
	do
	{
		// calculate lighting
		if (maskedtexturecol[dc_x] != MAXSHORT)
		{
			int topscreen;
			int bottomscreen;
			fixed_t basetexturemid;

			int yl, yh;
			short mfc_x, mcc_x;
			sprtopscreen = centeryfrac - FixedMulEDX(spryscale, dc_texturemid);

			// draw the texture

			tex = texnum;
			column = maskedtexturecol[dc_x];
			column &= texturewidthmask[tex];
			lump = texturecolumnlump[tex][column];
			ofs = texturecolumnofs[tex][column] - 3;

			if (lump > 0)
			{
				col = (column_t *)((byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs);
			}
			else
			{
				if (!texturecomposite[tex])
					R_GenerateComposite(tex);

				col = (column_t *)(texturecomposite[tex] + ofs);
			}

			basetexturemid = dc_texturemid;
			mfc_x = mfloorclip[dc_x];
			mcc_x = mceilingclip[dc_x];

			for (; col->topdelta != 0xff;)
			{
				// calculate unclipped screen coordinates
				//  for post
				topscreen = sprtopscreen + spryscale * col->topdelta;
				bottomscreen = topscreen + spryscale * col->length;

				yh = (bottomscreen - 1) >> FRACBITS;

				if (yh >= mfc_x)
					yh = mfc_x - 1;

				if (yh >= viewheight)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				yl = (topscreen + FRACUNIT - 1) >> FRACBITS;

				if (yl <= mcc_x)
					yl = mcc_x + 1;

				if (yl > yh)
				{
					col = (column_t *)((byte *)col + col->length + 4);
					continue;
				}

				dc_texturemid = basetexturemid - (col->topdelta << FRACBITS);

				dc_yh = yh;
				dc_yl = yl;

#if defined(MODE_CGA16) || defined(MODE_CVB)
				if (detailshift == DETAIL_HIGH)
				{
					if ((dc_x & 1) == 0)
						colfunc();
				}
				else
					colfunc();
#elif defined(MODE_CGA512)
				switch (detailshift)
				{
				case DETAIL_HIGH:
					if ((dc_x & 3) == 0)
						colfunc();
					break;
				case DETAIL_LOW:
					if ((dc_x & 1) == 0)
						colfunc();
					break;
				default:
					colfunc();
					break;
				}
#elif defined(MODE_MDA)
				if (dc_x == ds->x1 || dc_x == ds->x2)
					R_DrawLineColumnTextMDA();
				else
					R_DrawEmptyColumnTextMDA();
#else
				colfunc();
#endif

				col = (column_t *)((byte *)col + col->length + 4);
			}

			dc_texturemid = basetexturemid;

			maskedtexturecol[dc_x] = MAXSHORT;
		}
		spryscale += rw_scalestep;
		dc_x++;
	} while (dc_x <= ds->x2);
}

//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//
#define HEIGHTBITS 12
#define HEIGHTUNIT (1 << HEIGHTBITS)

void (*renderSegLoop)(void);
void (*renderMaskedSegRange)(drawseg_t *ds, int x1, int x2);
void (*renderMaskedSegRange2)(drawseg_t *ds);
void (*drawVisSprite)(vissprite_t *vis);
void (*drawPlanes)(void);
void (*mapPlane)(int y, int x1);
void (*clearPlanes)(void);
void (*drawSky)(visplane_t *pl);

void (*drawPlayerSprite)(vissprite_t *vis);

void R_RenderSegLoop(void)
{
	angle_t angle;
	unsigned index;
	int yl;
	int yh;
	int mid;
	fixed_t texturecolumn;
	int top;
	int bottom;

	int lump;
	int ofs;
	int tex;
	int col;

	int cc_rwx;
	int fc_rwx;

#if defined(MODE_MDA)
	int first = rw_x;
#endif

	// texturecolumn = 0;				// shut up compiler warning

	for (; rw_x < rw_stopx; rw_x++)
	{
		cc_rwx = ceilingclip[rw_x];
		fc_rwx = floorclip[rw_x];

		// mark floor / ceiling areas
		yl = (topfrac + HEIGHTUNIT - 1) >> HEIGHTBITS;

		// no space above wall?
		if (yl < cc_rwx + 1)
			yl = cc_rwx + 1;

		if (markceiling)
		{
			top = cc_rwx + 1;
			bottom = yl - 1;

			if (bottom >= fc_rwx)
				bottom = fc_rwx - 1;

			if (top <= bottom)
			{
				ceilingplane->top[rw_x] = top;
				ceilingplane->bottom[rw_x] = bottom;
				ceilingplane->modified = 1;
			}
		}

		yh = bottomfrac >> HEIGHTBITS;

		if (yh >= fc_rwx)
			yh = fc_rwx - 1;

		if (markfloor)
		{
			top = yh + 1;
			bottom = fc_rwx - 1;
			if (top <= cc_rwx)
				top = cc_rwx + 1;
			if (top <= bottom)
			{
				floorplane->top[rw_x] = top;
				floorplane->bottom[rw_x] = bottom;
				floorplane->modified = 1;
			}
		}

		// texturecolumn and lighting are independent of wall tiers
		if (segtextured)
		{
			// calculate texture offset
			angle = (rw_centerangle + xtoviewangle[rw_x]) >> ANGLETOFINESHIFT;
			texturecolumn = rw_offset - FixedMulEDX(finetangent[angle], rw_distance);
			texturecolumn >>= FRACBITS;
			// calculate lighting
			index = rw_scale >> LIGHTSCALESHIFT;
#if PIXEL_SCALING != 1
				index /= PIXEL_SCALING;
#endif

			if (index >= MAXLIGHTSCALE)
				index = MAXLIGHTSCALE - 1;

			dc_colormap = walllights[index];
			dc_x = rw_x;

			// VITI95: OPTIMIZE
			dc_iscale = 0xffffffffu / (unsigned)rw_scale;
		}

		// draw the wall tiers
		if (midtexture)
		{
			if (yl > yh)
			{
				cc_rwx = viewheight;
				fc_rwx = -1;
			}
			else
			{
				// single sided line
				dc_yl = yl;
				dc_yh = yh;
				dc_texturemid = rw_midtexturemid;

				tex = midtexture;
				col = texturecolumn;
				col &= texturewidthmask[tex];
				lump = texturecolumnlump[tex][col];
				ofs = texturecolumnofs[tex][col];

				if (lump > 0)
				{
					dc_source = (byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs;
				}
				else
				{
					if (!texturecomposite[tex])
						R_GenerateComposite(tex);

					dc_source = texturecomposite[tex] + ofs;
				}

#if defined(MODE_CGA16) || defined(MODE_CVB)
				if (detailshift == DETAIL_HIGH)
				{
					if ((dc_x & 1) == 0)
						colfunc();
				}
				else
					colfunc();
#elif defined(MODE_CGA512)
				switch (detailshift)
				{
				case DETAIL_HIGH:
					if ((dc_x & 3) == 0)
						colfunc();
					break;
				case DETAIL_LOW:
					if ((dc_x & 1) == 0)
						colfunc();
					break;
				default:
					colfunc();
					break;
				}
#elif defined(MODE_MDA)
				if (first == rw_x || rw_x == rw_stopx - 1)
					R_DrawLineColumnTextMDA();
				else
					R_DrawEmptyColumnTextMDA();
#else
				colfunc();
#endif

				cc_rwx = viewheight;
				fc_rwx = -1;
			}
		}
		else
		{
			// two sided line
			if (toptexture)
			{
				// top wall
				mid = pixhigh >> HEIGHTBITS;
				pixhigh -= pixhighstep;

				if (mid >= fc_rwx)
					mid = fc_rwx - 1;

				if (mid < yl)
				{
					cc_rwx = yl - 1;
				}
				else
				{
					dc_yl = yl;
					dc_yh = mid;
					dc_texturemid = rw_toptexturemid;

					tex = toptexture;
					col = texturecolumn;
					col &= texturewidthmask[tex];
					lump = texturecolumnlump[tex][col];
					ofs = texturecolumnofs[tex][col];

					if (lump > 0)
					{
						dc_source = (byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs;
					}
					else
					{
						if (!texturecomposite[tex])
							R_GenerateComposite(tex);

						dc_source = texturecomposite[tex] + ofs;
					}

#if defined(MODE_CGA16) || defined(MODE_CVB)
					if (detailshift == DETAIL_HIGH)
					{
						if ((dc_x & 1) == 0)
							colfunc();
					}
					else
						colfunc();
#elif defined(MODE_CGA512)
					switch (detailshift)
					{
					case DETAIL_HIGH:
						if ((dc_x & 3) == 0)
							colfunc();
						break;
					case DETAIL_LOW:
						if ((dc_x & 1) == 0)
							colfunc();
						break;
					default:
						colfunc();
						break;
					}
#elif defined(MODE_MDA)
					if (first == rw_x || rw_x == rw_stopx - 1)
						R_DrawLineColumnTextMDA();
					else
						R_DrawEmptyColumnTextMDA();
#else
					colfunc();
#endif

					cc_rwx = mid;
				}
			}
			else
			{
				// no top wall
				if (markceiling)
					cc_rwx = yl - 1;
			}

			if (bottomtexture)
			{
				// bottom wall
				mid = (pixlow + HEIGHTUNIT - 1) >> HEIGHTBITS;
				pixlow -= pixlowstep;

				// no space above wall?
				if (mid <= cc_rwx)
					mid = cc_rwx + 1;

				if (mid > yh)
				{
					fc_rwx = yh + 1;
				}
				else
				{
					dc_yl = mid;
					dc_yh = yh;
					dc_texturemid = rw_bottomtexturemid;

					tex = bottomtexture;
					col = texturecolumn;
					col &= texturewidthmask[tex];
					lump = texturecolumnlump[tex][col];
					ofs = texturecolumnofs[tex][col];

					if (lump > 0)
					{
						dc_source = (byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs;
					}
					else
					{
						if (!texturecomposite[tex])
							R_GenerateComposite(tex);

						dc_source = texturecomposite[tex] + ofs;
					}

#if defined(MODE_CGA16) || defined(MODE_CVB)
					if (detailshift == DETAIL_HIGH)
					{
						if ((dc_x & 1) == 0)
							colfunc();
					}
					else
						colfunc();
#elif defined(MODE_CGA512)
					switch (detailshift)
					{
					case DETAIL_HIGH:
						if ((dc_x & 3) == 0)
							colfunc();
						break;
					case DETAIL_LOW:
						if ((dc_x & 1) == 0)
							colfunc();
						break;
					default:
						colfunc();
						break;
					}
#elif defined(MODE_MDA)
					if (first == rw_x || rw_x == rw_stopx - 1)
						R_DrawLineColumnTextMDA();
					else
						R_DrawEmptyColumnTextMDA();
#else
					colfunc();
#endif

					fc_rwx = mid;
				}
			}
			else
			{
				// no bottom wall
				if (markfloor)
					fc_rwx = yh + 1;
			}

			if (maskedtexture)
			{
				// save texturecol
				//  for backdrawing of masked mid texture
				maskedtexturecol[rw_x] = texturecolumn;
			}
		}

		rw_scale += rw_scalestep;
		topfrac -= topstep;
		bottomfrac -= bottomstep;

		ceilingclip[rw_x] = cc_rwx;
		floorclip[rw_x] = fc_rwx;
	}
}

void R_RenderSegLoopFlat(void)
{
	angle_t angle;
	unsigned index;
	int yl;
	int yh;
	int mid;
	fixed_t texturecolumn;
	int top;
	int bottom;

	int lump;
	int ofs;
	int tex;
	int col;

	int cc_rwx;
	int fc_rwx;

	byte *firstPixel;
	byte color;

#if defined(MODE_MDA)
	int first = rw_x;
#endif

	// texturecolumn = 0;				// shut up compiler warning

	if (midtexture) {
		tex = midtexture;
	} else if (toptexture) {
		tex = toptexture;
	} else {
		tex = bottomtexture;
	}

	lump = texturecolumnlump[tex][0];
	ofs = texturecolumnofs[tex][0] - 3;

	if (lump > 0)
	{
		firstPixel = ((byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs);
	}
	else
	{
		if (!texturecomposite[tex])
			R_GenerateComposite(tex);

		firstPixel = (texturecomposite[tex] + ofs);
	}

	color = *(firstPixel + 3);
	dc_color = color;

	for (; rw_x < rw_stopx; rw_x++)
	{
		cc_rwx = ceilingclip[rw_x];
		fc_rwx = floorclip[rw_x];

		// mark floor / ceiling areas
		yl = (topfrac + HEIGHTUNIT - 1) >> HEIGHTBITS;

		// no space above wall?
		if (yl < cc_rwx + 1)
			yl = cc_rwx + 1;

		if (markceiling)
		{
			top = cc_rwx + 1;
			bottom = yl - 1;

			if (bottom >= fc_rwx)
				bottom = fc_rwx - 1;

			if (top <= bottom)
			{
				ceilingplane->top[rw_x] = top;
				ceilingplane->bottom[rw_x] = bottom;
				ceilingplane->modified = 1;
			}
		}

		yh = bottomfrac >> HEIGHTBITS;

		if (yh >= fc_rwx)
			yh = fc_rwx - 1;

		if (markfloor)
		{
			top = yh + 1;
			bottom = fc_rwx - 1;
			if (top <= cc_rwx)
				top = cc_rwx + 1;
			if (top <= bottom)
			{
				floorplane->top[rw_x] = top;
				floorplane->bottom[rw_x] = bottom;
				floorplane->modified = 1;
			}
		}

		// texturecolumn and lighting are independent of wall tiers
		if (segtextured)
		{
			// calculate texture offset
			angle = (rw_centerangle + xtoviewangle[rw_x]) >> ANGLETOFINESHIFT;
			texturecolumn = rw_offset - FixedMulEDX(finetangent[angle], rw_distance);
			texturecolumn >>= FRACBITS;
			// calculate lighting
			index = rw_scale >> LIGHTSCALESHIFT;
#if PIXEL_SCALING != 1
				index /= PIXEL_SCALING;
#endif

			if (index >= MAXLIGHTSCALE)
				index = MAXLIGHTSCALE - 1;

			dc_colormap = walllights[index];
			dc_color = dc_colormap[color];
			dc_x = rw_x;
		}

		// draw the wall tiers
		if (midtexture)
		{
			if (yl > yh)
			{
				cc_rwx = viewheight;
				fc_rwx = -1;
			}
			else
			{
				// single sided line
				dc_yl = yl;
				dc_yh = yh;

#if defined(MODE_CGA16) || defined(MODE_CVB)
				if (detailshift == DETAIL_HIGH)
				{
					if ((dc_x & 1) == 0)
						colfunc();
				}
				else
					colfunc();
#elif defined(MODE_CGA512)
				switch (detailshift)
				{
				case DETAIL_HIGH:
					if ((dc_x & 3) == 0)
						colfunc();
					break;
				case DETAIL_LOW:
					if ((dc_x & 1) == 0)
						colfunc();
					break;
				default:
					colfunc();
					break;
				}
#elif defined(MODE_MDA)
				if (first == rw_x || rw_x == rw_stopx - 1)
					R_DrawLineColumnTextMDA();
				else
					R_DrawEmptyColumnTextMDA();
#else
				colfunc();
#endif

				cc_rwx = viewheight;
				fc_rwx = -1;
			}
		}
		else
		{
			// two sided line
			if (toptexture)
			{
				// top wall
				mid = pixhigh >> HEIGHTBITS;
				pixhigh -= pixhighstep;

				if (mid >= fc_rwx)
					mid = fc_rwx - 1;

				if (mid < yl)
				{
					cc_rwx = yl - 1;
				}
				else
				{
					dc_yl = yl;
					dc_yh = mid;

#if defined(MODE_CGA16) || defined(MODE_CVB)
					if (detailshift == DETAIL_HIGH)
					{
						if ((dc_x & 1) == 0)
							colfunc();
					}
					else
						colfunc();
#elif defined(MODE_CGA512)
					switch (detailshift)
					{
					case DETAIL_HIGH:
						if ((dc_x & 3) == 0)
							colfunc();
						break;
					case DETAIL_LOW:
						if ((dc_x & 1) == 0)
							colfunc();
						break;
					default:
						colfunc();
						break;
					}
#elif defined(MODE_MDA)
					if (first == rw_x || rw_x == rw_stopx - 1)
						R_DrawLineColumnTextMDA();
					else
						R_DrawEmptyColumnTextMDA();
#else
					colfunc();
#endif

					cc_rwx = mid;
				}
			}
			else
			{
				// no top wall
				if (markceiling)
					cc_rwx = yl - 1;
			}

			if (bottomtexture)
			{
				// bottom wall
				mid = (pixlow + HEIGHTUNIT - 1) >> HEIGHTBITS;
				pixlow -= pixlowstep;

				// no space above wall?
				if (mid <= cc_rwx)
					mid = cc_rwx + 1;

				if (mid > yh)
				{
					fc_rwx = yh + 1;
				}
				else
				{
					dc_yl = mid;
					dc_yh = yh;

#if defined(MODE_CGA16) || defined(MODE_CVB)
					if (detailshift == DETAIL_HIGH)
					{
						if ((dc_x & 1) == 0)
							colfunc();
					}
					else
						colfunc();
#elif defined(MODE_CGA512)
					switch (detailshift)
					{
					case DETAIL_HIGH:
						if ((dc_x & 3) == 0)
							colfunc();
						break;
					case DETAIL_LOW:
						if ((dc_x & 1) == 0)
							colfunc();
						break;
					default:
						colfunc();
						break;
					}
#elif defined(MODE_MDA)
					if (first == rw_x || rw_x == rw_stopx - 1)
						R_DrawLineColumnTextMDA();
					else
						R_DrawEmptyColumnTextMDA();
#else
					colfunc();
#endif

					fc_rwx = mid;
				}
			}
			else
			{
				// no bottom wall
				if (markfloor)
					fc_rwx = yh + 1;
			}

			if (maskedtexture)
			{
				// save texturecol
				//  for backdrawing of masked mid texture
				maskedtexturecol[rw_x] = texturecolumn;
			}
		}

		rw_scale += rw_scalestep;
		topfrac -= topstep;
		bottomfrac -= bottomstep;

		ceilingclip[rw_x] = cc_rwx;
		floorclip[rw_x] = fc_rwx;
	}
}

void R_RenderSegLoopFlatter(void)
{
	angle_t angle;
	unsigned index;
	int yl;
	int yh;
	int mid;
	fixed_t texturecolumn;
	int top;
	int bottom;

	int lump;
	int ofs;
	int tex;
	int col;

	int cc_rwx;
	int fc_rwx;

	byte *firstPixel;

#if defined(MODE_MDA)
	int first = rw_x;
#endif

	// texturecolumn = 0;				// shut up compiler warning

	if (midtexture) {
		tex = midtexture;
	} else if (toptexture) {
		tex = toptexture;
	} else {
		tex = bottomtexture;
	}

	lump = texturecolumnlump[tex][0];
	ofs = texturecolumnofs[tex][0] - 3;

	if (lump > 0)
	{
		firstPixel = ((byte *)W_CacheLumpNum(lump, PU_CACHE) + ofs);
	}
	else
	{
		if (!texturecomposite[tex])
			R_GenerateComposite(tex);

		firstPixel = (texturecomposite[tex] + ofs);
	}

	dc_color = *(firstPixel + 3);

	for (; rw_x < rw_stopx; rw_x++)
	{
		cc_rwx = ceilingclip[rw_x];
		fc_rwx = floorclip[rw_x];

		// mark floor / ceiling areas
		yl = (topfrac + HEIGHTUNIT - 1) >> HEIGHTBITS;

		// no space above wall?
		if (yl < cc_rwx + 1)
			yl = cc_rwx + 1;

		if (markceiling)
		{
			top = cc_rwx + 1;
			bottom = yl - 1;

			if (bottom >= fc_rwx)
				bottom = fc_rwx - 1;

			if (top <= bottom)
			{
				ceilingplane->top[rw_x] = top;
				ceilingplane->bottom[rw_x] = bottom;
				ceilingplane->modified = 1;
			}
		}

		yh = bottomfrac >> HEIGHTBITS;

		if (yh >= fc_rwx)
			yh = fc_rwx - 1;

		if (markfloor)
		{
			top = yh + 1;
			bottom = fc_rwx - 1;
			if (top <= cc_rwx)
				top = cc_rwx + 1;
			if (top <= bottom)
			{
				floorplane->top[rw_x] = top;
				floorplane->bottom[rw_x] = bottom;
				floorplane->modified = 1;
			}
		}

		// texturecolumn and lighting are independent of wall tiers
		if (segtextured)
		{
			// calculate texture offset
			angle = (rw_centerangle + xtoviewangle[rw_x]) >> ANGLETOFINESHIFT;
			texturecolumn = rw_offset - FixedMulEDX(finetangent[angle], rw_distance);
			texturecolumn >>= FRACBITS;

			dc_x = rw_x;
		}

		// draw the wall tiers
		if (midtexture)
		{
			if (yl > yh)
			{
				cc_rwx = viewheight;
				fc_rwx = -1;
			}
			else
			{
				// single sided line
				dc_yl = yl;
				dc_yh = yh;

#if defined(MODE_CGA16) || defined(MODE_CVB)
				if (detailshift == DETAIL_HIGH)
				{
					if ((dc_x & 1) == 0)
						colfunc();
				}
				else
					colfunc();
#elif defined(MODE_CGA512)
				switch (detailshift)
				{
				case DETAIL_HIGH:
					if ((dc_x & 3) == 0)
						colfunc();
					break;
				case DETAIL_LOW:
					if ((dc_x & 1) == 0)
						colfunc();
					break;
				default:
					colfunc();
					break;
				}
#elif defined(MODE_MDA)
				if (first == rw_x || rw_x == rw_stopx - 1)
					R_DrawLineColumnTextMDA();
				else
					R_DrawEmptyColumnTextMDA();
#else
				colfunc();
#endif

				cc_rwx = viewheight;
				fc_rwx = -1;
			}
		}
		else
		{
			// two sided line
			if (toptexture)
			{
				// top wall
				mid = pixhigh >> HEIGHTBITS;
				pixhigh -= pixhighstep;

				if (mid >= fc_rwx)
					mid = fc_rwx - 1;

				if (mid < yl)
				{
					cc_rwx = yl - 1;
				}
				else
				{
					dc_yl = yl;
					dc_yh = mid;

#if defined(MODE_CGA16) || defined(MODE_CVB)
					if (detailshift == DETAIL_HIGH)
					{
						if ((dc_x & 1) == 0)
							colfunc();
					}
					else
						colfunc();
#elif defined(MODE_CGA512)
					switch (detailshift)
					{
					case DETAIL_HIGH:
						if ((dc_x & 3) == 0)
							colfunc();
						break;
					case DETAIL_LOW:
						if ((dc_x & 1) == 0)
							colfunc();
						break;
					default:
						colfunc();
						break;
					}
#elif defined(MODE_MDA)
					if (first == rw_x || rw_x == rw_stopx - 1)
						R_DrawLineColumnTextMDA();
					else
						R_DrawEmptyColumnTextMDA();
#else
					colfunc();
#endif

					cc_rwx = mid;
				}
			}
			else
			{
				// no top wall
				if (markceiling)
					cc_rwx = yl - 1;
			}

			if (bottomtexture)
			{
				// bottom wall
				mid = (pixlow + HEIGHTUNIT - 1) >> HEIGHTBITS;
				pixlow -= pixlowstep;

				// no space above wall?
				if (mid <= cc_rwx)
					mid = cc_rwx + 1;

				if (mid > yh)
				{
					fc_rwx = yh + 1;
				}
				else
				{
					dc_yl = mid;
					dc_yh = yh;

#if defined(MODE_CGA16) || defined(MODE_CVB)
					if (detailshift == DETAIL_HIGH)
					{
						if ((dc_x & 1) == 0)
							colfunc();
					}
					else
						colfunc();
#elif defined(MODE_CGA512)
					switch (detailshift)
					{
					case DETAIL_HIGH:
						if ((dc_x & 3) == 0)
							colfunc();
						break;
					case DETAIL_LOW:
						if ((dc_x & 1) == 0)
							colfunc();
						break;
					default:
						colfunc();
						break;
					}
#elif defined(MODE_MDA)
					if (first == rw_x || rw_x == rw_stopx - 1)
						R_DrawLineColumnTextMDA();
					else
						R_DrawEmptyColumnTextMDA();
#else
					colfunc();
#endif

					fc_rwx = mid;
				}
			}
			else
			{
				// no bottom wall
				if (markfloor)
					fc_rwx = yh + 1;
			}

			if (maskedtexture)
			{
				// save texturecol
				//  for backdrawing of masked mid texture
				maskedtexturecol[rw_x] = texturecolumn;
			}
		}

		topfrac -= topstep;
		bottomfrac -= bottomstep;

		ceilingclip[rw_x] = cc_rwx;
		floorclip[rw_x] = fc_rwx;
	}
}

//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void R_StoreWallRange(int start,
					  int stop)
{

	fixed_t hyp;
	angle_t distangle, offsetangle;
	fixed_t vtop;
	int lightnum;


	fixed_t frontsector_floorheight;
	fixed_t frontsector_ceilingheight;
	fixed_t backsector_floorheight;
	fixed_t backsector_ceilingheight;

	if (uncappedFPS) {
		frontsector_floorheight = FixedInterpolate(frontsector->prevfloorheight, frontsector->floorheight, interpolation_weight);
		frontsector_ceilingheight = FixedInterpolate(frontsector->prevceilingheight, frontsector->ceilingheight, interpolation_weight);
		backsector_floorheight = FixedInterpolate(backsector->prevfloorheight, backsector->floorheight, interpolation_weight);
		backsector_ceilingheight = FixedInterpolate(backsector->prevceilingheight, backsector->ceilingheight, interpolation_weight);
	} else {
		frontsector_floorheight = frontsector->floorheight;
		frontsector_ceilingheight = frontsector->ceilingheight;
		backsector_floorheight = backsector->floorheight;
		backsector_ceilingheight = backsector->ceilingheight;
	}

	// don't overflow and crash
	if (ds_p == &drawsegs[MAXDRAWSEGS])
		return;

	sidedef = curline->sidedef;
	linedef = curline->linedef;

	// mark the segment as visible for auto map
	linedef->flags |= ML_MAPPED;

	// calculate rw_distance for scale calculation
	rw_normalangle = curline->angle;
	offsetangle = abs(rw_normalangle - rw_angle1);

	distangle = ANG90 - offsetangle;
	hyp = R_PointToDist(curline->v1->x, curline->v1->y);

	rw_distance = FixedMulEDX(hyp, finesine[distangle >> ANGLETOFINESHIFT]);

	ds_p->x1 = rw_x = start;
	ds_p->x2 = stop;
	ds_p->curline = curline;
	rw_stopx = stop + 1;

	// calculate scale at both ends and step
	ds_p->scale1 = rw_scale = R_ScaleFromGlobalAngle(start);

	if (stop > start)
	{
		// VITI95: OPTIMIZE
		ds_p->scale2 = R_ScaleFromGlobalAngle(stop);
		ds_p->scalestep = rw_scalestep = (ds_p->scale2 - rw_scale) / (stop - start);
	}
	else
	{
		ds_p->scale2 = ds_p->scale1;
	}

	// calculate texture boundaries
	//  and decide if floor / ceiling marks are needed
	worldtop = frontsector_ceilingheight - viewz;
	worldbottom = frontsector_floorheight - viewz;

	midtexture = toptexture = bottomtexture = maskedtexture = 0;
	ds_p->maskedtexturecol = NULL;

	if (!backsector)
	{
		// single sided line
		midtexture = texturetranslation[sidedef->midtexture];
		// a single sided line is terminal, so it must mark ends
		markfloor = markceiling = 1;
		if (linedef->dontPegBottom)
		{
			vtop = frontsector_floorheight +
				   textureheight[sidedef->midtexture];
			// bottom of texture at bottom
			rw_midtexturemid = vtop - viewz;
		}
		else
		{
			// top of texture at top
			rw_midtexturemid = worldtop;
		}
		rw_midtexturemid += sidedef->rowoffset;

		ds_p->silhouette = SIL_BOTH;
		ds_p->sprtopclip = screenheightarray;
		ds_p->sprbottomclip = negonearray;
		ds_p->bsilheight = MAXINT;
		ds_p->tsilheight = MININT;
	}
	else
	{
		// two sided line
		ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
		ds_p->silhouette = SIL_NONE;

		if (frontsector_floorheight > backsector_floorheight)
		{
			ds_p->silhouette = SIL_BOTTOM;
			ds_p->bsilheight = frontsector_floorheight;
		}
		else if (backsector_floorheight > viewz)
		{
			ds_p->silhouette = SIL_BOTTOM;
			ds_p->bsilheight = MAXINT;
		}

		if (frontsector_ceilingheight < backsector_ceilingheight)
		{
			ds_p->silhouette |= SIL_TOP;
			ds_p->tsilheight = frontsector_ceilingheight;
		}
		else if (backsector_ceilingheight < viewz)
		{
			ds_p->silhouette |= SIL_TOP;
			ds_p->tsilheight = MININT;
		}

		if (backsector_ceilingheight <= frontsector_floorheight)
		{
			ds_p->sprbottomclip = negonearray;
			ds_p->bsilheight = MAXINT;
			ds_p->silhouette |= SIL_BOTTOM;
		}

		if (backsector_floorheight >= frontsector_ceilingheight)
		{
			ds_p->sprtopclip = screenheightarray;
			ds_p->tsilheight = MININT;
			ds_p->silhouette |= SIL_TOP;
		}

		worldhigh = backsector_ceilingheight - viewz;
		worldlow = backsector_floorheight - viewz;

		// hack to allow height changes in outdoor areas
		if (frontsector->ceilingpic == skyflatnum && backsector->ceilingpic == skyflatnum)
		{
			worldtop = worldhigh;
		}

		markfloor = worldlow != worldbottom || backsector->floorpic != frontsector->floorpic || backsector->lightlevel != frontsector->lightlevel;
		markceiling = worldhigh != worldtop || backsector->ceilingpic != frontsector->ceilingpic || backsector->lightlevel != frontsector->lightlevel;

		if (backsector_ceilingheight <= frontsector_floorheight || backsector_floorheight >= frontsector_ceilingheight)
		{
			// closed door
			markceiling = markfloor = 1;
		}

		if (worldhigh < worldtop)
		{
			// top texture
			toptexture = texturetranslation[sidedef->toptexture];
			if (linedef->dontPegTop)
			{
				// top of texture at top
				rw_toptexturemid = worldtop;
			}
			else
			{
				vtop = backsector_ceilingheight + textureheight[sidedef->toptexture];

				// bottom of texture
				rw_toptexturemid = vtop - viewz;
			}
		}
		if (worldlow > worldbottom)
		{
			// bottom texture
			bottomtexture = texturetranslation[sidedef->bottomtexture];

			if (linedef->dontPegBottom)
			{
				// bottom of texture at bottom
				// top of texture at top
				rw_bottomtexturemid = worldtop;
			}
			else // top of texture at top
				rw_bottomtexturemid = worldlow;
		}
		rw_toptexturemid += sidedef->rowoffset;
		rw_bottomtexturemid += sidedef->rowoffset;

		// allocate space for masked texture tables
		if (sidedef->midtexture)
		{
			// masked midtexture
			maskedtexture = 1;
			ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
			lastopening += rw_stopx - rw_x;
		}
	}

	// calculate rw_offset (only needed for textured lines)
	segtextured = midtexture | toptexture | bottomtexture | maskedtexture;

	if (segtextured)
	{
		offsetangle = rw_normalangle - rw_angle1;

		if (offsetangle > ANG180)
			offsetangle = -offsetangle;

		rw_offset = FixedMulEDX(hyp, finesine[offsetangle >> ANGLETOFINESHIFT]);

		if (rw_normalangle - rw_angle1 < ANG180)
			rw_offset = -rw_offset;

		rw_offset += sidedef->textureoffset + curline->offset;
		rw_centerangle = viewangle90 - rw_normalangle;

		// calculate light table
		//  use different light tables
		//  for horizontal / vertical / diagonal
		// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
		if (!fixedcolormap)
		{
			lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT) + extralight;

			if (curline->v1->y == curline->v2->y)
				lightnum--;
			else if (curline->v1->x == curline->v2->x)
				lightnum++;

			// Lightnum between 0 and 15
			if (lightnum < 0)
				walllights = scalelight[0];
			else if (lightnum > LIGHTLEVELS - 1)
				walllights = scalelight[LIGHTLEVELS - 1];
			else
				walllights = scalelight[lightnum];
		}
	}

	// if a floor / ceiling plane is on the wrong side
	//  of the view plane, it is definitely invisible
	//  and doesn't need to be marked.

	if (frontsector_floorheight >= viewz)
	{
		// above view plane
		markfloor = 0;
	}

	if (frontsector_ceilingheight <= viewz && frontsector->ceilingpic != skyflatnum)
	{
		// below view plane
		markceiling = 0;
	}

	// calculate incremental stepping values for texture edges
	worldtop >>= 4;
	worldbottom >>= 4;

	topstep = FixedMul(rw_scalestep, worldtop);
	topfrac = centeryfracshifted - FixedMul(worldtop, rw_scale);

	bottomstep = FixedMul(rw_scalestep, worldbottom);
	bottomfrac = centeryfracshifted - FixedMul(worldbottom, rw_scale);

	if (backsector)
	{
		worldhigh >>= 4;
		worldlow >>= 4;

		if (worldhigh < worldtop)
		{
			pixhigh = centeryfracshifted - FixedMul(worldhigh, rw_scale);
			pixhighstep = FixedMul(rw_scalestep, worldhigh);
		}

		if (worldlow > worldbottom)
		{
			pixlow = centeryfracshifted - FixedMul(worldlow, rw_scale);
			pixlowstep = FixedMul(rw_scalestep, worldlow);
		}
	}

	// render it
	if (markceiling)
		ceilingplane = R_CheckPlane(ceilingplane, rw_x, rw_stopx - 1);

	if (markfloor)
		floorplane = R_CheckPlane(floorplane, rw_x, rw_stopx - 1);

	renderSegLoop();

	// save sprite clipping info
	if (((ds_p->silhouette & SIL_TOP) || maskedtexture) && !ds_p->sprtopclip)
	{
		CopyWords(ceilingclip + start, lastopening, rw_stopx - start);
		// memcpy(lastopening, ceilingclip + start, 2 * (rw_stopx - start));
		ds_p->sprtopclip = lastopening - start;
		lastopening += rw_stopx - start;
	}

	if (((ds_p->silhouette & SIL_BOTTOM) || maskedtexture) && !ds_p->sprbottomclip)
	{
		CopyWords(floorclip + start, lastopening, rw_stopx - start);
		// memcpy(lastopening, floorclip + start, 2 * (rw_stopx - start));
		ds_p->sprbottomclip = lastopening - start;
		lastopening += rw_stopx - start;
	}

	if (maskedtexture)
	{
		if (!(ds_p->silhouette & SIL_TOP))
		{
			ds_p->silhouette |= SIL_TOP;
			ds_p->tsilheight = MININT;
		}
		if (!(ds_p->silhouette & SIL_BOTTOM))
		{
			ds_p->silhouette |= SIL_BOTTOM;
			ds_p->bsilheight = MAXINT;
		}
	}

	ds_p++;
}
