;
; Copyright (C) 1993-1996 Id Software, Inc.
; Copyright (C) 1993-2008 Raven Software
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
; DESCRIPTION: Assembly texture mapping routines for linear VGA mode
;

BITS 32
%include "macros.inc"

%ifdef USE_BACKBUFFER
%include "defs.inc"

extern _centery

;============================================================================
; unwound vertical scaling code
;
; eax   light table pointer, 0 lowbyte overwritten
; ebx   all 0, low byte overwritten
; ecx   fractional step value
; edx   fractional scale value
; esi   start of source pixels
; edi   bottom pixel in screenbuffer to blit into
;
; ebx should be set to 0 0 0 dh to feed the pipeline
;
; The graphics wrap vertically at 128 pixels
;============================================================================

BEGIN_DATA_SECTION

%macro SCALEDEFINE 1
  dd vscale%1
%endmacro

align 4

scalecalls:
  %assign LINE 0
  %rep SCREENHEIGHT+1
    SCALEDEFINE LINE
  %assign LINE LINE+1
  %endrep

; The rows of the column left to the loop of four of them, which is not of a
; register of the machine, of the five registers of them which the loop keeps
; being the source, the destination, the light table and the four texels of it
align 4
columnRowsLowSSE2: dd 0

BEGIN_CODE_SECTION

CODE_SYM_DEF R_PatchLinearLow
  push  ebx

  mov   ebx,[_centery]
  mov   eax,patchCentery+1
  mov   [eax],ebx
  mov   eax,patchCenteryMMX+1
  mov   [eax],ebx
  mov   eax,patchCenterySSE2+1
  mov   [eax],ebx

  mov   ebx,[_columnofs]
  mov   eax,patchColumnofs+2
  mov   [eax],ebx
  mov   eax,patchColumnofsLowMMX+3
  mov   [eax],ebx
  mov   eax,patchColumnofsLowSSE2+3
  mov   [eax],ebx

  pop   ebx
  ret

; =========================
; R_DrawColumnLowBackbuffer
; =========================
CODE_SYM_DEF R_DrawColumnLowBackbuffer
	push		edi
	push		esi
	push		edx
	push		ebp
  push		ebx
	push		ecx

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   short donel

  mov  ebx,[_dc_x]
  mov  ecx,[_dc_iscale]
  add  edi,[_columnofs+ebx*4]

patchCentery:
  sub   eax,0x12345678
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

donel:
	pop		ecx
	pop		ebx
  pop	  ebp
	pop		edx
	pop		esi
	pop		edi
  ret
; R_DrawColumnLowBackbuffer ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    mov  ebp,edx
    shr  ebp,25
    mov  al,[esi+ebp]                   ; get source pixel
    add  edx,ecx                        ; calculate next location
    mov  bl,[eax]                       ; translate the color
    mov  bh,bl
    mov  [edi-(LINE-1)*SCREENWIDTH],bx  ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  mov  ebp,edx
  pop	 ecx
  shr  ebp,25
  pop	ebx
  mov al,[esi+ebp]
  pop	ebp
  mov al,[eax]
  pop	edx
  mov ah, al
  pop	esi
  mov [edi],ax

vscale0:
	pop		edi
  ret

