#define LOCKMEMORY

#include <stddef.h>
#include "ns_llm.h"
#include "options.h"

#define OFFSET(structure, offset) \
    (*((char **)&(structure)[offset]))

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
