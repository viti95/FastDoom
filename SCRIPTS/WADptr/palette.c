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
	unsigned int i, j;
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
	unsigned int i, j;
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

void statusbar_merge(FILE *fp)
{
	char *barname = "STBAR";
	char *armsname = "STARMS";

	int barnum;
	unsigned char *bardata;

	int armsnum;
	unsigned char *armsdata;

	barnum = entry_exist(barname);
	bardata = cachelump(barnum);

	armsnum = entry_exist(armsname);
	armsdata = cachelump(armsnum);

	s_width = READ_SHORT(bardata);
	s_height = READ_SHORT(bardata + 2);
	s_loffset = READ_SHORT(bardata + 4);
	s_toffset = READ_SHORT(bardata + 6);
	s_columns = (unsigned char *)(bardata + 8);

	// READ STBAR

	unsigned char *newbar;

	newbar = malloc(32 * 320);

	for (int j = 0; j < s_width; j++)
	{
		long pointer = READ_LONG(s_columns + 4 * j);

		do
		{
			int row;
			int postHeight;

			row = bardata[pointer];

			if (row != 255 && (postHeight = bardata[++pointer]) != 255)
			{
				pointer++; // unused value

				for (int i = 0; i < postHeight; i++)
				{
					if (row + i < s_height && pointer < wadentry[barnum].length - 1)
					{
						newbar[((row + i) * s_width) + j] = bardata[++pointer];
					}
				}

				pointer++; // unused value
			}
			else
			{
				break;
			}
		} while (pointer < wadentry[barnum].length - 1 && bardata[++pointer] != 255);
	}

	// COPY STARMS INTO STBAR

	s_width = READ_SHORT(armsdata);
	s_height = READ_SHORT(armsdata + 2);
	s_loffset = READ_SHORT(armsdata + 4);
	s_toffset = READ_SHORT(armsdata + 6);
	s_columns = (unsigned char *)(armsdata + 8);

	for (int j = 0; j < s_width; j++)
	{
		long pointer = READ_LONG(s_columns + 4 * j);

		do
		{
			int row;
			int postHeight;

			row = armsdata[pointer];

			if (row != 255 && (postHeight = armsdata[++pointer]) != 255)
			{
				pointer++; // unused value

				for (int i = 0; i < postHeight; i++)
				{
					if (row + i < s_height && pointer < wadentry[armsnum].length - 1)
					{
						newbar[((row + i) * 320) + j + 104] = armsdata[++pointer];
					}
				}

				pointer++; // unused value
			}
			else
			{
				break;
			}
		} while (pointer < wadentry[armsnum].length - 1 && armsdata[++pointer] != 255);
	}

	// UPDATE STBAR
	s_width = READ_SHORT(bardata);
	s_height = READ_SHORT(bardata + 2);
	s_loffset = READ_SHORT(bardata + 4);
	s_toffset = READ_SHORT(bardata + 6);
	s_columns = (unsigned char *)(bardata + 8);

	for (int j = 0; j < s_width; j++)
	{
		long pointer = READ_LONG(s_columns + 4 * j);

		do
		{
			int row;
			int postHeight;

			row = bardata[pointer];

			if (row != 255 && (postHeight = bardata[++pointer]) != 255)
			{
				pointer++; // unused value

				for (int i = 0; i < postHeight; i++)
				{
					if (row + i < s_height && pointer < wadentry[barnum].length - 1)
					{
						gotolump(barnum, ++pointer);
						fwrite(&newbar[((row + i) * s_width) + j], 1, 1, wadfp);
					}
				}

				pointer++; // unused value
			}
			else
			{
				break;
			}
		} while (pointer < wadentry[barnum].length - 1 && bardata[++pointer] != 255);
	}
}