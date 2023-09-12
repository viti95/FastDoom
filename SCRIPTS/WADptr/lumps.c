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
 *                  The WADPTR project : LUMPS.C                          *
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

/******************************* INCLUDES **********************************/

#include <string.h>

#include "lumps.h"
#include "wadptr.h"

enum { false, true };

/****************************** PROTOTYPES ********************************/

void p_findinfo();
void p_dopack();
void p_update();
void p_buildlinedefs(linedef_t *linedefs);
void p_rebuild();

int s_findgraf(unsigned char *s);
int s_find_colsize(unsigned char *col1);
int s_comp_columns(unsigned char *col1, unsigned char *col2);

/******************************* GLOBALS **********************************/

int p_levelnum;   /* entry number (not 1 as in map01, 1 as in entry 1 in wad) */
int p_sidedefnum; /* sidedef wad entry number */
int p_linedefnum; /* linedef wad entry number */
int p_num_linedefs=0, p_num_sidedefs=0; /* number of sd/lds do not get confused */
char p_working[50]; /* the name of the level resource eg. "MAP01" */

sidedef_t *p_newsidedef;   /* the new sidedefs */
linedef_t *p_newlinedef;   /* the new linedefs */
int *p_movedto;          /* keep track of where the sidedefs are now */
int p_newnum=0;               /* the new number of sidedefs */

char *p_linedefres=0;         /* the new linedef resource */
char *p_sidedefres=0;         /* the new sidedef resource */

/*    Graphic squashing globals */
int s_equalcolumn[400];  /* 1 for each column: another column which is */
                         /* identical or -1 if there isn't one */
short s_height, s_width, s_loffset, s_toffset;  /* picture width, height etc. */
unsigned char *s_columns;         /* the location of each column in the lump */
long s_colsize[400];     /* the length(in bytes) of each column */


/* Pack a level ***********************************************************/

/* Call p_pack() with the level name eg. p_pack("MAP01");. p_pack will then */
/* pack that level. The new sidedef and linedef lumps are pointed to by */
/* p_sidedefres and p_linedefres. These must be free()d by other functions */
/* when they are no longer needed, as p_pack does not do this. */

void p_pack(char *levelname)
{
         sidedef_t *sidedefs;
         linedef_t *linedefs;

         strcpy(p_working,levelname);

         p_findinfo();

         p_rebuild();                 /* remove unused sidedefs */

         sidedefs=p_newsidedef;
         linedefs=p_newlinedef;

         p_findinfo();                /* find what's needed */

         p_dopack(sidedefs);          /* pack the sidedefs in memory */

         p_buildlinedefs(linedefs);   /* update linedefs + save to *??res */

         /* saving of the wad directory is left to external sources */

         p_linedefres=(char *)p_newlinedef;   /* point p_linedefres and p_sidedefres at */
         p_sidedefres=(char *)p_newsidedef; /* the new lumps */

         wadentry[p_sidedefnum].length=p_newnum * SDEF_SIZE;

}

/* Unpack a level *********************************************************/

/* Same thing, in reverse. Saves the new sidedef and linedef lumps to */
/* p_sidedefres and p_linedefres. */

void p_unpack(char *resname)
{
         strcpy(p_working,resname);

         p_findinfo();
         p_rebuild();

         p_linedefres=(char *)p_newlinedef;
         p_sidedefres=(char *)p_newsidedef;
}

/* Find if a level is packed **********************************************/

