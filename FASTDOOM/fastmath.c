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

#ifdef MAC

#include "fastmath.h"

fixed_t FixedMul(fixed_t a, fixed_t b)
{
	return ((long long) a * (long long) b) >> FRACBITS;
}

fixed_t FixedInterpolate(fixed_t a, fixed_t b, fixed_t frac)
{
    return a + FixedMul(b - a, frac);
}

fixed_t FixedMulHStep(fixed_t a, fixed_t b)
{
    // TODO: FIX THIS
    return FixedMul(a,b);
}

fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    if ( (abs(a)>>14) >= abs(b))
	    return (a^b)<0 ? INT_MIN : INT_MAX;
        
    return FixedDiv2(a,b);
}

fixed_t FixedDiv2(fixed_t a, fixed_t b)
{
    long long c;
    c = ((long long)a<<16) / ((long long)b);
    return (fixed_t) c;
}

fixed_t FixedDivDBITS(fixed_t a, fixed_t b)
{
    // TODO: FIX
    return FixedDiv(a, b);
}

unsigned char ROLAND1(int value)
{
    // TODO: FIX
    return 1;
}

#endif

