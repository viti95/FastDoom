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

	xor 	ebx,ebx

	mov		edx,_backbuffer
L$2:
	mov		edi, 0x50
L$3:
	mov		ebp,[_ptrlut16colors]
	movzx		eax,byte [edx]
	movzx		esi,byte [edx + 1]
	mov		ax,word [ebp+eax*2]
	mov		si,word [ebp+esi*2]
	and		eax,0c0c0H
	and		esi,3030H
	or		eax,esi
	movzx		esi,byte [edx + 2]
	mov		si,word [ebp+esi*2]
	and		esi,0c0cH
	or		eax,esi
	movzx		esi,byte [edx + 3]
	mov		si,word [ebp+esi*2]
	and		esi,303H
	or		eax,esi
	cmp		al,[_vrambuffer + ebx]
	jne		L$8
L$4:
	cmp		ah,[_vrambuffer + ebx + 0x4000]
	je		L$5
	mov		[0xB8000 + ebx + 0x4000],ah
	mov		[_vrambuffer + ebx + 0x4000],ah
L$5:
	mov		ebp,dword [_ptrlut16colors]
	movzx	eax,byte [edx + 320]
	movzx	esi,byte [edx + 321]
	mov		ax,word [ebp+eax*2]
	mov		si,word [ebp+esi*2]
	and		eax,0c0c0H
	and		esi,3030H
	or		eax,esi
	movzx	esi,byte [edx + 322]
	mov		si,word [ebp+esi*2]
	and		esi,0c0cH
	or		eax,esi
	movzx	esi,byte [edx + 323]
	mov		si,word [ebp+esi*2]
	and		esi,303H
	or		eax,esi
	cmp		al,[_vrambuffer + ebx + 0x2000]
	je		L$6
	mov		[0xB8000 + ebx + 0x2000],al
	mov		[_vrambuffer + ebx + 0x2000],al
L$6:
	cmp		ah,[_vrambuffer + ebx + 0x6000]
	je		L$7
	mov		[0xB8000 + ebx + 0x6000],ah
	mov		[_vrambuffer + ebx + 0x6000],ah
L$7:
	add		edx,4
	inc		ebx
	dec		edi
	ja		L$3
	add		edx,140H
	cmp		edx,_backbuffer + 0x0FA00
	jb		L$2
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
L$8:
	mov		[0xB8000 + ebx],al
	mov		[_vrambuffer + ebx],al
	jmp		near L$4

%endif
