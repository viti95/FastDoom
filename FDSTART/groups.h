/*
 * GROUPS.H - FastDoom text mode launcher, the executable groups
 */

#ifndef GROUPS_H
#define GROUPS_H

/*
 * A single launchable program: a short description and the
 * executable file name.
 */
typedef struct {
    const char *desc;
    const char *exe;
} item_t;

/*
 * A menu group: a title and the list of items it contains.
 */
typedef struct {
    const char *title;
    const item_t *items;
    int count;
} group_t;

/* The menu groups (defined in groups.c). */
#define NGROUPS 8

extern const group_t groups[NGROUPS];

#endif /* GROUPS_H */
