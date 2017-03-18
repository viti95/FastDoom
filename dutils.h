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
//	Cheat code checking.
//

#ifndef __DUTILS__
#define __DUTILS__

typedef struct lnode_s {
    void* value;
    struct lnode_s* prev;
    struct lnode_s* next;
} lnode_t;

typedef struct {
    lnode_t* start;
    lnode_t* end;
} list_t;

list_t* dll_NewList(void);
lnode_t* dll_AddEndNode(list_t* list, void* value);
lnode_t* dll_AddStartNode(list_t* list, void* value);
void* dll_DelNode(list_t* list, lnode_t* node);
void* dll_DelEndNode(list_t* list);
void* dll_DelStartNode(list_t* list);

//
// CHEAT SEQUENCE PACKAGE
//

#define SCRAMBLE(a) \
((((a)&1)<<7) + (((a)&2)<<5) + ((a)&4) + (((a)&8)<<1) \
 + (((a)&16)>>1) + ((a)&32) + (((a)&64)>>5) + (((a)&128)>>7))

typedef struct
{
    unsigned char*	sequence;
    unsigned char*	p;
    
} cheatseq_t;

int
cht_CheckCheat
( cheatseq_t*		cht,
  char			key );


void
cht_GetParam
( cheatseq_t*		cht,
  char*			buffer );


#endif
