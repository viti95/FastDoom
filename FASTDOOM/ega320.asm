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

%ifdef MODE_EGA

extern _backbuffer
extern _ptrlut16colors
extern _lut16colors

BEGIN_DATA_SECTION

align 4

_vrambufferR: times 8000 db 0
_vrambufferG: times 8000 db 0
_vrambufferB: times 8000 db 0
_vrambufferI: times 8000 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	push	ebp
	
  	mov   	ebp,_backbuffer
	mov		edi,[_ptrlut16colors]
	xor		esi,esi
	mov		edx,0x3C5
	xor		eax,eax
	
start:
	mov		al,[ebp]
	mov		ecx,[edi + eax*4]

	mov		al,[ebp+1]
	mov		ebx,[edi + eax*4]

	lea		ecx,[ecx*2 + ebx]

	mov		al,[ebp+2]
	mov		ebx,[edi + eax*4]

	lea		ecx,[ecx*2 + ebx]

	mov		al,[ebp+3]
	mov		ebx,[edi + eax*4]

	lea		ecx,[ecx*2 + ebx]

	mov		al,[ebp+4]
	mov		ebx,[edi + eax*4]

	lea		ecx,[ecx*2 + ebx]

	mov		al,[ebp+5]
	mov		ebx,[edi + eax*4]

	lea		ecx,[ecx*2 + ebx]

	mov		al,[ebp+6]
	mov		ebx,[edi + eax*4]

	lea		ecx,[ecx*2 + ebx]

	mov		al,[ebp+7]
	mov		ebx,[edi + eax*4]

	lea		ecx,[ecx*2 + ebx]

	cmp		[_vrambufferR + esi],cl
	je		nextR

	mov		[_vrambufferR + esi],cl

	mov		al,8
	out		dx,al

	mov		[0xA0000 + esi],cl

nextR:
	cmp		[_vrambufferG + esi],ch
	je		nextG

	mov		[_vrambufferG + esi],ch

	mov		al,4
	out		dx,al

	mov		[0xA0000 + esi],ch

nextG:

	shr		ecx,16
	
	cmp		[_vrambufferB + esi],cl
	je		nextB

	mov		[_vrambufferB + esi],cl

	mov		al,2
	out		dx,al

	mov		[0xA0000 + esi],cl

nextB:
	cmp		[_vrambufferI + esi],ch
	je		nextI

	mov		[_vrambufferI + esi],ch

	mov		al,1
	out		dx,al

	mov		[0xA0000 + esi],ch

nextI:

	inc		esi
	add		ebp,8
	cmp		esi,8000
	jne		start

	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
%endif
