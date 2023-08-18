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
	mov 	edi,_backbuffer
	mov		ebx,edx

L$2:
	mov		ebp,20
L$3:
	mov		dl,[edi+8]
	mov		bl,[edi+9]
	mov		ah,[edx]
	mov		ch,[ebx]
	mov		dl,[edi+12]
	mov		bl,[edi+13]
	mov		al,[edx]
	mov		cl,[ebx]
	bswap 	eax
	bswap 	ecx
	mov		dl,[edi+0]
	mov		bl,[edi+1]
	mov		ah,[edx]
	mov		ch,[ebx]
	mov		dl,[edi+4]
	mov		bl,[edi+5]
	mov		al,[edx]
	mov		cl,[ebx]
	mov		dl,[edi+10]
	lea		eax,[eax*4 + ecx]
	mov		bl,[edi+14]
	mov		ch,[edx]
	mov		dl,[edi+2]
	mov		cl,[ebx]
	mov		bl,[edi+6]
	bswap 	ecx
	mov		ch,[edx]
	mov		dl,[edi+11]
	mov		cl,[ebx]
	mov		bl,[edi+15]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		dl,[edi+3]
	mov		cl,[ebx]
	mov		bl,[edi+7]
	bswap 	ecx
	mov		ch,[edx]
	mov		cl,[ebx]
	lea		eax,[eax*4 + ecx]

	cmp		[_vrambuffer + esi],ah
	je		L$10
	mov		[_vrambuffer + esi],ah
	mov		[0xB8000 + esi],ah

L$10:
	cmp		[_vrambuffer + esi + 1],al
	je		L$4
	mov		[_vrambuffer + esi + 1],al
	mov		[0xB8000 + esi + 1],al

L$4:
	shr 	eax,16

	cmp		[_vrambuffer + esi + 2],ah
	je		L$11
	mov		[_vrambuffer + esi + 2],ah
	mov		[0xB8000 + esi + 2],ah

L$11:
	cmp		[_vrambuffer + esi + 3],al
	je		L$5
	mov		[_vrambuffer + esi + 3],al
	mov		[0xB8000 + esi + 3],al

L$5:
	add		edi,320

	mov		dl,[edi+8]
	mov		bl,[edi+9]
	mov		ah,[edx]
	mov		ch,[ebx]
	mov		dl,[edi+12]
	mov		bl,[edi+13]
	mov		al,[edx]
	mov		cl,[ebx]
	bswap 	eax
	bswap 	ecx
	mov		dl,[edi+0]
	mov		bl,[edi+1]
	mov		ah,[edx]
	mov		ch,[ebx]
	mov		dl,[edi+4]
	mov		bl,[edi+5]
	mov		al,[edx]
	mov		cl,[ebx]
	mov		dl,[edi+10]
	lea		eax,[eax*4 + ecx]
	mov		bl,[edi+14]
	mov		ch,[edx]
	mov		dl,[edi+2]
	mov		cl,[ebx]
	mov		bl,[edi+6]
	bswap 	ecx
	mov		ch,[edx]
	mov		dl,[edi+11]
	mov		cl,[ebx]
	mov		bl,[edi+15]
	lea		eax,[eax*4 + ecx]
	mov		ch,[edx]
	mov		dl,[edi+3]
	mov		cl,[ebx]
	mov		bl,[edi+7]
	bswap 	ecx
	mov		ch,[edx]
	mov		cl,[ebx]
	lea		eax,[eax*4 + ecx]

	cmp		[_vrambuffer + esi + 0x2000],ah
	je		L$20
	mov		[_vrambuffer + esi + 0x2000],ah
	mov		[0xB8000 + esi + 0x2000],ah

L$20:
	cmp		[_vrambuffer + esi + 0x2000 + 1],al
	je		L$24
	mov		[_vrambuffer + esi + 0x2000 + 1],al
	mov		[0xB8000 + esi + 0x2000 + 1],al

L$24:
	shr 	eax,16

	cmp		[_vrambuffer + esi + 0x2000 + 2],ah
	je		L$21
	mov		[_vrambuffer + esi + 0x2000 + 2],ah
	mov		[0xB8000 + esi + 0x2000 + 2],ah

L$21:
	cmp		[_vrambuffer + esi + 0x2000 + 3],al
	je		L$25
	mov		[_vrambuffer + esi + 0x2000 + 3],al
	mov		[0xB8000 + esi + 0x2000 + 3],al

L$25:
	add		esi,4
	sub		edi,304
	dec		ebp
	ja		L$3
	add		edi,140H
	cmp		si,0x1F40
	jb		L$2
	pop		ebp
	pop		edx
	pop		esi
	pop		edi
	pop		ecx
	pop		ebx
	ret

%endif
