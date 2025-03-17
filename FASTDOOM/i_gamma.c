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
//  System interface for sound.
//

#include <stdio.h>

#include "doomstat.h"
#include "options.h"
#include "i_debug.h"
#include "fastmath.h"

#define LN_MAX_ITER 20
#define EXP_MAX_ITER 20
#define LN2 45426 // ln(2) in 16.16 format

unsigned char gammatable[256];

// Natural logarithm approximation
int FixedLn(int x)
{
    int result = 0;
    int term;
    int i;

    if (x <= 0)
        return result; // Logarithm not defined for non-positive numbers

    while (x > FRACUNIT)
    {
        result += LN2;
        x = FixedDiv(x, 2 << FRACBITS);
    }
    while (x < FRACUNIT)
    {
        result -= LN2;
        x = FixedMul(x, 2 << FRACBITS);
    }
    x -= FRACUNIT;
    term = x;
    for (i = 1; i <= LN_MAX_ITER; i++)
    {
        if (i % 2 == 1)
        {
            result += FixedDiv(term, TO_FIXED(i));
        }
        else
        {
            result -= FixedDiv(term, TO_FIXED(i));
        }
        term = FixedMul(term, x);
    }
    return result;
}

// Exponential function approximation
int FixedExp(int x)
{
    int result = FRACUNIT;
    int term = FRACUNIT;
    int i;

    for (i = 1; i <= EXP_MAX_ITER; i++)
    {
        term = FixedMul(term, FixedDiv(x, TO_FIXED(i)));
        result += term;
        if (term == 0)
            break; // Early stopping for small terms
    }
    return result;
}

// Power function: x^y
int FixedPow(int x, int y)
{
    int log_x;
    int y_log_x;

    if (x <= 0)
        return 0; // Undefined for non-positive bases

    log_x = FixedLn(x);           // Compute ln(x)
    y_log_x = FixedMul(y, log_x); // Multiply by y

    return FixedExp(y_log_x); // Return exp(y * ln(x))
}

fixed_t levels[5] = {65536, 75366, 88474, 106168, 131072};

void I_SetGamma(int usegamma)
{

    int i = 0;

    fixed_t gamma = levels[usegamma];

    int inv_gamma = FixedDiv(TO_FIXED(1), gamma);

    for (i = 0; i < 256; i++)
    {

        int x_fixed = TO_FIXED(i);

        int x_divided = FixedDiv(x_fixed, TO_FIXED(255));

        int x_power = FixedPow(x_divided, inv_gamma);

        int x_mul_power = FixedMul(TO_FIXED(63), x_power);

        x_mul_power >>= FRACBITS;

        if (x_mul_power > 63)
        {
            x_mul_power = 63;
        }
        else if (x_mul_power < 0)
        {
            x_mul_power = 0;
        }

        gammatable[i] = x_mul_power;
    }
}