int p_ispacked(char *s)
{
         linedef_t *linedefs;
         int count;/*count2;*/

         strcpy(p_working,s);

         p_findinfo();

         /*linedefs = cachelump(p_linedefnum);*/
	 linedefs = readlinedefs(p_linedefnum, wadfp);

         /* alloc p_movedto. 10 extra sidedefs for safety */
         p_movedto = malloc(sizeof(int) * (p_num_sidedefs+10));

         if(!p_movedto) errorexit("p_ispacked: couldn't malloc p_movedto\n");

         /* uses p_movedto to find if */
         /* same sidedef has already been used */
         /* on an earlier linedef checked */

         for(count=0;count<p_num_sidedefs;count++) /* reset p_movedto to 0 */
                 p_movedto[count]=0;

         for(count=0;count<p_num_linedefs;count++)   /* now check */
         {
                 if(linedefs[count].sidedef1 != NO_SIDEDEF)
                 {                                 /* side */
                        if(p_movedto[linedefs[count].sidedef1]) /* already used */
                        {                            /* on a previous linedef */
                                free(linedefs);
                                free(p_movedto);
                                return true; /* must be packed */
                        }
                        else        /* mark it as used for later reference */
                                p_movedto[linedefs[count].sidedef1]=1;
                 }
                 if(linedefs[count].sidedef2 != NO_SIDEDEF)
                 {
                        if(p_movedto[linedefs[count].sidedef2])
                        {
                                free(linedefs);
                                free(p_movedto);
                                return true;
                        }
                        else
                                p_movedto[linedefs[count].sidedef2]=1;
                 }
         }
         free(linedefs);
         free(p_movedto);
         return false; /* cant be packed: none of the sidedefs are shared */
}

/* Find neccesary stuff before processing *********************************/

void p_findinfo()
{
         int count, n;

         /* first find the level entry */
         for(count=0;count<numentries;count++)
         {
                  if(!strncmp(wadentry[count].name,p_working,8))
                  {                   /* matches the name given */
                          p_levelnum=count;
                          break;
                  }
         }
         if(count==numentries)
          errorexit("p_findinfo: Couldn't find level: %s\n",p_working);


         n=0; /* bit of a hack */

         /* now find the sidedefs */
         for(count=p_levelnum+1;count<numentries;count++)
         {
                  if(!islevelentry(convert_string8(wadentry[count])))
                       errorexit("p_findinfo: Can't find sidedef/linedef entries!\n");

                  if(!strncmp(wadentry[count].name,"SIDEDEFS",8))
                  {
                           n++;
                           p_sidedefnum=count;
                  }
                  if(!strncmp(wadentry[count].name,"LINEDEFS",8))
                  {
                           n++;
                           p_linedefnum=count;
                  }
                  if(n==2) break;   /* found both :) */
         }
         /* find number of linedefs and sidedefs for later.. */
         p_num_linedefs = wadentry[p_linedefnum].length / LDEF_SIZE;
         p_num_sidedefs = wadentry[p_sidedefnum].length / SDEF_SIZE;
}


/* Actually pack the sidedefs *******************************************/

void p_dopack(sidedef_t *sidedefs)
{
         int count, count2;
         /*sidedef_t *newsidedef;*/

         p_newsidedef = malloc(wadentry[p_sidedefnum].length * 10);
         if(!p_newsidedef)
         {
            errorexit("p_dopack: could not alloc memory for new sidedefs\n");
         }

                /* allocate p_movedto */
         p_movedto = malloc(sizeof(int) * (p_num_sidedefs+10));

         p_newnum=0;
         for(count=0;count<p_num_sidedefs;count++) /* each sidedef in turn */
         {
                if((count % 100) == 0)
                {
                        /* time for a percent-done update */
                        int x, y;
                        x = wherex(); y = wherey();
                        double c = count;
                        double p = p_num_sidedefs;
                        printf("%%%.0f  ", 100*(c*c+c)/(p*p+p));
                        fflush(stdout);
                        gotoxy(x, y);
                }
                for(count2=0;count2<p_newnum;count2++) /* check previous */
                {
                        if(!memcmp(&p_newsidedef[count2],
                              &sidedefs[count], sizeof(sidedef_t)))
                        {      /* they are identical: this one can be removed */
                              p_movedto[count]=count2;
                              break;
                        }
                }
                /* a sidedef like this does not yet exist: add one */
		if (count2 >= p_newnum)
		{
                  memcpy(&p_newsidedef[p_newnum],&sidedefs[count],sizeof(sidedef_t));
                  p_movedto[count]=p_newnum;
                  p_newnum++;
		}
         }
         /* all done! */
         free(sidedefs);   /* fly free, little sidedefs!! */
}

