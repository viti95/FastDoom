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

%ifdef MODE_HERC

extern _backbuffer
extern _ptrlutcolors

BEGIN_DATA_SECTION

_vrambuffer: times 32768 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp
	mov		edi,[_ptrlutcolors]
	xor 	ebx,ebx
	mov		ebp,_backbuffer
	xor		esi,esi
	xor		eax,eax
L$12:
	mov		esi, 0x50
L$13:
	mov		al,[ebp]
	mov		edx,[edi+eax*4]
	and		edx,0x80408040

	mov		al,[ebp+1]
	mov		ecx,[edi+eax*4]
	and		ecx,0x20102010

	or		edx,ecx

	mov		al,[ebp+2]
	mov		ecx,[edi+eax*4]
	and		ecx,0x8040804

	or		edx,ecx

	mov		al,[ebp+3]
	mov		ecx,[edi+eax*4]
	and		ecx,0x2010201

	or		edx,ecx

	or		dl, dh

	cmp		dl,[_vrambuffer + ebx]
	je		L$14
	mov		[0xB0000 + ebx],dl
	mov		[_vrambuffer + ebx],dl
L$14:
	shr		edx,16
	or		dl,dh

	cmp		dl,[_vrambuffer + ebx + 0x2000]
	je		L$15
	mov		[0xB0000 + ebx + 0x2000],dl
	mov		[_vrambuffer + ebx + 0x2000],dl
L$15:
	mov		al,[ebp+320]
	mov		edx,[edi+eax*4]
	and		edx,0x80408040

	mov		al,[ebp+321]
	mov		ecx,[edi+eax*4]
	and		ecx,0x20102010

	or		edx,ecx

	mov		al,[ebp+322]
	mov		ecx,[edi+eax*4]
	and		ecx,0x8040804

	or		edx,ecx

	mov		al,[ebp+323]
	mov		ecx,[edi+eax*4]
	and		ecx,0x2010201

	or		edx,ecx

	or		dl, dh

	cmp		dl,[_vrambuffer + ebx + 0x4000]
	je		L$16
	mov		[0xB0000 + ebx + 0x4000],dl
	mov		[_vrambuffer + ebx + 0x4000],dl
L$16:
	shr		edx,16
	or		dl,dh

	cmp		dl,[_vrambuffer + ebx + 0x6000]
	je		L$17
	mov		[0xB0000 + ebx + 0x6000],dl
	mov		[_vrambuffer + ebx + 0x6000],dl
L$17:
	add		ebp,4
	inc		ebx
	dec		esi
	ja		L$13
	add		ebp,140H
	cmp		ebx,0x1F40
	jb		L$12
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
