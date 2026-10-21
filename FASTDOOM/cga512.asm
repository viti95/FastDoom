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

%ifdef MODE_CGA512

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
extern _ptrlut256colors

BEGIN_DATA_SECTION

_vrambuffer: times 8000 dw 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	push	ebp

	xor		esi,esi

	mov		edi,_backbuffer
	mov		ebp,[_ptrlut256colors]
	mov		dx,0x03da

	xor		eax,eax
	
L$20:
	mov		bl,0x50
L$21:
	mov		al,byte [edi]
	add		esi,0x2
	mov		cx,[eax*2+ebp]
	add		edi,0x4
	cmp		[_vrambuffer+esi-2],cx
	je		L$14
	mov		[_vrambuffer+esi-2],cx
L$17:
	in		al,dx
	test	al,0x1
	je		L$17
	mov		[0xB8000+esi-2],cx
L$14:
	dec		bl
	jne		L$21
	add		edi,BACKBUFFER_LINEWIDTH
	cmp		si,0x3E80-2
	jb		L$20

	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret


%endif
