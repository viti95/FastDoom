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

%ifdef MODE_CGA_BW

// Mirror of the BACKBUFFER_POW2_WIDTH logic in doomdef.h: when the
// flag is set, pad backbuffer rows to the next power of two so the
// assembly copyers advance rows by the same stride as the C code.
// These drivers all use a 320 pixel wide backbuffer (the C default;
// wider modes expand the pixels in assembly), so the padded stride
// is 512 bytes.
%ifndef SCREENWIDTH
%define SCREENWIDTH 320
%endif
%ifdef BACKBUFFER_POW2_WIDTH
%if SCREENWIDTH <= 512
%define BACKBUFFER_LINEWIDTH 512
%elif SCREENWIDTH <= 1024
%define BACKBUFFER_LINEWIDTH 1024
%elif SCREENWIDTH <= 2048
%define BACKBUFFER_LINEWIDTH 2048
%else
%define BACKBUFFER_LINEWIDTH 4096
%endif
%else
%define BACKBUFFER_LINEWIDTH SCREENWIDTH
%endif

extern _backbuffer
extern _ptrlutcolors
extern _lutcolors

BEGIN_DATA_SECTION

_vrambuffer: times 16384 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_SetPalette
	shl		eax,8
	add		eax,(_lutcolors+0xff)
	mov		[_ptrlutcolors],eax
	ret

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edi
	push		esi
	push		edx
	push		ebp

	mov		edx,[_ptrlutcolors]
	xor		esi,esi
	mov 	edi,_backbuffer
	xor		ecx,ecx
	mov		ebx,edx

L$2:
	mov		ebp,40
L$3:
	mov		dl,[edi]
	mov		bl,[edi+4]
	mov		ah,[edx]
	mov		dl,[edi+1]
	mov		al,[ebx]
	mov		bl,[edi+5]
	mov		ch,[edx]
	mov		dl,[edi+2]
	mov		cl,[ebx]
	mov		bl,[edi+6]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		dl,[edi+3]
	mov		cl,[ebx]
	mov		bl,[edi+7]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		cl,[ebx]
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
	mov		bl,[edi+324]
	mov		ah,[edx]
	mov		dl,[edi+321]
	mov		al,[ebx]
	mov		bl,[edi+325]
	mov		ch,[edx]
	mov		dl,[edi+322]
	mov		cl,[ebx]
	mov		bl,[edi+326]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		dl,[edi+323]
	mov		cl,[ebx]
	mov		bl,[edi+327]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		cl,[ebx]
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
	add		edi,BACKBUFFER_LINEWIDTH
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
