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

%ifdef MODE_SIGMA

extern _backbuffer
extern _ptrlut16colors

BEGIN_DATA_SECTION

_vrambuffer_p2: times 32768 db 0
_vrambuffer_p3: times 32768 db 0

BEGIN_CODE_SECTION

CODE_SYM_DEF Sigma_Init
        pushad

        ; set video mode 4 (320x200 - 4 colors)
        mov     ax,4
        int     10h

        ; initialize palette
        mov     dx,2dah
        in      al,dx
        and     al,0e0h
        or      al,0dh
        dec     dx
        out     dx,al
        push    ax
        mov     dl,0deh
        mov     bl,0fh
        mov     cx,16
        mov     al,bl
.ploop:
        out     dx,al
        add     bl,10h
        sub     bl,01h
        mov     al,bl
        mov     ah,al
        and     ax,07f8h
        shr     ah,1
        jnc     .zero
        or      ah,4
.zero:  
        or      al,ah
        loop    .ploop

        pop     ax
        and     al,0feh
        mov     dl,0d9h
        out     dx,al

        popad
        ret

CODE_SYM_DEF I_FinishUpdate
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	ebp
        push    edi

	xor     edi,edi

	mov	esi,_backbuffer
	mov	ebp,[_ptrlut16colors]

	xor	ebx,ebx

        mov     dx,0x2DE

        mov     [patchESP+2],esp

L$2:
	sub     esp,80
L$3:
	mov 	bl,[esi]
	mov	ax,[ebp+ebx*2]
	mov	bl,[esi+1]
	mov	cx,[ebp+ebx*2]
	mov	bl,[esi+2]
	lea	eax,[eax*4 + ecx]
	mov	cx,[ebp+ebx*2]
	mov	bl,[esi+3]
	lea	eax,[eax*4 + ecx]
	mov	cx,[ebp+ebx*2]

	lea	eax,[eax*4 + ecx]

	cmp	[_vrambuffer_p2 + edi],al
	je	L$4
	mov	[0xB8000 + edi],al
	mov	[_vrambuffer_p2 + edi],al
L$4:
	cmp	[_vrambuffer_p3 + edi],ah
	je	L$5
	mov	[0xB8000 + edi],ah
	mov	[_vrambuffer_p3 + edi],ah
L$5:
	mov 	bl,[esi+320]
	mov	ax,[ebp+ebx*2]
	mov	bl,[esi+321]
	mov	cx,[ebp+ebx*2]
	mov	bl,[esi+322]
	lea	eax,[eax*4 + ecx]
	mov	cx,[ebp+ebx*2]
	mov	bl,[esi+323]
	lea	eax,[eax*4 + ecx]
	mov	cx,[ebp+ebx*2]
	lea	eax,[eax*4 + ecx]

	cmp	[_vrambuffer_p2 + edi + 0x2000],al
	je	L$6
	mov	[0xB8000 + edi + 0x2000],al
	mov	[_vrambuffer_p2 + edi + 0x2000],al
L$6:
	cmp	[_vrambuffer_p3 + edi + 0x2000],ah
	je	L$7
	mov	[0xB8000 + edi + 0x2000],ah
	mov	[_vrambuffer_p3 + edi + 0x2000],ah
L$7:
        inc	edi
	add	esi,4

        inc	esp
patchESP:
  	cmp	esp, 0x12345678
	jne	L$3

	add	esi,140H
	cmp	di,0x1F40
	jb	L$2

        pop     edi
	pop	ebp
	pop	esi
	pop	edx
	pop	ecx
	pop	ebx
	ret

%endif
