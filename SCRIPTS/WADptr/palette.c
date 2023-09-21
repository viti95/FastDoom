/**************************************************************************
 *                                                                        *
 * Copyright(C) 1998-2011 Simon Howard, Andreas Dehmel                    *
 *                                                                        *
 * This program is free software; you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation; either version 2 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program; if not, write to the Free Software            *
 * Foundation, Inc, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA *
 *                                                                        *
 *                     The WADPTR Project: PALETTE.C                      *
 *                                                                        *
 *  Compresses WAD files by merging identical lumps in a WAD file and     *
 *  sharing them between multiple wad directory entries.                  *
 *                                                                        *
 **************************************************************************/

#include "wadmerge.h"
#include "wadptr.h"
#include "palette.h"

#define VGA_PALETTE_SIZE 768
#define COLORMAP_ENTRY_SIZE 256

void pal_compress(FILE *fp)
{
	char *lumppal = "PLAYPAL";

	int entrynum;
	unsigned char *working;
	unsigned int i,j;
	unsigned int posold = 0;
	unsigned int posnew = 0;
	unsigned char newpalette[12 * VGA_PALETTE_SIZE];

	entrynum = entry_exist(lumppal);
	working = cachelump(entrynum);

	for (i = 0; i < 14; i++)
	{
		if ((i == 1) || (i == 9))
		{
			posold += VGA_PALETTE_SIZE;
			continue;
		}
			
		for (j = 0; j < VGA_PALETTE_SIZE; j++)
		{
			newpalette[posnew] = working[posold];
			posnew++;
			posold++;
		}
	}

	wadentry[entrynum].length = 12 * VGA_PALETTE_SIZE;
	wadentry[entrynum].offset = ftell(fp);

	fwrite(newpalette, 1, 12 * VGA_PALETTE_SIZE, fp);
}

void colormap_compress(FILE *fp)
{
	char *colormap = "COLORMAP";

	int entrynum;
	unsigned char *working;
	unsigned int i,j;
	unsigned int posold = 0;
	unsigned int posnew = 0;
	unsigned char newcolormap[33 * COLORMAP_ENTRY_SIZE];

	entrynum = entry_exist(colormap);
	working = cachelump(entrynum);

	memcpy(newcolormap, working, 33 * COLORMAP_ENTRY_SIZE);

	wadentry[entrynum].length = 33 * COLORMAP_ENTRY_SIZE;
	wadentry[entrynum].offset = ftell(fp);

	fwrite(newcolormap, 1, 33 * COLORMAP_ENTRY_SIZE, fp);
}
