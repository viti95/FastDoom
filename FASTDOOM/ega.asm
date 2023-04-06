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
	xor		esi,esi
	xor		ebx,ebx
  mov   ecx,_backbuffer
L$2:
	movzx		eax,byte [ecx]
	mov		ebp,[_ptrlut16colors]
	mov		dx,[ebp+eax*2]
	movzx		eax,byte [ecx+1]
	mov		di,[ebp+eax*2]
	and		dx,0f000H
	and		di,0f00H
	movzx		eax,byte [ecx+2]
	or		dx,di
	mov		di,[ebp+eax*2]
	movzx		eax,byte [ecx+3]
	mov		ax,[ebp+eax*2]
	and		di,0f0H
	xor		ah,ah
	or		dx,di
	and		al,0fH
	or		dx,ax
	cmp		dx,[_vrambuffer + ebx]
	jne		L$4
L$3:
	add		ebx,2
	inc		esi
	add		ecx,4
	cmp		esi,3e80H
	jb		L$2
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
L$4:
	movzx		eax,dx
	sar		eax,4
	mov		[_vrambuffer + ebx],dx
	cmp		ax,[_lastlatch]
	je		L$5
	mov		[_lastlatch],ax
	movzx		eax,ax
	movzx		eax,byte [0xA3E80 + eax]
	mov		al,[eax]
L$5:
	mov		[0xA0000 + esi],dl
	jmp		L$3

%endif
