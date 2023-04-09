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

_vrambuffer: times 16384 dw 0

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
	movzx		eax,byte [edx]
	add		eax,eax
	lea		ecx,[eax+ebp]
	mov		ax,word [ecx]
	movzx		ecx,byte 1[edx]
	mov		cx,word [ebp+ecx*2]
	and		eax,8040H
	and		ecx,2010H
	or		eax,ecx
	movzx		ecx,byte 2[edx]
	mov		cx,word [ebp+ecx*2]
	and		ecx,804H
	or		eax,ecx
	movzx		ecx,byte 3[edx]
	mov		cx,word [ebp+ecx*2]
	and		ecx,201H
	or		eax,ecx
	cmp		ax,word [ebx]
	jne		L$10
L$10:
	mov		word [ebx],ax
	or		al,ah
	mov		byte [esi],al
L$8:
	movzx		ecx,byte 140H[edx]
	add		ecx,ecx
	mov		ax,word [ecx+ebp]
	movzx		ecx,byte 141H[edx]
	mov		cx,word [ebp+ecx*2]
	and		eax,8040H
	and		ecx,2010H
	or		eax,ecx
	movzx		ecx,byte 142H[edx]
	mov		cx,word [ebp+ecx*2]
	and		ecx,804H
	or		eax,ecx
	movzx		ecx,byte 143H[edx]
	mov		cx,word [ebp+ecx*2]
	and		ecx,201H
	or		eax,ecx
	cmp		ax,word 4000H[ebx]
	je		L$9
	mov		word 4000H[ebx],ax
	or		al,ah
	mov		byte 2000H[esi],al
L$9:
	inc		esi
	add		edx,4
	add		ebx,2
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
