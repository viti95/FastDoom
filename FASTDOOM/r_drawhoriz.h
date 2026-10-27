//
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
//	Horizontal (two-pass, cache-friendly) column rendering
//	(r_drawhoriz.c).
//

#ifndef __R_DRAWHORIZ__
#define __R_DRAWHORIZ__

#include "options.h"

#if defined(SUPPORTS_HORIZ_RENDER)

// Wall colfunc replacement: stage the dc_* column into the current
// four-column group instead of drawing it to the screen directly.
extern void R_StageColumnHoriz(void);

// Flush any pending wall columns to the screen. Must be called
// whenever wall drawing switches context (after a seg loop or a
// masked seg range), so a later seg can still overwrite columns.
extern void R_FlushWallHoriz(void);

#else

// The horizontal wall renderer is not available in this video
// mode, so nothing ever gets staged and flushing is a no-op.
#define R_FlushWallHoriz() ((void)0)

#endif

#endif
