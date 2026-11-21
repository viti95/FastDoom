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
columnRowsSSE2: dd 0

BEGIN_CODE_SECTION

CODE_SYM_DEF R_PatchLinearHigh
  push  ebx
  
  mov   ebx,[_centery]
  mov   eax,patchCentery+1
  mov   [eax],ebx
  mov   eax,patchCenteryRoll+1
  mov   [eax],ebx
  mov   eax,patchCenteryMMX+1
  mov   [eax],ebx
  mov   eax,patchCenterySSE2+1
  mov   [eax],ebx

  mov   ebx,[_columnofs]
  mov   eax,patchColumnofs+2
  mov   [eax],ebx
  mov   eax,patchColumnofsMMX+3
  mov   [eax],ebx
  mov   eax,patchColumnofsSSE2+3
  mov   [eax],ebx
  mov   eax,patchColumnofsRoll+3
  mov   [eax],ebx

  pop   ebx
  ret

; ======================
; R_DrawColumnBackbuffer
; ======================
CODE_SYM_DEF R_DrawColumnBackbuffer
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   short doneh

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

  mov  ebx,edx
  shr  ebx,25 ; get address of first location
  jmp  [scalecalls+4+ebp*4]

doneh:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnBackbuffer ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    add  edx,ecx                        ; calculate next location
    mov  al,[esi+ebx]                   ; get source pixel
    mov  ebx,edx
    mov  al,[eax]                       ; translate the color
    shr  ebx,25
    mov  [edi-(LINE-1)*SCREENWIDTH],al  ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  pop	ebp
  mov al,[esi+ebx]
  pop	esi
  mov al,[eax]
  pop	edx
  mov [edi],al

vscale0:
	pop		ecx
	pop		ebx
  pop		edi
  ret

;============================================================================
; unwound horizontal texture mapping code
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
  %rep SCREENWIDTH+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0
returnpoint: dd 0

CONTINUE_CODE_SECTION

; ====================
; R_DrawSpanBackbuffer
; ====================
CODE_SYM_DEF R_DrawSpanBackbuffer
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
  mov     edx,[_ds_step]        ; build composite step
  mov     eax,[mapcalls+4+ebx*4]
  mov     esi,[_ds_source]
  mov     [returnpoint],eax     ; spot to patch a ret at
  mov     edi,[_ds_y]
  mov     [eax], byte OP_RET

  mov     edi,[_ylookup+edi*4]
  mov     eax,[_ds_colormap]

patchColumnofs:
  add     edi,0x12345678

  ; feed the pipeline and jump in

  shld    ebx,ecx,22  ; shift y units in
  mov     ebp,0x0FFF  ; used to mask off slop high bits from position
  shld    ebx,ecx,6   ; shift x units in
  and     ebx,ebp     ; mask off slop bits
  add     ecx,edx
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
; R_DrawSpanBackbuffer ends

%macro MAPLABEL 1
  hmap%1
%endmacro

%assign LINE 0
%assign PCOL 0
%rep SCREENWIDTH
  %assign PLANE 0
    MAPLABEL LINE:
      %assign LINE LINE+1
      %if LINE = SCREENWIDTH
        mov   al,[esi+ebx]           ; get source pixel
        mov   al,[eax]               ; translate color
        mov   [edi+PLANE+PCOL],al  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        shld  ebx,ecx,22             ; shift y units in
        mov   al,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   [edi+PLANE+PCOL],al  ; write pixel
        and   ebx,ebp                ; mask off slop bits
        %if LINE < SCREENWIDTH-1
        add   ecx,edx                ; position += step
        %endif
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

MAPLABEL LINE:
  ret

CODE_SYM_DEF R_DrawColumnBackbufferRoll
  push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  mov  edi,[_ylookup+eax*4]
  sub  ebp,eax         ; ebp = pixel count
  js   donehr

  mov  ebx,[_dc_x]
  mov  ecx,[_dc_iscale]
  add  edi,[_columnofs+ebx*4]

patchCenteryRoll:
  sub   eax,0x12345678
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  mov  ebx,edx
  shr  ebx,25 ; get address of first location

LoopRoll:
  add  edx,ecx                        ; calculate next location
  mov  al,[esi+ebx]                   ; get source pixel
  mov  ebx,edx
  mov  al,[eax]                       ; translate the color
  shr  ebx,25
  mov  [edi],al  ; draw a pixel to the buffer
  add  edi, SCREENWIDTH
  dec  ebp
  jns  LoopRoll

donehr:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