/* Update the linedefs and save sidedefs *********************************/

void p_buildlinedefs(linedef_t *linedefs)
{
       int count;

       /* update the linedefs with where the sidedefs have been moved to, */
       /* using p_movedto[] to find where they now are.. */

       for(count=0;count<p_num_linedefs;count++)
       {
               if(linedefs[count].sidedef1 != NO_SIDEDEF)
                 linedefs[count].sidedef1=p_movedto[linedefs[count].sidedef1];
               if(linedefs[count].sidedef2 != NO_SIDEDEF)
                 linedefs[count].sidedef2=p_movedto[linedefs[count].sidedef2];
       }

       p_newlinedef=linedefs;

       /* free p_movedto now. */
       free(p_movedto);
}

/* Rebuild the sidedefs ***************************************************/

void p_rebuild()
{
         sidedef_t *sidedefs;
         linedef_t *linedefs;
         int count;

         /*sidedefs=cachelump(p_sidedefnum);
         linedefs=cachelump(p_linedefnum);*/
	 sidedefs = readsidedefs(p_sidedefnum, wadfp);
	 linedefs = readlinedefs(p_linedefnum, wadfp);

         p_newsidedef=malloc(wadentry[p_sidedefnum].length * 10);

         if(!p_newsidedef)
         errorexit("p_rebuild: could not alloc memory for new sidedefs\n");

         p_newnum=0;

         for(count=0;count<p_num_linedefs;count++)
         {
                if(linedefs[count].sidedef1 != NO_SIDEDEF)
                {
                       memcpy(&(p_newsidedef[p_newnum]),
                          &(sidedefs[linedefs[count].sidedef1]),
                                  sizeof(sidedef_t));
                       linedefs[count].sidedef1=p_newnum;
                       p_newnum++;
                }
                if(linedefs[count].sidedef2 != NO_SIDEDEF)
                {
                       memcpy(&(p_newsidedef[p_newnum]),
                          &(sidedefs[linedefs[count].sidedef2]),
                                  sizeof(sidedef_t));
                       linedefs[count].sidedef2=p_newnum;
                       p_newnum++;
                }
         }
         /* update the wad directory */
         wadentry[p_sidedefnum].length = p_newnum * SDEF_SIZE;

         free(sidedefs);   /* no longer need the old sidedefs */
         p_newlinedef=linedefs;  /* still need the old linedefs: */
                                         /* they have been updated */
}


/*
 *  compress by matching entire columns if defined (old approach), or by
 *  matching postfixes as well if undefined (new approach, experimental)
 */
/*#define ENTIRE_COLUMNS*/

/* Graphic squashing routines *********************************************/

/* Squash a graphic lump **************************************************/

/* Squashes a graphic. Call with the lump name - eg. s_squash("TITLEPIC"); */
/* returns a pointer to the new(compressed) lump. This must be free()d when */
/* it is no longer needed, as s_squash() does not do this itself. */

