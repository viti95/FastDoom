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

fixed_t FixedMulECX(fixed_t a, fixed_t b);
#pragma aux FixedMulECX = \
    "imul ecx",        \
    "shrd eax,edx,16" parm[eax][ecx] value[eax] modify exact[eax edx]

fixed_t FixedMulEDX(fixed_t a, fixed_t b);
#pragma aux FixedMulEDX = \
    "imul edx",        \
    "shrd eax,edx,16" parm[eax][edx] value[eax] modify exact[eax edx]

fixed_t FixedMulHStep(fixed_t a, fixed_t b);
#pragma aux FixedMulHStep = \
    "imul edx",             \
    "shrd eax,edx,6"        \
    "xor ax,ax"       parm[eax][edx] value[eax] modify exact[eax edx]

fixed_t FixedMulLStep(fixed_t a, fixed_t b);
#pragma aux FixedMulLStep = \
    "imul edx",             \
    "shrd eax,edx,22"        \
    "and eax,0xFFFF"       parm[eax][edx] value[eax] modify exact[eax edx]

#define FixedDiv(a,b) (((abs(a) >> 14) >= abs(b)) ? (((a) ^ (b)) >> 31) ^ MAXINT : FixedDiv2(a, b))
fixed_t FixedDiv2(fixed_t a, fixed_t b);
#pragma aux FixedDiv2 =        \
    "cdq",                     \
    "shld edx,eax,16", \
    "shl eax,16",      \
    "idiv ebx" parm[eax][ebx] value[eax] modify exact[eax edx]

fixed_t FixedDivDBITS(fixed_t a, fixed_t b);
#pragma aux FixedDivDBITS =        \
    "cdq",                     \
    "shld edx,eax,11", \
    "shl eax,11",      \
    "idiv ebx" parm[eax][ebx] value[eax] modify exact[eax edx]

fixed_t FixedDiv65536(fixed_t b);
#pragma aux FixedDiv65536 = \
    "mov edx, 1",  \
    "xor eax,eax", \
    "idiv ebx" parm[ebx] value[eax] modify exact[eax edx]

unsigned char ROLAND1(int value);
#pragma aux ROLAND1 = \
    "rol eax, 1", \
    "and eax, 1" parm[eax] value[al] modify exact[eax]

int Mul20(int value);
#pragma aux Mul20 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax*4]" parm[eax] value[eax] modify exact[eax]

int Mul40(int value);
#pragma aux Mul40 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax*8]" parm[eax] value[eax] modify exact[eax]

int Mul80(int value);
#pragma aux Mul80 = \
    "lea eax, [eax+eax*4]", \
    "shl eax, 4" parm[eax] value[eax] modify exact[eax]

int Mul320(int value);
#pragma aux Mul320 = \
    "lea eax, [eax+eax*4]", \
    "shl eax, 6" parm[eax] value[eax] modify exact[eax]

int Mul640(int value);
#pragma aux Mul640 = \
    "lea eax, [eax+eax*4]", \
    "shl eax, 7" parm[eax] value[eax] modify exact[eax]

int Mul1280(int value);
#pragma aux Mul1280 = \
    "lea eax, [eax+eax*4]", \
    "shl eax, 8" parm[eax] value[eax] modify exact[eax]

int Mul64(int value);
#pragma aux Mul64 = \
    "shl eax, 6" parm[eax] value[eax] modify exact[eax]

int Mul128(int value);
#pragma aux Mul128 = \
    "shl eax, 7" parm[eax] value[eax] modify exact[eax]

int Mul256(int value);
#pragma aux Mul256 = \
    "shl eax, 8" parm[eax] value[eax] modify exact[eax]

int Mul512(int value);
#pragma aux Mul512 = \
    "shl eax, 9" parm[eax] value[eax] modify exact[eax]

int Mul1024(int value);
#pragma aux Mul1024 = \
    "shl eax, 10" parm[eax] value[eax] modify exact[eax]

int Mul10(int value);
#pragma aux Mul10 = \
    "lea eax, [eax+eax*4]", \
    "add eax, eax" parm[eax] value[eax] modify exact[eax]

unsigned short USMul10(unsigned short value);
#pragma aux USMul10 = \
    "lea eax, [eax+eax*4]", \
    "add eax, eax" parm[ax] value[ax] modify exact[eax]

int Mul100(int value);
#pragma aux Mul100 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax*4]" parm[eax] value[eax] modify exact[eax]

int Mul200(int value);
#pragma aux Mul200 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "shl eax, 3" parm[eax] value[eax] modify exact[eax]

int Mul400(int value);
#pragma aux Mul400 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "shl eax, 4" parm[eax] value[eax] modify exact[eax]

int Mul800(int value);
#pragma aux Mul800 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "shl eax, 5" parm[eax] value[eax] modify exact[eax]

unsigned short USMul100(unsigned short value);
#pragma aux USMul100 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax*4]" parm[ax] value[ax] modify exact[eax]

int Mul1000(int value);
#pragma aux Mul1000 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax*8]" parm[eax] value[eax] modify exact[eax]

unsigned short USMul1000(unsigned short value);
#pragma aux USMul1000 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax*8]" parm[ax] value[ax] modify exact[eax]

