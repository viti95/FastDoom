#define LOCKMEMORY

#include <stddef.h>
#include "ns_llm.h"

#ifdef LOCKMEMORY
#include "ns_dpmi.h"
#endif

#define OFFSET(structure, offset) \
    (*((char **)&(structure)[offset]))

/**********************************************************************

   Memory locked functions:

**********************************************************************/

#define LL_LockStart LL_AddNode

void LL_AddNode(
    char *item,
    char **head,
    char **tail,
    int next,
    int prev)

{
    OFFSET(item, prev) = NULL;
    OFFSET(item, next) = *head;

    if (*head)
    {
        OFFSET(*head, prev) = item;
    }
    else
    {
        *tail = item;
    }

    *head = item;
}

void LL_RemoveNode(
    char *item,
    char **head,
    char **tail,
    int next,
    int prev)

{
    if (OFFSET(item, prev) == NULL)
    {
        *head = OFFSET(item, next);
    }
    else
    {
        OFFSET(OFFSET(item, prev), next) = OFFSET(item, next);
    }

    if (OFFSET(item, next) == NULL)
    {
        *tail = OFFSET(item, prev);
    }
    else
    {
        OFFSET(OFFSET(item, next), prev) = OFFSET(item, prev);
    }

    OFFSET(item, next) = NULL;
    OFFSET(item, prev) = NULL;
}

static void LL_LockEnd(
    void)

{
}

void LL_UnlockMemory(
    void)

{
#ifdef LOCKMEMORY

    DPMI_UnlockMemoryRegion(LL_LockStart, LL_LockEnd);

#endif
}

int LL_LockMemory(
    void)

{

#ifdef LOCKMEMORY

    int status;

    status = DPMI_LockMemoryRegion(LL_LockStart, LL_LockEnd);
    if (status != DPMI_Ok)
    {
        return (LL_Error);
    }

#endif

    return (LL_Ok);
}
