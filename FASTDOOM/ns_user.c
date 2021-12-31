#include <dos.h>
#include <string.h>
#include "ns_user.h"

#define TRUE (1 == 1)
#define FALSE (!TRUE)

extern int _argc;
extern char **_argv;

char *USER_GetText(const char *parameter)
{
    int i;
    char *text;
    char *ptr;

    text = NULL;
    /*i = 1;
    while (i < _argc)
    {
        ptr = _argv[i];

        // Only check parameters preceded by - or /
        if ((*ptr == '-') || (*ptr == '/'))
        {
            ptr++;
            if (stricmp(parameter, ptr) == 0)
            {
                i++;
                text = _argv[i];
                break;
            }
        }

        i++;
    }*/

    return (text);
}
