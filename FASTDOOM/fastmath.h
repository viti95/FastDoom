//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 1993-2008 Raven Software
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
//  Internally used data structures for virtually everything,
//   key definitions, lots of other stuff.
//

#ifndef __DOOMMATH__
#define __DOOMMATH__

#include "std_func.h"

typedef int fixed_t;

#define FRACBITS 16
#define FRACUNIT (1 << FRACBITS)
  
#define PI_F 3.14159265f
#define FIXED_TO_FLOAT(inp)       ((float)inp) / (1 << FRACBITS)
#define FLOAT_TO_FIXED(inp)       (fixed_t)(inp * (1 << FRACBITS))
#define ANGLE_TO_FLOAT(x)       (x * ((float)(PI_F / 4096.0f)))

fixed_t FixedMul(fixed_t a, fixed_t b);
#pragma aux FixedMul = \
    "imul ebx",        \
    "shrd eax,edx,16" parm[eax][ebx] value[eax] modify exact[eax edx]

#define FixedDiv(a,b) (((abs(a) >> 14) >= abs(b)) ? (((a) ^ (b)) >> 31) ^ MAXINT : FixedDiv2(a, b))
fixed_t FixedDiv2(fixed_t a, fixed_t b);
#pragma aux FixedDiv2 =        \
    "cdq",                     \
    "shld edx,eax,16", \
    "sal eax,16",      \
    "idiv ebx" parm[eax][ebx] value[eax] modify exact[eax edx]

int Mul40(int value);
#pragma aux Mul40 = \
    "lea eax, [eax+eax*4]", \
    "shl eax, 3" parm[eax] value[eax] modify exact[eax]

int Mul80(int value);
#pragma aux Mul80 = \
    "lea eax, [eax+eax*4]", \
    "shl eax, 4" parm[eax] value[eax] modify exact[eax]

int Mul320(int value);
#pragma aux Mul320 = \
    "lea eax, [eax+eax*4]", \
    "sal eax, 6" parm[eax] value[eax] modify exact[eax]

int Mul10(int value);
#pragma aux Mul10 = \
    "lea eax, [eax+eax*4]", \
    "add eax, eax" parm[eax] value[eax] modify exact[eax]

int Mul100(int value);
#pragma aux Mul100 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 2" parm[eax] value[eax] modify exact[eax]

int Mul1000(int value);
#pragma aux Mul1000 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 2" parm[eax] value[eax] modify exact[eax]

int Mul819200(int value);
#pragma aux Mul819200 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 15" parm[eax] value[eax] modify exact[eax]

int Mul35(int value);
#pragma aux Mul35 = \
    "lea eax, [edx+edx*8]", \
    "sal eax, 2", \
    "sub eax, edx" parm[edx] value[eax] modify exact[eax edx]

int Mul768(int value);
#pragma aux Mul768 = \
    "lea eax, [edx+edx]", \
    "add eax, edx", \
    "sal eax, 8" parm[edx] value[eax] modify exact[eax edx]

int Mul85(int value);
#pragma aux Mul85 = \
    "lea edx, [eax+eax*4]", \
    "lea edx, [eax+edx*4]", \
    "lea eax, [eax+edx*4]" parm[eax] value[eax] modify exact[eax edx]

int Mul160(int value);
#pragma aux Mul160 = \
    "lea eax, [eax+eax*4]", \
    "sal eax, 5" parm[eax] value[eax] modify exact[eax]

int Mul200(int value);
#pragma aux Mul200 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 3" parm[eax] value[eax] modify exact[eax]

int Mul409(int value);
#pragma aux Mul409 = \
    "lea eax, [edx+edx*4]", \
    "lea eax, [eax+eax*4]", \
    "add eax, eax", \
    "add eax, edx", \
    "lea eax, [edx+eax*8]" parm[edx] value[eax] modify exact[eax edx]

int Mul26843545(int value);
#pragma aux Mul26843545 = \
    "lea eax, [edx+edx]", \
    "add eax, edx", \
    "lea ecx, [edx+eax*4]", \
    "mov eax, ecx", \
    "sal eax, 6", \
    "sub eax, ecx", \
    "mov ecx, eax", \
    "sal ecx, 12", \
    "add eax, ecx", \
    "lea eax, [edx+eax*8]" parm[edx] value[eax] modify exact[eax ecx edx]

int Mul70(int value);
#pragma aux Mul70 = \
    "lea eax, [edx+edx*8]", \
    "sal eax, 2", \
    "sub eax, edx", \
    "add eax, eax" parm[edx] value[eax] modify exact[eax edx]

