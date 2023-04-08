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
	sub		esp,4
	mov		edi,[_ptrlutcolors]
	xor 	ebx,ebx
	mov		ebp,_backbuffer
	xor		esi,esi
L$12:
	mov		esi, 0x50
L$13:
	xor		eax,eax

	mov		al,byte [ebp]
	mov		ecx,[edi+eax*4]
	and		ecx,80408040H

	mov		edx,ecx

	mov		al,byte [ebp+1]
	mov		ecx,[edi+eax*4]
	and		ecx,20102010H

	or		edx,ecx

	mov		al,byte [ebp+2]
	mov		ecx,[edi+eax*4]
	and		ecx,8040804H

	or		edx,ecx

	mov		al,byte [ebp+3]
	mov		ecx,[edi+eax*4]
	and		ecx,2010201H

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
	movzx		ecx,byte [ebp+320]
	mov		ecx,[edi+ecx*4]
	and		ecx,80408040H
	mov		dword  [esp],ecx
	movzx		ecx,byte [ebp+321]
	mov		ecx,[edi+ecx*4]
	and		ecx,20102010H
	or		dword  [esp],ecx
	movzx		ecx,byte [ebp+322]
	mov		ecx,[edi+ecx*4]
	and		ecx,8040804H
	or		dword  [esp],ecx
	movzx		ecx,byte [ebp+323]
	mov		ecx,[edi+ecx*4]
	and		ecx,2010201H
	or		dword  [esp],ecx
	mov		cl,byte  [esp]
	or		cl,byte  1[esp]
	cmp		cl,[_vrambuffer + ebx + 0x4000]
	je		L$16
	mov		[0xB0000 + ebx + 0x4000],cl
	mov		[_vrambuffer + ebx + 0x4000],cl
L$16:
	mov		cl,byte  2[esp]
	or		cl,byte  3[esp]
	cmp		cl,[_vrambuffer + ebx + 0x6000]
	je		L$17
	mov		[0xB0000 + ebx + 0x6000],cl
	mov		[_vrambuffer + ebx + 0x6000],cl
L$17:
	dec 	esi
	add		ebp,4
	inc		ebx
	cmp		esi, 0
	ja		L$13
	add		ebp,140H
	cmp		ebx,0x1F40
	jb		L$12
	add		esp,4
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