CODE_SYM_DEF R_DrawSpanBackbufferRoll
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov     eax,[_ds_x1]
  mov     ebp,[_ds_x2]
  mov     ecx,[_ds_frac]        ; build composite position
  mov     esi,[_ds_source]
  mov     edi,[_ds_y]

  sub     ebp, eax              ; EBP tiene el numero de pixels

  mov     edi,[_ylookup+edi*4]
  
  shld  ebx,ecx,22
patchColumnofsRoll:
  lea   edi,[edi+eax+0x12345678]
  shld  ebx,ecx,6
  mov   eax,[_ds_colormap]
  and   ebx,0xFFF
  mov   edx,[_ds_step]        ; build composite step
  add   ecx,edx                ; position += step
  
LoopSpanRoll:
  mov   al,[esi+ebx]           ; get source pixel
  shld  ebx,ecx,22             ; shift y units in
  mov   al,[eax]               ; translate color
  shld  ebx,ecx,6              ; shift x units in
  mov   [edi],al               ; write pixel
  and   ebx,0xFFF              ; mask off slop bits
  add   ecx,edx                ; position += step
  inc   edi
  dec   ebp
  jns   LoopSpanRoll

  pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret

CODE_SYM_DEF R_DrawColumnBackbufferMMX
  push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  mov  edi,[_ylookup+eax*4]
  sub  ebp,eax         ; ebp = pixel count
  js   donehm

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

  cmp   ebp, 0
  je    SinglePixel

  test  ebp,1
  jnz   Even

   mov  ebx,edx
  shr  ebx,25 ; get address of first location
  mov  al,[esi+ebx]                   ; get source pixel
  add  edx,ecx
  mov  al,[eax]                       ; translate the color
  dec  ebp
  mov  [edi],al  ; draw a pixel to the buffer

  add  edi, SCREENWIDTH
  
  ; MMX 2 pixels render
Even:

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

LoopMMX:

  movd       ebx, mm3

  paddd      mm0, mm1

  psrlq      mm3,32

  mov  al,[ebx+esi]

  movd       edx, mm3

  movq       mm3, mm0

  mov  al,[eax]

  mov  cl,[edx+esi]

  mov  [edi], al

  mov  cl,[ecx]

  psrld      mm3, 25

  mov  [edi+SCREENWIDTH], cl

  add  edi, 2*SCREENWIDTH

  sub  ebp, 2
  jns  LoopMMX

donehm:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

SinglePixel:

  shr  edx,25 ; get address of first location
  pop		ebp
  mov  al,[esi+edx]                   ; get source pixel
  pop		esi
  pop		edx
  mov  al,[eax]                       ; translate the color
  pop		ecx
	pop		ebx
  mov  [edi],al  ; draw a pixel to the buffer
  pop		edi
  ret

; =========================
;
; R_DrawColumnBackbufferSSE2
;
; Four pixels per iteration of the loop, of the positions of the four rows of
; them in the four dwords of one register of SSE2, walked with one "paddd" of
; the step of a row broadcasted to the four dwords of another one.
;
; The texels of the four of them are out of the positions with one "psrld",
; joined in the four bytes of one dword with one "packssdw" and one
; "packuswb", of the texels being of seven bits only, the graphics wrapping
; vertically at 128 pixels, and gotten to the registers of the machine with
; one "movd", three "movzx" and one "shr".  The byte of the source of every
; one of them names its pixel of the light table with the address of it, the
; table being of 256 bytes, which the renderer of the mode of the file names
; with its low byte written over.
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
; =========================
CODE_SYM_DEF R_DrawColumnBackbufferSSE2
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
  js      near donehs     ; a column of no rows
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
  mov     [columnRowsSSE2],ebx
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

  js      near TailColumnSSE2 ; there are not groups of four rows

align 16
LoopColumnSSE2:
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
  mov     [edi],dl            ; the pixel of the row

  movzx   ebx,byte [esi+ebx]  ; the byte of the source of the second row
  movzx   ebx,byte [ebp+ebx]  ; its pixel of the light table
  mov     [edi+SCREENWIDTH],bl

  movzx   ecx,byte [esi+ecx]  ; the byte of the source of the third row
  movzx   ecx,byte [ebp+ecx]  ; its pixel of the light table
  mov     [edi+2*SCREENWIDTH],cl

  movzx   eax,byte [esi+eax]  ; the byte of the source of the fourth row
  movzx   eax,byte [ebp+eax]  ; its pixel of the light table
  mov     [edi+3*SCREENWIDTH],al

  add     edi,4*SCREENWIDTH

  dec     dword [columnRowsSSE2]
  jns     near LoopColumnSSE2

  ; the rows of the column which are not a full group of four of them, one by
  ; one, with the positions of them in the dwords of xmm0

