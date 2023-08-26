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

	xor		esi,esi

	mov		ecx,_backbuffer
	
L$20:
	mov		ebx,0x50
L$21:
	movzx	edx,byte [ecx]
	mov		eax,[_ptrlut256colors]
	add		edx,edx
	mov		di,[edx+eax]
	cmp		di,[_vrambuffer+esi]
	je		L$14
	mov		[_vrambuffer+esi],di
	mov		dx,0x03da
L$17:
	in		al,dx
	test	al,0x01
	je		L$17
	mov		[0xB8000+esi],di
L$14:
	add		esi,0x2
	add		ecx,0x4
	dec		ebx
	jne		L$21
	add		ecx,0x140
	cmp		si,0x3E80
	jb		L$20
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret


%endif