char *s_squash(char *s)
{
       unsigned char *working , *newres;
       int entrynum, count;
       /*int in_post, n, n2, count2;*/
       unsigned char *newptr;
       long lastpt;

       if(!s_isgraphic(s)) return NULL;
       entrynum=entry_exist(s);
       working=cachelump(entrynum);
       if((long)working==-1) errorexit("squash: Couldn't find %s\n",s);

       if(!s_findgraf(working)) return (char*)working;    /* find posts to be killed */
                                           /* if none, return original lump */
       newres=malloc(100000);  /* alloc memory for the new pic resource */

       WRITE_SHORT(newres, s_width);   /* find various info: size,offset etc. */
       WRITE_SHORT(newres+2, s_height);
       WRITE_SHORT(newres+4, s_loffset);
       WRITE_SHORT(newres+6, s_toffset);

       newptr=(unsigned char *)(newres+8); /* the new column pointers for the new lump */

       lastpt=8+(s_width*4);     /* last point in the lump */

       for(count=0;count<s_width;count++) /* go through each column in turn */
       {
               if(s_equalcolumn[count]==-1) /* add a new column */
               {
	               WRITE_LONG(newptr + 4*count, lastpt); /* point this column to lastpt */
                       memcpy(newres+lastpt,working+READ_LONG(s_columns+4*count),
                                   s_colsize[count]); /* add the new column */
                       lastpt+=s_colsize[count]; /* update lastpt */
               }
               else
               {
#ifdef ENTIRE_COLUMNS
                       /* identical column already in: use that one */
                       memcpy(newptr + 4*count, newptr + 4*s_equalcolumn[count], 4);
#else
                       /* postfix compression, see s_findgraf() */
                       long identOff;

                       identOff = READ_LONG(newptr + 4*s_equalcolumn[count]);
                       identOff += s_colsize[s_equalcolumn[count]] - s_colsize[count];
                       WRITE_LONG(newptr+4*count, identOff);
		       /*{
		               long o1, o2;
		               o1 = READ_LONG(s_columns+4*s_equalcolumn[count])
			          + s_colsize[s_equalcolumn[count]]-s_colsize[count];
		               o2 = READ_LONG(newptr+4*count);
		               if (memcmp(working+o1, newres+o2, s_colsize[count]) != 0) errorexit("ARGH!\n");
		       }*/
#endif		       
	       }
       }

       if(lastpt>wadentry[entrynum].length )    /* use the smallest */
       {
             free(newres); /* the new resource was bigger than the old one! */
             return (char*)working; /* use the old one */
       }
       else
       {               /* new one was smaller: use it */
             wadentry[entrynum].length=lastpt; /* update the lump size */
             free(working);  /* free back the original lump */
             return (char*)newres; /* use the new one */
       }
}

/* Unsquash a picture ******************************************************/

/* Exactly the same format as s_squash(). See there for more details. */

char *s_unsquash(char *s)
{
       unsigned char *working , *newres;
       int entrynum, count;
       /*int in_post, n, n2, count2;*/
       long lastpt;
       unsigned char *newptr;

       if(!s_isgraphic(s)) return NULL;
       entrynum=entry_exist(s);     /* cache the lump */
       working=cachelump(entrynum);
       if((long)working==-1) errorexit("unsquash: Couldn't find %s\n",s);

       s_width=READ_SHORT(working);       /* find various info */
       s_height=READ_SHORT(working+2);
       s_loffset=READ_SHORT(working+4);
       s_toffset=READ_SHORT(working+6);
       s_columns=(unsigned char*)(working+8);

       /* find column lengths */
       for(count=0;count<s_width;count++)
              s_colsize[count]=s_find_colsize(working+READ_LONG(s_columns+4*count));

       newres=malloc(100000);  /* alloc memory for the new pic resource */

       WRITE_SHORT(newres, s_width);   /* find various info: size,offset etc. */
       WRITE_SHORT(newres+2, s_height);
       WRITE_SHORT(newres+4, s_loffset);
       WRITE_SHORT(newres+6, s_toffset);

       newptr=(unsigned char *)(newres+8); /* the new column pointers for the new lump */

       lastpt=8+(s_width*4);     /* last point in the lump- point to start */
                                 /* of column data */

       for(count=0;count<s_width;count++) /* go through each column in turn */
       {
               WRITE_LONG(newptr + 4*count, lastpt); /* point this column to lastpt */
               memcpy(newres+lastpt,working+READ_LONG(s_columns+4*count),
                          s_colsize[count]); /* add the new column */
               lastpt+=s_colsize[count]; /* update lastpt */
       }

       wadentry[entrynum].length=lastpt; /* update the lump size */
       free(working);                    /* free back the original lump */
       return (char*)newres;                    /* use the new one */
}


