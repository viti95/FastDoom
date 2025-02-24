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

%ifdef MODE_VBE2

extern _backbuffer
extern _ptrprocessedpalette
extern _pcscreen

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate15bpp16bppLinear
	push	ebx
	push	ecx
	push	edx
	push	edi
	push	esi
	push	ebp

	mov		esi,[_ptrprocessedpalette]
	mov		edi,[_pcscreen]
	xor		ebp,ebp
	xor		ecx,ecx
	xor		eax,eax
	xor		ebx,ebx

loop1516linear:
	mov		eax,_backbuffer[ebp]
	
	mov		bl,ah
	mov		cl,al
	
	mov		dx,[esi+ebx*2]
	shl		edx,16
	mov		dx,[esi+ecx*2]

	add		ebp,4

	mov		[edi],edx

	shr		eax,16

	mov		bl,ah
	mov		cl,al
	
	mov		dx,[esi+ebx*2]

	add		edi,8

	shl		edx,16

	mov		dx,[esi+ecx*2]

	cmp		ebp,SCREENWIDTH*SCREENHEIGHT

	mov		[edi-4],edx

	jl		loop1516linear

	pop		ebp
	pop		esi
	pop		edi
	pop		edx
	pop		ecx
	pop		ebx
	ret

CODE_SYM_DEF I_FinishUpdate32bppLinear
	push	ebx
	push	ecx
	push	edx
	push	edi
	push	esi
	push	ebp

	mov		ebx,[_ptrprocessedpalette]
	mov		edx,[_pcscreen]
	xor		eax,eax

loop32linear:
	mov		cl,_backbuffer[eax]
	movzx	ecx,cl
	add		edx,0x00000004
	mov		ecx,[ebx+ecx*4]
	inc		eax
	mov		-0x4[edx],ecx
	cmp		eax,SCREENWIDTH*SCREENHEIGHT
	jl		loop32linear

	pop		ebp
	pop		esi
	pop		edi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
