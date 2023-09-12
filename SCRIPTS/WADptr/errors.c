/**************************************************************************
 *                                                                        *
 * Copyright(C) 1998-2011 Simon Howard, Andreas Dehmel                    *
 *                                                                        *
 * This program is free software; you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation; either version 2 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program; if not, write to the Free Software            *
 * Foundation, Inc, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA *
 *                                                                        *
 *                          The WADPTR project                            *
 *                                                                        *
 * Error handling routines:                                               *
 *                                                                        *
 **************************************************************************/

#include <stdlib.h>
#include "errors.h"
#ifdef __riscos
#include "ROlib.h"
#endif

/* Display an error (Last remnant of the DMWAD heritage) ******************/

void errorexit(char *s, ...)
{

        va_list args;
        va_start(args, s);

#ifndef ANSILIBS
        sound( 640);                    /* thanks to the deu authors! */
        delay( 100);
        nosound();
#endif

        vfprintf(stderr, s, args);

        exit(0xff);
}

/* Signal handling stuff **************************************************/

void sig_func(int signalnum)
{
        printf("\n\n");
        switch(signalnum)
        {
                default:        errorexit("Bizarre signal error?\n");
                case SIGINT:    errorexit("User Interrupted\n");
                case SIGNOFP:   errorexit("Error:no FPU\n");
                case SIGSEGV:   errorexit("Segment violation error(memory fault)\n");
        }
}
