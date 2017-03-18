;
; Copyright (C) 1993-1996 Id Software, Inc.
; Copyright (C) 1993-2008 Raven Software
; Copyright (C) 2016-2017 Alexey Khokholov (Nuke.YKT)
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
SKIPPRIMITIVES	=  0			; set to 1 to skip unwound drawing


SCREEN  =       0a0000h
SCREENWIDTH     =   320
SCREENHEIGHT	=	200
PLANEWIDTH		=	80
PLANESIZE		=	80*200

PEL_WRITE_ADR	=	03c8h
PEL_DATA		=	03c9h

SC_INDEX		=	03C4h
SC_MAPMASK		=	2

OP_RET		=	0c3h
OP_MOVAL	= 	08ah
OP_MOVDEST	= 	088h


	.DATA

EXTRN	_dc_colormap:DWORD
EXTRN	_dc_x:DWORD
EXTRN	_dc_yl:DWORD
EXTRN	_dc_yh:DWORD
EXTRN	_dc_iscale:DWORD
EXTRN	_dc_texturemid:DWORD
EXTRN	_dc_source:DWORD

EXTRN	_ylookup:DWORD
EXTRN	_columnofs:DWORD


EXTRN	_ds_y:DWORD
EXTRN	_ds_x1:DWORD
EXTRN	_ds_x2:DWORD
EXTRN	_ds_colormap:DWORD
EXTRN	_ds_xfrac:DWORD
EXTRN	_ds_yfrac:DWORD
EXTRN	_ds_xstep:DWORD
EXTRN	_ds_ystep:DWORD
EXTRN	_ds_source:DWORD

PUSHR	MACRO
	pushad
ENDM

POPR	MACRO
	popad
ENDM
