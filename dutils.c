//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 2016-2017 Alexey Khokholov (Nuke.YKT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Cheat sequence checking.
//

#include "dutils.h"
#include "i_system.h"
#include "z_zone.h"

list_t* dll_NewList(void)
{
    list_t* list;
    list = (list_t*)Z_Malloc(sizeof(list_t), PU_STATIC, 0);
    list->start = list->end = NULL;
    return list;
}

lnode_t* dll_AddEndNode(list_t* list, void* value)
{
    lnode_t* node;

    if (!list)
    {
        I_Error("Bad list in dll_AddEndNode");
    }

    node = (lnode_t*)Z_Malloc(sizeof(lnode_t), PU_STATIC, 0);

    node->value = value;
    node->next = NULL;
    node->prev = list->end;

    if (list->end)
    {
        list->end->next = node;
    }
    else
    {
        list->start = node;
    }

    list->end = node;

    return node;
}

lnode_t* dll_AddStartNode(list_t* list, void* value)
{
    lnode_t* node;

    if (!list)
    {
        I_Error("Bad list in dll_AddStartNode");
    }

    node = (lnode_t*)Z_Malloc(sizeof(lnode_t), PU_STATIC, 0);

    node->value = value;
    node->prev = NULL;
    node->next = list->start;

    if (list->start)
    {
        list->start->next = node;
    }
    else
    {
        list->end = node;
    }

    list->start = node;

    return node;
}

void* dll_DelNode(list_t* list, lnode_t* node)
{
    void* value;
    if (!list)
    {
        I_Error("Bad list in dll_DelNode");
    }
    if (!list->start)
    {
        I_Error("Empty list in dll_DelNode");
    }
    value = node->value;
    if (node->prev)
    {
        node->prev->next = node->next;
    }
    if (node->next)
    {
        node->next->prev = node->prev;
    }
    if (node == list->start)
    {
        list->start = node->next;
    }
    if (node == list->end)
    {
        list->end = node->prev;
    }
    Z_Free(node);
    return value;
}

void* dll_DelEndNode(list_t* list)
{
    return dll_DelNode(list, list->end);
}

void* dll_DelStartNode(list_t* list)
{
    return dll_DelNode(list, list->start);
}

//
// CHEAT SEQUENCE PACKAGE
//

static int		firsttime = 1;
static unsigned char	cheat_xlate_table[256];


//
// Called in st_stuff module, which handles the input.
// Returns a 1 if the cheat was successful, 0 if failed.
//
int
cht_CheckCheat
( cheatseq_t*	cht,
  char		key )
{
    int i;
    int rc = 0;

    if (firsttime)
    {
	firsttime = 0;
	for (i=0;i<256;i++) cheat_xlate_table[i] = SCRAMBLE(i);
    }

    if (!cht->p)
	cht->p = cht->sequence; // initialize if first time

    if (*cht->p == 0)
	*(cht->p++) = key;
    else if
	(cheat_xlate_table[(unsigned char)key] == *cht->p) cht->p++;
    else
	cht->p = cht->sequence;

    if (*cht->p == 1)
	cht->p++;
    else if (*cht->p == 0xff) // end of sequence character
    {
	cht->p = cht->sequence;
	rc = 1;
    }

    return rc;
}

void
cht_GetParam
( cheatseq_t*	cht,
  char*		buffer )
{

    unsigned char *p, c;

    p = cht->sequence;
    while (*(p++) != 1);
    
    do
    {
	c = *p;
	*(buffer++) = c;
	*(p++) = 0;
    }
    while (c && *p!=0xff );

    if (*p==0xff)
	*buffer = 0;

}


