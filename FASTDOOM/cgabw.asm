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

extern _backbuffer
extern _ptrlutcolors

BEGIN_DATA_SECTION

_vrambuffer: times 16384 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp
	mov		esi,0b8000H
	mov		ebx,_vrambuffer
	mov		edx,_backbuffer
	mov		ebp,[_ptrlutcolors]
L$6:
	mov		edi,50H
L$7:
	xor		eax,eax
	mov		al,[edx]
	xor		ecx,ecx
	mov		ax,[ebp+eax*2]
	mov		cl,[edx+1]
	shr		ax,14
	mov		cx,[ebp+ecx*2]
	shld	ax,cx,2
	xor		ecx,ecx
	mov		cl,[edx+2]
	mov		cx,[ebp+ecx*2]
	shld	ax,cx,2
	xor		ecx,ecx
	mov		cl,[edx+3]
	mov		cx,[ebp+ecx*2]
	shld	ax,cx,2
	cmp		al,[ebx]
	je		L$8
	mov		[ebx],al
	mov		[esi],al
L$8:
	xor		eax,eax
	mov		al,[edx+320]
	xor		ecx,ecx
	mov		ax,[ebp+eax*2]
	shr		ax,14
	mov		cl,[edx+321]
	mov		cx,[ebp+ecx*2]
	shld	ax,cx,2
	xor		ecx,ecx
	mov		cl,[edx+322]
	mov		cx,[ebp+ecx*2]
	shld	ax,cx,2
	xor		ecx,ecx
	mov		cl,[edx+323]
	mov		cx,[ebp+ecx*2]
	shld	ax,cx,2
	cmp		al,[ebx + 0x2000]
	je		L$9
	mov		[ebx + 0x2000],al
	mov		[esi + 0x2000],al
L$9:
	inc		esi
	add		edx,4
	inc		ebx
	dec		edi
	ja		L$7
	add		edx,140H
	cmp		esi,0b9f40H
	jb		L$6
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
