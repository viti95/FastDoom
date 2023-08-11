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

%macro PIXEL 1
	mov		al,[ebp + (%1 * 4) + 0]
	mov		dx,[edi+eax*2]

	mov		al,[ebp + (%1 * 4) + 1]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	mov		al,[ebp + (%1 * 4) + 2]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	mov		al,[ebp + (%1 * 4) + 3]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	cmp		[_vrambuffer + ebx + %1],dh
	je		%%L$14
	mov		[0xB0000 + ebx + %1],dh
	mov		[_vrambuffer + ebx + %1],dh
%%L$14:
	cmp		[_vrambuffer + ebx + 0x2000 + %1],dl
	je		%%L$15
	mov		[0xB0000 + ebx + 0x2000 + %1],dl
	mov		[_vrambuffer + ebx + 0x2000 + %1],dl
%%L$15:

	mov		al,[ebp + (%1 * 4) + 320]
	mov		dx,[edi+eax*2]

	mov		al,[ebp + (%1 * 4) + 321]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	mov		al,[ebp + (%1 * 4) + 322]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	mov		al,[ebp + (%1 * 4) + 323]
	mov		cx,[edi+eax*2]

	lea		edx,[edx*4+ecx]

	cmp		[_vrambuffer + ebx + 0x4000 + %1],dh
	je		%%L$16
	mov		[0xB0000 + ebx + 0x4000 + %1],dh
	mov		[_vrambuffer + ebx + 0x4000 + %1],dh
%%L$16:
	cmp		[_vrambuffer + ebx + 0x6000 + %1],dl
	je		%%L$17
	mov		[0xB0000 + ebx + 0x6000 + %1],dl
	mov		[_vrambuffer + ebx + 0x6000 + %1],dl
%%L$17:
%endmacro

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

	%assign POSITION 0
	%rep 80
		PIXEL POSITION
	%assign POSITION POSITION+1
	%endrep

	add		ebx,80
	add		ebp,640
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