TailColumnSSE2:
  pop     ecx                 ; the rows which are not a full group of four
  test    ecx,ecx
  je      near EndColumnSSE2

  ; the source is in esi, the destination of the pixels in edi and the light
  ; table in ebp

align 16
LoopTailColumnSSE2:
  movd    edx,xmm0            ; the position of the row
  psrldq  xmm0,4              ; the positions of the rows to come
  shr     edx,25              ; the texel of the row
  movzx   edx,byte [esi+edx]  ; the byte of the source
  movzx   edx,byte [ebp+edx]  ; its pixel of the light table
  mov     [edi],dl            ; the pixel of the row
  add     edi,SCREENWIDTH

  dec     ecx
  jnz     near LoopTailColumnSSE2

EndColumnSSE2:
  pop     ebp
  pop     esi
  pop     edx
  pop     ecx
  pop     ebx
  pop     edi
  ret

  ; a column of no rows, of the rows of it being in the sign of the flags

donehs:
  pop     ebp
  pop     esi
  pop     edx
  pop     ecx
  pop     ebx
  pop     edi
  ret
; R_DrawColumnBackbufferSSE2 ends

CODE_SYM_DEF R_DrawSpanBackbufferMMX
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov     eax,[_ds_x1]
  mov     ebp,[_ds_x2]
  mov     ecx,[_ds_frac]        ; build composite position
  mov     esi,[_ds_source]
  mov     edi,[_ds_y]

  sub     ebp, eax              ; EBP tiene el numero de pixels

  mov     edi,[_ylookup+edi*4]
  
  mov   edx,ecx
  mov   ebx,ecx
  shr   edx,4
  shr   ebx,26
  and   edx,0xFC0
patchColumnofsMMX:
  lea     edi,[edi+eax+0x12345678]
  or    ebx,edx

  mov   edx,[_ds_step]        ; build composite step
  mov   eax,[_ds_colormap]

  cmp   ebp, 0
  je    SinglePixelSpan

  test  ebp,1
  jnz   EvenSpan

  mov  al,[esi+ebx]                   ; get source pixel
  dec   ebp
  mov  al,[eax]
  mov  [edi],al

  inc   edi

  ; MMX 2 pixels render
EvenSpan:

  mov   ebx, eax

  movd  mm0, ecx
  movd  mm1, edx
  movq  mm2, mm0
  paddd mm0, mm1
  mov   edx, 0x00000FC0
  psllq mm0, 32
  movd  mm4, edx
  por   mm0, mm2
  punpckldq mm1, mm1
  movq  mm3, mm0
  movq  mm2, mm0
  psrld mm3, 4
  psrld mm2, 26
  punpckldq mm4, mm4
  pand  mm3, mm4
  por   mm2, mm3
  paddd mm1,mm1

LoopSpanMMX:

  movd  ecx, mm2
  psrlq mm2,32
  mov  al,[esi+ecx]                   ; get source pixel
  movd  ecx, mm2
  paddd mm0, mm1
  mov  bl,[esi+ecx]                   ; get source pixel
  movq  mm3, mm0
  movq  mm2, mm0
  mov  dl,[eax]
  psrld mm3, 4
  psrld mm2, 26
  mov  dh,[ebx]
  pand  mm3, mm4
  mov  [edi],dx
  por   mm2, mm3
  add  edi, 2

  sub  ebp, 2
  jns  LoopSpanMMX

	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanBackbuffer ends

SinglePixelSpan:

  mov  al,[esi+ebx]                   ; get source pixel
  pop		ebp
  mov  al,[eax]
  mov  [edi],al	
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret

; =========================
;
; R_DrawSpanBackbufferSSE2
;
; Four pixels per iteration of the loop, with the texture offsets of the
; four of them calculated at once with the SSE2 instructions and with
; the two colors of them joined and written to the backbuffer with 32
; bit writes.
;
; The pixels which are not a full group of four of them are rendered
; one by one with a scalar loop at the end.
; =========================
CODE_SYM_DEF R_DrawSpanBackbufferSSE2
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov     eax,[_ds_x1]
  mov     ebp,[_ds_x2]
  mov     ecx,[_ds_frac]        ; build composite position
  mov     esi,[_ds_source]
  mov     edi,[_ds_y]

  sub     ebp,eax               ; EBP has the number of pixels less one

  mov     edi,[_ylookup+edi*4]

  mov     edx,ecx
  mov     ebx,ecx
  shr     edx,4
  shr     ebx,26
  and     edx,0xFC0
