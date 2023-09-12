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
 *                          The WADPTR project                            *
 *                                                                        *
 * Compresses Doom WAD files through several methods:                     *
 *                                                                        *
 * - Merges identical lumps (see wadmerge.c)                              *
 * - 'Squashes' graphics (see lumps.c)                                    *
 * - Packs the sidedefs in levels (see lumps.c)                           *
 *                                                                        *
 **************************************************************************/

/******************************* INCLUDES **********************************/

#include "wadptr.h"

/******************************* GLOBALS ***********************************/

int g_argc; /* global cmd-line list */
char **g_argv;
char filespec[256] = ""; /* file spec on command line eg. *.wad */
char wadname[256] = "";  /* WAD file name */
static char outputwad[256] = "";
int action;            /* list, compress */
int allowpack = 1;     /* level packing on */
int allowsquash = 1;   /* picture squashing on */
int allowmerge = 1;    /* lump merging on */
int allowfastdoom = 1; /* remove not needed lumps on FastDoom */

const char *pwad_name = "PWAD";
const char *iwad_name = "IWAD";

static const char *tempwad_name = "~wptmp" EXTSEP "wad";

/* Main ********************************************************************/

int main(int argc, char *argv[])
{
        /*int action=0; unused */

        g_argc = argc; /* Set global cmd-line list */
        g_argv = argv;

        printf(
            "\n" /* Display startup message */
            "WADPTR - WAD Compressor  Version " VERSION "\n"
            "Copyright (c)1997-2011 Simon Howard.\n"
            "Enhancements/Portability by Andreas Dehmel\n"
            "http://www.soulsphere.org/projects/wadptr/\n");

        /* set error signals */
        signal(SIGINT, sig_func);
        signal(SIGSEGV, sig_func);
        signal(SIGNOFP, sig_func);

        parsecmdline(); /* Check cmd-lines */

        eachwad(filespec); /* do each wad */

        return 0;
}

/****************** Command line handling, open wad etc. *******************/

/* Parse cmd-line options **************************************************/

int parsecmdline()
{
        int count;

        action = HELP; /* default to help if not told what to do */

        count = 1;
        while (count < g_argc)
        {
                if ((!strcmp(g_argv[count], "-help")) || (!strcmp(g_argv[count], "-h")))
                        action = HELP;

                if ((!strcmp(g_argv[count], "-list")) || (!strcmp(g_argv[count], "-l")))
                        action = LIST;

                if ((!strcmp(g_argv[count], "-compress")) || (!strcmp(g_argv[count], "-c")))
                        action = COMPRESS;

                /* specific disabling */
                if (!strcmp(g_argv[count], "-nomerge"))
                        allowmerge = 0;
                if (!strcmp(g_argv[count], "-nosquash"))
                        allowsquash = 0;
                if (!strcmp(g_argv[count], "-nopack"))
                        allowpack = 0;
                if (!strcmp(g_argv[count], "-nofastdoom"))
                        allowfastdoom = 0;

                if (g_argv[count][0] != '-')
                        if (!strcmp(filespec, ""))
                                strcpy(filespec, g_argv[count]);

                if ((!strcmp(g_argv[count], "-output")) || (!strcmp(g_argv[count], "-o")))
                        strcpy(outputwad, g_argv[++count]);

                count++;
        }

        if (!strcmp(filespec, "")) /* no wad file given */
        {
                if (action == HELP)
                {
                        doaction();
                        exit(0);
                }
                else
                        errorexit("No input WAD file specified.\n");
        }

        return 0;
}

/* Do each wad specified by the wildcard in turn **************************/

