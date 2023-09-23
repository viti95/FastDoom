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

#include "palette.h"
#include "wadptr.h"

/******************************* GLOBALS ***********************************/

int g_argc; /* global cmd-line list */
char **g_argv;
char filespec[256] = ""; /* file spec on command line eg. *.wad */
char wadname[256] = "";  /* WAD file name */
static char outputwad[256] = "";
int action; /* list, compress */

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

                if ((!strcmp(g_argv[count], "-compress")) || (!strcmp(g_argv[count], "-c")))
                        action = COMPRESS;

                if (g_argv[count][0] != '-')
                {
                        if (!strcmp(filespec, ""))
                                strcpy(filespec, g_argv[count]);
                }

                if ((!strcmp(g_argv[count], "-output")) || (!strcmp(g_argv[count], "-o")))
                        strcpy(outputwad, g_argv[++count]);

                count++;
        }

        if (!strcmp(filespec, ""))
        { /* no wad file given */
                if (action == HELP)
                {
                        doaction();
                        exit(0);
                }
                else
                {
                        errorexit("No input WAD file specified.\n");
                }
        }

        return 0;
} /* parsecmdline */

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
                dirname = CURDIR; /* use the current directory */
        /* if none specified */
        directory = opendir(dirname); /* open the directory */
        while (1)
        {                                      /* go through entries */
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
} /* eachwad */

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
        if (!wadfp)
        { /* can't */
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
            " -c         :   Compress WAD\n"
            " -o <file>  :   Write output WAD to <file>\n"
            " -h         :   Help\n");
}

/* Compress a WAD **********************************************************/

