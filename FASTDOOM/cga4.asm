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

BEGIN_DATA_SECTION

_vrambuffer: times 16384 dw 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp
	xor		esi,esi
	xor		ecx,ecx
	mov		edi,[_ptrlut4colors]
	mov 	edx,_backbuffer
L$2:
	xor		ebp,ebp
L$3:
	xor		eax,eax
	xor		ebx,ebx
	
	mov		al,[edx]	
	mov		bl,[edx+1]
	mov		ah,[edi+eax]
	mov		al,[edi+ebx]

	mov		bl,[edx+2]
	mov		cl,[edx+3]
	mov		bh,[edi+ebx]
	mov		bl,[edi+ecx]
	and		eax,0c030H
	and		ebx,0c03H

	or		eax,ebx
	cmp		ax,[_vrambuffer + esi*2]
	je		L$4
	mov		[_vrambuffer + esi*2],ax
	or		al,ah
	mov		[0xB8000 + esi],al

L$4:
	xor		eax,eax
	xor		ebx,ebx
	
	mov		al,[edx+320]	
	mov		bl,[edx+321]
	mov		ah,[edi+eax]
	mov		al,[edi+ebx]

	mov		bl,[edx+322]
	mov		cl,[edx+323]
	mov		bh,[edi+ebx]
	mov		bl,[edi+ecx]
	and		eax,0c030H
	and		ebx,0c03H

	or		eax,ebx
	cmp		ax,[_vrambuffer + esi*2 + 0x4000]
	je		L$5
	mov		[_vrambuffer + esi*2 + 0x4000],ax
	or		al,ah
	mov		[0xBA000 + esi],al
L$5:
	inc		ebp
	add		edx,4
	inc		esi
	cmp		ebp,50H
	jl		L$3
	add		edx,140H
	cmp		edx,_backbuffer + 0xFA00
	jb		L$2
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
