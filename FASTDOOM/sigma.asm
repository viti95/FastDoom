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

CODE_SYM_DEF Sigma_Init
        pushad

        ; get current video mode
        mov     ah,0fh
        int     10h

        ; save mode number
        push    ax

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

        ; clear blue/intensity page
        mov     dx,2deh
        mov     al,3
        out     dx,al
        ;mov     ax,0b800h
        ;mov     es,ax
        ;xor     di,di
        ;mov     cx,8192
        ;xor     ax,ax
        ;rep     stosw
        mov     dx,2deh
        mov     al,2
        out     dx,al

        ; restore mode number
        pop     ax
        xor     ah,ah

        popad
        ret

        ; red/green page
        ;mov     dx,2deh
        ;mov     al,3
        ;out     dx,al

        ; blue/intensity page
        ;mov     dx,2deh
        ;mov     al,3
        ;out     dx,al

%endif