void eachwad(char *filespec)
{
        DIR *directory;           /* the directory */
        struct dirent *direntry;  /* directory entry */
        char *dirname, *filename; /* directory/file names */
        int wadcount = 0;

        dirname = filespec; /* get the directory name from the filespec */

        filename = find_filename(dirname); /* find the filename */

        if (dirname == filename)
                dirname = CURDIR;     /* use the current directory */
                                      /* if none specified */
        directory = opendir(dirname); /* open the directory */
        while (1)                     /* go through entries */
        {
                direntry = readdir(directory); /* read the next entry */
                if (!direntry)
                        break; /* no more entries */
                if (filecmp(direntry->d_name, filename))
                { /* see if it conforms to the wildcard */
                        /* build the wad name from dirname and filename */
                        sprintf(wadname, "%s" DIRSEP "%s", dirname, direntry->d_name);
                        if (!strcmp(dirname, CURDIR)) /* don't bother with dirname "." */
                                strcpy(wadname, direntry->d_name);
                        if (!(openwad(wadname)))
                        {                   /* no problem with wad.. do whatever */
                                doaction(); /* do whatever(compress, etc) */
                        }
                        wadcount++;    /* another one done.. */
                        fclose(wadfp); /* close the wad */
                }
        }
        closedir(directory);

        if (!wadcount)
                errorexit("\neachwad: No files found!\n");
}

/* Open the original WAD ***************************************************/

int openwad(char *filename)
{
        /*char tempstr[50]; unused */
        int a;

        if (!action)
                action = LIST; /* no action but i've got a wad.. */
                               /* whats in it? default to list */
                               /* open the wad */
        wadfp = fopen(filename, "rb+");
        if (!wadfp) /* can't */
        {
                printf("%s does not exist\n", wadname);
                return 1;
        }

        printf("\nSearching WAD: %s\n", filename);
        a = readwad(); /* read the directory */
        if (a)
                return 1; /* problem with wad */

        printf("\n"); /* leave a space if wads ok :) */
        return 0;
}

/* Does an action based on command line ************************************/

void doaction()
{
        switch (action)
        {
        case HELP:
                help();
                break;

        case LIST:
                list_entries();
                break;

        case COMPRESS:
                compress();
                break;
        }
}

/************************ Command line functions ***************************/

/* Display help ************************************************************/

void help()
{
        printf(
            "\n"
            "Usage:  WADPTR inputwad [outputwad] options\n"
            "\n"
            " -c        :   Compress WAD\n"
            " -l        :   List WAD\n"
            " -o <file> :   Write output WAD to <file>\n"
            " -h        :   Help\n"
            "\n"
            " -nomerge  :   Disable lump merging\n"
            " -nosquash :   Disable graphic squashing\n"
            " -nopack   :   Disable sidedef packing\n");
}

/* Compress a WAD **********************************************************/

