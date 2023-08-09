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

%ifdef MODE_CGA

extern _backbuffer
extern _ptrlut4colors
extern _lut4colors

BEGIN_DATA_SECTION

_vrambuffer: times 16384 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_SetPalette
	shl		eax,8
	add		eax,(_lut4colors+0xff)
	mov		[_ptrlut4colors],eax
	ret

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edi
	push		esi
	push		edx
	push		ebp

	mov		edx,[_ptrlut4colors]
	xor		esi,esi
	mov 	edi,_backbuffer
	xor		ecx,ecx

L$2:
	mov		ebp,40
L$3:
	mov		dl,[edi]
	mov		ah,[edx]

	mov		dl,[edi+4]
	mov		al,[edx]

	mov		dl,[edi+1]
	mov		ch,[edx]

	mov		dl,[edi+5]
	mov		cl,[edx]

	lea		eax,[eax*4 + ecx]

	mov		dl,[edi+2]
	mov		ch,[edx]

	mov		dl,[edi+6]
	mov		cl,[edx]

	lea		eax,[eax*4 + ecx]

	mov		dl,[edi+3]
	mov		ch,[edx]

	mov		dl,[edi+7]
	mov		cl,[edx]

	lea		eax,[eax*4 + ecx]

	cmp		[_vrambuffer + esi],ah
	je		L$10
	mov		[_vrambuffer + esi],ah
	mov		[0xB8000 + esi],ah

L$10:
	cmp		[_vrambuffer + esi + 1],al
	je		L$4
	mov		[_vrambuffer + esi + 1],al
	mov		[0xB8000 + esi + 1],al

L$4:
	mov		dl,[edi+320]
	mov		ah,[edx]

	mov		dl,[edi+324]
	mov		al,[edx]

	mov		dl,[edi+321]
	mov		ch,[edx]

	mov		dl,[edi+325]
	mov		cl,[edx]

	lea		eax,[eax*4 + ecx]

	mov		dl,[edi+322]
	mov		ch,[edx]

	mov		dl,[edi+326]
	mov		cl,[edx]

	lea		eax,[eax*4 + ecx]

	mov		dl,[edi+323]
	mov		ch,[edx]

	mov		dl,[edi+327]
	mov		cl,[edx]

	lea		eax,[eax*4 + ecx]

	cmp		[_vrambuffer + esi + 0x2000],ah
	je		L$11
	mov		[_vrambuffer + esi + 0x2000],ah
	mov		[0xBA000 + esi],ah

L$11:
	cmp		[_vrambuffer + esi + 0x2000 + 1],al
	je		L$5
	mov		[_vrambuffer + esi + 0x2000 + 1],al
	mov		[0xBA000 + esi + 1],al
L$5:
	add		esi,2
	add		edi,8
	dec		ebp
	ja		L$3
	add		edi,140H
	cmp		si,0x1F40
	jb		L$2
	pop		ebp
	pop		edx
	pop		esi
	pop		edi
	pop		ecx
	pop		ebx
	ret

%endif
