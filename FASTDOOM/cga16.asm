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

%ifdef MODE_CGA16

extern _backbuffer
extern _ptrlut16colors
extern _lut16colors
global _vrambuffer

BEGIN_DATA_SECTION

_vrambuffer: times 16000 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_SetPalette
	shl		eax,8
	add		eax,(_lut16colors+0xff)
	mov		[_ptrlut16colors],eax
	ret

CODE_SYM_DEF CGA_16_DrawBackbuffer
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

	mov		eax,[_ptrlut16colors]
	mov		ebp,40
	mov		edi,_backbuffer
	xor		esi,esi
L$9:
	mov		al,[edi]
	mov		bh,[eax]
	
	mov		al,[edi+2]
	mov		ch,[eax]
	
	mov		al,[edi+4]
	mov		bl,[eax]

	mov		al,[edi+6]
	mov		cl,[eax]

	and		bx,0xF0F0
	and		cx,0x0F0F
	or		ebx,ecx

	cmp		[_vrambuffer + esi],bh
	je		L$13
	mov		[0xB8001 + esi],bh
	mov		[_vrambuffer + esi],bh
L$13:
	cmp		[_vrambuffer + esi + 2],bl
	je		L$10
	mov		[0xB8001 + esi + 2],bl
	mov		[_vrambuffer + esi + 2],bl
L$10:
	add		esi,4
	add		edi,8
	dec		ebp
	jne		L$11
	mov		ebp,40
	add		edi,0x140
L$11:
	cmp		esi,0x3E7F
	jb		L$9
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
