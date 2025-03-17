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

#include <string.h>
#include <stdio.h>

#include "std_func.h"
#include "i_system.h"
#include "dstrings.h"
#include "doomstat.h"
#include "options.h"
#include "i_debug.h"
#include "z_zone.h"

#define LN_MAX_ITER 20
#define EXP_MAX_ITER 20
#define FIXED_POINT_SHIFT 16
#define FIXED_POINT_ONE (1 << FIXED_POINT_SHIFT)
#define LN2 45426 // ln(2) in 16.16 format

// Converts integer to fixed-point 16.16
#define TO_FIXED(x) ((x) << FIXED_POINT_SHIFT)

#define BASE (16)

unsigned char gammatable[256];

// Natural logarithm approximation
int fixed_ln(int x) {
    int result = 0;
    int term;
    int i;
    
    if (x <= 0) return result; // Logarithm not defined for non-positive numbers
    
    while (x > FIXED_POINT_ONE) {
        result += LN2;
        x = FixedDiv(x, 2 << FIXED_POINT_SHIFT);
    }
    while (x < FIXED_POINT_ONE) {
        result -= LN2;
        x = FixedMul(x, 2 << FIXED_POINT_SHIFT);
    }
    x -= FIXED_POINT_ONE;
    term = x;
    for (i = 1; i <= LN_MAX_ITER; i++) {
        if (i % 2 == 1) {
            result += FixedDiv(term, TO_FIXED(i));
        } else {
            result -= FixedDiv(term, TO_FIXED(i));
        }
        term = FixedMul(term, x);
    }
    return result;
}

// Exponential function approximation
int fixed_exp(int x) {
    int result = FIXED_POINT_ONE;
    int term = FIXED_POINT_ONE;
    int i;

    for (i = 1; i <= EXP_MAX_ITER; i++) {
        term = FixedMul(term, FixedDiv(x, TO_FIXED(i)));
        result += term;
        if (term == 0) break; // Early stopping for small terms
    }
    return result;
}

// Power function: x^y
int FixedPow(int x, int y) {
    int log_x;
    int y_log_x;

    if (x <= 0) return 0; // Undefined for non-positive bases
    
    log_x = fixed_ln(x); // Compute ln(x)
    y_log_x = FixedMul(y, log_x); // Multiply by y
    
    return fixed_exp(y_log_x); // Return exp(y * ln(x))
}

void I_SetGamma(fixed_t gamma) {
    int i = 0;

    int inv_gamma = FixedDiv(TO_FIXED(1), gamma);
    
    for (i = 0; i < 256; i++) {

        //BITRSHIFT(ROUND(255 * POWER(H2 / 255;1/gamma); 0); 2)

        int x_fixed = TO_FIXED(i);

        int x_divided = FixedDiv(x_fixed, TO_FIXED(255));

        int x_power = FixedPow(x_divided, inv_gamma);

        int x_mul_power = FixedMul(TO_FIXED(63), x_power);

        x_mul_power >>= 16;

        // Clamp to byte range
        gammatable[i] = x_mul_power;
    }
}
