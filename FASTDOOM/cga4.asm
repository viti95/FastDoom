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
extern _lut4colors

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
	sub		esp,8
	mov		esi,0b8000H
	mov		ecx,_vrambuffer
	xor		edx,edx
	mov		edi,_lut4colors
L$2:
	xor		eax,eax
	mov		dword  [esp],eax
L$3:
	xor		eax,eax
	xor		ebx,ebx
	
	mov		al,[_backbuffer + edx]	
	mov		bl,[_backbuffer + edx + 1]
	mov		ah,[edi+eax]
	mov		al,[edi+ebx]
	and		eax,0c030H

	movzx	ebp,byte [_backbuffer + edx + 2]
	mov		bh,[edi+ebp]
	movzx	ebp,byte [_backbuffer + edx + 3]
	mov		bl,[edi+ebp]
	and		ebx,0c03H

	or		eax,ebx
	cmp		ax,word  [ecx]
	je		L$4
	mov		word  [ecx],ax
	or		al,ah
	mov		byte  [esi],al

L$4:
	xor		eax,eax
	xor		ebx,ebx
	
	mov		al,[_backbuffer + edx + 320]	
	mov		bl,[_backbuffer + edx + 321]
	mov		ah,[edi+eax]
	mov		al,[edi+ebx]
	and		eax,0c030H

	movzx	ebp,byte [_backbuffer + edx + 322]
	mov		bh,[edi+ebp]
	movzx	ebp,byte [_backbuffer + edx + 323]
	mov		bl,[edi+ebp]
	and		ebx,0c03H

	or		eax,ebx
	cmp		ax,word  4000H[ecx]
	je		L$5
	mov		word  4000H[ecx],ax
	or		al,ah
	mov		byte  2000H[esi],al
L$5:
	inc		dword  [esp]
	add		edx,4
	inc		esi
	add		ecx,2
	cmp		dword  [esp],50H
	jl		L$3
	add		edx,140H
	cmp		edx,0fa00H
	jb		L$2
	add		esp,8
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
