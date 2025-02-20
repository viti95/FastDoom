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

	mov		ebx,[_ptrprocessedpalette]
	mov		edi,[_pcscreen]
	xor		eax,eax
	xor		ecx,ecx

L$62:
	mov		cl,_backbuffer[eax]
	add		edi,0x00000002
	mov		dx,[ebx+ecx*2]
	inc		eax
	mov		-0x2[edi],dx
	cmp		eax,0x0000fa00
	jl		L$62

	pop		edi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
