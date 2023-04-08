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

%ifdef MODE_PCP

extern _backbuffer
extern _ptrlut16colors

BEGIN_DATA_SECTION

_vrambuffer: times 32768 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

	xor 	ebx,ebx

	mov		esi,_backbuffer
	mov		ebp,[_ptrlut16colors]

	xor		edx,edx

L$2:
	mov		edi, 0x50
L$3:
	mov 	dl,[esi]
	mov		ax,[ebp+edx*2]

	and		eax,0c0c0H

	mov		dl,[esi+1]
	mov		cx,[ebp+edx*2]

	and		ecx,3030H

	or		eax,ecx

	mov		dl,[esi+2]
	mov		cx,[ebp+edx*2]

	and		ecx,0c0cH

	or		eax,ecx

	mov		dl,[esi+3]
	mov		cx,[ebp+edx*2]

	and		ecx,303H

	or		eax,ecx

	cmp		al,[_vrambuffer + ebx]
	je		L$4
	mov		[0xB8000 + ebx],al
	mov		[_vrambuffer + ebx],al
L$4:
	cmp		ah,[_vrambuffer + ebx + 0x4000]
	je		L$5
	mov		[0xB8000 + ebx + 0x4000],ah
	mov		[_vrambuffer + ebx + 0x4000],ah
L$5:
	movzx	eax,byte [esi+320]
	movzx	ecx,byte [esi+321]

	mov		ax,word [ebp+eax*2]
	mov		cx,word [ebp+ecx*2]

	and		eax,0c0c0H
	and		ecx,3030H

	or		eax,ecx

	movzx	ecx,byte [esi+322]
	mov		cx,word [ebp+ecx*2]

	and		ecx,0c0cH

	or		eax,ecx

	movzx	ecx,byte [esi+323]
	mov		cx,word [ebp+ecx*2]

	and		ecx,303H

	or		eax,ecx

	cmp		al,[_vrambuffer + ebx + 0x2000]
	je		L$6
	mov		[0xB8000 + ebx + 0x2000],al
	mov		[_vrambuffer + ebx + 0x2000],al
L$6:
	cmp		ah,[_vrambuffer + ebx + 0x6000]
	je		L$7
	mov		[0xB8000 + ebx + 0x6000],ah
	mov		[_vrambuffer + ebx + 0x6000],ah
L$7:
	inc		ebx
	add		esi,4
	dec		edi
	ja		L$3
	add		esi,140H
	cmp		esi,_backbuffer + 0x0FA00
	jb		L$2
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
