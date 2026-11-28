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
; DESCRIPTION: Assembly renderer of two wall columns at a time, of the middle
; rows of a group, for the CPUs of no renderer of their own.
;

BITS 32
%include "macros.inc"

; two columns of the view are the four bytes of a row of the modes which keep
; two bytes of them for one pixel of it, of the detail of the low: the modes
; which draw to the backbuffer and the direct mode of VESA.  Of the CGA modes
; whose seg loops ask for every second and every fourth column the two columns
; asked for are not the four bytes of a row, and of the other modes the
; renderer of r_col2.c is not chosen.  The guard of r_col2.c is the same as
; this one.
%define COL2_DRAW
%ifdef MODE_CGA16
%undef COL2_DRAW
%endif
%ifdef MODE_CVB
%undef COL2_DRAW
%endif
%ifdef MODE_CGA512
%undef COL2_DRAW
%endif
%ifndef USE_BACKBUFFER
%ifndef MODE_VBE2_DIRECT
%undef COL2_DRAW
%endif
%endif

%ifdef COL2_DRAW

%include "defs.inc"

extern _col2slots
extern _col2x
extern _col2y
extern _col2rows
extern _centery

%ifdef MODE_VBE2_DIRECT
extern _destview
%endif

; the slot of a column kept, as r_col2.c lays it out
SLOT_SIZE equ 24
SLOT_SOURCE equ 0
SLOT_COLORMAP equ 4
SLOT_STEP equ 8
SLOT_MID equ 12

BEGIN_DATA_SECTION

align 4
c2pos:  dd 0 ; the position of the row of the leftmost column
c2d1:   dd 0 ; the position of the second column over that of the first one
c2step: dd 0 ; the step of a row
c2left: dd 0 ; the rows of the group left, which the loops count in a register

; the rows of the group left to the loop of the renderer of SSE2, which is not
; of a register of the machine, of the registers of them which the loop keeps
; being the destination, the light table and the two texels of it
align 4
c2leftSSE2: dd 0

; of the group of columns of light tables, of steps and of sources of their
; own, the source of every column, its light table, the step of a row of it
; and the position of the row drawn of it, and the rows left, the first row of
; the view of the group and its first row of the view of them
align 4
c2asrc:  dd 0,0
c2amap:  dd 0,0
c2astep: dd 0,0
c2apos:  dd 0,0
c2aleft: dd 0
c2adest: dd 0

BEGIN_CODE_SECTION

;============================================================================
;
; R_DrawColumns2Mid
;
; The middle rows of a group of two columns of one light table and of one
; step, which is what a seg of one texture gives: the rows of the view where
; the two of them are visible, drawn with one 32 bit store per row.
;
; r_col2.c has kept the two columns in col2slots and has written col2x, col2y
; and col2rows of the rows which the two of them show.  The columns are of
; positions of their own, of the same step, so the position of a row of the
; column of the right is that of the column of the left plus the difference of
; the positions of the two of them, which the columns of a seg of one texture
; give as a constant.
;
; Of the detail of the low a pixel of the view is of the two bytes of a row of
; it, the byte of the light table of it in the two of them, as the renderers
; of the modes of the file write it, "mov bx,ax" of their byte of the table in
; the two bytes of the register of them.  The two pixels of a row of a group
; are the four bytes of it, the pixel of the column of the left in the two
; bytes of it of the low, the pixel of the column of the right in the two of
; it of the high.
;
; eax   the position of the row of the leftmost column
; ebx   light table pointer, 0 lowbyte, its byte written over by the source
; ecx   the texel of a column, the pixel of the column of the right
; edx   the source of the column drawn, the pixel of a column to its bytes
; esi   the pixels of the row, the pixel of the column of the left to its bytes
; edi   the four bytes of the row of the view
; ebp   the rows of the group left, the loop of a register
;
; The texel of a row is out of its position with the "shr ebx,25" of the byte
; renderers, the seven bits which wrap a texture vertically at 128 pixels.
; The byte of the source names its pixel of the light table as the byte
; renderers name it, "mov al,[eax]" of a table of 256 bytes, of the low byte
; of the address of the table written over by the byte of the source.
;============================================================================
CODE_SYM_DEF R_DrawColumns2Mid
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  call near SetupColumns2Mid
  js   near done2mid ; a group without a row is not to be drawn

  ; the call gives the rows of the group left in eax, of none a negative
  ; number: the rows are counted in a register, which the loop of the machine
  ; keeps whole, of "dec [mem]" being of three uops of the Pentium, of the
  ; write of the rows left joining the loop of its own
  mov  ebp,eax ; the rows of the group left, the loop of a register

  ; the four bytes of the first row of the view, of the two bytes of a column
  mov  eax,[_col2y]
