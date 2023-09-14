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

void pal_compress(FILE *fp)
{
	char *lumppal = "PLAYPAL";

	int entrynum;
	unsigned char *working;
	int i,j;
	int posold = 0;
	int posnew = 0;
	unsigned char newpalette[12 * VGA_PALETTE_SIZE];
	entry_t newplaypal;

	entrynum = entry_exist(convert_string8_lumpname(lumppal));
	working = cachelump(entrynum);

	for (i = 0; i < 14; i++)
	{
		if (i == 1 || i == 9) // Omit palettes 1 and 9
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

	removeentry(lumppal);
	
	newplaypal.length = 12 * VGA_PALETTE_SIZE;
	strcpy(newplaypal.name, lumppal);
	newplaypal.offset = ftell(fp);

	fwrite(newpalette, 1, 12 * VGA_PALETTE_SIZE, fp);

	addentry(newplaypal);

	writewad();
}
