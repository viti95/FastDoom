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

%ifdef MODE_CGA_AFH

extern _backbuffer
extern _ptrlut16colors
extern _lut16colors
extern _ansifromhellLUT
extern _vrambuffer

BEGIN_CODE_SECTION

CODE_SYM_DEF I_SetPalette
	shl		eax,8
	add		eax,(_lut16colors+0xff)
	mov		[_ptrlut16colors],eax
	ret

CODE_SYM_DEF CGA_AFH_DrawBackbuffer_Snow
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	
	mov		eax,[_ptrlut16colors]
	mov		edi,_backbuffer
	xor		esi,esi
	xor		ebx,ebx
	xor		ecx,ecx
	mov		ebp,80
	mov		dx,0x3DA

.START:
	mov		al,[edi]
	mov		cl,[eax]
	mov		al,[edi+1]
	mov		bl,[eax]

	mov		al,[edi+2]
	mov		ch,[eax]
	mov		al,[edi+3]
	mov		bh,[eax]

	and		cx,0x0F0F
	and		bx,0xF0F0
	or		ebx,ecx

	cmp		[_vrambuffer + esi],bx
	je 		.NEXT

	mov		cx,[_ansifromhellLUT + ebx*2]
	mov		[_vrambuffer + esi],bx

	;I_WaitCGA();
.WDN:
	in		al,dx
	test	al,1
	jz		.WDN

	mov		[0xB8000 + esi],cx
	
.NEXT:
	add		esi,2
	add		edi,4
	dec		ebp
	jnz		.START
	mov		bp,80
	add		edi,320
	cmp		si,16000
	jne		.START
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

CODE_SYM_DEF CGA_AFH_DrawBackbuffer
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	
	mov		eax,[_ptrlut16colors]
	mov		edi,_backbuffer
	mov		edx,eax
	xor		esi,esi
	xor		ebx,ebx
	xor		ecx,ecx
	mov		ebp,80

.START:
	mov		al,[edi]
	mov		dl,[edi+1]
	
	mov		cl,[eax]
	mov		bl,[edx]

	mov		al,[edi+2]
	mov		dl,[edi+3]

	mov		ch,[eax]
	mov		bh,[edx]

	and		cx,0x0F0F
	and		bx,0xF0F0
	or		ebx,ecx

	cmp		[_vrambuffer + esi],bx
	je 		.NEXT
	mov		cx,[_ansifromhellLUT + ebx*2]
	mov		[_vrambuffer + esi],bx
	mov		[0xB8000 + esi],cx
	
.NEXT:
	add		esi,2
	add		edi,4
	dec		ebp
	jnz		.START
	mov		bp,80
	add		edi,320
	cmp		si,16000
	jne		.START
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