patchColumnofsSSE2:
  lea     edi,[edi+eax+0x12345678]
  or      ebx,edx               ; texture offset of the first pixel

  mov     edx,[_ds_step]        ; build composite step

  inc     ebp                   ; EBP has the number of pixels
  mov     ebx,ebp
  and     ebx,3                 ; pixels which are not a full group of four
  push    ebx                   ; them kept in the stack for the scalar loop
  shr     ebp,2                 ; groups of four pixels
  dec     ebp                   ; iterations of the loop less one

  ; xmm0 has the four positions of the loop, one in each dword of them,
  ; and xmm1 has the step of the position multiplied by four in all of
  ; them, with xmm4 having the mask of the vertical component

  movd    xmm0,ecx              ; position of the first pixel
  lea     eax,[ecx+edx]         ; position of the second pixel
  movd    xmm1,eax
  lea     eax,[eax+edx]         ; position of the third pixel
  movd    xmm2,eax
  lea     eax,[eax+edx]         ; position of the fourth pixel
  movd    xmm3,eax
  shufps  xmm0,xmm1,0           ; the first two positions of them
  shufps  xmm2,xmm3,0           ; the last two positions of them
  shufps  xmm0,xmm2,0x88        ; the four positions of them
  lea     eax,[edx*4]           ; step of the position multiplied by four
  movd    xmm1,eax
  shufps  xmm1,xmm1,0           ; it broadcasted to the four dwords
  mov     edx,0xFC0
  movd    xmm4,edx
  shufps  xmm4,xmm4,0           ; mask of the vertical component

  mov     eax,[_ds_colormap]    ; base of the color table
  mov     ebx,eax               ; the base of the color table for the pair
                                ;  of source pixels of each two pixels

  ; the instructions of them above do not modify the flags, so the sign
  ; of the iterations of the loop less one calculated with them is still
  ; in them

  js    TailSpanSSE2            ; there are not groups of four pixels

LoopSpanSSE2:
  movdqa  xmm2,xmm0
  psrld   xmm2,26               ; horizontal components of the positions
  movdqa  xmm3,xmm0
  psrld   xmm3,4
  pand    xmm3,xmm4             ; vertical components of the positions
  por     xmm2,xmm3             ; texture offsets of the four pixels
  paddd   xmm0,xmm1             ; positions of the next four pixels

  movd    ecx,xmm2              ; offset of the first pixel
  psrldq  xmm2,4
  movd    edx,xmm2              ; offset of the second pixel

  ; the source pixels are gotten with the low byte of the base of the
  ; color table in the eax and ebx registers of them, with the two
  ; colors of them kept in xmm5 while the offsets are gotten

  mov     al,[esi+ecx]          ; get first source pixel
  mov     bl,[esi+edx]          ; get second source pixel
  mov     cl,[eax]              ; translate color of the first pixel
  mov     ch,[ebx]              ; translate color of the second pixel
  movd    xmm5,ecx              ; the first two colors of them

  psrldq  xmm2,4
  movd    ecx,xmm2              ; offset of the third pixel
  psrldq  xmm2,4
  movd    edx,xmm2              ; offset of the fourth pixel

  mov     al,[esi+ecx]          ; get third source pixel
  mov     bl,[esi+edx]          ; get fourth source pixel
  mov     dl,[eax]              ; translate color of the third pixel
  mov     dh,[ebx]              ; translate color of the fourth pixel

  ; the four colors of them joined in a dword, two of them in the low
  ; half of it and the other two in the high half of it

  movd    ecx,xmm5              ; the first two colors of them
  movzx   ecx,cx
  shl     edx,16                ; the last two colors of them
  or      edx,ecx
  mov     [edi],edx             ; write four pixels of them
  add     edi,4

  dec     ebp
  jns   LoopSpanSSE2

  ; the pixels which are not a multiple of four of them, one by one,
  ; with the positions of them in the dwords of xmm0

TailSpanSSE2:
  pop   ecx                     ; pixels which are not a full group of four

  test  ecx,ecx
  je    EndSpanSSE2

  ; the base of the color table is in eax, the source is in esi and the
  ; destination of the pixels is in edi

LoopTailSpanSSE2:
  movd  edx,xmm0                ; position of the pixel
  psrldq xmm0,4                 ; positions of the following pixels
  mov   ebx,edx
  shr   edx,26                  ; horizontal component of the position
  shr   ebx,4
  and   ebx,0xFC0               ; vertical component of the position
  or    edx,ebx                 ; texture offset of the pixel

  mov   al,[esi+edx]            ; get source pixel
  mov   al,[eax]                ; translate color
  mov   [edi],al                ; write the pixel
  inc   edi

  dec ecx
  jnz LoopTailSpanSSE2

EndSpanSSE2:
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanBackbufferSSE2 ends

%endif
