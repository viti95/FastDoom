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

_lastlatch:   dw 0
_vrambuffer: times 16000 dw 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp
  mov		ebp,[_ptrlut16colors]
  mov   esi,0xA0000
	mov   ebx,_vrambuffer
  mov   ecx,_backbuffer
L$2:
  xor   eax,eax
	mov		al,byte [ecx+3]
	mov		dx,[ebp+eax*2]
	mov   al,byte [ecx+2]
	mov		di,[ebp+eax*2]
	mov   al,byte [ecx+1]
  shrd  dx,di,4
	mov		di,[ebp+eax*2]
	mov   al,byte [ecx]
  shrd  dx,di,4
	mov		ax,[ebp+eax*2]
	shrd  dx,ax,4
	cmp		dx,[ebx]
	jne		L$4
L$3:
	add		ebx,2
	inc		esi
	add		ecx,4
	cmp		esi,0xA3E80
	jb		L$2
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
L$4:
	mov   ax,dx
	shr		ax,4
	mov		[ebx],dx
	cmp		ax,[_lastlatch]
	je		L$5
	mov		[_lastlatch],ax
	mov   al,byte [0xA3E80 + eax]
L$5:
	mov		[esi],dl
	jmp		L$3

%endif