void compress()
{
        int count, findshrink;
        long wadsize; /* wad size(to find % smaller) */
        FILE *fstream;
        int written = 0; /* if 0:write 1:been written 2:write silently */
        char *temp, resname[10], a[50];

        wadsize = diroffset + (ENTRY_SIZE * numentries); /* find wad size */

        fstream = fopen(tempwad_name, "wb+");
        if (!fstream)
                errorexit("compress: Couldn't write a temporary file\n");

        memset(a, 0, 12);
        fwrite(a, 12, 1, fstream); /* temp header. */

        printf("Applying FastDoom optimizations... ");

        removeentry("WIP1");
        removeentry("WIP2");
        removeentry("WIP3");
        removeentry("WIP4");
        removeentry("WIBP1");
        removeentry("WIBP2");
        removeentry("WIBP3");
        removeentry("WIBP4");
        removeentry("STFB0");
        removeentry("STFB1");
        removeentry("STFB2");
        removeentry("STFB3");
        removeentry("STPB0");
        removeentry("STPB1");
        removeentry("STPB2");
        removeentry("STPB3");
        removeentry("STDISK");
        removeentry("STCDROM");
        removeentry("WIOSTS");
        removeentry("WIOSTF");
        removeentry("WIMSTAR");
        removeentry("WIMINUS");
        removeentry("WIFRGS");
        removeentry("WIKILRS");
        removeentry("WIVCTMS");
        removeentry("HELP");
        removeentry("HELP1");
        removeentry("DSITMBK");
        removeentry("DSTINK");
        removeentry("DSRADIO");
        removeentry("DSSKLDTH");
        removeentry("DPSKLDTH");

        pal_compress(fstream);
        colormap_compress(fstream);
        statusbar_merge(fstream);

        removeentry("STARMS");

        printf("done\n"); /* all done! */

        printf("Packing and squashing... ");

        char *lumppal = "PLAYPAL";
        int palnum = entry_exist(lumppal);

        char *lumpcolormap = "COLORMAP";
        int colormapnum = entry_exist(lumpcolormap);

        for (count = 0; count < numentries; count++)
        {
                /* add each wad entry in turn */
                strcpy(resname, convert_string8(wadentry[count])); /* find */
                                                                   /* resource name */
                written = 0;                                       /* reset written */

                if (count == palnum || count == colormapnum) /* PALETTE / COLORMAP already written */
                        written = 1;

                if (islevelentry(resname))
                        written = 2; /* silently write entry: level entry */

                /* sidedef packing disabling */
                if (islevel(count))
                {                        /* level name */
                        p_pack(resname); /* pack the level */
                        written = 2;     /* silently write this lump (if any) */
                }
                if (!strcmp(resname, "SIDEDEFS"))
                {
                        /* write the pre-packed sidedef entry */
                        wadentry[count].offset = ftell(fstream);
                        writesidedefs((sidedef_t *)p_sidedefres, wadentry[count].length, fstream);
                        free(p_sidedefres); /* sidedefs no longer needed */
                        written = 1;        /* now written */
                }
                if (!strcmp(resname, "LINEDEFS"))
                {
                        /* write the pre-packed linedef entry */
                        wadentry[count].offset = ftell(fstream);
                        writelinedefs((linedef_t *)p_linedefres, wadentry[count].length, fstream);
                        free(p_linedefres);
                        written = 1; /* now written */
                }

                /* squash disabling */
                if (s_isgraphic(resname))
                {                                 /* graphic */
                        temp = s_squash(resname); /* get the squashed graphic */

                        int is_PFUB1 = strcmp(resname, "PFUB1") == 0;
                        int is_PFUB2 = strcmp(resname, "PFUB2") == 0;

                        if (s_width == 320 && s_height == 200 && !(is_PFUB1 || is_PFUB2))
                                wadentry[count].length = 64000;

                        wadentry[count].offset = ftell(fstream); /*update dir */
                                                                 /* write it */

                        fwrite(temp, wadentry[count].length, 1, fstream);

                        free(temp); /* graphic no longer needed: free it */

                        written = 1; /* now written */
                }

                if ((written == 0) || (written == 2))
                {                                                /* write or silently */
                        temp = cachelump(count);                 /* get the lump */
                        wadentry[count].offset = ftell(fstream); /*update dir */
                        /* write lump */
                        fwrite(temp, wadentry[count].length, 1, fstream);
                        free(temp); /* now free the lump */
                }
        }
        diroffset = ftell(fstream); /* update the directory location */
        writewaddir(fstream);
        writewadheader(fstream);

        fclose(fstream);
        fclose(wadfp);

        printf("done\n"); /* all done! */

        wadfp = fopen(tempwad_name, "rb+"); /* reload the temp file as the wad */
        printf("Merging identical lumps... ");

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

        printf("done\n"); /* all done! */

        fclose(wadfp);
        remove(tempwad_name); /* delete the temp file */

        wadfp = fopen(wadname, "rb+"); /* so there is something to close */

        long originalsize = wadsize / 1024;
        long newsize = (diroffset + (numentries * ENTRY_SIZE)) / 1024;

        long percentage = ((originalsize * 100) / newsize) - 100;

        printf("\nOriginal WAD size: %ld Kb\n", originalsize);
        printf("Optimized WAD size: %ld Kb\n", newsize);
        printf("\n%s is ~%ld%% smaller\n", wadname, percentage);
} /* compress */

/*********************** Wildcard Functions *******************************/

/* Break a filename into filename and extension ***************************/

char *find_filename(char *s)
{
        char *tempstr, *backstr;

        backstr = s;

        while (1)
        {
                tempstr = strchr(backstr, DIRSEP[0]);
                if (!tempstr)
                { /* no more slashes */
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
        {
                filename2 = ""; /* no extension */
        }
        else
        {                       /* extension */
                *filename2 = 0; /* end of main filename */
                filename2++;    /* set to start of extension */
        }

        template2 = strchr(template1, EXTSEP[0]);
        if (!template2)
        {
                template2 = "";
        }
        else
        {
                *template2 = 0;
                template2++;
        }

        for (count = 0; count < 8; count++)
        { /* compare the filenames */
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

        for (count = 0; count < 3; count++)
        { /* compare the extensions */
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
} /* filecmp */

/* Removes command line globbing ******************************************/

void *__crt0_glob_function()
{
        return 0;
}