void compress()
{
        int count, findshrink;
        long wadsize; /* wad size(to find % smaller) */
        FILE *fstream;
        int written = 0; /* if 0:write 1:been written 2:write silently */
        char *temp, resname[10], a[50];

        if (wad == IWAD)
                if (!iwad_warning())
                        return;

        wadsize = diroffset + (ENTRY_SIZE * numentries); /* find wad size */

        fstream = fopen(tempwad_name, "wb+");
        if (!fstream)
                errorexit("compress: Couldn't write a temporary file\n");

        memset(a, 0, 12);
        fwrite(a, 12, 1, fstream); /* temp header. */

        for (count = 0; count < numentries; count++) /* add each wad entry in turn */
        {
                strcpy(resname, convert_string8(wadentry[count])); /* find */
                                                                   /* resource name */
                written = 0;                                       /* reset written */
                if (!islevelentry(resname))                        /* hide individual level entries */
                {
                        printf("Adding: %s       ", resname);
                        fflush(stdout);
                }
                else
                        written = 2; /* silently write entry: level entry */

                if (allowpack) /* sidedef packing disabling */
                {
                        if (islevel(count)) /* level name */
                        {
                                printf("\tPacking ");
                                fflush(stdout);
                                findshrink = findlevelsize(resname);

                                p_pack(resname); /* pack the level */

                                findshrink = findperc(findshrink, /* % shrunk */
                                                      findlevelsize(resname));
                                printf("(%i%%), done.\n", findshrink);

                                written = 2; /* silently write this lump (if any) */
                        }
                        if (!strcmp(resname, "SIDEDEFS"))
                        {
                                /* write the pre-packed sidedef entry */
                                wadentry[count].offset = ftell(fstream);
                                /*fwrite(p_sidedefres,wadentry[count].length,1,fstream);*/
                                writesidedefs((sidedef_t *)p_sidedefres, wadentry[count].length, fstream);
                                free(p_sidedefres); /* sidedefs no longer needed */
                                written = 1;        /* now written */
                        }
                        if (!strcmp(resname, "LINEDEFS"))
                        {
                                /* write the pre-packed linedef entry */
                                wadentry[count].offset = ftell(fstream);
                                /*fwrite(p_linedefres,wadentry[count].length,1,fstream);*/
                                writelinedefs((linedef_t *)p_linedefres, wadentry[count].length, fstream);
                                free(p_linedefres);
                                written = 1; /* now written */
                        }
                }

                if (allowsquash)                  /* squash disabling */
                        if (s_isgraphic(resname)) /* graphic */
                        {
                                printf("\tSquashing ");
                                fflush(stdout);
                                findshrink = wadentry[count].length;

                                temp = s_squash(resname);                /* get the squashed graphic */
                                wadentry[count].offset = ftell(fstream); /*update dir */
                                                                         /* write it */
                                fwrite(temp, wadentry[count].length, 1, fstream);

                                free(temp); /* graphic no longer needed: free it */
                                /* % shrink  */
                                findshrink = findperc(findshrink, wadentry[count].length);
                                printf("(%i%%), done.\n", findshrink);
                                written = 1; /* now written */
                        }

                if ((written == 0) || (written == 2)) /* write or silently */
                {
                        if (written == 0) /* only if not silent */
                        {
                                printf("\tStoring ");
                                fflush(stdout);
                        }
                        temp = cachelump(count);                 /* get the lump */
                        wadentry[count].offset = ftell(fstream); /*update dir */
                        /* write lump */
                        fwrite(temp, wadentry[count].length, 1, fstream);
                        free(temp);       /* now free the lump */
                        if (written == 0) /* always 0% */
                                printf("(0%%), done.\n");
                }
        }
        diroffset = ftell(fstream);                              /* update the directory location */
        /*fwrite(wadentry,numentries,sizeof(entry_t),fstream);*/ /*write dir */
        writewaddir(fstream);
        writewadheader(fstream);

        fclose(fstream);
        fclose(wadfp);

        if (allowmerge)
        {
                wadfp = fopen(tempwad_name, "rb+"); /* reload the temp file as the wad */
                printf("\nMerging identical lumps.. ");
                fflush(stdout);

                if (outputwad[0] == 0)
                {
                        remove(wadname);  /* delete the old wad */
                        rebuild(wadname); /* run rebuild() to remove identical lumps: */
                                          /* rebuild them back to the original filename */
                }
                else
                {
                        rebuild(outputwad);
                }
                printf("done.\n"); /* all done! */
                fclose(wadfp);
                remove(tempwad_name); /* delete the temp file */
        }
        else
        {
                if (outputwad[0] == 0)
                {
                        remove(wadname);
                        rename(tempwad_name, wadname);
                }
                else
                {
                        rename(tempwad_name, outputwad);
                }
        }

        wadfp = fopen(wadname, "rb+"); /* so there is something to close */

        findshrink = findperc(wadsize, diroffset + (numentries * ENTRY_SIZE));

        printf("*** %s is %i%% smaller ***\n", wadname, findshrink);
}

/* List WAD entries ********************************************************/

