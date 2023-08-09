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

	mov		ebx,-1

	mov		esi,_backbuffer
	mov		ebp,[_ptrlut16colors]

	xor		edx,edx

L$2:
	mov		edi, 0x50
L$3:
	mov 	dl,[esi]
	inc		ebx
	mov		ax,[ebp+edx*2]
	mov		dl,[esi+1]
	mov		cx,[ebp+edx*2]

	lea		eax,[eax*4 + ecx]

	mov		dl,[esi+2]
	mov		cx,[ebp+edx*2]

	lea		eax,[eax*4 + ecx]

	mov		dl,[esi+3]
	mov		cx,[ebp+edx*2]

	lea		eax,[eax*4 + ecx]

	cmp		[_vrambuffer + ebx],al
	je		L$4
	mov		[0xB8000 + ebx],al
	mov		[_vrambuffer + ebx],al
L$4:
	cmp		[_vrambuffer + ebx + 0x4000],ah
	je		L$5
	mov		[0xB8000 + ebx + 0x4000],ah
	mov		[_vrambuffer + ebx + 0x4000],ah
L$5:
	mov 	dl,[esi+320]
	mov		ax,[ebp+edx*2]
	mov		dl,[esi+321]
	mov		cx,[ebp+edx*2]

	lea		eax,[eax*4 + ecx]

	mov		dl,[esi+322]
	mov		cx,[ebp+edx*2]

	lea		eax,[eax*4 + ecx]

	mov		dl,[esi+323]
	mov		cx,[ebp+edx*2]

	lea		eax,[eax*4 + ecx]

	cmp		[_vrambuffer + ebx + 0x2000],al
	je		L$6
	mov		[0xB8000 + ebx + 0x2000],al
	mov		[_vrambuffer + ebx + 0x2000],al
L$6:
	cmp		[_vrambuffer + ebx + 0x6000],ah
	je		L$7
	mov		[0xB8000 + ebx + 0x6000],ah
	mov		[_vrambuffer + ebx + 0x6000],ah
L$7:
	add		esi,4
	dec		edi
	ja		L$3
	add		esi,140H
	cmp		bx,0x1F40
	jb		L$2
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
