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

%ifdef MODE_EGA640

extern _backbuffer
extern _ptrlutcolors
extern _lutcolors
extern _destscreen

extern _buffer
extern _vrambuffer1
extern _vrambuffer2

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp

	xor eax,eax
	mov ebp, _backbuffer						; PTR backbuffer
	mov edi, [_ptrlutcolors]					; PTR lutcolors
	xor esi, esi								; Counter

LoopBackbuffer:

	; ---------------------------------
	; |Pixel 0|Pixel 1|Pixel 2|Pixel 3|
	; ---------------------------------
	; 32 bit register, each pixel block has two colors
	;
	; ---------------------
	; |R1R2|G1G2|B1B2|I1I2|
	; ---------------------
	; 8 bit LUT entry

	; Backbuffer pixel 1
	mov al, [ebp]
	mov bh, [edi + eax]

	; Backbuffer pixel 2
	mov al, [ebp + 1]
	mov ch, [edi + eax]

	; Backbuffer pixel 3
	mov al, [ebp + 2]
	mov bl, [edi + eax]

	; Backbuffer pixel 4
	mov al, [ebp + 3]
	mov cl, [edi + eax]
	
	shl ebx, 16
	shl ecx, 16

	; Process red block
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	ror ebx,6
	shld edx, ecx, 2

	mov [_buffer + esi], dl

	; Process green block
	ror ecx,6
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	ror ebx,6
	shld edx, ecx, 2

	mov [_buffer + esi + 16000], dl

	; Process blue block
	ror ecx,6
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	ror ebx,6
	shld edx, ecx, 2

	mov [_buffer + esi + 32000], dl

	; Process intensity block
	ror ecx,6
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	shld edx, ecx, 2

	mov [_buffer + esi + 48000], dl

	; Loop
	add ebp, 4
	inc esi
	cmp si,0x3E80
	jb LoopBackbuffer

	mov edi, dword [_destscreen]

	; Select vrambuffer 1 or 2
	cmp edi,0xA0000
	jne	SecondVRAMBuffer
	mov ebx,_vrambuffer1
	jmp ProcessPlanes
SecondVRAMBuffer:
	mov ebx,_vrambuffer2

ProcessPlanes:
	mov	edx,0x3C5

	; Change to red plane
	mov al,8
	out	dx,al

	xor esi, esi
LoopRedPlane:
	mov al, [_buffer + esi]
	cmp al, [ebx + esi]
	je NextBlockRedPlane
	mov [ebx + esi], al
	mov [edi + esi], al
NextBlockRedPlane:
	inc	esi
	cmp	si,0x3E80
	jb LoopRedPlane

	; Change to green plane
	mov	al,4
	out	dx,al

	xor esi, esi
LoopGreenPlane:
	mov al, [_buffer + esi + 16000]
	cmp al, [ebx + esi + 16000]
	je NextBlockGreenPlane
	mov [ebx + esi + 16000], al
	mov [edi + esi], al
NextBlockGreenPlane:
	inc	esi
	cmp	si,0x3E80
	jb LoopGreenPlane

	; Change to blue plane
	mov	al,2
	out	dx,al

	xor esi, esi
LoopBluePlane:
	mov al, [_buffer + esi + 32000]
	cmp al, [ebx + esi + 32000]
	je NextBlockBluePlane
	mov [ebx + esi + 32000], al
	mov [edi + esi], al
NextBlockBluePlane:
	inc	esi
	cmp	si,0x3E80
	jb LoopBluePlane

	; Change to intensity plane
	mov	al,1
	out	dx,al

	xor esi, esi
LoopIntensityPlane:
	mov al, [_buffer + esi + 48000]
	cmp al, [ebx + esi + 48000]
	je NextBlockIntensityPlane
	mov [ebx + esi + 48000], al
	mov [edi + esi], al
NextBlockIntensityPlane:
	inc	esi
	cmp	si,0x3E80
	jb LoopIntensityPlane

	mov	eax,edi
	and	eax,0xFF00
	mov edx,0x3D4
	add	eax,0xC
	out	dx,ax
	cmp	edi,0xA4000
	jne	IncreaseDestScreen
	mov	dword [_destscreen],0xA0000
	jmp Finish
IncreaseDestScreen:
	add	dword [_destscreen],0x4000

Finish:
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

%endif
