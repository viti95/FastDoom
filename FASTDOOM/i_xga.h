/*
 * Copyright (C) 2026 FastDoom contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *   IBM Micro Channel XGA video driver, see i_xga.c.
 */

#ifndef __I_XGA_H__
#define __I_XGA_H__

#include "doomtype.h"

void XGA_InitGraphics(void);
void XGA_ShutdownGraphics(void);

/* XGA palette hooks (see I_ProcessPalette/I_SetPalette in i_system.h) */
void I_ProcessPalette(byte *palette);
void I_SetPalette(int numpalette);

void I_FinishUpdate(void);

#endif
