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
	mov al, [ebp + 4]
	mov bh, [edi + eax]

	; Backbuffer pixel 2
	mov al, [ebp + 5]
	mov ch, [edi + eax]

	; Backbuffer pixel 3
	mov al, [ebp + 6]
	mov bl, [edi + eax]

	; Backbuffer pixel 4
	mov al, [ebp + 7]
	mov cl, [edi + eax]
	
	shl ebx, 16
	shl ecx, 16

	; Backbuffer pixel 5
	mov al, [ebp]
	mov bh, [edi + eax]

	; Backbuffer pixel 6
	mov al, [ebp + 1]
	mov ch, [edi + eax]

	; Backbuffer pixel 7
	mov al, [ebp + 2]
	mov bl, [edi + eax]

	; Backbuffer pixel 8
	mov al, [ebp + 3]
	mov cl, [edi + eax]

	; Process red block
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	shld edx, ecx, 2

	mov [_buffer + esi], dx

	; Process green block
	rol ebx,10
	rol ecx,10
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	shld edx, ecx, 2

	mov [_buffer + esi + 16000], dx

	; Process blue block
	rol ebx,10
	rol ecx,10
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	shld edx, ecx, 2

	mov [_buffer + esi + 32000], dx

	; Process intensity block
	rol ebx,10
	rol ecx,10
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	rol ebx,8
	shld edx, ecx, 2
	rol ecx,8
	shld edx, ebx, 2
	shld edx, ecx, 2

	mov [_buffer + esi + 48000], dx

	; Loop
	add ebp, 8
	add esi, 2
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
	xor esi, esi
	out	dx,al

LoopRedPlane:
	mov eax, [_buffer + esi]
	cmp ah, [ebx + esi + 1]
	je NextBlockRedPlane1
	mov [ebx + esi + 1], ah
	mov [edi + esi + 1], ah
NextBlockRedPlane1:
	cmp al, [ebx + esi]
	je NextBlockRedPlane2
	mov [ebx + esi], al
	mov [edi + esi], al
NextBlockRedPlane2:
	shr eax, 16
	cmp ah, [ebx + esi + 3]
	je NextBlockRedPlane3
	mov [ebx + esi + 3], ah
	mov [edi + esi + 3], ah
NextBlockRedPlane3:
	cmp al, [ebx + esi + 2]
	je NextBlockRedPlane4
	mov [ebx + esi + 2], al
	mov [edi + esi + 2], al
NextBlockRedPlane4:
	add esi, 4
	cmp	si,0x3E80
	jb LoopRedPlane

	; Change to green plane
	mov	al,4
	xor esi, esi
	out	dx,al

LoopGreenPlane:
	mov eax, [_buffer + esi + 16000]
	cmp ah, [ebx + esi + 1 + 16000]
	je NextBlockGreenPlane1
	mov [ebx + esi + 1 + 16000], ah
	mov [edi + esi + 1], ah
NextBlockGreenPlane1:
	cmp al, [ebx + esi + 16000]
	je NextBlockGreenPlane2
	mov [ebx + esi + 16000], al
	mov [edi + esi], al
NextBlockGreenPlane2:
	shr eax, 16
	cmp ah, [ebx + esi + 3 + 16000]
	je NextBlockGreenPlane3
	mov [ebx + esi + 3 + 16000], ah
	mov [edi + esi + 3], ah
NextBlockGreenPlane3:
	cmp al, [ebx + esi + 2 + 16000]
	je NextBlockGreenPlane4
	mov [ebx + esi + 2 + 16000], al
	mov [edi + esi + 2], al
NextBlockGreenPlane4:
	add esi, 4
	cmp	si,0x3E80
	jb LoopGreenPlane

	; Change to blue plane
	mov	al,2
	xor esi, esi
	out	dx,al

LoopBluePlane:
	mov eax, [_buffer + esi + 32000]
	cmp ah, [ebx + esi + 1 + 32000]
	je NextBlockBluePlane1
	mov [ebx + esi + 1 + 32000], ah
	mov [edi + esi + 1], ah
NextBlockBluePlane1:
	cmp al, [ebx + esi + 32000]
	je NextBlockBluePlane2
	mov [ebx + esi + 32000], al
	mov [edi + esi], al
NextBlockBluePlane2:
	shr eax, 16
	cmp ah, [ebx + esi + 3 + 32000]
	je NextBlockBluePlane3
	mov [ebx + esi + 3 + 32000], ah
	mov [edi + esi + 3], ah
NextBlockBluePlane3:
	cmp al, [ebx + esi + 2 + 32000]
	je NextBlockBluePlane4
	mov [ebx + esi + 2 + 32000], al
	mov [edi + esi + 2], al
NextBlockBluePlane4:
	add esi, 4
	cmp	si,0x3E80
	jb LoopBluePlane

	; Change to intensity plane
	mov	al,1
	xor esi, esi
	out	dx,al

LoopIntensityPlane:
	mov eax, [_buffer + esi + 48000]
	cmp ah, [ebx + esi + 1 + 48000]
	je NextBlockIntensityPlane1
	mov [ebx + esi + 1 + 48000], ah
	mov [edi + esi + 1], ah
NextBlockIntensityPlane1:
	cmp al, [ebx + esi + 48000]
	je NextBlockIntensityPlane2
	mov [ebx + esi + 48000], al
	mov [edi + esi], al
NextBlockIntensityPlane2:
	shr eax, 16
	cmp ah, [ebx + esi + 3 + 48000]
	je NextBlockIntensityPlane3
	mov [ebx + esi + 3 + 48000], ah
	mov [edi + esi + 3], ah
NextBlockIntensityPlane3:
	cmp al, [ebx + esi + 2 + 48000]
	je NextBlockIntensityPlane4
	mov [ebx + esi + 2 + 48000], al
	mov [edi + esi + 2], al
NextBlockIntensityPlane4:
	add esi, 4
	cmp	si,0x3E80
	jb LoopIntensityPlane

	mov	eax,edi
	mov	al,0xC
	add edx,0x11
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
