/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/**********************************************************************
   module: PCFX.H

   author: James R. Dose
   date:   April 1, 1994

   Public header for ADLIBFX.C

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef __PCFX_H
#define __PCFX_H

enum PCFX_Errors
   {
   PCFX_Warning = -2,
   PCFX_Error   = -1,
   PCFX_Ok      = 0,
   PCFX_NoVoices,
   PCFX_VoiceNotFound,
   PCFX_DPMI_Error
   };

#define PCFX_MaxVolume      255
#define PCFX_MinVoiceHandle 1

typedef	struct
   {
   unsigned long  length;
   short int      priority;
   char           data[];
   } PCSound;

char *PCFX_ErrorString( int ErrorNumber );
int   PCFX_Stop( int handle );
void  PCFX_UseLookup( int use, unsigned value );
int   PCFX_VoiceAvailable( int priority );
int   PCFX_Play( PCSound *sound, int priority, unsigned long callbackval );
int   PCFX_SoundPlaying( int handle );
int   PCFX_SetTotalVolume( int volume );
int   PCFX_GetTotalVolume( void );
void  PCFX_SetCallBack( void ( *function )( unsigned long ) );
int   PCFX_Init( void );
int   PCFX_Shutdown( void );
   #pragma aux PCFX_Shutdown frame;
void  PCFX_UnlockMemory( void );
int   PCFX_LockMemory( void );

#endif