%ifdef MODE_VBE2_DIRECT
  MulScreenWidthStart edi,eax
  MulScreenWidthEnd edi
  add  edi,[_destview]
  mov  eax,[_col2x]
  add  edi,eax
  add  edi,eax
%else
  mov  edi,[_ylookup+eax*4]
  mov  eax,[_col2x]
  add  edi,[_columnofs+eax*4]
%endif

  mov  ebx,[_col2slots+SLOT_COLORMAP]
  and  ebx,-256 ; the tables of the game are of 256 bytes, as "mov al,[eax]"
  mov  eax,[c2pos] ; the position of the row drawn, stepped after it is drawn

align 16
LoopColumns2Mid:
  mov  ecx,eax
  shr  ecx,25 ; the texel of the column of the left
  mov  edx,[_col2slots+SLOT_SOURCE]
  movzx esi,byte [ecx+edx] ; the byte of the source, over the low byte of the
  movzx esi,byte [ebx+esi] ; table: the pixel of the column of the left
  mov  edx,esi
  shl  edx,8
  or   esi,edx ; its byte in the two bytes of it of the row

  mov  ecx,eax
  add  ecx,[c2d1] ; the position of the column of the right
  shr  ecx,25
  mov  edx,[_col2slots+SLOT_SIZE+SLOT_SOURCE]
  movzx ecx,byte [ecx+edx]
  movzx ecx,byte [ebx+ecx] ; the pixel of the column of the right
  shl  ecx,16
  mov  edx,ecx
  shl  edx,8
  or   ecx,edx ; its byte in the two bytes of it of the row
  or   esi,ecx ; the two pixels of the row, of the four bytes of it

  mov  [edi],esi ; the four bytes of the row at once
  add  edi,SCREENWIDTH

  add  eax,[c2step] ; the position of the row to come

  dec  ebp ; the rows of the group left
  jns  near LoopColumns2Mid

done2mid:
  pop  ebp
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret
; R_DrawColumns2Mid ends

;============================================================================
;
; R_DrawColumns2MidMMX
;
; The same rows for the CPUs of MMX, of the positions of the two columns in
; the two dwords of one register of them, where a step of a row of the two of
; them is one "paddd", and of no position of a column in the registers of the
; machine, which the light table and the two pixels of the row fill.
;
; The two texels of a row are out of the positions with one "psrld", joined in
; the bytes of one register with one "packssdw" and one "packuswb", which puts
; the two of them, of seven bits, in the two bytes of it of the low, the
; graphics wrapping vertically at 128 pixels, and of the two of them again
; above of them.  The two bytes of them go to one register of the machine with
; one "movd", of the two bytes of it two "movzx" of their own, of the second
; byte of it being of "movzx" of its own and of no "shr" of the register, of
; the texels being none of them of a move of one register of MMX to the other.
;
; Of the CPUs of MMX the two first are of the Pentium, where the byte of the
; renderers of the mode, written over the low byte of an address, is of no
; cost, and of the Pentium II the same byte is of the merging of a register,
; which costs.  The table is of 256 bytes, so a byte of the source added to
; the address of the table, its low byte zero, is the byte of the table which
; the renderers of the mode name with their low byte written over, and this
; renderer adds it, of the registers of the machine staying whole.
;
; The sources of the two columns are of the slots kept, of the loop taking
; them of them every row, as the loop of the registers of the machine, the
; registers of MMX of the positions being none of them of a load of a byte of
; a table.
;
; eax   the source of a column, the pixel of the column of the right to its
;       bytes, the four pixels of the row of them joined
; ebx   light table, its address of the low byte zero
; ecx   the two texels of the row out of the register of MMX, the pixel of the
;       column of the left to its bytes, the two pixels of the row joined
; edx   the texel of the column of the right, the pixel of it, the byte of it
;       to its bytes of the row
; esi   the texel of the column of the left, the byte of the source of it, the
;       source of a column, the pixel of it of the row
; edi   the four bytes of the row of the view
; ebp   the rows of the group left, the loop of a register
;
; mm0   the positions of the two columns, one of them in each one of its dwords
; mm1   the step of a row, of both the halves of the register
; mm4   the texels of the two columns, of the packing of them to their bytes
;
; "emms" is not of the renderers of the project, which are of no x87 code of
; their own, and this renderer is of the same kind.
;============================================================================
CODE_SYM_DEF R_DrawColumns2MidMMX
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    edi
  push    ebp

  call    near SetupColumns2Mid
  js      near done2midmmx

  ; the four bytes of the first row of the view, of the two bytes of a column
  mov     eax,[_col2y]