/* Find the redundant columns **********************************************/

int s_findgraf(unsigned char *x)
{
       int count, count2;
       /*int entrynum;*/    /* entry number in wad */
       int num_killed=0;

       s_width=READ_SHORT(x);
       s_height=READ_SHORT(x+2);
       s_loffset=READ_SHORT(x+4);
       s_toffset=READ_SHORT(x+6);

       s_columns=(unsigned char*)(x+8);

       for(count=0;count<s_width;count++) /* each column in turn */
       {
              long tmpcol;

              s_equalcolumn[count]=-1; /* first assume no identical column */
                                       /* exists */

                                      /* find the column size */
	      tmpcol = READ_LONG(s_columns + 4*count);
              s_colsize[count]=s_find_colsize((unsigned char*)x+tmpcol);

              for(count2=0;count2<count;count2++) /*check all previous columns */
              {
#ifdef ENTIRE_COLUMNS
                     if(s_colsize[count]!=s_colsize[count2])
                            continue;   /* columns are different sizes: must */
                                        /* be different */
                     if(!memcmp(x+tmpcol,x+READ_LONG(s_columns+4*count2), s_colsize[count]))
                     {                  /* columns are identical */
                            s_equalcolumn[count]=count2;
                            num_killed++;        /* increase deathcount */
                            break;      /* found one, exit the loop */
                     };
#else
                     /* compression is also possible if col is a postfix of col2 */
                     if (s_colsize[count] > s_colsize[count2])
                            continue;   /* new column longer than previous, can't be postfix */

                     if (!memcmp(x+tmpcol, x+READ_LONG(s_columns+4*count2)+s_colsize[count2]-s_colsize[count], s_colsize[count]))
                     {
                            s_equalcolumn[count]=count2;
                            num_killed++;
                            break;
                     }
#endif
              }
       }
       return num_killed;  /* tell squash how many can be 'got rid of' */
}

/* Find the size of a column ***********************************************/

int s_find_colsize(unsigned char *col1)
{
       int count=0;

       while(1)
       {
            if(col1[count]==255)
            {                   /* no more posts */
                  return count+1;  /* must be +1 or the pic gets cacked up */
            }
            count=count+col1[count+1]+4; /* jump to the beginning of the next */
                                         /* post */
       }
}

/* Find if a graphic is squashed *******************************************/

int s_is_squashed(char *s)
{
      int entrynum;
      char *pic;
      int count,count2;

      entrynum=entry_exist(s);
      if(entrynum==-1) errorexit("is_squashed: %s does not exist!\n",s);
      pic=cachelump(entrynum); /* cache the lump */

      s_width=READ_SHORT(pic);      /* find lump info */
      s_height=READ_SHORT(pic+2);
      s_loffset=READ_SHORT(pic+4);
      s_toffset=READ_SHORT(pic+6);

      s_columns=(unsigned char*)(pic+8); /* find the column locations */

      for(count=0;count<s_width;count++) /* each column */
      {
            long tmpcol;

	    tmpcol = READ_LONG(s_columns+4*count);
            for(count2=0;count2<count;count2++) /* every previous column */
            {
                  if(tmpcol==READ_LONG(s_columns+4*count2))
                  {  /* these columns have the same lump location */
                        free(pic);
                        return true;  /* it is squashed */
                  }
            }
      }
      free(pic);
      return false; /* it cant be : no 2 columns have the same lump location */

}

/* Is this a graphic ? *****************************************************/

