#ifndef __LL_MAN_H
#define __LL_MAN_H

enum LL_Errors
{
    LL_Warning = -2,
    LL_Error = -1,
    LL_Ok = 0
};

typedef struct list
{
    void *start;
    void *end;
} list;

void LL_AddNode(char *node, char **head, char **tail, int next, int prev);
void LL_RemoveNode(char *node, char **head, char **tail, int next, int prev);

#define LL_AddToHead(type, listhead, node)    \
    LL_AddNode((char *)(node),                \
               (char **)&((listhead)->start), \
               (char **)&((listhead)->end),   \
               (int)&((type *)0)->next,       \
               (int)&((type *)0)->prev)

#define LL_AddToTail(type, listhead, node)    \
    LL_AddNode((char *)(node),                \
               (char **)&((listhead)->end),   \
               (char **)&((listhead)->start), \
               (int)&((type *)0)->prev,       \
               (int)&((type *)0)->next)

#define LL_Remove(type, listhead, node)          \
    LL_RemoveNode((char *)(node),                \
                  (char **)&((listhead)->start), \
                  (char **)&((listhead)->end),   \
                  (int)&((type *)0)->next,       \
                  (int)&((type *)0)->prev)

#define LL_NextNode(node) ((node)->next)
#define LL_PreviousNode(node) ((node)->prev)

#endif