%ifdef MODE_VBE2_DIRECT
  MulScreenWidthStart edi,eax
  MulScreenWidthEnd   edi
  add     edi,[_destview]
  mov     eax,[_col2x]
  add     edi,eax
  add     edi,eax
%else
  mov     edi,[_ylookup+eax*4]
  mov     eax,[_col2x]
  add     edi,[_columnofs+eax*4]
%endif

  ; the light table of its low byte zero, of the byte of the source naming its
  ; pixel of the table being of one "movzx" of the table added
  mov     ebx,[_col2slots+SLOT_COLORMAP]
  and     ebx,-256

  ; the positions of the two columns of the first row of the group, the one of
  ; the column of the right out of that of the column of the left
  mov     eax,[c2pos]
  mov     ecx,eax
  add     ecx,[c2d1]
  movd    mm0,eax
  movd    mm1,ecx
  punpckldq mm0,mm1           ; mm0 = [pos1 | pos0]

  ; the step of a row, of the two dwords of the register of them
  movd    mm1,[c2step]
  punpckldq mm1,mm1           ; mm1 = [step | step]

  mov     ebp,[c2left]        ; the rows of the group left, the loop of a register

align 16
LoopColumns2MidMMX:
  ; the texels of the two columns of the row, and the positions of the columns
  ; of the row to come
  movq    mm4,mm0
  psrld   mm4,25
  paddd   mm0,mm1
  packssdw mm4,mm4            ; the two texels of them, of the words of them
  packuswb mm4,mm4            ; ... and of their bytes, the two of them low
  movd    ecx,mm4

  movzx   esi,cl              ; the texel of the column of the left
  movzx   edx,ch              ; ... of the column of the right

  mov     eax,[_col2slots+SLOT_SOURCE]
  movzx   esi,byte [eax+esi]  ; the byte of the source of the column
  mov     eax,[_col2slots+SLOT_SIZE+SLOT_SOURCE]
  movzx   edx,byte [eax+edx]  ; ... of the column of the right
  movzx   esi,byte [ebx+esi]  ; the pixel of the column of the left
  movzx   edx,byte [ebx+edx]  ; ... of the column of the right

  ; the two pixels of the row, of the byte of every one of them in the two
  ; bytes of it, the column of the left of the two bytes of the low
  mov     ecx,esi
  shl     esi,8
  or      esi,ecx
  shl     edx,16
  mov     eax,edx
  shl     eax,8
  or      edx,eax
  or      esi,edx

  mov     [edi],esi           ; the four bytes of the row of the view
  add     edi,SCREENWIDTH

  dec     ebp                 ; the rows of the group left
  jns     near LoopColumns2MidMMX

done2midmmx:
  pop     ebp
  pop     edi
  pop     esi
  pop     edx
  pop     ecx
  pop     ebx
  ret
; R_DrawColumns2MidMMX ends

; the position of the first row, the step of a row, the position of the column
; of the right over that of the column of the left and the rows left of the
; group, out of the columns kept, and the rows left in eax, of none being a
; negative number
SetupColumns2Mid:
  mov  eax,[_col2rows]
  dec  eax
  mov  [c2left],eax
  js   near SetupColumns2MidDone ; a group without a row is not to be drawn

  ; the position of the first row of the leftmost column
  mov  eax,[_col2y]
  sub  eax,[_centery]
  mov  ecx,[_col2slots+SLOT_STEP]
  imul ecx
  add  eax,[_col2slots+SLOT_MID]
  shl  eax,9 ; 7 significant bits, 25 frac
  shl  ecx,9
  mov  [c2step],ecx
  mov  [c2pos],eax

  ; the position of the column of the right over that of the column of the left
  mov  ecx,[_col2slots+SLOT_MID]
  mov  eax,[_col2slots+SLOT_SIZE+SLOT_MID]
  sub  eax,ecx
  shl  eax,9
  mov  [c2d1],eax

SetupColumns2MidDone:
  mov  eax,[c2left] ; the rows left, of none a negative number for the call
  test eax,eax ; the sign of the rows left, for the caller of the renderer
  ret