int Mul819200(int value);
#pragma aux Mul819200 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "shl eax, 15" parm[eax] value[eax] modify exact[eax]

int Mul35(int value);
#pragma aux Mul35 = \
    "lea eax, [edx+edx*8]", \
    "lea eax, [eax*4]", \
    "sub eax, edx" parm[edx] value[eax] modify exact[eax edx]

int Mul768(int value);
#pragma aux Mul768 = \
    "lea eax, [eax+eax*2]", \
    "shl eax, 8" parm[eax] value[eax] modify exact[eax]

int Mul160(int value);
#pragma aux Mul160 = \
    "lea eax, [eax+eax*4]", \
    "shl eax, 5" parm[eax] value[eax] modify exact[eax]

int Mul200(int value);
#pragma aux Mul200 = \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax*8]" parm[eax] value[eax] modify exact[eax]

int Mul409(int value);
#pragma aux Mul409 = \
    "lea eax, [edx+edx*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [edx+eax*2]", \
    "lea eax, [edx+eax*8]" parm[edx] value[eax] modify exact[eax edx]

int Mul70(int value);
#pragma aux Mul70 = \
    "lea eax, [edx+edx*8]", \
    "lea eax, [eax*4]", \
    "sub eax, edx", \
    "add eax, eax" parm[edx] value[eax] modify exact[eax edx]

int Mul47000(int value);
#pragma aux Mul47000 = \
    "lea eax, [edx+edx*2]", \
    "lea eax, [edx+eax*4]", \
    "lea eax, [eax+eax*8]", \
    "lea eax, [edx+eax*2]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax+eax*4]", \
    "lea eax, [eax*8]" parm[edx] value[eax] modify exact[eax edx]

int Div1000(int value);
#pragma aux Div1000 = \
    "mov edx, 0x10624DD3", \
    "mul edx", \
    "shr edx, 6" parm[eax] value[edx] modify exact[eax edx]

int Div10(int value);
#pragma aux Div10 = \
    "mov eax, 0x066666667", \
    "imul ecx", \
    "sar edx, 2", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div3(int value);
#pragma aux Div3 = \
    "mov eax, 0x55555556", \
    "imul ecx", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div63(int value);
#pragma aux Div63 = \
    "mov eax, 0x082082083", \
    "imul ecx", \
    "add edx, ecx", \
    "sar edx, 5", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div101(int value);
#pragma aux Div101 = \
    "mov eax, 0x0288DF0CB", \
    "imul ecx", \
    "sar edx, 4", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div35(int value);
#pragma aux Div35 = \
    "mov eax, 0x0EA0EA0EB", \
    "imul ecx", \
    "add edx, ecx", \
    "sar edx, 5", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int DivSKULLSPEED(int value);
#pragma aux DivSKULLSPEED = \
    "mov eax, 1717986919", \
    "imul ecx", \
    "sar edx, 19", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div100(int value);
#pragma aux Div100 = \
    "mov eax, 0x051EB851F", \
    "imul ecx", \
    "sar edx, 5", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Mul25(int value);
#pragma aux Mul25 = \
    "lea     eax, [eax+eax*4]", \
    "lea     eax, [eax+eax*4]" parm[eax] value[eax] modify exact[eax]

int Mul75(int value);
#pragma aux Mul75 = \
    "lea     edx, [eax+eax*8]", \
    "lea     edx, [eax+edx*4]", \
    "add     edx, edx", \
    "add     eax, edx" parm[eax] value[eax] modify exact[eax edx]

int Div255(int value);
#pragma aux Div255 = \
    "mov eax, 0x080808081", \
    "imul ecx", \
    "add edx, ecx", \
    "sar edx, 7", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

unsigned long Div51200(unsigned long value);
#pragma aux Div51200 = \
    "mov eax, 0x051EB851F", \
    "mul ecx", \
    "shr edx, 14" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div70(int value);
#pragma aux Div70 = \
    "mov eax, 0x0EA0EA0EB", \
    "imul ecx", \
    "add edx, ecx", \
    "sar edx, 6", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

int Div84(int value);
#pragma aux Div84 = \
    "mov eax, 0x030C30C31", \
    "imul ecx", \
    "sar edx, 4", \
    "shr ecx, 31", \
    "add edx, ecx" parm[ecx] value[edx] modify exact[eax ecx edx]

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

unsigned char InByte60h(void);
#pragma aux InByte60h = \
    "in al, 0x60" \
    value[al] modify[al] nomemory ;

unsigned char InByte61h(void);
#pragma aux InByte61h = \
    "in al, 0x61" \
    value[al] modify[al] nomemory ;

void OutByte20h(unsigned char al);
#pragma aux OutByte20h = \
    "out 0x20, al" \
    parm[al] nomemory;

void OutByteA0h(unsigned char al);
#pragma aux OutByteA0h = \
    "out 0xA0, al" \
    parm[al] nomemory;

void OutByte42h(unsigned char al);
#pragma aux OutByte42h = \
    "out 0x42, al" \
    parm[al] nomemory;

void OutByte43h(unsigned char al);
#pragma aux OutByte43h = \
    "out 0x43, al" \
    parm[al] nomemory;

void OutByte61h(unsigned char al);
#pragma aux OutByte61h = \
    "out 0x61, al" \
    parm[al] nomemory;

#endif // __DOOMMATH__