; ==============================
;
; R_DrawColumnLowBackbufferMMX
;
; The column of the loop of the registers of MMX of the file of the detail of
; one pixel of each row, of two rows of them in each one of the iterations of
; the loop, the one of them in each one of the two dwords of the register of
; their positions, walked to the rows to come with one "paddd" of the step of
; a row, the two of them broadcasted to the two dwords of the register of it.
;
; The two pixels of a row of the detail of the file, which are of the byte of
; the same one of them, go to the view with one store of 16 bits, of the byte
; of the table of the light being copied to the high byte of the register of
; the store, as of the renderer of the mode of the file of the same name.
;
; The texels of the two rows go out of their positions with one "psrld", of
; the seven bits of them only, the graphics wrapping vertically at 128 pixels.
; The texel of the low dword of the positions goes to the registers of the
; machine with one "movd" and the texel of the high one with one "psrlq" and
; one "movd" of them.
;
; The byte of the source of every one of them names its pixel of the light
; table with the address of it, the table being of 256 bytes, which the
; renderers of the mode of the file name with their low byte written over.
; The two pixels of a row go to the registers of the texels of them, of the
; texels being read already, for the bytes of the high of the registers of the
; tables of the light, the ones of their addresses, not to be broken by the
; byte of the pixel of the row of the store of 16 bits of them. The rows of
; them are of two registers of the table, the one of the row of the register
; of it and the one of the row to come of the copy of it.
;
; The row of the column which is not a pair of them of the loop goes before
; it, of the registers of the machine, of the rows of the column being an odd
; number of them, and of one row only of the column the row of it is of a loop
; of none of them, as of the renderer of the mode of the file.
;
; eax   the table of the light, of its low byte the texel of a row
; ebx   the texel of the low dword of the positions, and its pixel of it
; ecx   the copy of the table of the light, of its low byte the texel of the
;       row to come
; edx   the texel of the high dword of the positions, and its pixel of it
; esi   the source of the column
; edi   the pixel of the row of the view
; ebp   the rows of the column left, the loop of a register
;
; mm0   the positions of the two rows, one of them in each one of them
; mm1   the step of the two of them, of both the dwords of it
; mm3   the texels of the two rows, of the packing of their positions
;
; "emms" is not of the renderers of the project, which are of no x87 code of
; their own, and this renderer is of the same kind.
; ==============================
CODE_SYM_DEF R_DrawColumnLowBackbufferMMX
  push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  mov  edi,[_ylookup+eax*4]
  sub  ebp,eax         ; ebp = the rows of the column less one
  js   near donelmmx

  mov  ebx,[_dc_x]
  mov  ecx,[_dc_iscale]
  add  edi,[_columnofs+ebx*4]

patchCenteryMMX:
  sub   eax,0x12345678
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  cmp   ebp,0
  je    near SinglePixelLowMMX

  test  ebp,1
  jnz   near EvenLowMMX

  ; the row of the column which is not a pair of them of the loop, of the
  ; rows of the column being an odd number of them

  mov   ebx,edx
  shr   ebx,25 ; get address of first location
  mov   al,[esi+ebx]                   ; get source pixel
  add   edx,ecx                        ; the position of the rows to come
  mov   bl,[eax]                       ; translate the color, the register of
  dec   ebp                            ;  the table being kept for the loop
  mov   bh,bl                          ; the two pixels of the row, of the
  mov   [edi],bx                       ;  byte of the table in the two bytes

  add   edi,SCREENWIDTH

  ; MMX 2 pixels render
EvenLowMMX:

  movd       mm0, edx

  movd       mm1, ecx

  mov        ecx, eax

  paddd      mm0, mm1

  movd       mm2, edx

  psllq      mm0, 32

  punpckldq  mm1, mm1

  por        mm0, mm2

  paddd      mm1, mm1

  movq       mm3, mm0

  psrld      mm3, 25

LoopLowMMX:

  movd       ebx, mm3         ; the texel of the row

  paddd      mm0, mm1         ; the positions of the two rows to come

  psrlq      mm3, 32

  mov  al,[ebx+esi]           ; the source pixel of the row

  movd       edx, mm3         ; the texel of the row to come

  movq       mm3, mm0         ; the texels of the two rows of the next one

  mov  bl,[eax]               ; its pixel of the table of the light, the byte
                              ;  of the pixel going to the register of it
  mov  cl,[edx+esi]           ; the source pixel of the row to come, its
                              ;  texel being of the copy of the table

  mov  bh,bl
  mov  [edi],bx

  mov  dl,[ecx]               ; its pixel of the table of the light

  psrld      mm3, 25

  mov  dh,dl
  mov  [edi+SCREENWIDTH],dx

  add  edi, 2*SCREENWIDTH

  sub  ebp, 2
  jns  LoopLowMMX

donelmmx:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnLowBackbufferMMX ends

SinglePixelLowMMX:

  shr  edx,25 ; get address of first location
  pop		ebp
  mov  al,[esi+edx]                   ; get source pixel
  pop		esi
  pop		edx
  mov  al,[eax]                       ; translate the color
  pop		ecx
	pop		ebx
  mov  ah,al
  mov  [edi],ax                       ; the two pixels of the row
  pop		edi
  ret

