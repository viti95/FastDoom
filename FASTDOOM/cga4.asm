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

%macro PIXEL 1
	mov		dl,[edi + (%1 * 4) + 0]
	mov		bl,[edi + (%1 * 4) + 4]
	mov		ah,[edx]
	mov		dl,[edi + (%1 * 4) + 1]
	mov		al,[ebx]
	mov		bl,[edi + (%1 * 4) + 5]
	mov		ch,[edx]
	mov		dl,[edi + (%1 * 4) + 2]
	mov		cl,[ebx]
	mov		bl,[edi + (%1 * 4) + 6]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		dl,[edi + (%1 * 4) + 3]
	mov		cl,[ebx]
	mov		bl,[edi + (%1 * 4) + 7]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		cl,[ebx]
	lea		eax,[eax*4 + ecx]

	cmp		[_vrambuffer + esi + %1],ah
	je		%%L$10
	mov		[_vrambuffer + esi + %1],ah
	mov		[0xB8000 + esi + %1],ah

%%L$10:
	cmp		[_vrambuffer + esi + %1 + 1],al
	je		%%L$4
	mov		[_vrambuffer + esi + %1 + 1],al
	mov		[0xB8000 + esi + %1 + 1],al

%%L$4:
	mov		dl,[edi + (%1 * 4) + 320]
	mov		bl,[edi + (%1 * 4) + 324]
	mov		ah,[edx]
	mov		dl,[edi + (%1 * 4) + 321]
	mov		al,[ebx]
	mov		bl,[edi + (%1 * 4) + 325]
	mov		ch,[edx]
	mov		dl,[edi + (%1 * 4) + 322]
	mov		cl,[ebx]
	mov		bl,[edi + (%1 * 4) + 326]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		dl,[edi + (%1 * 4) + 323]
	mov		cl,[ebx]
	mov		bl,[edi + (%1 * 4) + 327]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		cl,[ebx]
	lea		eax,[eax*4 + ecx]

	cmp		[_vrambuffer + esi + 0x2000 + %1],ah
	je		%%L$11
	mov		[_vrambuffer + esi + 0x2000 + %1],ah
	mov		[0xBA000 + esi + %1],ah

%%L$11:
	cmp		[_vrambuffer + esi + 0x2000 + %1 + 1],al
	je		%%L$5
	mov		[_vrambuffer + esi + 0x2000 + %1 + 1],al
	mov		[0xBA000 + esi + %1 + 1],al
%%L$5:
%endmacro

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
	mov		ebx,edx

.SCANLINE:

	%assign POSITION 0
	%rep 40
		PIXEL POSITION
	%assign POSITION POSITION+2
	%endrep

	add		esi,80
	add		edi,640
	cmp		si,0x1F40
	jb		.SCANLINE
	pop		ebp
	pop		edx
	pop		esi
	pop		edi
	pop		ecx
	pop		ebx
	ret

%endif
