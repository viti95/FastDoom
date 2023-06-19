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
  	mov		eax,[_ptrlut16colors]
	mov		[patch1+2],eax
	mov		[patch2+2],eax
	mov		[patch3+2],eax
	mov		[patch4+2],eax
  	mov		esi,0xA0000-1
	mov   	ebx,_vrambuffer-2
  	mov   	ebp,_backbuffer
	xor   	eax,eax
	mov		di,[_lastlatch]
	xor   	ecx,ecx
L$2:
	mov		al,byte [ebp]
	add		ebx,2
patch1:
	mov		dl,[0xDEADBEEF+eax]
	mov   	al,byte [ebp+1]
	inc		esi
patch2:
	mov		ch,[0xDEADBEEF+eax]
	mov   	al,byte [ebp+2]
  	shld  	dx,cx,4
patch3:
	mov		ch,[0xDEADBEEF+eax]
	mov   	al,byte [ebp+3]
  	shld  	dx,cx,4
patch4:
	mov		ch,[0xDEADBEEF+eax]
	add		ebp,4
	shld  	dx,cx,4
	cmp		[ebx],dx
	je		L$3
	mov   	cx,dx
	shr		cx,4
	mov		[ebx],dx
	cmp		di,cx
	je		L$4
	mov		di,cx
	mov   	al,byte [0xA3E80 + ecx]
L$4:
	mov		[esi],dl
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