; =========================
;
; R_DrawColumnLowBackbufferSSE2
;
; Four rows per iteration of the loop, of the positions of them in the four
; dwords of one register of SSE2, walked with one "paddd" of the step of a row
; broadcasted to the four dwords of another one.
;
; The texels of the four of them are out of the positions with one "psrld",
; joined in the four bytes of one dword with one "packssdw" and one
; "packuswb", of the texels being of seven bits only, the graphics wrapping
; vertically at 128 pixels, and gotten to the registers of the machine with
; one "movd", three "movzx" and one "shr".  The byte of the source of every
; one of them names its pixel of the light table with the address of it, the
; table being of 256 bytes, the register of it being of the "and" of it with
; the value of them, for the byte of the source naming its pixel of the table
; being of one "movzx" of them.
;
; The two pixels of a row of the detail of the file, which are of the byte of
; the same one of them, go to the view with one store of 16 bits, the byte of
; the pixel of the light table being multiplied by the value of the two bytes
; of one of them, 1.01 hex, with one "imul" of the registers of the machine,
; as the renderer of the mode of the file writes the two of them with the byte
; of the table in the two bytes of the register of them.
;
; The rows of the column which are not a full group of four of them are
; rendered one by one with a scalar loop at the end, of their positions in the
; dwords of the register which the loop of the groups leaves, the last group
; of them having stepped to the rows of them.
;
; eax   the rows of the column and a register of the texels
; ebx   a register of the texels
; ecx   a register of the texels
; edx   a register of the texels
; esi   the source of the column
; edi   the pixel of the row of the view
; ebp   the light table, of its low byte zero
;
; xmm0  the positions of the four rows, one of them in each dword
; xmm1  the step of a row, of the four dwords of it
; xmm2  the positions of the four rows, of their texels in the bytes of them
;
; The registers of SSE2 are not of the ones of the registers of MMX, so one
; "emms" is not needed of the return of this renderer, as of the renderers of
; the project which are of the instructions of SSE2.
; =========================
CODE_SYM_DEF R_DrawColumnLowBackbufferSSE2
  push    edi
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    ebp

  mov     ebp,[_dc_yh]
  mov     eax,[_dc_yl]
  mov     edi,[_ylookup+eax*4]
  sub     ebp,eax         ; ebp = pixels of the column less one
  js      near donels     ; a column of no rows
  inc     ebp             ; ebp = the rows of the column

  mov     ebx,[_dc_x]
  mov     ecx,[_dc_iscale]
  add     edi,[_columnofs+ebx*4]

patchCenterySSE2:
  sub     eax,0x12345678
  imul    ecx
  mov     edx,[_dc_texturemid]
  shl     ecx,9 ; 7 significant bits, 25 frac
  add     edx,eax
  mov     esi,[_dc_source]
  shl     edx,9 ; 7 significant bits, 25 frac
  mov     eax,[_dc_colormap]
  and     eax,-256 ; the tables of the game are of 256 bytes, as the byte of
                   ; the source naming its pixel of the table is of one "movzx"
  xchg    eax,ebp ; the light table of the column in ebp, the rows in eax

  mov     ebx,eax
  and     eax,3 ; the rows of the column which are not a full group of four
  shr     ebx,2 ; groups of four rows
  dec     ebx   ; iterations of the loop less one
  mov     [columnRowsLowSSE2],ebx
  push    eax   ; the rows of the column left for the scalar loop

  ; xmm0 has the positions of the first group of four rows, one of them in
  ; each dword of it, and xmm1 the step of a row in all of the four of them

  movd    xmm0,edx          ; the position of the first row
  lea     eax,[edx+ecx]
  movd    xmm1,eax          ; ... of the second one
  lea     eax,[eax+ecx]
  movd    xmm2,eax          ; ... of the third one
  lea     eax,[eax+ecx]
  movd    xmm3,eax          ; ... of the fourth one
  shufps  xmm0,xmm1,0       ; the two first positions of them
  shufps  xmm2,xmm3,0       ; the two last positions of them
  shufps  xmm0,xmm2,0x88    ; the four positions of them
  lea     eax,[ecx*4]       ; the step of four rows
  movd    xmm1,eax
  shufps  xmm1,xmm1,0       ; it broadcasted to the four dwords

  ; the instructions of them above do not modify the flags, so the sign of
  ; the iterations of the loop less one calculated with them is still in them

  js      near TailLowSSE2 ; there are not groups of four rows

