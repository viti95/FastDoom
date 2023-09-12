#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>

char *find_filename(char *s);
int filecmp(char *filename, char *template);
void *__crt0_glob_function();   /* needed to disable globbing(expansion of */
                                /* wildcards on the command line) */

void main(int argc,  char *argv[])
{
        DIR *directory;
        struct dirent *direntry;
        char *dirname, *filename;

        if(argc<2)
               dirname="./*.*";
        else
               dirname=argv[1];

        filename=find_filename(dirname);

        if(dirname==filename) dirname=".";

        directory=opendir(dirname);
        while (1)
        {
                direntry=readdir(directory);
                if(!direntry) break;
                if(filecmp(direntry->d_name,filename))
                        puts(direntry->d_name);
        }
        closedir(directory);

}

char *find_filename(char *s)
{
        char *tempstr, *backstr;

        backstr=s;

        while(1)
        {
                tempstr=strchr(backstr,'\\');
                if(!tempstr)    /* no more slashes */
                {
                        tempstr=strchr(backstr,'/');
                        if(!tempstr)
                        {
                                *(backstr-1)=0;
                                return backstr;
                        }
                }
                backstr=tempstr+1;
        }
}

int filecmp(char *filename, char *template)
{
        char filename1[50], template1[50];      /* filename */
        char *filename2 ,   *template2;      /* extension */
        int count;

        strcpy(filename1,filename);
        strcpy(template1,template);

        filename2=strchr(filename1,'.');
        if(!filename2) filename2=""; /* no extension */
        else
        {                       /* extension */
                *filename2=0; /* end of main filename */
                filename2++;  /* set to start of extension */
        }

        template2=strchr(template1,'.');
        if(!template2) template2="";
        else
        {
                *template2=0;
                template2++;
        }

        for(count=0;count<8;count++)    /* compare the filenames */
        {
                if(filename1[count]=='\0'
                 && template1[count]!='\0') return 0;
                if(template1[count]=='?') continue;
                if(template1[count]=='*') break;
                if(template1[count]!=filename1[count]) return 0;
                if(template1[count]=='\0') break; /* end of string */
        }

        for(count=0;count<3;count++)    /* compare the extensions */
        {
                if(filename2[count]=='\0'
                 && template2[count]!='\0') return 0;
                if(template2[count]=='?') continue;
                if(template2[count]=='*') break;
                if(template2[count]!=filename2[count]) return 0;
                if(template2[count]=='\0') break; /* end of string */
        }

        return 1;
}

void *__crt0_glob_function()
{
        return 0;
}

