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
; DESCRIPTION: Assembly texture mapping routines for planar VGA mode
;

BITS 32
%include "macros.inc"

%ifdef MODE_13H

extern _backbuffer

BEGIN_DATA_SECTION

align 4

_vrambuffer: times 64000 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_CopyLine
	push	ecx
	mov		ecx,edx
L$1:
	mov		edx,_backbuffer[eax]
	cmp		dl,_vrambuffer[eax]
	je		L$2
	mov		_vrambuffer[eax],dl
	mov		0xa0000[eax],dl
L$2:
	cmp		dh,_vrambuffer[eax+1]
	je		L$3
	mov		_vrambuffer[eax+1],dh
	mov		0xa0000[eax+1],dh
L$3:
	shr		edx,16
	cmp		dl,_vrambuffer[eax+2]
	je		L$4
	mov		_vrambuffer[eax+2],dl
	mov		0xa0000[eax+2],dl
L$4:
	cmp		dh,_vrambuffer[eax+3]
	je		L$5
	mov		_vrambuffer[eax+3],dh
	mov		0xa0000[eax+3],dh
L$5:
	add		eax,4
	cmp		eax,ecx
	jb		L$1
	pop		ecx
	ret
%endif
