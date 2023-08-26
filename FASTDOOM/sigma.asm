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

_vrambuffer_p2: times 16384 db 0
_vrambuffer_p3: times 16384 db 0

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
        xor     eax,eax

        mov     dx,0x2DE

        mov     [patchESP+2],esp

L$2:
	sub     esp,80
L$3:
	mov 	al,[esi]
	mov	bx,[ebp+eax*2]
	mov	al,[esi+1]
	mov	cx,[ebp+eax*2]
	mov	al,[esi+2]
	lea	ebx,[ebx*4 + ecx]
	mov	cx,[ebp+eax*2]
	mov	al,[esi+3]
	lea	ebx,[ebx*4 + ecx]
	mov	cx,[ebp+eax*2]

	lea	ebx,[ebx*4 + ecx]

	cmp	[_vrambuffer_p2 + edi],bl
	je	L$4
        mov     al,2
        out     dx,al

	mov	[0xB8000 + edi],bl
	mov	[_vrambuffer_p2 + edi],bl
L$4:
	cmp	[_vrambuffer_p3 + edi],bh
	je	L$5
        mov     al,3
        out     dx,al

	mov	[0xB8000 + edi],bh
	mov	[_vrambuffer_p3 + edi],bh
L$5:
	mov 	al,[esi+320]
	mov	bx,[ebp+eax*2]
	mov	al,[esi+321]
	mov	cx,[ebp+eax*2]
	mov	al,[esi+322]
	lea	ebx,[ebx*4 + ecx]
	mov	cx,[ebp+eax*2]
	mov	al,[esi+323]
	lea	ebx,[ebx*4 + ecx]
	mov	cx,[ebp+eax*2]
	lea	ebx,[ebx*4 + ecx]

	cmp	[_vrambuffer_p2 + edi + 0x2000],bl
	je	L$6
        mov     al,2
        out     dx,al

	mov	[0xB8000 + edi + 0x2000],bl
	mov	[_vrambuffer_p2 + edi + 0x2000],bl
L$6:
	cmp	[_vrambuffer_p3 + edi + 0x2000],bh
	je	L$7
        mov     al,3
        out     dx,al

	mov	[0xB8000 + edi + 0x2000],bh
	mov	[_vrambuffer_p3 + edi + 0x2000],bh
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
