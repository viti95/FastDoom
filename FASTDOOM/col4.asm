;
; Copyright (C) 1993-1996 Id Software, Inc.
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
; DESCRIPTION: Assembly renderer of four wall columns at a time, of the middle
; rows of a group, for the CPUs of no renderer of their own.
;

BITS 32
%include "macros.inc"

; four columns of the view are the four bytes of a row of the modes which
; draw a byte per pixel to the backbuffer, and of the direct VESA mode.  Of
; the CGA modes whose seg loops ask for every second and every fourth column
; the four columns asked for are not the four bytes of a row, and of the
; other modes the renderer of r_col4.c is not chosen.  The guard of r_col4.c
; is the same as this one.
%define COL4_DRAW
%ifdef MODE_CGA16
%undef COL4_DRAW
%endif
%ifdef MODE_CVB
%undef COL4_DRAW
%endif
%ifdef MODE_CGA512
%undef COL4_DRAW
%endif
%ifndef USE_BACKBUFFER
%ifndef MODE_VBE2_DIRECT
%undef COL4_DRAW
%endif
%endif

%ifdef COL4_DRAW

%include "defs.inc"

extern _col4slots
extern _col4x
extern _col4y
extern _col4rows
extern _centery

%ifdef MODE_VBE2_DIRECT
extern _destview
%endif

; the slot of a column kept, as r_col4.c lays it out
SLOT_SIZE equ 24
SLOT_SOURCE equ 0
SLOT_COLORMAP equ 4
SLOT_STEP equ 8
SLOT_MID equ 12

BEGIN_DATA_SECTION

align 4
c4pos:  dd 0 ; the position of the row of the leftmost column
c4d1:   dd 0
c4d2:   dd 0
c4d3:   dd 0
c4step: dd 0 ; the step of a row
c4left: dd 0 ; the rows of the group left to draw

BEGIN_CODE_SECTION

;============================================================================
;
; R_DrawColumns4Mid
;
; The middle rows of a group of four columns of one light table and of one
; step, which is what a seg of one texture gives: the rows of the view where
; the four of them are visible, drawn with one 32 bit store per row.
;
; r_col4.c has kept the four columns in col4slots and has written col4x,
; col4y and col4rows of the rows which the four of them show.  The columns
; are of positions of their own, of the same step, so the position of a row
; of a column is that of the leftmost column plus the difference of the
; positions of the two of them, which the columns of a seg of one texture
; give as a constant.
;
; eax   the position of the row of the leftmost column
; ebx   light table pointer, 0 lowbyte, its byte written over by the source
; ecx   the texel of a column, the pixel of a column
; edx   the source of the column drawn
; esi   the pixels of the row, the leftmost column to the low byte
; edi   the four bytes of the row of the view
;
; The texel of a row is out of its position with the "shr ebx,25" of the byte
; renderers, the seven bits which wrap a texture vertically at 128 pixels.
; The byte of the source names its pixel of the light table as the byte
; renderers name it, "mov al,[eax]" of a table of 256 bytes, of the low byte
; of the address of the table written over by the byte of the source.
;
; The pixels of a row go in from the left column to the right one, so that
; every one of them moves the former ones to its byte, the leftmost column
; ending in the low byte, which is the first of the four bytes.
;============================================================================
CODE_SYM_DEF R_DrawColumns4Mid
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  call near SetupColumns4Mid
  js   near done4mid ; a group without a row is not to be drawn

  ; the four bytes of the first row of the view
  mov  eax,[_col4y]
%ifdef MODE_VBE2_DIRECT
  MulScreenWidthStart edi,eax
  MulScreenWidthEnd edi
  add  edi,[_destview]
  add  edi,[_col4x]
%else
  mov  edi,[_ylookup+eax*4]
  mov  eax,[_col4x]
  add  edi,[_columnofs+eax*4]
%endif

  mov  ebx,[_col4slots+SLOT_COLORMAP]
  and  ebx,-256 ; the tables of the game are of 256 bytes, as "mov al,[eax]"
  mov  eax,[c4pos] ; the position of the row drawn, stepped after it is drawn

