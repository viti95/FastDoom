/**************************************************************************
 *                  The WADPTR project : LUMPS.C                          *
 *                      Header file for LUMPS.C                           *
 *                                                                        *
 *            Functions for compressing individual lumps                  *
 *                                                                        *
 * P_* : Sidedef packing extension routines. Combines sidedefs which are  *
 *       identical in a level, and shares them between multiple linedefs  *
 *                                                                        *
 * S_* : Graphic squashing routines. Combines identical columns in        *
 *       graphic lumps to make them smaller                               *
 *                                                                        *
 **************************************************************************/

#ifndef __LUMPS_H_INCLUDED__
#define __LUMPS_H_INCLUDED__

/****************************** Includes **********************************/

#include <stdio.h>
#include <stdlib.h>

#include "waddir.h"
#include "errors.h"

/****************************** Globals ***********************************/

extern char *p_linedefres; /* the new linedef resource */
extern char *p_sidedefres; /* the new sidedef resource */
extern short s_height, s_width, s_loffset, s_toffset; /* picture width, height etc. */
extern unsigned char *s_columns;					   /* the location of each column in the lump */
extern long s_colsize[400];						   /* the length(in bytes) of each column */

/***************************** Prototypes *********************************/

void p_pack(char *levelname);
int p_ispacked(char *s);

char *s_squash(char *s);
int s_isgraphic(char *s);

/******************************* Structs **********************************/

typedef struct
{
	short xoffset;
	short yoffset;
	char upper[8];
	char middle[8];
	char lower[8];
	unsigned short sector_ref;
} sidedef_t;

typedef struct
{
	unsigned short vertex1;
	unsigned short vertex2;
	unsigned short flags;
	unsigned short types;
	unsigned short tag;
	unsigned short sidedef1;
	unsigned short sidedef2;
} linedef_t;

#define NO_SIDEDEF ((unsigned short)-1)

/*
 * Portable structure IO
 * (to handle endianness; also neither struct is a multiple of 4 in size)
 */
#define SDEF_XOFF 0
#define SDEF_YOFF 2
#define SDEF_UPPER 4
#define SDEF_MIDDLE 12
#define SDEF_LOWER 20
#define SDEF_SECTOR 28
#define SDEF_SIZE 30

#define LDEF_VERT1 0
#define LDEF_VERT2 2
#define LDEF_FLAGS 4
#define LDEF_TYPES 6
#define LDEF_TAG 8
#define LDEF_SDEF1 10
#define LDEF_SDEF2 12
#define LDEF_SIZE 14

linedef_t *
readlinedefs(int lumpnum, FILE *fp);
int writelinedefs(linedef_t *lines, int bytes, FILE *fp);
sidedef_t *
readsidedefs(int lumpnum, FILE *fp);
int writesidedefs(sidedef_t *sides, int bytes, FILE *fp);

#endif // ifndef __LUMPS_H_INCLUDED__