;============================================================================
;
; R_DrawColumns2MidSSE2
;
; The middle rows of a group of two columns, two pixels per iteration of the
; loop, one of them per column, of the positions of the two of them in the two
; dwords of one register of SSE2, walked with one "paddd" of the step of a row,
; which is the same for the two of them.
;
; The two texels of a row are out of the positions with one "psrld", joined in
; the two bytes of one dword with one "packssdw" and one "packuswb", of the
; texels being of seven bits only, the graphics wrapping vertically at 128
; pixels, and gotten to the registers of the machine with one "movd" and two
; "movzx" of the two bytes of it of the low, of no "shr" of the register of
; them.  The byte of the source of every one of them names its pixel of the
; light table with the address of it, the table being of 256 bytes, which the
; renderer of the mode of the file names with its low byte written over.
;
; The sources of the two columns are of the slots kept, of the loop taking
; them of them every row, as the renderer of the scalar of them.
;
; ebx   the four bytes of the row of the view
; ecx   the texel of the column of the left, the pixel of it, the two pixels
;       of the row joined over its bytes
; edx   the texel of the column of the right, the pixel of it, the byte of it
;       to its bytes of the row
; esi   the rows of the group left, the loop of a register
; eax   the two texels of the row out of the register of SSE2, the byte of a
;       pixel of the row to its bytes of it
; edi   a register of the sources of the columns
; ebp   the light table, of its low byte zero
;
; xmm0  the positions of the two columns, one of them in each dword
; xmm1  the step of a row, of the two dwords of it
; xmm2  the positions of the two columns, of their texels in the bytes of them
;
; "emms" is not of the renderers of the project, which are of no x87 code of
; their own, and this renderer is of the same kind.
;============================================================================
CODE_SYM_DEF R_DrawColumns2MidSSE2
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    edi
  push    ebp

  call    near SetupColumns2Mid
  js      near done2midsse2

  ; the destination of the row of the four bytes, of the two bytes of a column
  mov     eax,[_col2y]
%ifdef MODE_VBE2_DIRECT
  MulScreenWidthStart ebx,eax
  MulScreenWidthEnd   ebx
  add     ebx,[_destview]
  mov     eax,[_col2x]
  add     ebx,eax
  add     ebx,eax
%else
  mov     ebx,[_ylookup+eax*4]
  mov     eax,[_col2x]
  add     ebx,[_columnofs+eax*4]
%endif

  mov     ebp,[_col2slots+SLOT_COLORMAP]
  and     ebp,-256 ; the tables of the game are of 256 bytes, as the byte of
                   ; the source naming its pixel of the table is of one "movzx"

  ; the positions of the two columns of the row, of the first of them that of
  ; the column of the left, of the setup of the call
  mov     edx,[c2pos]
  mov     eax,[c2d1]
  add     eax,edx
  movd    xmm0,edx
  movd    xmm1,eax
  punpckldq xmm0,xmm1       ; the positions of the two columns

  mov     eax,[c2step]
  movd    xmm1,eax
  shufps  xmm1,xmm1,0       ; the step of a row, broadcasted to the two dwords

  mov     eax,[_col2rows]
  dec     eax               ; the rows of the group left
  mov     [c2leftSSE2],eax

align 16
LoopColumns2MidSSE2:
  movdqa  xmm2,xmm0
  psrld   xmm2,25           ; the texels of the two columns
  paddd   xmm0,xmm1         ; the positions of the columns of the row to come
  packssdw xmm2,xmm2
  packuswb xmm2,xmm2        ; the two texels of them, one in each byte

  movd    eax,xmm2
  movzx   ecx,al            ; the texel of the column of the left
  movzx   edx,ah            ; ... of the column of the right

  mov     edi,[_col2slots+SLOT_SOURCE]
  movzx   ecx,byte [edi+ecx] ; the byte of the source of the first column
  movzx   ecx,byte [ebp+ecx] ; its pixel of the light table

  mov     edi,[_col2slots+SLOT_SIZE+SLOT_SOURCE]
  movzx   edx,byte [edi+edx] ; the byte of the source of the second column
  movzx   edx,byte [ebp+edx] ; its pixel of the light table

  ; the two pixels of the row, of the byte of every one of them in the two
  ; bytes of it, the column of the left of the two bytes of the low, and
  ; written to the row of the view with one store of 32 bits, the row of the
  ; view being of the four bytes of the two columns of the group kept, the
  ; group being of the alignment of a row of them.
  mov     esi,ecx
  shl     ecx,8
  or      ecx,esi           ; the pixel of the column of the left, of its byte
                            ; in the two bytes of it
  shl     edx,16
  mov     eax,edx
  shl     eax,8
  or      edx,eax           ; ... of the column of the right, of its byte in
                            ; the two bytes of it
  or      ecx,edx           ; the two pixels of the row
  mov     [ebx],ecx         ; the four bytes of the row of the view

  add     ebx,SCREENWIDTH   ; the row to come

  dec     dword [c2leftSSE2]
  jns     near LoopColumns2MidSSE2