int s_isgraphic(char *s)
{
      unsigned char *graphic;
      int entrynum , count;
      short width, height,loffset, toffset;
      unsigned char *columns;

      if(!strcmp(s,"ENDOOM")) return false;   /* endoom */
      /* if(islevel(s)) return false; */
      if(islevelentry(s)) return false;
      if(s[0]=='D' && ((s[1]=='_') || (s[1]=='S'))) /* sfx or music */
       {
                   return false;
       }

      entrynum=entry_exist(s);
      if(entrynum==-1) errorexit("isgraphic: %s does not exist!\n",s);
      if (wadentry[entrynum].length <= 0) return false;	/* don't read data from 0 size lumps */
      graphic=cachelump(entrynum);

      width=READ_SHORT(graphic);
      height=READ_SHORT(graphic+2);
      loffset=READ_SHORT(graphic+4);
      toffset=READ_SHORT(graphic+6);
      columns=(unsigned char*)(graphic+8);

      if((width>320) || (height>200) || (width==0) || (height==0)
        || (width<0) || (height<0) )
      {
            free(graphic);
            return false;
      }

         /* it could be a graphic, but better safe than sorry */
      if((wadentry[entrynum].length==4096) || /* flat; */
         (wadentry[entrynum].length==4000))  /* endoom */
      {
            free(graphic);
            return false;
      }

      for(count=0;count<width;count++)
      {
             if(READ_LONG(columns + 4*count)>wadentry[entrynum].length)
             {    /* cant be a graphic resource then -offset outside lump */
                   free(graphic);
                   return false;
             }
      }
      free(graphic);
      return true;   /* if its passed all these checks it must be(well probably) */
}



/*
 *  portable reading / writing of linedefs and sidedefs
 *  by Andreas Dehmel (dehmel@forwiss.tu-muenchen.de)
 */

static const int convbuffsize = 0x8000;
static unsigned char convbuffer[0x8000];

linedef_t *readlinedefs(int lumpnum, FILE *fp)
{
  linedef_t *lines;
  int i, numlines, validbytes;
  unsigned char *cptr;

  numlines = wadentry[lumpnum].length / LDEF_SIZE;
  if ((lines = (linedef_t*)malloc(numlines * sizeof(linedef_t))) == NULL)
  {
    fprintf(stderr, "Unable to claim memory for linedefs\n");
    exit(-1);
  }
  fseek(fp, wadentry[lumpnum].offset, SEEK_SET);
  validbytes = 0; cptr = convbuffer;
  for (i=0; i<numlines; i++)
  {
    /* refill buffer? */
    if (validbytes < LDEF_SIZE)
    {
      if (validbytes != 0) memcpy(convbuffer, cptr, validbytes);
      validbytes += fread(convbuffer + validbytes, 1, convbuffsize - validbytes, fp);
      cptr = convbuffer;
    }
    lines[i].vertex1 = READ_SHORT(cptr + LDEF_VERT1);
    lines[i].vertex2 = READ_SHORT(cptr + LDEF_VERT2);
    lines[i].flags = READ_SHORT(cptr + LDEF_FLAGS);
    lines[i].types = READ_SHORT(cptr + LDEF_TYPES);
    lines[i].tag = READ_SHORT(cptr + LDEF_TAG);
    lines[i].sidedef1 = READ_SHORT(cptr + LDEF_SDEF1);
    lines[i].sidedef2 = READ_SHORT(cptr + LDEF_SDEF2);
    cptr += LDEF_SIZE; validbytes -= LDEF_SIZE;
  }
  return lines;
}

int writelinedefs(linedef_t *lines, int bytes, FILE *fp)
{
  int i;
  unsigned char *cptr;

  /*printf("Write linedefs: %d (mod %d)\n", bytes / LDEF_SIZE, bytes - LDEF_SIZE*(bytes/LDEF_SIZE));*/
  cptr = convbuffer;
  for (i=0; bytes > 0; i++)
  {
    if (cptr - convbuffer > convbuffsize - LDEF_SIZE)
    {
      fwrite(convbuffer, 1, cptr-convbuffer, fp);
      cptr = convbuffer;
    }
    WRITE_SHORT(cptr + LDEF_VERT1, lines[i].vertex1);
    WRITE_SHORT(cptr + LDEF_VERT2, lines[i].vertex2);
    WRITE_SHORT(cptr + LDEF_FLAGS, lines[i].flags);
    WRITE_SHORT(cptr + LDEF_TYPES, lines[i].types);
    WRITE_SHORT(cptr + LDEF_TAG, lines[i].tag);
    WRITE_SHORT(cptr + LDEF_SDEF1, lines[i].sidedef1);
    WRITE_SHORT(cptr + LDEF_SDEF2, lines[i].sidedef2);
    cptr += LDEF_SIZE; bytes -= LDEF_SIZE;
  }
  if (cptr != convbuffer)
  {
    fwrite(convbuffer, 1, cptr-convbuffer, fp);
  }
  return 0;
}

