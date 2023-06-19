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

BEGIN_DATA_SECTION

align 4

_lastlatch:   dw 0
_vrambuffer: times 16000 dw 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	push	ebp
  	mov		ebp,[_ptrlut16colors]
  	mov		esi,0xA0000-1
	mov   	ebx,_vrambuffer-2
  	mov   	edi,_backbuffer
L$2:
  	xor   	eax,eax
	mov		al,byte [edi]
	add		ebx,2
	mov		dl,[ebp+eax]
	mov   	al,byte [edi+1]
	inc		esi
	mov		ch,[ebp+eax]
	mov   	al,byte [edi+2]
  	shld  	dx,cx,4
	mov		ch,[ebp+eax]
	mov   	al,byte [edi+3]
  	shld  	dx,cx,4
	mov		ah,[ebp+eax]
	add		edi,4
	shld  	dx,ax,4
	cmp		[ebx],dx
	je		L$3
L$4:
	mov   	ax,dx
	shr		ax,4
	mov		[ebx],dx
	cmp		[_lastlatch],ax
	je		L$5
	mov		[_lastlatch],ax
	mov   	al,byte [0xA3E80 + eax]
L$5:
	mov		[esi],dl
L$3:
	cmp		esi,0xA3E80-1
	jb		L$2
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
%endif