done2midsse2:
  pop     ebp
  pop     edi
  pop     esi
  pop     edx
  pop     ecx
  pop     ebx
  ret
; R_DrawColumns2MidSSE2 ends

;============================================================================
;
; R_DrawColumns2MidAll
;
; The middle rows of a group of two columns which is not of the shape of the
; loops above: of a light table of its own, of a step of its own and of a
; source of its own every one of the two of them, which is what the groups of
; the seg loops give of the columns of more than one seg, a seg being of a
; light level of its own and of a step of its own, and of the groups of the
; last two columns of a seg with the first two of the next one, the two
; columns of a group of the segs of the tiers of one column being of a light
; table of their own.
;
; Every column of the group is walked on its own, of the position of a row of
; it, of the source of it, of the light table of it and the step of it, and
; the two of them are drawn together as above, a row of the view being of one
; 32 bit store.  Of the two columns the sources, the tables, the steps and the
; positions are eight values, of the data of the file, the registers of the
; machine being of the two positions, of the row of the view and of the pixels
; of it: the positions of the two columns are of two registers, walked with
; the step of each one of the data.
;
; A column is of two loads, of the byte of the source of the texel of the row
; and of the pixel of the light table of the byte, as of the loops above, and
; of one load more of the source of it and of one more of its table, the two
; columns being of sources of their own and of tables of their own.  The
; walking of the two positions and the taking of the two texels of them are of
; the registers of MMX and of the registers of SSE2 of the loops of the CPUs of
; them, as of the loop of the middle rows of one table and of one step above,
; the loads of the bytes of the sources and of the pixels of the tables being
; of the registers of the machine of the three of the loops of them.
;
; The pixels of the row go in from the column of the left to the one of the
; right, the byte of the light table of every column going over the two bytes
; of the row of the column of its left with a shift and with an "or", as of the
; loops above, of the byte of the column of the right being shifted to its two
; bytes of them, the four bytes of the row of the view being of the two bytes
; of every one of the two pixels of it.
;
; eax   the position of the row drawn, of the first column
; ebx   the position of the row drawn, of the second column
; ecx   the texel of a column, the pixel of the column of the right, its byte
;       to its two bytes of the row, the two pixels of the row joined
; edx   the pixel of the column of the left, the byte of it to its two bytes
;       of the row, the byte of a pixel of the row to its bytes of them
; esi   the texel of the column drawn, the byte of the source of it, the pixel
;       of the column of the left of the row
; edi   the source of a column, the light table of a column, the base of a load
; ebp   the four bytes of the row of the view
;
; The texel of a row is out of its position with the "shr ebx,25" of the byte
; renderers, and the byte of the source names its pixel of the light table as
; the renderers of the mode name it, of the table being of 256 bytes.
;
; The rows left of the group are of the data, of the registers of the machine
; being all of the two positions, of the row of the view and of the pixels of
; it.  The same rows of the CPUs of MMX and of the CPUs of the instructions of
; SSE2 are of the two renderers of the end of the file, of the same setup of
; them, SetupColumns2All, and of the same one 32 bit store of the four bytes of
; a row, r_main.c choosing between the three of them of its switch of the CPU.
;============================================================================
CODE_SYM_DEF R_DrawColumns2MidAll
	push	ebx
	push	esi
	push	edi
	push	ebp

  call	near SetupColumns2All
  js	near done2all ; a group without a row is not to be drawn

  mov  eax,[c2apos]  ; the position of the row drawn, of the first column
  mov  ebx,[c2apos+4] ; ... of the second one
  mov  ebp,[c2adest] ; the four bytes of the first row of the view