align 16
LoopLowSSE2:
  movdqa  xmm2,xmm0
  psrld   xmm2,25             ; the texels of the four rows
  paddd   xmm0,xmm1           ; the positions of the four rows to come
  packssdw xmm2,xmm2
  packuswb xmm2,xmm2          ; the four texels of them, one in each byte

  movd    eax,xmm2
  movzx   edx,al              ; the texel of the first row
  movzx   ebx,ah              ; ... of the second one
  shr     eax,16
  movzx   ecx,al              ; ... of the third one
  movzx   eax,ah              ; ... of the fourth one

  movzx   edx,byte [esi+edx]  ; the byte of the source of the first row
  movzx   edx,byte [ebp+edx]  ; its pixel of the light table
  imul    edx,edx,0x0101      ; the two bytes of the pixel of it
  mov     [edi],dx            ; the two pixels of the row

  movzx   ebx,byte [esi+ebx]  ; the byte of the source of the second row
  movzx   ebx,byte [ebp+ebx]  ; its pixel of the light table
  imul    ebx,ebx,0x0101      ; the two bytes of the pixel of it
  mov     [edi+SCREENWIDTH],bx

  movzx   ecx,byte [esi+ecx]  ; the byte of the source of the third row
  movzx   ecx,byte [ebp+ecx]  ; its pixel of the light table
  imul    ecx,ecx,0x0101      ; the two bytes of the pixel of it
  mov     [edi+2*SCREENWIDTH],cx

  movzx   eax,byte [esi+eax]  ; the byte of the source of the fourth row
  movzx   eax,byte [ebp+eax]  ; its pixel of the light table
  imul    eax,eax,0x0101      ; the two bytes of the pixel of it
  mov     [edi+3*SCREENWIDTH],ax

  add     edi,4*SCREENWIDTH

  dec     dword [columnRowsLowSSE2]
  jns     near LoopLowSSE2

  ; the rows of the column which are not a full group of four of them, one by
  ; one, with the positions of them in the dwords of xmm0

TailLowSSE2:
  pop     ecx                 ; the rows which are not a full group of four
  test    ecx,ecx
  je      near EndLowSSE2

  ; the source is in esi, the destination of the pixels in edi and the light
  ; table in ebp

align 16
LoopTailLowSSE2:
  movd    edx,xmm0            ; the position of the row
  psrldq  xmm0,4              ; the positions of the rows to come
  shr     edx,25              ; the texel of the row
  movzx   edx,byte [esi+edx]  ; the byte of the source
  movzx   edx,byte [ebp+edx]  ; its pixel of the light table
  imul    edx,edx,0x0101      ; the two bytes of the pixel of it
  mov     [edi],dx            ; the two pixels of the row
  add     edi,SCREENWIDTH

  dec     ecx
  jnz     near LoopTailLowSSE2

EndLowSSE2:
  pop     ebp
  pop     esi
  pop     edx
  pop     ecx
  pop     ebx
  pop     edi
  ret

  ; a column of no rows, of the rows of it being in the sign of the flags

donels:
  pop     ebp
  pop     esi
  pop     edx
  pop     ecx
  pop     ebx
  pop     edi
  ret
; R_DrawColumnLowBackbufferSSE2 ends

