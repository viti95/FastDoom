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
#define MAXINT ((int)0x7fffffff)

#define PI_F 3.14159265f
#define FIXED_TO_FLOAT(inp) ((float)inp) / (1 << FRACBITS)
#define FLOAT_TO_FIXED(inp) (fixed_t)(inp * (1 << FRACBITS))
#define ANGLE_TO_FLOAT(x) (x * ((float)(PI_F / 4096.0f)))

inline static const fixed_t FixedMul(fixed_t a, fixed_t b)
{
    fixed_t result;
    int dummy;

    asm("  imull %3 ;"
        "  shrdl $16,%1,%0 ;"
        : "=a"(result), /* eax is always the result */
          "=d"(dummy)   /* cphipps - fix compile problem with gcc-2.95.1
				   edx is clobbered, but it might be an input */
        : "0"(a),       /* eax is also first operand */
          "r"(b)        /* second operand could be mem or reg before,
				   but gcc compile problems mean i can only us reg */
        : "%cc"         /* edx and condition codes clobbered */
    );

    return result;
}

inline static const fixed_t FixedDiv2(fixed_t a, fixed_t b)
{
    fixed_t result;
    int dummy;
    asm(" idivl %4 ;"
        : "=a"(result),
          "=d"(dummy) /* cphipps - fix compile problems with gcc 2.95.1
			     edx is clobbered, but also an input */
        : "0"(a << 16),
          "1"(a >> 16),
          "r"(b)
        : "%cc");
    return result;
}

#define abs2(x) (((x) < 0) ? -(x) : (x))
//#define abs(x) (((x) + ((x) >> 31)) ^ ((x) >> 31))

inline static const fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    if (abs2(a) >> 14 < abs2(b))
    {
        return FixedDiv2(a, b);
    }
    return ((a ^ b) >> 31) ^ MAXINT;
}

inline static const void CopyBytes(void *src, void *dest, int num_bytes)
{
    asm volatile("rep movsb"
                 : "+D"(dest), "+S"(src), "+c"(num_bytes)::"memory");
}

inline static const void CopyWords(void *src, void *dest, int num_words)
{
    asm volatile("rep movsw"
                 : "+D"(dest), "+S"(src), "+c"(num_words)::"memory");
}

inline static const void CopyDWords(void *src, void *dest, int num_dwords)
{
    asm volatile("rep movsd"
                 : "+D"(dest), "+S"(src), "+c"(num_dwords)::"memory");
}

inline static const void SetBytes(void *dest, unsigned char value, int num_bytes)
{
    asm volatile("rep stosb"
                 : "+c"(num_bytes), "+D"(dest)
                 : "a"(value)
                 : "cc", "memory");
}

inline static const void SetWords(void *dest, short value, int num_words)
{
    asm volatile("rep stosw"
                 : "+c"(num_words), "+D"(dest)
                 : "a"(value)
                 : "cc", "memory");
}

inline static const void SetDWords(void *dest, int value, int num_dwords)
{
    asm volatile("rep stosl"
                 : "+c"(num_dwords), "+D"(dest)
                 : "a"(value)
                 : "cc", "memory");
}

inline static const void OutString(unsigned short port, unsigned char *addr, int c)
{
    asm volatile("rep outsb %%ds:(%0), %3"
                 : "+S"(addr), "+c"(c)
                 : "m"(addr), "Nd"(port), "0"(addr), "1"(c));
}

#endif // __DOOMMATH__