align 16
LoopColumns2MidAll:
  ; the column of the left, its pixel to the two bytes of the row of them of
  ; the low
  mov  esi,eax
  shr  esi,25 ; the texel of the first column
  mov  edi,[c2asrc] ; the source of the column
  movzx esi,byte [edi+esi] ; the byte of the source, of the whole register
  mov  edi,[c2amap] ; the light table of the column
  movzx esi,byte [edi+esi] ; its pixel
  mov  edx,esi
  shl  edx,8
  or   esi,edx ; its byte in the two bytes of it of the row
  add  eax,[c2astep] ; the position of the row to come

  ; the column of the right, its pixel to the two bytes of the row of them of
  ; the high, the register of the byte of the source of it being free of its
  ; own load of the pixel of the table of it
  mov  ecx,ebx
  shr  ecx,25 ; the texel of the second column
  mov  edi,[c2asrc+4] ; the source of the column
  movzx ecx,byte [edi+ecx]
  mov  edi,[c2amap+4]
  movzx ecx,byte [edi+ecx] ; its pixel, of the register of the texel of it
  shl  ecx,16
  mov  edx,ecx
  shl  edx,8
  or   ecx,edx ; its byte in the two bytes of it of the row
  or   esi,ecx ; over the two bytes below of the row
  add  ebx,[c2astep+4] ; the position of the row to come

  mov  [ebp],esi ; the four bytes of the row at once
  add  ebp,SCREENWIDTH

  ; the rows of the group left, of the data, of the registers of the machine
  ; being all of the two positions, of the row of the view and of the pixels
  ; of it
  dec  dword [c2aleft]
  jns  near LoopColumns2MidAll

done2all:
  pop  ebp
  pop  edi
  pop  esi
  pop  ebx
  ret
; R_DrawColumns2MidAll ends

; the position of the row drawn of one column kept, the step of a row of it,
; its source and its light table, out of the first row of the group of it, as
; r_col2.c seeds the columns it hands to the renderer
%macro SEEDC2ALL 1
  mov  eax,esi ; the first row of the group
  sub  eax,[_centery]
  mov  ecx,[_col2slots+%1*SLOT_SIZE+SLOT_STEP]
  imul ecx ; the rows of the column over the middle of the view, of edx:eax
  add  eax,[_col2slots+%1*SLOT_SIZE+SLOT_MID]
  shl  eax,9 ; 7 significant bits, 25 frac
  mov  [c2apos+%1*4],eax
  shl  ecx,9 ; the step of a row, of the same format
  mov  [c2astep+%1*4],ecx
  mov  eax,[_col2slots+%1*SLOT_SIZE+SLOT_SOURCE]
  mov  [c2asrc+%1*4],eax
  mov  eax,[_col2slots+%1*SLOT_SIZE+SLOT_COLORMAP]
  and  eax,-256 ; the tables of the game are of 256 bytes, as "mov al,[eax]"
  mov  [c2amap+%1*4],eax
%endmacro

; the rows left of the group in eax, of none being a negative number, and the
; data of the two columns of the group and of the row of the view of the first
; of their rows
SetupColumns2All:
  mov  eax,[_col2rows]
  dec  eax
  mov  [c2aleft],eax
  js   near SetupColumns2AllDone ; a group without a row is not to be drawn

  ; the first row of the group, of the register of it for the seeds of the two
  ; columns, and the four bytes of its row of the view
  mov  esi,[_col2y]
  mov  eax,esi
%ifdef MODE_VBE2_DIRECT
  MulScreenWidthStart edi,eax
  MulScreenWidthEnd edi
  add  edi,[_destview]
  mov  ecx,[_col2x]
  add  edi,ecx
  add  edi,ecx
%else
  mov  edi,[_ylookup+eax*4]
  mov  ecx,[_col2x]
  add  edi,[_columnofs+ecx*4]
%endif
  mov  [c2adest],edi

  SEEDC2ALL 0
  SEEDC2ALL 1

SetupColumns2AllDone:
  mov  eax,[c2aleft] ; the rows left, of none a negative number for the call
  test eax,eax ; the sign of the rows left, for the caller of the renderer
  ret