sidedef_t *readsidedefs(int lumpnum, FILE *fp)
{
  sidedef_t *sides;
  int i, numsides, validbytes;
  unsigned char *cptr;

  numsides = wadentry[lumpnum].length / SDEF_SIZE;
  if ((sides = (sidedef_t*)malloc(numsides * sizeof(sidedef_t))) == NULL)
  {
    fprintf(stderr, "Unable to claim memory for sidedefs\n");
    exit(-1);
  }
  fseek(fp, wadentry[lumpnum].offset, SEEK_SET);
  validbytes = 0; cptr = convbuffer;
  for (i=0; i<numsides; i++)
  {
    if (validbytes < SDEF_SIZE)
    {
      if (validbytes != 0) memcpy(convbuffer, cptr, validbytes);
      validbytes += fread(convbuffer + validbytes, 1, convbuffsize - validbytes, fp);
      cptr = convbuffer;
    }
    sides[i].xoffset = READ_SHORT(cptr + SDEF_XOFF);
    sides[i].yoffset = READ_SHORT(cptr + SDEF_YOFF);
    memset(sides[i].upper, 0, 8);
    strncpy(sides[i].upper, (char*)cptr + SDEF_UPPER, 8);
    memset(sides[i].middle, 0, 8);
    strncpy(sides[i].middle, (char*)cptr + SDEF_MIDDLE, 8);
    memset(sides[i].lower, 0, 8);
    strncpy(sides[i].lower, (char*)cptr + SDEF_LOWER, 8);
    sides[i].sector_ref = READ_SHORT(cptr + SDEF_SECTOR);
    cptr += SDEF_SIZE; validbytes -= SDEF_SIZE;
  }
  return sides;
}

int writesidedefs(sidedef_t *sides, int bytes, FILE *fp)
{
  int i;
  unsigned char *cptr;

  /*printf("Write sidedefs %d (mod %d)\n", bytes / SDEF_SIZE, bytes - SDEF_SIZE*(bytes/SDEF_SIZE));*/
  cptr = convbuffer;
  for (i=0; bytes > 0; i++)
  {
    if (cptr - convbuffer > convbuffsize - SDEF_SIZE)
    {
      fwrite(convbuffer, 1, cptr-convbuffer, fp);
      cptr = convbuffer;
    }
    WRITE_SHORT(cptr + SDEF_XOFF, sides[i].xoffset);
    WRITE_SHORT(cptr + SDEF_YOFF, sides[i].yoffset);
    memset(cptr + SDEF_UPPER, 0, 8);
    strncpy((char*)cptr + SDEF_UPPER, sides[i].upper, 8);
    memset(cptr + SDEF_MIDDLE, 0, 8);
    strncpy((char*)cptr + SDEF_MIDDLE, sides[i].middle, 8);
    memset(cptr + SDEF_LOWER, 0, 8);
    strncpy((char*)cptr + SDEF_LOWER, sides[i].lower, 8);
    WRITE_SHORT(cptr + SDEF_SECTOR, sides[i].sector_ref);
    cptr += SDEF_SIZE; bytes -= SDEF_SIZE;
  }
  if (cptr != convbuffer)
  {
    fwrite(convbuffer, 1, cptr-convbuffer, fp);
  }
  return 0;
}