void list_entries()
{
        int count, count2;
        int ypos;
        char resname[10];

        printf(
            " Number Length  Offset          Method      Name        Shared\n"
            " ------ ------  ------          ------      ----        ------\n");

        for (count = 0; count < numentries; count++)
        {
                strcpy(resname, convert_string8(wadentry[count]));
                if (islevelentry(resname))
                        continue;
                ypos = wherey();

                /* wad entry number */
                printf(" %i  \t", count + 1);

                /* size */
                if (islevel(count))
                { /* the whole level not just the id lump */
                        printf("%i\t", findlevelsize(resname));
                }
                else /*not a level, doesn't matter */
                        printf("%ld\t", wadentry[count].length);

                /* file offset */
                printf("0x%08lx     \t", wadentry[count].offset);

                /* compression method */
                if (islevel(count))
                { /* this is a level */
                        if (p_ispacked(resname))
                                printf("Packed      "); /* packed */
                        else
                                printf("Unpacked    "); /* not */
                }
                else
                {
                        if (s_isgraphic(resname))
                        { /* this is a graphic */
                                if (s_is_squashed(resname))
                                        printf("Squashed    "); /* squashed */
                                else
                                        printf("Unsquashed  "); /* not */
                        }
                        else /* ordinary lump w/no compression */
                                printf("Stored      ");
                }

                /* resource name */
                printf("%s%s\t", resname, (strlen(resname) < 4) ? "\t" : "");

                /* shared resource */
                if (wadentry[count].length == 0)
                {
                        printf("No\n");
                        continue;
                }
                for (count2 = 0; count2 < count; count2++)
                { /* same offset + size */
                        if ((wadentry[count2].offset == wadentry[count].offset) && (wadentry[count2].length == wadentry[count].length))
                        {
                                printf("%s\n", convert_string8(wadentry[count2]));
                                break;
                        }
                }
                if (count2 == count)    /* no identical lumps if it */
                        printf("No\n"); /* reached the last one */
        }
}

/*********************** Wildcard Functions *******************************/

/* Break a filename into filename and extension ***************************/

char *find_filename(char *s)
{
        char *tempstr, *backstr;

        backstr = s;

        while (1)
        {
                tempstr = strchr(backstr, DIRSEP[0]);
                if (!tempstr) /* no more slashes */
                {
                        tempstr = strchr(backstr, '/');
                        if (!tempstr)
                        {
                                if (backstr != s)
                                        *(backstr - 1) = 0;
                                return backstr;
                        }
                }
                backstr = tempstr + 1;
        }
}

/* Compare two filenames ***************************************************/

int filecmp(char *filename, char *templaten)
{
        char filename1[50], template1[50]; /* filename */
        char *filename2, *template2;       /* extension */
        int count;

        strcpy(filename1, filename);
        strcpy(template1, templaten);

        filename2 = strchr(filename1, EXTSEP[0]);
        if (!filename2)
                filename2 = ""; /* no extension */
        else
        {                       /* extension */
                *filename2 = 0; /* end of main filename */
                filename2++;    /* set to start of extension */
        }

        template2 = strchr(template1, EXTSEP[0]);
        if (!template2)
                template2 = "";
        else
        {
                *template2 = 0;
                template2++;
        }

        for (count = 0; count < 8; count++) /* compare the filenames */
        {
                if (filename1[count] == '\0' && template1[count] != '\0')
                        return 0;
                if (template1[count] == '?')
                        continue;
                if (template1[count] == '*')
                        break;
                if (template1[count] != filename1[count])
                        return 0;
                if (template1[count] == '\0')
                        break; /* end of string */
        }

        for (count = 0; count < 3; count++) /* compare the extensions */
        {
                if (filename2[count] == '\0' && template2[count] != '\0')
                        return 0;
                if (template2[count] == '?')
                        continue;
                if (template2[count] == '*')
                        break;
                if (template2[count] != filename2[count])
                        return 0;
                if (template2[count] == '\0')
                        break; /* end of string */
        }

        return 1;
}

/* Removes command line globbing ******************************************/

void *__crt0_glob_function()
{
        return 0;
}

/************************** Misc. functions ********************************/

/* Find how much smaller something is: returns a percentage ****************/

int findperc(int before, int after)
{
        double perc;

        perc = 1 - (((double)after) / before);

        return (int)(100 * perc);
}

/* Warning not to change IWAD **********************************************/

int iwad_warning()
{
        char tempchar;
        printf("Are you sure you want to change the main IWAD?");
        fflush(stdout);
        while (1)
        {
                tempchar = fgetc(stdin);
                if ((tempchar == 'Y') || (tempchar == 'y'))
                {
                        printf("\n");
                        return 1;
                }
                if ((tempchar == 'N') || (tempchar == 'n'))
                {
                        printf("\n");
                        return 0;
                }
        }
}

#ifdef ANSILIBS
int wherex(void)
{
        return 0;
}

int wherey(void)
{
        return 0;
}

int gotoxy(int x, int y)
{
        printf("                \r");
        return 0;
}
#endif