;============================================================================
;
; R_DrawColumns2MidAllMMX
;
; The same rows of the group of a light table, a step and a source of their
; own, for the CPUs of MMX: of the registers of MMX the two positions of the
; row drawn and the two texels of them, and of the registers of the machine the
; four loads of the bytes of the sources and of the pixels of the tables, which
; the registers of MMX of the two bytes and of the two words do not name.
;
; The two positions are of one register, and the two steps of another one, so
; a row of the group walks the two of the positions with one "paddd" of its
; own, and the texels of them go out of the positions with one "psrld".  The
; two dwords of the texels of a row become two words of one register with one
; "packssdw" and two bytes of it with one "packuswb", the graphics wrapping
; vertically at 128 pixels, and the two of them go to one register of the
; machine with one "movd", of the two bytes of it two "movzx" of their own, of
; no register of MMX being moved to the other and of no "shr" of the register
; of the two bytes of them.
;
; The positions being of the registers of MMX, the loop is of six registers of
; the machine, one of them for the four bytes of the row of the view, one of
; them for the rows of the group left, two of them for the source and the
; table of the column drawn and two of them for the texel and the pixel of
; every one of the two columns, the two pixels being joined with a shift and
; with an "or" of their own as of the loops above, the pixel of the column of
; the left being of the register of the byte of the source of it, which the
; load of the pixel of the table leaves free of its own load.
;
; eax   the texel of the first column, the source of a column, the pixel of it,
;       the two pixels of the row joined, the register written to the row
; ebx   the source of a column, the light table of a column, the byte of a
;       pixel of the row to its bytes of them, the base of a load
; ecx   the two texels of the row out of the register of MMX, the pixel of the
;       first column, the two pixels of the row joined
; edx   the texel of the second column, the pixel of it, its byte to its two
;       bytes of the row
; esi   the source of a column, the light table of a column, the base of a load
; edi   the four bytes of the row of the view
; ebp   the rows of the group left, the loop of a register
;
; mm0   the positions of the two columns, one of them in each one of its dwords
; mm1   the steps of a row of the two of them
; mm4   the texels of the two columns, of the packing of them to their bytes
;
; "emms" is not of the renderers of the project, which are of no x87 code of
; their own, and this renderer is of the same kind.
;============================================================================
CODE_SYM_DEF R_DrawColumns2MidAllMMX
	push	ebx
	push	esi
	push	edi
	push	ebp

  call	near SetupColumns2All
  js	near done2allmmx ; a group without a row is not to be drawn

  ; the positions of the two columns of the first row of the group, one of them
  ; in each one of the dwords of the register of them
  mov  esi,[c2apos]
  mov  eax,[c2apos+4]
  movd mm0,esi
  movd mm1,eax
  punpckldq mm0,mm1 ; mm0 = [pos1 | pos0]

  ; the step of a row of every one of the columns, of the same shape, the
  ; columns of the group being of steps of their own
  mov  esi,[c2astep]
  mov  eax,[c2astep+4]
  movd mm1,esi
  movd mm2,eax
  punpckldq mm1,mm2 ; mm1 = [step1 | step0]

  mov  edi,[c2adest] ; the four bytes of the first row of the view
  mov  ebp,[c2aleft] ; the rows of the group left, the loop of a register

align 16
LoopColumns2MidAllMMX:
  ; the texels of the two columns of the row, and the positions of the columns
  ; of the row to come
  movq  mm4,mm0
  psrld mm4,25
  paddd mm0,mm1
  packssdw mm4,mm4 ; the two texels of them, of the words of them
  packuswb mm4,mm4 ; ... and of their bytes, the two of them low
  movd  ecx,mm4

  movzx eax,cl ; the texel of the first column
  movzx edx,ch ; ... of the second one
  mov   ebx,[c2asrc]
  movzx eax,byte [ebx+eax] ; the byte of the source of the column
  mov   ebx,[c2amap] ; the light table of the column, of its low byte zero
  movzx eax,byte [ebx+eax] ; its pixel

  mov   esi,[c2asrc+4]
  movzx edx,byte [esi+edx]
  mov   esi,[c2amap+4]
  movzx edx,byte [esi+edx] ; its pixel of the column of the right

  ; the two pixels of the row, of the byte of every one of them in the two
  ; bytes of it, of the column of the left of the two bytes of the low
  mov   ecx,eax
  shl   eax,8
  or    eax,ecx
  shl   edx,16
  mov   ebx,edx
  shl   ebx,8
  or    edx,ebx
  or    eax,edx

  mov   [edi],eax ; the four bytes of the row of the view at once
  add   edi,SCREENWIDTH

  dec   ebp ; the rows of the group left
  jns   near LoopColumns2MidAllMMX

done2allmmx:
  pop  ebp
  pop  edi
  pop  esi
  pop  ebx
  ret
; R_DrawColumns2MidAllMMX ends

