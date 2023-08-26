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

%ifdef MODE_CVB

extern _backbuffer
extern _ptrlut16colors
extern _lut16colors

BEGIN_DATA_SECTION

_vrambuffer: times 16384 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_SetPalette
	shl		eax,8
	add		eax,(_lut16colors+0xff)
	mov		[_ptrlut16colors],eax
	ret

CODE_SYM_DEF I_FinishUpdate
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	push	ebp

	mov		eax,[_ptrlut16colors]

	mov		esi,_backbuffer

	xor		ecx,ecx
	xor		ebx,ebx

	xor		edi,edi

start:
	mov		ebp,40

twoScanlines:
	
	mov		al,[esi]
	mov		ch,[eax]

	mov		al,[esi+4]
	mov		cl,[eax]

	and		cx,0xF0F0

	mov		al,[esi+2]
	mov		bh,[eax]

	mov		al,[esi+6]
	mov		bl,[eax]

	and		bx,0x0F0F

	or		ecx,ebx

	cmp		[_vrambuffer+edi],ch
	je		lowECX		
	mov		[_vrambuffer+edi],ch
	mov		[0xB8000+edi],ch

lowECX:

	cmp		[_vrambuffer+edi+1],cl
	je		secondScanline		
	mov		[_vrambuffer+edi+1],cl
	mov		[0xB8000+edi+1],cl

secondScanline:

	mov		al,[esi+320]
	mov		ch,[eax]

	mov		al,[esi+4+320]
	mov		cl,[eax]

	and		cx,0xF0F0

	mov		al,[esi+2+320]
	mov		bh,[eax]

	mov		al,[esi+6+320]
	mov		bl,[eax]

	and		bx,0x0F0F

	or		ecx,ebx

	cmp		[_vrambuffer+edi+0x2000],ch
	je		lowECX2		
	mov		[_vrambuffer+edi+0x2000],ch
	mov		[0xB8000+edi+0x2000],ch

lowECX2:

	cmp		[_vrambuffer+edi+1+0x2000],cl
	je		nextIteration		
	mov		[_vrambuffer+edi+1+0x2000],cl
	mov		[0xB8000+edi+1+0x2000],cl

nextIteration:

	add		edi,2
	add		esi,8

	dec		ebp
	ja		twoScanlines

	add		esi,320

	cmp		di,0x1F40
	jb		start

	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
