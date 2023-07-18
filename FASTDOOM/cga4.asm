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
extern _lut4colors
extern _numpalette

BEGIN_DATA_SECTION

_vrambuffer: times 16384 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_SetPalette
	shl		eax,8
	add		eax,(_lut4colors+0xff)
	mov		[_ptrlut4colors],eax
	ret

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edi
	push		esi
	push		edx
	push		ebp

	mov		edx,[_ptrlut4colors]
	xor		esi,esi
	xor		eax,eax
	xor		ebx,ebx
	mov 	edi,_backbuffer
	mov		ecx,edx
	
L$2:
	mov		ebp,0x50
L$3:
	mov		dl,[edi]	
	mov		cl,[edi+1]
	mov		ah,[edx]
	mov		al,[ecx]

	mov		dl,[edi+2]
	mov		cl,[edi+3]
	mov		bh,[edx]
	mov		bl,[ecx]
	
	and		eax,0c030H
	and		ebx,0c03H

	or		eax,ebx
	or		al,ah
	cmp		[_vrambuffer + esi],al
	je		L$4
	mov		[_vrambuffer + esi],al
	
	mov		[0xB8000 + esi],al

L$4:
	mov		dl,[edi+320]	
	mov		cl,[edi+321]
	mov		ah,[edx]
	mov		al,[ecx]

	mov		dl,[edi+322]
	mov		cl,[edi+323]
	mov		bh,[edx]
	mov		bl,[ecx]
	
	and		eax,0c030H
	and		ebx,0c03H

	or		eax,ebx
	or		al,ah
	cmp		[_vrambuffer + esi + 0x2000],al
	je		L$5
	mov		[_vrambuffer + esi + 0x2000],al
	
	mov		[0xBA000 + esi],al
L$5:
	inc		esi
	add		edi,4
	dec		ebp
	ja		L$3
	add		edi,140H
	cmp		edi,_backbuffer + 0xFA00
	jb		L$2
	pop		ebp
	pop		edx
	pop		esi
	pop		edi
	pop		ecx
	pop		ebx
	ret

%endif
