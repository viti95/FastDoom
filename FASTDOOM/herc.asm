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
	xor		ebx,ebx
	mov		ebp,_backbuffer
	xor		eax,eax
.L$12:
	mov		esi, 0x50
.L$13:

	mov		al,[ebp]
	mov		dx,[edi+eax*2]

	mov		al,[ebp+1]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	mov		al,[ebp+2]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	mov		al,[ebp+3]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	cmp		[_vrambuffer + ebx],dh
	je		.L$14
	mov		[0xB0000 + ebx],dh
	mov		[_vrambuffer + ebx],dh
.L$14:
	cmp		[_vrambuffer + ebx + 0x2000],dl
	je		.L$15
	mov		[0xB0000 + ebx + 0x2000],dl
	mov		[_vrambuffer + ebx + 0x2000],dl
.L$15:

	mov		al,[ebp+320]
	mov		dx,[edi+eax*2]

	mov		al,[ebp+321]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	mov		al,[ebp+322]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	mov		al,[ebp+323]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	cmp		[_vrambuffer + ebx + 0x4000],dh
	je		.L$16
	mov		[0xB0000 + ebx + 0x4000],dh
	mov		[_vrambuffer + ebx + 0x4000],dh
.L$16:
	cmp		[_vrambuffer + ebx + 0x6000],dl
	je		.L$17
	mov		[0xB0000 + ebx + 0x6000],dl
	mov		[_vrambuffer + ebx + 0x6000],dl
.L$17:
	inc		ebx
	add		ebp,4
	dec		esi
	ja		.L$13
	add		ebp,320
	cmp		bx,0x1F40
	jb		.L$12
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