; ==============================
;
; R_DrawSpanLowBackbufferMMX
;
; The span of the loop of the registers of MMX of the file of the detail of
; two pixels of the row, of the position of one of them in each one of the two
; dwords of the register of them, walked to the pixels to come with one
; "paddd" of the step of a pixel, the step being broadcasted to the two dwords
; of the register of it, the position of them being of the sixteen bits of the
; horizontal of them and of the sixteen bits of the vertical.
;
; The four pixels of the two texels of them go to the buffer of the view with
; one store of 32 bits, of the two pixels of every one of the texels being the
; byte of the pixel of the table of the light in the two bytes of them, as the
; renderer of the mode of the file of the same name writes the two of them
; with one store of 16 bits of them. The bytes of the two texels, the one of
; the pixel of them and the one of the pixel to come, are packed in the two
; bytes of the low of one register of MMX, of the four pixels of them being
; gotten from them with one "punpcklbw" of the register of itself.
;
; The texels of the two positions go out of them with one "psrld" of the six
; bits of the vertical of them and with one of the six bits of the horizontal,
; the graphics wrapping at 64 pixels in the two directions of them, the mask of
; the horizontal being of the value of them of the register of MMX, the texels
; of the two positions to come being calculated of the positions of them.
;
; The byte of the source of every one of them names its pixel of the light
; table with the address of it, the table being of 256 bytes, which the
; renderers of the mode of the file name with their low byte written over. The
; two texels of the pair of them are of two registers of the table, the one of
; the pixel of the register of it and the one of the pixel to come of the copy
; of it, the bytes of the high of the two of them not being broken by the byte
; of the pixel of them.
;
; The pixel of the span which is not a pair of them of the loop goes before
; it, of the registers of the machine, the texels of the span being an odd
; number of them, its two pixels being of one copy of the register of the
; table of the light, the register of the pixel of them being kept whole of
; the byte of the pixel of the table of the light of it. And of one texel only
; of the span the pixel of it is of a loop of none of them, as of the renderer
; of the mode of the file.
;
; eax   the table of the light, of its low byte the texel of a pixel
; ebx   the copy of the table of the light, of its low byte the texel of the
;       pixel to come
; ecx   the texel of the pixel of the pair of them, of the registers of MMX
; edx   the two pixels of the two texels of them, and the mask of them, and of
;       the pixel which is not a pair of them the copy of the table of the
;       light of it
; esi   the source of the span
; edi   the pixel of the row of the view
; ebp   the texels of the span left, the loop of a register
;
; mm0   the positions of the two pixels, one of them in each one of them
; mm1   the step of the two of them, of both the dwords of it
; mm2   the texels of the two of them, of the packing of their positions
; mm3   the packing of the positions of the pixels to come
; mm4   the mask of the horizontal component of the positions
; mm5   the four pixels of the two texels of them
;
; "emms" is not of the renderers of the project, which are of no x87 code of
; their own, and this renderer is of the same kind.
; ==============================
CODE_SYM_DEF R_DrawSpanLowBackbufferMMX
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov     eax,[_ds_x1]
  mov     ebp,[_ds_x2]
  mov     ecx,[_ds_frac]        ; the position of the texel of the span
  mov     esi,[_ds_source]
  mov     edi,[_ds_y]

  sub     ebp,eax               ; the texels of the span less one

  mov     edi,[_ylookup+edi*4]

  mov     edx,ecx
  mov     ebx,ecx
  shr     edx,4
  shr     ebx,26
  and     edx,0xFC0
patchColumnofsLowMMX:
  lea     edi,[edi+eax*2+0x12345678] ; the pixel of the row of the view, the
                                     ;  table of the columns being of two of
                                     ;  them in the detail of the low
  or      ebx,edx               ; the texel of the source of the span

  mov     edx,[_ds_step]        ; the step of the position of a pixel
  mov     eax,[_ds_colormap]

  cmp     ebp,0
  je      near SingleTexelLowMMX

  test    ebp,1
  jnz     near EvenSpanLowMMX

  ; the pixel of the span which is not a pair of them of the loop, of the
  ; texels of the span being an odd number of them

  mov     al,[esi+ebx]          ; the byte of the source of the texel
  dec     ebp
  mov     al,[eax]              ; its pixel of the table of the light
  mov     ebx,eax               ; the copy of the register of the table of the
                                ;  light, its low byte being of the pixel of
                                ;  the table of the light, the byte of the
                                ;  pixel of the register of it being kept
  mov     bh,al                 ; the two pixels of it
  mov     [edi],bx

  add     edi,2

  add     ecx,edx               ; the position of the texel which the loop of
                                ;  the two of them renders next

  ; MMX 4 pixels render
EvenSpanLowMMX:

  mov     ebx,eax               ; the table of the light of the pixel to come

  movd    mm0,ecx               ; the position of the texel of the pixel
  movd    mm1,edx               ; its step
  movq    mm2,mm0
  paddd   mm0,mm1               ; the position of the texel to come
  mov     edx,0x00000FC0
  psllq   mm0,32
  movd    mm4,edx
  por     mm0,mm2               ; the positions of the two pixels of them
  punpckldq mm1,mm1
  movq    mm3,mm0
  movq    mm2,mm0
  psrld   mm3,4
  psrld   mm2,26
  punpckldq mm4,mm4
  pand    mm3,mm4
  por     mm2,mm3               ; the texels of the two positions of them
  paddd   mm1,mm1               ; the step of the two positions to come

