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

BEGIN_DATA_SECTION

_vrambuffer: times 16384 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	push	ebp
	sub		esp,0x14

	xor		edx,edx
	xor		ebx,ebx
L$2:
	xor		ch,ch
L$3:
	movzx	esi,bx
	mov		ebp,[_ptrlut16colors]
	movzx	edi,byte _backbuffer[esi]
	mov		cl,[edi+ebp]
	movzx	esi,byte _backbuffer+0x2[esi]
	mov		0x10[esp],cl
	mov		cl,[esi+ebp]
	mov		0x8[esp],cl
	mov		cl,0x10[esp]
	shl		cl,0x4
	or		cl,0x8[esp]
	cmp		cl,[_vrambuffer+edx]
	je		L$4
L$7:
	mov		[0xB8000+edx],cl
	mov		[_vrambuffer+edx],cl
L$4:
	inc		ch
	add		ebx,0x4
	inc		edx
	cmp		ch,0x50
	jb		L$3
	add		edx,0x1FB0
	xor		ch,ch
L$5:
	movzx	esi,bx
	movzx	edi,byte _backbuffer[esi]
	mov		[esp],edi
	mov		ebp,[esp]
	mov		edi,[_ptrlut16colors]
	add		ebp,edi
	mov		cl,[ebp]
	movzx	esi,byte _backbuffer+0x2[esi]
	mov		0xc[esp],cl
	mov		cl,[edi+esi]
	mov		0x4[esp],cl
	mov		cl,0xc[esp]
	shl		cl,0x4
	or		cl,0x4[esp]
	cmp		cl,[_vrambuffer+edx]
	je		L$6
	mov		[0xB8000+edx],cl
	mov		[_vrambuffer+edx],cl
L$6:
	inc		ch
	add		ebx,0x4
	inc		edx
	cmp		ch,0x50
	jb		L$5
	sub		edx,0x2000
	cmp		bx,0xFA00
	jb		L$2
	add		esp,0x14
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