align 16
LoopColumns4Mid:
  mov  ecx,eax
  shr  ecx,25 ; the texel of the column
  mov  edx,[_col4slots+SLOT_SOURCE]
  mov  bl,[ecx+edx] ; the byte of the source, over the low byte of the table
  mov  bl,[ebx] ; the pixel of the light table
  movzx ecx,bl
  mov  esi,ecx ; the pixel of the leftmost column to the low byte

  mov  ecx,eax
  add  ecx,[c4d1]
  shr  ecx,25
  mov  edx,[_col4slots+SLOT_SIZE+SLOT_SOURCE]
  mov  bl,[ecx+edx]
  mov  bl,[ebx]
  movzx ecx,bl
  shl  ecx,8
  or   esi,ecx

  mov  ecx,eax
  add  ecx,[c4d2]
  shr  ecx,25
  mov  edx,[_col4slots+2*SLOT_SIZE+SLOT_SOURCE]
  mov  bl,[ecx+edx]
  mov  bl,[ebx]
  movzx ecx,bl
  shl  ecx,16
  or   esi,ecx

  mov  ecx,eax
  add  ecx,[c4d3]
  shr  ecx,25
  mov  edx,[_col4slots+3*SLOT_SIZE+SLOT_SOURCE]
  mov  bl,[ecx+edx]
  mov  bl,[ebx]
  movzx ecx,bl
  shl  ecx,24
  or   esi,ecx ; the four pixels of the row, of the leftmost to the low byte

  mov  [edi],esi ; the four bytes of the row at once
  add  edi,SCREENWIDTH

  add  eax,[c4step] ; the position of the row to come

  dec  dword [c4left]
  jns  near LoopColumns4Mid

done4mid:
  pop  ebp
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret
; R_DrawColumns4Mid ends

;============================================================================
;
; R_DrawColumns4MidMMX
;
; The same rows for the CPUs of MMX, of the positions of the four columns in
; the registers of MMX, where a step of a row of the four of them is one
; "paddd", and of no position of a column in the registers of the machine,
; which the sources of the four columns and the table fill.
;
; The four texels of a row are out of the positions with two "psrld" and one
; "packssdw", which puts the four of them, of seven bits, in the four halves
; of one register, two texels of a "movd" to the registers of the machine.
; The second texel of a "movd" is out of it with one "shr", so that the four
; texels of a row take no move of one register of MMX to the other and one
; "psrlq" only.
;
; Of the CPUs of MMX the two first are of the Pentium, where the byte of the
; renderers of the mode, written over the low byte of an address, is of no
; cost, and of the Pentium II the same byte is of the merging of a register,
; which costs.  The table is of 256 bytes, so a byte of the source added to
; the address of the table, its low byte zero, is the byte of the table which
; the renderers of the mode name with their low byte written over, and this
; renderer adds it, of the registers of the machine staying whole.
;
; eax   the source of a column
; ebx   light table, its address of the low byte zero
; ecx   the texels of the two first columns, the pixel of a column
; edx   the texels of the two last columns
; ebp   the rows of the group left, the loop of a register
; esi   the pixels of the row, the leftmost column to the low byte
; edi   the four bytes of the row of the view
;
; mm0   the positions of the first and of the second column
; mm1   the step of a row, of both the halves of a register
; mm2   the positions of the third and of the fourth column
; mm4   the texels of the four columns, of the packing of the two of mm5 in
; mm5   the texels of the third and of the fourth column
;
; "emms" is not of the renderers of the project, which are of no x87 code of
; their own, and this renderer is of the same kind.
;============================================================================
CODE_SYM_DEF R_DrawColumns4MidMMX
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    edi
  push    ebp

  call    near SetupColumns4Mid
  js      near done4midmmx

  ; Dirección inicial de destino en pantalla
  mov     eax, [_col4y]
%ifdef MODE_VBE2_DIRECT
  MulScreenWidthStart edi, eax
  MulScreenWidthEnd   edi
  add     edi, [_destview]
  add     edi, [_col4x]
%else
  mov     edi, [_ylookup + eax*4]
  mov     eax, [_col4x]
  add     edi, [_columnofs + eax*4]
%endif

  ; Colormap alineado a 256 bytes
  mov     ebx, [_col4slots + SLOT_COLORMAP]
  and     ebx, -256

  ; Inicializar posiciones de las 4 columnas en mm0 y mm2
  mov     ebp, [c4pos]
  mov     eax, ebp
  mov     ecx, ebp
  mov     edx, ebp
  add     eax, [c4d1]
  add     ecx, [c4d2]
  add     edx, [c4d3]

  movd    mm0, ebp
  movd    mm1, eax
  punpckldq mm0, mm1              ; mm0 = [pos1 | pos0]

  movd    mm2, ecx
  movd    mm3, edx
  punpckldq mm2, mm3              ; mm2 = [pos3 | pos2]

  ; Paso vertical (step) duplicado en mm1
  movd    mm1, [c4step]
  punpckldq mm1, mm1              ; mm1 = [step | step]

  ; Guardar los 4 punteros de textura en registros MMX (mm6 y mm3)
  movd    mm6, [_col4slots + SLOT_SOURCE]
  movd    mm4, [_col4slots + SLOT_SIZE + SLOT_SOURCE]
  punpckldq mm6, mm4              ; mm6 = [src1 | src0]

  movd    mm3, [_col4slots + 2*SLOT_SIZE + SLOT_SOURCE]
  movd    mm4, [_col4slots + 3*SLOT_SIZE + SLOT_SOURCE]
  punpckldq mm3, mm4              ; mm3 = [src3 | src2]

  ; Contador de filas en ebp (libre durante todo el bucle)
  mov     ebp, [c4left]

