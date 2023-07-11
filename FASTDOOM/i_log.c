#include "i_log.h"

#define LOG_ENABLED 1

#if (LOG_ENABLED == 0)

void I_Log(const char *format, ...)
{
    FILE *f_log;
    va_list ap;

    f_log = fopen("fdoom.log", "a");
    va_start(ap, format);
    vfprintf(f_log, format, ap);
    va_end(ap);
    fclose(f_log);
}

#endif