align 4
LoopSpanLowMMX:

  movd    ecx,mm2               ; the texel of the source of the pixel
  psrlq   mm2,32
  mov     al,[esi+ecx]          ; its byte of the source
  movd    ecx,mm2               ; the texel of the source of the pixel to come
  paddd   mm0,mm1               ; the positions of the two pixels to come
  mov     bl,[esi+ecx]          ; its byte of the source
  movq    mm3,mm0
  movq    mm2,mm0
  mov     dl,[eax]              ; its pixel of the table of the light
  psrld   mm3,4
  psrld   mm2,26
  mov     dh,[ebx]              ; the pixel of the table of the light of it
  pand    mm3,mm4
  movd    mm5,edx
  punpcklbw mm5,mm5             ; the four pixels of the two texels of them
  por     mm2,mm3               ; the texels of the two positions to come
  movd    ecx,mm5
  mov     [edi],ecx             ; the four pixels of them to the view
  add     edi,4

  sub     ebp,2
  jns     near LoopSpanLowMMX

	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanLowBackbufferMMX ends

SingleTexelLowMMX:

  mov     al,[esi+ebx]          ; the byte of the source of the texel
  mov     al,[eax]              ; its pixel of the table of the light
  mov     ah,al
  mov     [edi],ax              ; the two pixels of it to the view

	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
;
; eax   lighttable
; ebx   scratch register
; ecx   position 6.10 bits x, 6.10 bits y
; edx   step 6.10 bits x, 6.10 bits y
; esi   start of block
; edi   dest
; ebp   fff to mask bx
;
; ebp should by preset from ebx / ecx before calling
;============================================================================

CONTINUE_DATA_SECTION

%macro MAPDEFINE 1
  dd hmap%1
%endmacro

align 4

mapcalls:
  %assign LINE 0
  %rep SCREENWIDTH/2+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0
returnpoint: dd 0

CONTINUE_CODE_SECTION

; =======================
; R_DrawSpanLowBackbuffer
; =======================
CODE_SYM_DEF R_DrawSpanLowBackbuffer
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov     eax,[_ds_x1]
  mov     ebx,[_ds_x2]
  mov     eax,[mapcalls+eax*4]
  mov     ecx,[_ds_frac]        ; build composite position
  mov     [callpoint],eax       ; spot to jump into unwound
  mov     ebp,[_ds_step]        ; build composite step
  mov     eax,[mapcalls+4+ebx*4]
  mov     esi,[_ds_source]
  mov     [returnpoint],eax     ; spot to patch a ret at
  mov     edi,[_ds_y]
  mov     [eax], byte OP_RET

  mov     edi,[_ylookup+edi*4]

  ; feed the pipeline and jump in

  shld    ebx,ecx,22  ; shift y units in
  mov     eax,[_ds_colormap]
  shld    ebx,ecx,6   ; shift x units in

patchColumnofs:
  add     edi,0x12345678

  and     ebx,0x0FFF  ; mask off slop bits
  add     ecx,ebp
  call    [callpoint]

  mov     ebx,[returnpoint]
	pop		ebp
  mov     [ebx],byte OP_MOVAL         ; remove the ret patched in

	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanLowBackbuffer ends

