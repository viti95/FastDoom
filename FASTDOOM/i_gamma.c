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

#define FIXED_POINT_SHIFT 16
#define FIXED_POINT_ONE (1 << FIXED_POINT_SHIFT)
#define TABLE_SIZE 256

// Converts integer to fixed-point 16.16
#define TO_FIXED(x) ((x) << FIXED_POINT_SHIFT)

unsigned char gammatable[256];

// Raises a fixed-point number to an integer power
int FixedPow(int base, int exp) {
    int result = FIXED_POINT_ONE;
    while (exp > 0) {
        if (exp & 1) {
            result = FixedMul(result, base);
        }
        base = FixedMul(base, base);
        exp >>= 1;
    }
    return result;
}

void I_SetGamma(fixed_t gamma) {
    int i = 0;
    
    for (i = 0; i < TABLE_SIZE; i++) {
        int x_fixed = TO_FIXED(i);

        // Normalize x to range [0, 1]
        int x_norm = FixedMul(x_fixed, TO_FIXED(1) / (TABLE_SIZE - 1));

        // Apply gamma correction: x^gamma
        int corrected = FixedPow(x_norm, gamma);

        // Scale back to 0-255 range
        int scaled = (corrected * 63) >> FIXED_POINT_SHIFT;

        // Clamp to byte range
        gammatable[i] = (unsigned char)(scaled < 0 ? 0 : (scaled > 63 ? 63 : scaled));
    }
}
