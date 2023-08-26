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

%ifdef MODE_CGA512

extern _backbuffer
extern _ptrlut256colors

BEGIN_DATA_SECTION

_vrambuffer: times 8000 dw 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	sub		esp,0x00000004
	mov		esi,0x000b8000
	mov		ecx,_backbuffer
	mov		ebx,_vrambuffer
	mov		byte [esp],0x50
L$13:
	movzx	edx,byte [ecx]
	mov		eax,[_ptrlut256colors]
	add		edx,edx
	mov		di,[edx+eax]
	cmp		di,[ebx]
	jne		L$16
L$14:
	add		esi,0x00000002
	add		ebx,0x00000002
	add		ecx,0x00000004
	dec		byte [esp]
	jne		L$15
	mov		byte [esp],0x50
	add		ecx,0x00000140
L$15:
	cmp		esi,0x000bbe80
	jb		L$13
	add		esp,0x00000004
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
L$16:
	mov		[ebx],di
	mov		dx,0x03da
L$17:
	in		al,dx
	test	al,0x01
	je		L$17
	mov		[esi],di
	jmp		L$14

%endif