;============================================================================
;
; R_DrawColumns2MidAllSSE2
;
; The same rows for the CPUs of the instructions of SSE2, of the two positions
; of the row drawn in the two dwords of one register of them, of the two steps
; of them in the two dwords of another one, and of the loop walking the two of
; the positions with one "paddd" of its own, the columns of the group being of
; steps of their own, as the group is of light tables of their own and of
; sources of their own.
;
; The texels of a row are out of the positions with one "psrld", joined in the
; two bytes of one dword with one "packssdw" and one "packuswb", the graphics
; wrapping vertically at 128 pixels, and gotten to the registers of the machine
; with one "movd" and two "movzx" of the two bytes of them.  The four loads of
; the bytes of the sources and of the pixels of the tables are of the registers
; of the machine, the registers of SSE2 being of no load of a byte of a table,
; and the two pixels of the row are joined of their own, of the byte of every
; one of them in the two bytes of it of the row.
;
; ecx   the texel of the first column, the pixel of it, the two pixels of the
;       row joined over its bytes
; edx   the texel of the second column, the pixel of it, its byte to its two
;       bytes of the row
; esi   the byte of a pixel of the row to its two bytes of it, the base of a
;       shift of them, the source of a column, the light table of a column
; eax   the two texels of the row out of the register of SSE2, the byte of a
;       pixel of the row to its two bytes of them
; ebx   the four bytes of the row of the view
; edi   the source of a column, the light table of a column, the base of a load
; ebp   the rows of the group left, the loop of a register
;
; xmm0  the positions of the two columns, one of them in each dword
; xmm1  the steps of a row of the two of them, one of them in each dword
; xmm2  the positions of the two columns, of their texels in the bytes of it
;       (xmm3 and xmm4 are of the building of the two dwords of the two of
;       them above, of the setup of them, and of the loop of none of them)
;
; "emms" is not of the renderers of the project, which are of no x87 code of
; their own, and this renderer is of the same kind.
;============================================================================
CODE_SYM_DEF R_DrawColumns2MidAllSSE2
	push	ebx
	push	esi
	push	edi
	push	ebp

  call	near SetupColumns2All
  js	near done2allsse2 ; a group without a row is not to be drawn

  ; the positions of the two columns of the first row of the group, one of them
  ; in each dword of the register of them
  mov  esi,[c2apos]
  mov  eax,[c2apos+4]
  movd xmm0,esi
  movd xmm1,eax
  punpckldq xmm0,xmm1

  ; the step of a row of every one of the two columns, of the same shape, the
  ; columns of the group being of steps of their own
  mov  esi,[c2astep]
  mov  eax,[c2astep+4]
  movd xmm1,esi
  movd xmm2,eax
  punpckldq xmm1,xmm2

  mov  ebx,[c2adest] ; the four bytes of the first row of the view
  mov  ebp,[c2aleft] ; the rows of the group left, the loop of a register

align 16
LoopColumns2MidAllSSE2:
  movdqa  xmm2,xmm0
  psrld   xmm2,25     ; the texels of the two columns
  paddd   xmm0,xmm1   ; the positions of the columns of the row to come
  packssdw xmm2,xmm2
  packuswb xmm2,xmm2  ; the two texels of them, one in each byte

  movd    eax,xmm2
  movzx   ecx,al      ; the texel of the first column
  movzx   edx,ah      ; ... of the second one

  mov     edi,[c2asrc]
  movzx   ecx,byte [edi+ecx] ; the byte of the source of the first column
  mov     edi,[c2amap] ; the light table of the column, of its low byte zero
  movzx   ecx,byte [edi+ecx] ; its pixel of the light table

  mov     edi,[c2asrc+4]
  movzx   edx,byte [edi+edx]
  mov     edi,[c2amap+4]
  movzx   edx,byte [edi+edx] ; its pixel of the light table

  ; the two pixels of the row, of the byte of every one of them in the two
  ; bytes of it, the column of the left of the two bytes of the low, and
  ; written to the row of the view with one store of 32 bits, the row of the
  ; view being of the four bytes of the two columns of the group kept, the
  ; group of the columns kept being of the alignment of a row of it.  The shift
  ; of the column of the right to its two bytes of them and the two "or" of
  ; them are of their own, the register of the pixel of the column of the left
  ; being the last written and the first read of them.
  mov     esi,ecx
  shl     ecx,8
  or      ecx,esi
  shl     edx,16
  mov     eax,edx
  shl     eax,8
  or      edx,eax
  or      ecx,edx
  mov     [ebx],ecx   ; the four bytes of the row of the view

  add     ebx,SCREENWIDTH ; the row to come

  dec     ebp       ; the rows of the group left
  jns     near LoopColumns2MidAllSSE2

done2allsse2:
  pop  ebp
  pop  edi
  pop  esi
  pop  ebx
  ret
; R_DrawColumns2MidAllSSE2 ends

%endif