int Mul47000(int value);
#pragma aux Mul47000 = \
    "lea eax, [edx+edx]", \
    "add eax, edx", \
    "lea eax, [edx+eax*4]", \
    "lea eax, [eax+eax*8]", \
    "add eax, eax", \
    "add eax, edx", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "sal eax, 3" parm[edx] value[eax] modify exact[eax edx]

int Div1000(int value);
#pragma aux Div1000 = \
    "mov edx, 0x10624DD3", \
    "mul edx", \
    "shr edx, 6" parm[eax] value[edx] modify exact[eax edx]

int Div10(int value);
#pragma aux Div10 = \
    "mov eax, 1717986919", \
    "imul ecx", \
    "mov eax, edx", \
    "sar eax, 2", \
    "sar ecx, 31", \
    "sub eax, ecx" parm[ecx] value[eax] modify exact[eax ecx edx]

int Div3(int value);
#pragma aux Div3 = \
    "mov eax, 0x55555556", \
    "imul ecx", \
    "mov eax, ecx", \
    "shr eax, 31", \
    "add edx, eax" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div63(int value);
#pragma aux Div63 = \
    "mov edx, -2113396605", \
    "mov eax, ecx", \
    "imul edx", \
    "add edx, ecx", \
    "sar edx, 5", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div101(int value);
#pragma aux Div101 = \
    "mov edx, 680390859", \
    "mov eax, ecx", \
    "imul edx", \
    "sar edx, 4", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div35(int value);
#pragma aux Div35 = \
    "mov edx, -368140053", \
    "mov eax, ecx", \
    "imul edx", \
    "add edx, ecx", \
    "sar edx, 5", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int DivSKULLSPEED(int value);
#pragma aux DivSKULLSPEED = \
    "mov edx, 1717986919", \
    "mov eax, ecx", \
    "imul edx", \
    "sar edx, 19", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div100(int value);
#pragma aux Div100 = \
    "mov edx, 1374389535", \
    "mov eax, ecx", \
    "imul edx", \
    "sar edx, 5", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div255(int value);
#pragma aux Div255 = \
    "mov edx, -2139062143", \
    "mov eax, ecx", \
    "imul edx", \
    "add edx, ecx", \
    "sar edx, 7", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

unsigned long Div51200(unsigned long value);
#pragma aux Div51200 = \
    "mov ecx, 1374389535", \
    "mov eax, edx", \
    "mul ecx", \
    "shr edx, 14" parm[edx] value[edx] modify exact[eax ecx edx]

int Div70(int value);
#pragma aux Div70 = \
    "mov edx, -368140053", \
    "mov eax, ecx", \
    "imul edx", \
    "add edx, ecx", \
    "sar edx, 6", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div96(int value);
#pragma aux Div96 = \
    "mov edx, 715827883", \
    "mov eax, ecx", \
    "imul edx", \
    "sar edx, 4", \
    "sar ecx, 31", \
    "sub edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

void CopyBytes(void *src, void *dest, int num_bytes);
#pragma aux CopyBytes = \
    "rep movsb" \
    parm [esi] [edi] [ecx] modify[edi esi ecx];

void CopyWords(void *src, void *dest, int num_words);
#pragma aux CopyWords =     \
    "rep movsw"          \
    parm [esi] [edi] [ecx] modify[edi esi ecx];

void CopyDWords(void *src, void *dest, int num_dwords);
#pragma aux CopyDWords =     \
    "rep movsd"             \
    parm [esi] [edi] [ecx]  \
    modify [esi edi ecx];

void SetBytes(void *dest, unsigned char value, int num_bytes);
#pragma aux SetBytes = \
    "rep stosb" \
    parm [edi] [al] [ecx] \
    modify [edi ecx];

void SetWords(void *dest, short value, int num_words);
#pragma aux SetWords = \
    "rep stosw" \
    parm [edi] [ax] [ecx] \
    modify [edi ecx];

void SetDWords(void *dest, int value, int num_dwords);
#pragma aux SetDWords = \
    "rep stosd" \
    parm [edi] [eax] [ecx] \
    modify [edi ecx];

void OutString(unsigned short Port, unsigned char *addr, int c);
#pragma aux OutString = \
    "rep outsb" \
    parm [dx] [si] [cx] nomemory \
    modify exact [si cx] nomemory;

#endif // __DOOMMATH__
