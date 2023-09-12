/**************************************************************************
 *                     WAD loading and saving routines                    *
 *                                                                        *
 * WAD loading and reading routines: by me, me, me!                       *
 *                                                                        *
 **************************************************************************/

#ifndef __WADDIR_H_INCLUDED__
#define __WADDIR_H_INCLUDED__

/*************************** Includes *************************************/

#include<stdio.h>
#include<stdlib.h>

#include "errors.h"

/*************************** Defines **************************************/

#define MAXENTRIES 5000

typedef enum
{
      IWAD,
      PWAD,
      NONWAD
} wadtype;

/*************************** Structs *************************************/

typedef struct
{
        long offset;
        long length;
        char name[8];
} entry_t;

/* portable structure IO (see lumps.h) */
#define ENTRY_OFF	0
#define ENTRY_LEN	4
#define ENTRY_NAME	8
#define ENTRY_SIZE	16

/*************************** Prototypes ***********************************/

int readwad();
void writewad();
char *convert_string8(entry_t entry);
entry_t findinfo(char *entrytofind);
void addentry(entry_t entry);
int entry_exist(char *entrytofind);
void *cachelump(int entrynum);
void copywad(char *newfile);

int readwadheader(FILE *fp);
int writewadheader(FILE *fp);
int readwaddir(FILE *fp);
int readwadentry(FILE *fp, entry_t *entry);
int writewaddir(FILE *fp);
int writewadentry(FILE *fp, entry_t *entry);

int islevel(int entry);
int isnum(char n);
int islevelname(char *s);
int islevelentry(char *s);
int findlevelsize(char *s);

/*************************** Globals *************************************/

/** WADDIR.C **/

extern FILE *wadfp;
extern union REGS r;       /*how am I supposed to twiddle any frobs without this? :) */
extern char picentry[8];
extern long numentries, diroffset;
extern entry_t wadentry[MAXENTRIES];
extern wadtype wad;

#endif