align 16
LoopColumns4MidMMX:
  ; 1. Calcular los 4 índices de texel (7 bits) y avanzar posiciones
  movq    mm4, mm0
  psrld   mm4, 25                 ; mm4 = [0 | tex1 | 0 | tex0]
  paddd   mm0, mm1                ; pos0 += step, pos1 += step

  movq    mm5, mm2
  psrld   mm5, 25                 ; mm5 = [0 | tex3 | 0 | tex2]
  paddd   mm2, mm1                ; pos2 += step, pos3 += step

  packssdw mm4, mm5               ; empaquetar 4 dwords -> 4 words
  pxor    mm5, mm5                ; mm5 = 0
  packuswb mm4, mm5               ; empaquetar 4 words -> 4 bytes bajos en mm4

  ; 2. Extraer los 4 índices a ecx y preparar punteros [src1|src0] en mm4
  movd    ecx, mm4                ; cl=tex0, ch=tex1, bits 16..23=tex2, bits 24..31=tex3
  movq    mm4, mm6                ; mm4 = [src1 | src0]
  movq    mm5, mm3                ; mm5 = [src3 | src2]

  ; --- Columna 0 y Columna 1 ---
  movd    eax, mm4                ; eax = src0
  psrlq   mm4, 32                 ; mm4 baja = src1
  movzx   esi, cl                 ; esi = tex0
  movzx   edx, ch                 ; edx = tex1
  shr     ecx, 16                 ; cl = tex2, ch = tex3

  movzx   esi, byte [eax + esi]   ; texel 0
  movd    eax, mm4                ; eax = src1
  movzx   esi, byte [ebx + esi]   ; pixel 0 (bits 0..7)
  movzx   edx, byte [eax + edx]   ; texel 1
  movzx   edx, byte [ebx + edx]   ; pixel 1
  shl     edx, 8                  ; pixel 1 (bits 8..15)
  or      esi, edx                ; esi = [0 | 0 | pix1 | pix0]

  ; --- Columna 2 y Columna 3 ---
  movd    eax, mm5                ; eax = src2
  psrlq   mm5, 32                 ; mm5 baja = src3
  movzx   edx, cl                 ; edx = tex2
  shr     ecx, 8                  ; ecx = tex3

  movzx   edx, byte [eax + edx]   ; texel 2
  movd    eax, mm5                ; eax = src3
  movzx   edx, byte [ebx + edx]   ; pixel 2
  shl     edx, 16                 ; pixel 2 (bits 16..23)
  or      esi, edx                ; esi = [0 | pix2 | pix1 | pix0]

  movzx   ecx, byte [eax + ecx]   ; texel 3
  movzx   ecx, byte [ebx + ecx]   ; pixel 3
  shl     ecx, 24                 ; pixel 3 (bits 24..31)
  or      esi, ecx                ; esi = [pix3 | pix2 | pix1 | pix0]

  ; Escribir los 4 píxeles y avanzar fila
  mov     [edi], esi
  add     edi, SCREENWIDTH

  dec     ebp
  jns     near LoopColumns4MidMMX

done4midmmx:
  pop     ebp
  pop     edi
  pop     esi
  pop     edx
  pop     ecx
  pop     ebx
  ret

; the position of the first row, the step of a row, the position of every
; column over that of the leftmost one and the rows left of the group, out of
; the columns kept, and the rows left in eax, of none being a negative number
SetupColumns4Mid:
  mov  eax,[_col4rows]
  dec  eax
  mov  [c4left],eax
  js   near SetupColumns4MidDone ; a group without a row is not to be drawn

  ; the position of the first row of the leftmost column
  mov  eax,[_col4y]
  sub  eax,[_centery]
  mov  ecx,[_col4slots+SLOT_STEP]
  imul ecx
  add  eax,[_col4slots+SLOT_MID]
  shl  eax,9 ; 7 significant bits, 25 frac
  shl  ecx,9
  mov  [c4step],ecx
  mov  [c4pos],eax

  ; the position of every column over that of the leftmost one
  mov  ecx,[_col4slots+SLOT_MID]
  mov  eax,[_col4slots+SLOT_SIZE+SLOT_MID]
  sub  eax,ecx
  shl  eax,9
  mov  [c4d1],eax
  mov  eax,[_col4slots+2*SLOT_SIZE+SLOT_MID]
  sub  eax,ecx
  shl  eax,9
  mov  [c4d2],eax
  mov  eax,[_col4slots+3*SLOT_SIZE+SLOT_MID]
  sub  eax,ecx
  shl  eax,9
  mov  [c4d3],eax

SetupColumns4MidDone:
  mov  eax,[c4left] ; the rows left, of none a negative number for the call
  test eax,eax ; the sign of the rows left, for the caller of the renderer
  ret

%endif
