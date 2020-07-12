#ifndef __STANDARD_H
#define __STANDARD_H

typedef int boolean;
typedef int errorcode;

#ifndef TRUE
#define TRUE (1 == 1)
#define FALSE (!TRUE)
#endif

enum STANDARD_ERRORS
{
    Warning = -2,
    FatalError = -1,
    Success = 0
};

#define BITSET(data, bit) \
    (((data) & (bit)) == (bit))

#define ARRAY_LENGTH(array) \
    (sizeof(array) / sizeof((array)[0]))

#define WITHIN_BOUNDS(array, index) \
    ((0 <= (index)) && ((index) < ARRAY_LENGTH(array)))

#define FOREVER for (;;)

#ifdef NDEBUG
#define DEBUGGING 0
#else
#define DEBUGGING 1
#endif

#define DEBUG_CODE      \
    if (DEBUGGING == 0) \
    {                   \
    }                   \
    else

#endif
