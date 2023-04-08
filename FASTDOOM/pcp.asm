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

%ifdef MODE_PCP

extern _backbuffer
extern _ptrlut16colors

BEGIN_DATA_SECTION

_vrambuffer: times 32768 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp
	sub		esp,4
	mov		ebx,0b8000H
	mov		ecx,_vrambuffer
	xor		edx,edx
L$2:
	xor		eax,eax
	mov		dword [esp],eax
	lea		eax,[eax]
L$3:
	mov		ebp,[_ptrlut16colors]
	movzx		eax,byte [_backbuffer + edx]
	movzx		esi,byte [_backbuffer + edx + 1]
	mov		ax,word [ebp+eax*2]
	mov		si,word [ebp+esi*2]
	and		eax,0c0c0H
	and		esi,3030H
	or		eax,esi
	movzx		esi,byte [_backbuffer + edx + 2]
	mov		si,word [ebp+esi*2]
	and		esi,0c0cH
	or		eax,esi
	movzx		esi,byte [_backbuffer + edx + 3]
	mov		si,word [ebp+esi*2]
	and		esi,303H
	or		eax,esi
	cmp		al,byte [ecx]
	jne		L$8
L$4:
	cmp		ah,byte 4000H[ecx]
	je		L$5
	mov		byte 4000H[ebx],ah
	mov		byte 4000H[ecx],ah
L$5:
	mov		ebp,dword [_ptrlut16colors]
	movzx		eax,byte [_backbuffer + edx + 320]
	movzx		esi,byte [_backbuffer + edx + 321]
	mov		ax,word [ebp+eax*2]
	mov		si,word [ebp+esi*2]
	and		eax,0c0c0H
	and		esi,3030H
	or		eax,esi
	movzx		esi,byte [_backbuffer + edx + 322]
	mov		si,word [ebp+esi*2]
	and		esi,0c0cH
	or		eax,esi
	movzx		esi,byte [_backbuffer + edx + 323]
	mov		si,word [ebp+esi*2]
	and		esi,303H
	or		eax,esi
	cmp		al,byte 2000H[ecx]
	je		L$6
	mov		byte 2000H[ebx],al
	mov		byte 2000H[ecx],al
L$6:
	cmp		ah,byte 6000H[ecx]
	je		L$7
	mov		byte 6000H[ebx],ah
	mov		byte 6000H[ecx],ah
L$7:
	inc		dword [esp]
	add		edx,4
	inc		ebx
	inc		ecx
	cmp		dword [esp],50H
	jl		L$3
	add		edx,140H
	cmp		edx,0fa00H
	jb		L$2
	add		esp,4
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
L$8:
	mov		byte [ebx],al
	mov		byte [ecx],al
	jmp		near L$4

%endif
