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

extern _bufferR
extern _bufferG
extern _bufferB
extern _bufferI

extern _vrambufferR1
extern _vrambufferG1
extern _vrambufferB1
extern _vrambufferI1

extern _vrambufferR2
extern _vrambufferG2
extern _vrambufferB2
extern _vrambufferI2

BEGIN_CODE_SECTION

CODE_SYM_DEF I_FinishUpdate
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp
	sub		esp,0x18
	cmp		dword [_destscreen],0xA0000
	jne		L$11
	mov		dword [esp],_vrambufferR1
	mov		dword 0x0C[esp],_vrambufferG1
	mov		dword 0x10[esp],_vrambufferB1
	mov		dword 4[esp],_vrambufferI1
L$1:
	mov		eax,_backbuffer
	xor		esi,esi
	nop
L$2:
	mov		edi,[_ptrlutcolors]
	movzx		edx,byte [eax]
	mov		dl,byte [edx+edi]
	movzx		ebx,byte 1[eax]
	mov		byte 0x14[esp],dl
	movzx		edx,byte 3[eax]
	mov		bh,byte [edi+ebx]
	mov		bl,byte [edx+edi]
	mov		dl,bh
	and		dl,0xC0
	movzx		ecx,byte 2[eax]
	movzx		edx,dl
	mov		cl,byte [ecx+edi]
	mov		edi,edx
	mov		dl,byte 0x14[esp]
	and		dl,0xC0
	sar		edi,2
	movzx		edx,dl
	or		edi,edx
	mov		dl,cl
	and		dl,0xC0
	movzx		edx,dl
	sar		edx,4
	or		edi,edx
	mov		dl,bl
	and		dl,0xC0
	movzx		edx,dl
	sar		edx,6
	or		edx,edi
	mov		ch,dl
	movzx		edx,si
	mov		byte _bufferR[edx],ch
	mov		ch,byte 0x14[esp]
	and		ch,0x30
	movzx		edi,ch
	mov		ch,bh
	and		ch,0x30
	shl		edi,2
	movzx		ebp,ch
	mov		ch,cl
	or		ebp,edi
	and		ch,0x30
	movzx		edi,ch
	mov		ch,bl
	sar		edi,2
	and		ch,0x30
	or		ebp,edi
	movzx		edi,ch
	sar		edi,4
	or		ebp,edi
	mov		dword 8[esp],ebp
	mov		ch,byte 8[esp]
	mov		byte _bufferG[edx],ch
	mov		ch,byte 0x14[esp]
	and		ch,0xC
	movzx		edi,ch
	mov		ch,bh
	and		ch,0xC
	shl		edi,4
	movzx		ebp,ch
	mov		ch,cl
	shl		ebp,2
	and		ch,0xC
	or		ebp,edi
	movzx		edi,ch
	mov		ch,bl
	and		ch,0xC
	or		edi,ebp
	movzx		ebp,ch
	sar		ebp,2
	or		ebp,edi
	mov		dword 8[esp],ebp
	mov		ch,byte 8[esp]
	mov		byte _bufferB[edx],ch
	mov		ch,byte 0x14[esp]
	and		ch,3
	and		bh,3
	shl		ch,6
	shl		bh,4
	or		ch,bh
	mov		bh,cl
	and		bh,3
	shl		bh,2
	or		bh,ch
	add		eax,4
	and		bl,3
	inc		esi
	or		bl,bh
	mov		byte _bufferI[edx],bl
	cmp		si,3e80H
	jb		L$2
	mov		al,8
	mov		edx,3c5H
	out		dx,al
	xor		ebx,ebx
	lea		eax,[eax]
L$3:
	mov		edx,dword [esp]
	movzx		eax,bx
	add		edx,eax
	mov		cl,byte _bufferR[eax]
	cmp		cl,byte [edx]
	je		L$4
	mov		byte [edx],cl
	mov		edx,dword [_destscreen]
	mov		byte [edx+eax],cl
L$4:
	inc		ebx
	cmp		bx,3e80H
	jb		L$3
	mov		al,4
	mov		edx,3c5H
	out		dx,al
	xor		ebx,ebx
	nop
L$5:
	mov		edx,dword 0xC[esp]
	movzx		eax,bx
	add		edx,eax
	mov		cl,byte _bufferG[eax]
	cmp		cl,byte [edx]
	je		L$6
	mov		byte [edx],cl
	mov		edx,dword [_destscreen]
	mov		byte [edx+eax],cl
L$6:
	inc		ebx
	cmp		bx,3e80H
	jb		L$5
	mov		al,2
	mov		edx,3c5H
	out		dx,al
	xor		ebx,ebx
L$7:
	mov		edx,dword 0x10[esp]
	movzx		eax,bx
	add		edx,eax
	mov		cl,byte _bufferB[eax]
	cmp		cl,byte [edx]
	je		L$8
	mov		byte [edx],cl
	mov		edx,dword [_destscreen]
	mov		byte [edx+eax],cl
L$8:
	inc		ebx
	cmp		bx,3e80H
	jb		L$7
	mov		al,1
	mov		edx,3c5H
	out		dx,al
	xor		ebx,ebx
L$9:
	mov		edx,dword 4[esp]
	movzx		eax,bx
	add		edx,eax
	mov		cl,byte _bufferI[eax]
	cmp		cl,byte [edx]
	je		L$10
	mov		byte [edx],cl
	mov		edx,dword [_destscreen]
	mov		byte [edx+eax],cl
L$10:
	inc		ebx
	cmp		bx,0x3E80
	jb		L$9
	mov		eax,dword [_destscreen]
	and		eax,0xFF00
	mov		edx,0x3D4
	add		eax,0xC
	out		dx,ax
	cmp		dword [_destscreen],0xA4000
	jne		L$12
	mov		dword [_destscreen],0xA0000
	add		esp,0x18
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
L$11:
	mov		dword [esp],_vrambufferR2
	mov		dword 0xC[esp],_vrambufferG2
	mov		dword 0x10[esp],_vrambufferB2
	mov		dword 4[esp],_vrambufferI2
	jmp		L$1
L$12:
	add		dword [_destscreen],0x4000
	add		esp,0x18
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
%endif
