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
 *                     The WADPTR Project: WADMERGE.C                     *
 *                                                                        *
 *  Compresses WAD files by merging identical lumps in a WAD file and     *
 *  sharing them between multiple wad directory entries.                  *
 *                                                                        *
 **************************************************************************/

#include "wadmerge.h"
#include "wadptr.h"

/***************************** Prototypes *********************************/

int suggest();

/***************************** Globals ************************************/

short sameas[4000];

/* Suggest ****************************************************************/

int suggest()
{
        int count, count2, linkcnt=0;
        short *links;     /* possible links */
        char *check1, *check2;
        /*long shrink=0;*/   /* count on how much would be saved */
        /*char filen[30];*/
        int px, py;
        int perctime=20;
        int maxlinks = MAXLINKS;
	
	if ((links = (short*)malloc(2*maxlinks*sizeof(short))) == NULL)
	       errorexit("suggest: couldn't alloc memory for links!\n");
	       
        px=wherex(); py=wherey(); /* for % count */

               /* find similar entries */
        for(count=0;count<numentries;count++) /* each lump in turn */
        {
               for(count2=0;count2<count;count2++) /* check against previous */
               {
                      if((wadentry[count].length == wadentry[count2].length)
                          && wadentry[count].length!=0)
                      {                   /* same length, might be same lump */
                          if(linkcnt>=maxlinks)
			  {
			          maxlinks += MAXLINKS;
				  if ((links = (short*)realloc(links, 2*maxlinks*sizeof(short))) == NULL)
				          errorexit("suggest: couldn't realloc memory for links!\n");
			  }
                          links[2*linkcnt]=count;
                          links[2*linkcnt+1]=count2;
                          ++linkcnt;
                      }
               }
        }

        /* now check 'em out + see if they really are the same */
        memset(sameas,-1,2*4000);

        for(count=0;count<linkcnt;count++)
        {
               if(sameas[links[2*count]]!=-1) continue; /*already done that */
                                                          /* one */
               check1=cachelump(links[2*count]); /* cache both lumps */
               check2=cachelump(links[2*count+1]);

                              /* compare them */
               if(!memcmp(check1,check2,wadentry[links[2*count]].length))
               {                 /* they are the same ! */
                       sameas[links[2*count]]=links[2*count+1];
               }

               free(check1); /* free back both lumps */
               free(check2);

               perctime--;    /* % count */
               if(!perctime)
               {
                     gotoxy(px,py); printf("%i%%",(50*count)/linkcnt);
                     fflush(stdout);
                     perctime=50;
               }
        }
        gotoxy(px,py);

        free(links);

	return 0;
}

/* Rebuild the WAD, making it smaller in the process ***********************/

void rebuild(char *newname)
{
       /*int num_suggest=0, count2;
       char tempstr[5];
       char a;*/
       int count;
       char *tempchar;
       FILE *newwad;
       long along=0 ,filepos;
       int nextperc=50, px, py;

            /* first run suggest mode to find how to make it smaller */
       suggest();

       newwad=fopen(newname,"wb+");   /* open the new wad */

       if(!newwad) errorexit("rebuild: Couldn't open to %s\n",newname);

       fwrite(iwad_name,1,4,newwad);   /* temp header */
       fwrite(&along,4,1,newwad);
       fwrite(&along,4,1,newwad);

       px=wherex(); py=wherey(); /* for % count */

       for(count=0;count<numentries;count++)  /* add each entry in turn */
       {

               /* first check if it's a compressed(linked) */
               if(sameas[count]!=-1)
               {
                      wadentry[count].offset=wadentry[sameas[count]].offset;
               }
	       else
	       {
                      filepos = ftell(newwad); /* find where this lump will be */
                      tempchar=cachelump(count); /* cache the lump */
                               /*write to new file */
                      fwrite(tempchar,1,wadentry[count].length,newwad);
                      free(tempchar); /* free lump back again */
                      wadentry[count].offset=filepos; /* update the wad directory */
                                               /* with new lump location */
               }
               nextperc--;       /* % count */
               if(!nextperc)
               {
                      gotoxy(px,py);
                      printf("%ld%%",50+(count*50)/numentries); fflush(stdout);
                      nextperc=50;
               }
       }
       diroffset = ftell(newwad);  /* write the wad directory */
       writewaddir(newwad);
       writewadheader(newwad);

       fclose(newwad);     /* close the new wad */

       gotoxy(px,py); printf("         "); fflush(stdout); /* remove % count */
       gotoxy(px,py);
}


