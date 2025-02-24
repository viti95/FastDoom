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

%ifndef SCREENWIDTH
%define SCREENWIDTH   320
%define SCREENHEIGHT  200
%endif

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

CODE_SYM_DEF I_FinishUpdate24bppLinear
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

	mov		edi,[_ptrprocessedpalette]
	mov		ebp,[_pcscreen]
	xor		eax,eax ; position of screen
	xor		ebx,ebx
	xor		ecx,ecx
	xor		edx,edx

loop24linear:

	mov		bl, _backbuffer[eax]
	lea		ecx,[ebx+ebx*2] ; fast multiply by 3

	mov		dx, [edi+ecx]

	mov		[ebp],dx

	mov		dl,	[edi+ecx+2]

	mov		[ebp+2],dl

	inc		eax
	add		ebp,3

	cmp		eax,SCREENWIDTH*SCREENHEIGHT
	jl		loop24linear

	pop		ebp
	pop		edi
	pop		esi
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

	mov		esi,[_ptrprocessedpalette]
	mov		edi,[_pcscreen]
	xor		eax,eax
	xor		ecx,ecx
	xor		ebx,ebx

loop32linear:
	mov		cl,_backbuffer[eax]
	mov		bl,_backbuffer[eax+1]
	add		eax,2
	mov		ebp,[esi+ecx*4]
	mov		edx,[esi+ebx*4]
	add		edi,8
	cmp		eax,SCREENWIDTH*SCREENHEIGHT
	mov		[edi],ebp
	mov		[edi+4],edx
	jl		loop32linear

	pop		ebp
	pop		esi
	pop		edi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