; ====================================================
;
; R_DrawSpanLowBackbufferSSE2
;
; The span of the loop of the registers of SSE2 of the file of the detail of
; the low, of four texels of them in each one of the iterations of the loop,
; their positions in the four dwords of one register of them, walked to the
; texels to come with one "paddd" of the step of a texel multiplied by four of
; them, the step of it being broadcasted to the four dwords of its own register
; with one "shufps" of them.
;
; The texels of the four texels of them go out of their positions with one
; "psrld" of the six bits of the horizontal of them and with one "psrld" of
; the six bits of the vertical, the graphics wrapping at 64 pixels in the two
; directions of them, the mask of the vertical being of the value of them of
; one register of SSE2 of its own, the texel of every one of the dwords of the
; positions going to the registers of the machine with one "movd" of them and
; the dwords of them to come with one "psrldq" of them, of the four of the
; loads of the four bytes of the source being of the registers of the machine,
; no texel of them being of a load of a table of the registers of SSE2.
;
; The byte of the source of every one of them names its pixel of the light
; table with the address of it, the table being of 256 bytes, which the
; renderers of the modes of the file name with their low byte written over,
; the four pixels of the table of the light of them being joined in the four
; bytes of one dword of them, two of them of the low half of it and the other
; two of the high one, as of the span of the detail of the file of the
; registers of SSE2 of the file.
;
; The eight pixels of the four texels of them, the two pixels of every one of
; them being of the byte of the same one of them, go to the view with one store
; of 64 bits, the four bytes of the four pixels of the table of the light of
; them being spread to their two pixels of them with one "punpcklbw" of the
; register of itself, as the renderer of the mode of the file writes the two
; pixels of a texel of them with one store of 16 bits of it, as the span of the
; detail of the low of the registers of MMX of the file spreads the two bytes
; of them to the four pixels of them and as the column of the detail of the low
; of the registers of SSE2 of the file multiplies its byte of the table of the
; light by the value of 1.01 hex of them with one "imul" of the registers of
; the machine.
;
; The texels of the span which are not a full group of four of them are
; rendered one by one with a loop of the scalar of them at the end, of their
; positions in the dwords of the register which the loop of the groups leaves,
; the last group of them having stepped to the texels of them, every one of
; them writing its two pixels with one store of 16 bits of them, of the
; "imul" of the byte of its pixel of the table of the light by the value of
; them, as of the column of the detail of the low of the registers of SSE2 of
; the file.
;
; eax   the base of the table of the light, of its low byte the pixel of a
;       texel
; ebx   the copy of it, of its low byte the pixel of the texel to come, and a
;       register of the positions of them
; ecx   the texels of them of the group of four of them, and the register of
;       the two pixels of them
; edx   the step of the position of a texel, the register of the two pixels of
;       them to come, and a register of the positions of them
; esi   the source of the span
; edi   the pixel of the row of the view
; ebp   the groups of four texels of the span left, the loop of a register
;
; xmm0  the positions of the four texels, one of them in each dword
; xmm1  the step of a texel multiplied by four, of the four dwords of it
; xmm2  the positions of the four texels, of their texels in the dwords of them
; xmm3  the positions of them of the packing of their texels
; xmm4  the mask of the vertical of the positions, of the four dwords of it
; xmm5  the eight pixels of the four texels of them
;
; The registers of SSE2 are not of the ones of the registers of MMX, so one
; "emms" is not needed of the return of this renderer, as of the renderers of
; the modes of the file of the instructions of SSE2.
; ====================================================
CODE_SYM_DEF R_DrawSpanLowBackbufferSSE2
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov     eax,[_ds_x1]
  mov     ebp,[_ds_x2]
  mov     ecx,[_ds_frac]        ; the position of the texel of the span
  mov     esi,[_ds_source]
  mov     edi,[_ds_y]

  sub     ebp,eax               ; the texels of the span less one

  mov     edi,[_ylookup+edi*4]

patchColumnofsLowSSE2:
  lea     edi,[edi+eax*2+0x12345678] ; the pixel of the row of the view, the
                                     ;  table of the columns being of two of
                                     ;  them in the detail of the low

  mov     edx,[_ds_step]        ; the step of the position of a texel

  inc     ebp                   ; the texels of the span
  mov     ebx,ebp
  and     ebx,3                 ; the texels of them which are not a full
  push    ebx                   ;  group of four of them, them kept in the
  shr     ebp,2                 ;  stack for the loop of the scalar of them
  dec     ebp                   ; the iterations of the loop less one

  ; xmm0 has the four positions of the loop, one in each dword of them,
  ; and xmm1 has the step of the position multiplied by four in all of
  ; them, with xmm4 having the mask of the vertical component

  movd    xmm0,ecx              ; the position of the first texel
  lea     eax,[ecx+edx]         ; the position of the second one of them
  movd    xmm1,eax
  lea     eax,[eax+edx]         ; the position of the third one of them
  movd    xmm2,eax
  lea     eax,[eax+edx]         ; the position of the fourth one of them
  movd    xmm3,eax
  shufps  xmm0,xmm1,0           ; the first two positions of them
  shufps  xmm2,xmm3,0           ; the last two positions of them
  shufps  xmm0,xmm2,0x88        ; the four positions of them
  lea     eax,[edx*4]           ; the step of the position multiplied by four
  movd    xmm1,eax
  shufps  xmm1,xmm1,0           ; it broadcasted to the four dwords
  mov     edx,0xFC0
  movd    xmm4,edx
  shufps  xmm4,xmm4,0           ; the mask of the vertical component

  mov     eax,[_ds_colormap]    ; the base of the table of the light
  mov     ebx,eax               ; the base of it for the pair of the texels

  ; the instructions of them above do not modify the flags, so the sign
  ; of the iterations of the loop less one calculated with them is still
  ; in them

  js    TailSpanLowSSE2         ; there are not groups of four texels

