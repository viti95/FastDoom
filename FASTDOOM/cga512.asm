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
	push	ebp

	xor		esi,esi

	mov		edi,_backbuffer
	mov		ebp,[_ptrlut256colors]
	mov		dx,0x03da

	xor		eax,eax
	
L$20:
	mov		ebx,0x50
L$21:
	mov		al,byte [edi]
	add		edi,0x4
	mov		cx,[eax*2+ebp]
	cmp		[_vrambuffer+esi],cx
	je		L$14
	mov		[_vrambuffer+esi],cx
L$17:
	in		al,dx
	test	al,0x1
	je		L$17
	mov		[0xB8000+esi],cx
L$14:
	add		esi,0x2
	dec		ebx
	jne		L$21
	add		edi,0x140
	cmp		si,0x3E80
	jb		L$20

	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret


%endif
