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
extern _lutcolors

BEGIN_DATA_SECTION

_vrambuffer: times 16384 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_SetPalette
	shl		eax,8
	add		eax,(_lutcolors+0xff)
	mov		[_ptrlutcolors],eax
	ret

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		ebp
	push		esi
	push		edx
	push		edi
	mov		ebp,_backbuffer
	mov		eax,[_ptrlutcolors]
	mov		edx,eax
	xor 	esi,esi
L$6:
	mov		edi,50H
L$7:
	mov		al,[ebp]
	mov		dl,[ebp+1]
	mov		bh,[eax]
	mov		ch,[edx]
	mov		al,[ebp+2]
	mov		dl,[ebp+3]
	mov		bl,[eax]
	mov		cl,[edx]
	and		bx, 0xC00C
	and 	cx, 0x3003
	or		ebx,ecx
	or		bl,bh
	cmp		[_vrambuffer + esi],bl
	je		L$8
	mov		[_vrambuffer + esi],bl
	mov		[0xB8000 + esi],bl
L$8:
	mov		al,[ebp+320]
	mov		dl,[ebp+321]
	mov		bh,[eax]
	mov		ch,[edx]
	mov		al,[ebp+322]
	mov		dl,[ebp+323]
	mov		bl,[eax]
	mov		cl,[edx]
	and		bx, 0xC00C
	and 	cx, 0x3003
	or		ebx,ecx
	or		bl,bh
	cmp		[_vrambuffer + esi + 0x2000],bl
	je		L$9
	mov		[_vrambuffer + esi + 0x2000],bl
	mov		[0xB8000 + esi + 0x2000],bl
L$9:
	inc		esi
	add		ebp,4
	dec		edi
	ja		L$7
	add		ebp,140H
	cmp		esi,0x1F40
	jb		L$6
	pop		edi
	pop		edx
	pop		esi
	pop		ebp
	pop		ecx
	pop		ebx
	ret

%endif
