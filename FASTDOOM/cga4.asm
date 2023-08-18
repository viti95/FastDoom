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
	mov 	edi,_backbuffer
	mov		ebx,edx

	mov		esi,_vrambuffer
	mov		ebp,0xB8000

	mov   [patchESP+2],esp

L$2:
	sub		esp,20
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

	cmp		[esi],ah
	je		L$10
	mov		[esi],ah
	mov		[ebp],ah

L$10:
	cmp		[esi + 1],al
	je		L$4
	mov		[esi + 1],al
	mov		[ebp + 1],al

L$4:
	bswap 	eax

	cmp		[esi + 2],ah
	je		L$11
	mov		[esi + 2],ah
	mov		[ebp + 2],ah

L$11:
	cmp		[esi + 3],al
	je		L$5
	mov		[esi + 3],al
	mov		[ebp + 3],al

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
	add		esi,0x2000
	mov		cl,[ebx]
	add		ebp,0x2000
	lea		eax,[eax*4 + ecx]

	sub		edi,304

	cmp		[esi],ah
	je		L$20
	mov		[esi],ah
	mov		[ebp],ah

L$20:
	cmp		[esi + 1],al
	je		L$24
	mov		[esi + 1],al
	mov		[ebp + 1],al

L$24:
	bswap 	eax

	inc		esp

	cmp		[esi + 2],ah
	je		L$21
	mov		[esi + 2],ah
	mov		[ebp + 2],ah

L$21:
	cmp		[esi + 3],al
	je		L$25
	mov		[esi + 3],al
	mov		[ebp + 3],al

L$25:

	sub		esi,0x1FFC
	sub		ebp,0x1FFC

patchESP:
  	cmp		esp, 0x12345678

	jne		L$3
	add		edi,320
	cmp		bp,0x9F40
	jb		L$2
	pop		ebp
	pop		edx
	pop		esi
	pop		edi
	pop		ecx
	pop		ebx
	ret

%endif
