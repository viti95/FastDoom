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

%ifdef MODE_EGA

extern _backbuffer
extern _ptrlut16colors
extern _lut16colors
extern _numpalette

BEGIN_DATA_SECTION

align 4

_lastlatch:   dw 0
_vrambuffer: times 16000 dw 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_SetPalette
	shl		eax,8
	add		eax,(_lut16colors+0xff)
	mov		[_ptrlut16colors],eax
	ret

CODE_SYM_DEF I_FinishUpdate
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	push	ebp
  	mov		esi,0xA0000-1
	mov   	ebx,_vrambuffer-2
  	mov   	ebp,_backbuffer
	mov		eax,[_ptrlut16colors]
	mov		di,[_lastlatch]
	xor		edx,edx
L$2:
	mov		al,byte [ebp]
	add		ebx,2
	mov		dh,[eax]
	mov   	al,byte [ebp+1]
	inc		esi
	mov		ch,[eax]
	mov   	al,byte [ebp+2]
	add		ebp,4
	mov		dl,[eax]
	mov   	al,byte [ebp-1]
	and		dx, 0xF0F0
	mov		cl,[eax]
	and		cx, 0x0F0F
	or		edx,ecx
	cmp		[ebx],dx
	je		L$3
	mov		[ebx],dx
	shr		dx,4
	cmp		di,dx
	je		L$4
	mov		di,dx
	mov   	al,byte [0xA3E80 + edx]
L$4:
	mov		[esi],cl
L$3:
	cmp		esi,0xA3E80-1
	jb		L$2
	mov		[_lastlatch],di
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
%endif
