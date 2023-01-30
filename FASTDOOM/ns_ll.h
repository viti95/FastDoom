#ifndef __linklist_h
#define __linklist_h

#define LL_CreateNewLinkedList(rootnode, type, next, prev) \
    {                                                      \
        (rootnode) = NewNode(type);                        \
        (rootnode)->prev = (rootnode);                     \
        (rootnode)->next = (rootnode);                     \
    }

#define LL_AddNode(rootnode, newnode, next, prev) \
    {                                             \
        (newnode)->next = (rootnode);             \
        (newnode)->prev = (rootnode)->prev;       \
        (rootnode)->prev->next = (newnode);       \
        (rootnode)->prev = (newnode);             \
    }

#define LL_TransferList(oldroot, newroot, next, prev) \
    {                                                 \
        if ((oldroot)->prev != (oldroot))             \
        {                                             \
            (oldroot)->prev->next = (newroot);        \
            (oldroot)->next->prev = (newroot)->prev;  \
            (newroot)->prev->next = (oldroot)->next;  \
            (newroot)->prev = (oldroot)->prev;        \
            (oldroot)->next = (oldroot);              \
            (oldroot)->prev = (oldroot);              \
        }                                             \
    }

#define LL_ReverseList(root, type, next, prev)                  \
    {                                                           \
        type *newend, *trav, *tprev;                            \
                                                                \
        newend = (root)->next;                                  \
        for (trav = (root)->prev; trav != newend; trav = tprev) \
        {                                                       \
            tprev = trav->prev;                                 \
            LL_MoveNode(trav, newend, next, prev);              \
        }                                                       \
    }

#define LL_RemoveNode(node, next, prev)    \
    {                                      \
        (node)->prev->next = (node)->next; \
        (node)->next->prev = (node)->prev; \
        (node)->next = (node);             \
        (node)->prev = (node);             \
    }

#define LL_SortedInsertion(rootnode, insertnode, next, prev, type, sortparm)      \
    {                                                                             \
        type *hoya;                                                               \
                                                                                  \
        hoya = (rootnode)->next;                                                  \
        while ((hoya != (rootnode)) && ((insertnode)->sortparm > hoya->sortparm)) \
        {                                                                         \
            hoya = hoya->next;                                                    \
        }                                                                         \
        LL_AddNode(hoya, (insertnode), next, prev);                               \
    }

#define LL_MoveNode(node, newroot, next, prev)     \
    {                                              \
        LL_RemoveNode((node), next, prev);         \
        LL_AddNode((newroot), (node), next, prev); \
    }

#define LL_ListEmpty(list, next, prev) \
    (                                  \
        ((list)->next == (list)) &&    \
        ((list)->prev == (list)))

#define LL_Reset(list, next, prev) (list)->next = (list)->prev = (list)
#define LL_Remove LL_RemoveNode
#define LL_Add LL_AddNode
#define LL_Empty LL_ListEmpty
#define LL_Move LL_MoveNode

#endif