LoopSpanLowSSE2:
  movdqa  xmm2,xmm0
  psrld   xmm2,26               ; the horizontal components of the positions
  movdqa  xmm3,xmm0
  psrld   xmm3,4
  pand    xmm3,xmm4             ; the vertical components of the positions
  por     xmm2,xmm3             ; the texels of the source of the four pixels
  paddd   xmm0,xmm1             ; the positions of the four texels to come

  movd    ecx,xmm2              ; the texel of the source of the first pixel
  psrldq  xmm2,4
  movd    edx,xmm2              ; the texel of the source of the second one

  ; the bytes of the source of them are gotten with the low byte of the base
  ; of the table of the light in the eax and ebx registers of them, the two
  ; pixels of them of the table being kept in xmm5 while the texels of the two
  ; of them to come are gotten

  mov     al,[esi+ecx]          ; the byte of the source of the first pixel
  mov     bl,[esi+edx]          ; the byte of the source of the second one
  mov     cl,[eax]              ; its pixel of the table of the light
  mov     ch,[ebx]              ; the pixel of the table of the light of it
  movd    xmm5,ecx              ; the two pixels of them of the table

  psrldq  xmm2,4
  movd    ecx,xmm2              ; the texel of the source of the third pixel
  psrldq  xmm2,4
  movd    edx,xmm2              ; the texel of the source of the fourth one

  mov     al,[esi+ecx]          ; the byte of the source of the third pixel
  mov     bl,[esi+edx]          ; the byte of the source of the fourth one
  mov     dl,[eax]              ; its pixel of the table of the light
  mov     dh,[ebx]              ; the pixel of the table of the light of it

  ; the four pixels of them of the table of the light joined in a dword, two
  ; of them in the low half of it and the other two in the high one of it

  movd    ecx,xmm5              ; the first two pixels of them of it
  movzx   ecx,cx
  shl     edx,16                ; the last two pixels of them of it
  or      edx,ecx

  movd    xmm5,edx
  punpcklbw xmm5,xmm5           ; the two pixels of the four bytes of them
  movq    [edi],xmm5            ; the eight pixels of them to the view
  add     edi,8

  dec     ebp
  jns   LoopSpanLowSSE2

  ; the texels of the span which are not a full group of four of them, one by
  ; one, with their positions in the dwords of the register of them of them

TailSpanLowSSE2:
  pop   ecx                     ; the texels which are not a full group of four

  test  ecx,ecx
  je    EndSpanLowSSE2

  ; the base of the table of the light is in eax, the source is in esi and the
  ; destination of the pixels is in edi

LoopTailSpanLowSSE2:
  movd  edx,xmm0                ; the position of the texel
  psrldq xmm0,4                 ; the positions of the texels following it
  mov   ebx,edx
  shr   edx,26                  ; the component of the horizontal of it
  shr   ebx,4
  and   ebx,0xFC0               ; the component of the vertical of it
  or    edx,ebx                 ; the texel of the source of the span

  mov   al,[esi+edx]            ; the byte of the source of the texel
  mov   al,[eax]                ; its pixel of the table of the light
  movzx ebx,al                  ; the pixel of it
  imul  ebx,ebx,0x0101          ; the two pixels of it
  mov   [edi],bx                ; the two pixels of it to the view
  add   edi,2

  dec ecx
  jnz LoopTailSpanLowSSE2

EndSpanLowSSE2:
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanLowBackbufferSSE2 ends

%macro MAPLABEL 1
  hmap%1
%endmacro

%assign LINE 0
%assign PCOL 0
%rep SCREENWIDTH/2
  %assign PLANE 0
    MAPLABEL LINE:
      %assign LINE LINE+1
      %if LINE = SCREENWIDTH/2
        mov   al,[esi+ebx]           ; get source pixel
        mov   al,[eax]               ; translate color
        mov   ah,al
        mov   [edi+PLANE+PCOL*2],ax  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        shld  ebx,ecx,22             ; shift y units in
        mov   dl,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   dh,dl
        mov   [edi+PLANE+PCOL*2],dx  ; write pixel
        and   ebx,0x0FFF             ; mask off slop bits
        %if LINE < (SCREENWIDTH/2)-1
        add   ecx,ebp                ; position += step
        %endif
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

MAPLABEL LINE:
  ret

%endif
