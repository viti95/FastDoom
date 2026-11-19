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

%ifdef MODE_Y
%include "defs.inc"

extern _destview
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
;
; The roll versions of the file keep the lea/shr flip flop of the file, but
; walking down the screen one row at a time (add edi,PLANEWIDTH) instead of
; jumping into the unwound loop, as the roll renderers of linearh.asm and of
; vbe2dh.asm do.  The unwound loop of the file is of the parity of the pixel
; count, the phase of the flip flop of the row that runs first being picked
; by the entry code of its renderer; the roll loops of the file are of two
; rows per pass, one of them of each one of the two phases of the flip flop,
; so the phase of them is kept over the passes of the loop and the row of
; the turn of every one of the two rows of a pass is fed by the row which
; goes before it, exactly as of the unwound loop.  Of the odd number of the
; rows of a column the first of them is drawn out of the loop, the loop of
; them being entered with the phase of the flip flop of the row of the turn.
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

BEGIN_CODE_SECTION

CODE_SYM_DEF R_PatchCenteryPlanarKN
  push ebx
  mov   ebx,[_centery]
  mov   eax,patchCentery1+1
  mov   [eax],ebx
  mov   eax,patchCentery2+1
  mov   [eax],ebx
  mov   eax,patchCentery3+1
  mov   [eax],ebx
  mov   eax,patchCentery1Roll+1
  mov   [eax],ebx
  mov   eax,patchCentery2Roll+1
  mov   [eax],ebx
  mov   eax,patchCentery3Roll+1
  mov   [eax],ebx
  pop ebx
  ret

pdone:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

; ==================
; R_DrawColumnPotato
; ==================
CODE_SYM_DEF R_DrawColumnPotatoFastLEA
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
  js   near pdone

  add  edi,[_destview]
  add  edi,[_dc_x]

  mov   ecx,[_dc_iscale]

patchCentery1:
  sub   eax,0x12345678
  imul  ecx

  mov   esi,[_dc_source]

  shl   ecx,9 ; 7 significant bits, 25 frac

  ; EVEN/ODD ?
  test ebp,1
  jne .odd

.even:
  mov   ebx,[_dc_texturemid]

  add   ebx,eax
  shl   ebx,9 ; 7 significant bits, 25 frac

  mov  eax,[_dc_colormap]

  lea  edx,[ebx+ecx]
  shr  ebx, 25

  jmp  [scalecalls+4+ebp*4]

.odd:
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov  eax,[_dc_colormap]

  lea  ebx,[edx+ecx]
  shr  edx,25

  jmp  [scalecalls+4+ebp*4]
; R_DrawColumnPotato ends

ldone:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

; ===============
; R_DrawColumnLow
; ===============
CODE_SYM_DEF R_DrawColumnLowFastLEA
  push		edi
  push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  ebx,[_dc_yl]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,ebx         ; ebp = pixel count
  js   near ldone

  ; set plane
  mov  ecx,[_dc_x]
  add  edi,[_destview]
  mov  esi, ecx
  
  and  ecx,1
  shr esi,1
  lea  eax,[ecx+ecx*8+3]
  mov  dx,SC_INDEX+1
  out  dx,al

  mov eax, ebx
  add edi,esi

  mov   ecx,[_dc_iscale]

patchCentery2:
  sub   eax,0x12345678
  imul  ecx

  mov   esi,[_dc_source]

  shl   ecx,9 ; 7 significant bits, 25 frac

  ; EVEN/ODD ?
  test ebp,1
  jne .odd

.even:
  mov   ebx,[_dc_texturemid]

  add   ebx,eax
  shl   ebx,9 ; 7 significant bits, 25 frac

  mov  eax,[_dc_colormap]

  lea  edx,[ebx+ecx]
  shr  ebx, 25

  jmp  [scalecalls+4+ebp*4]

.odd:
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov  eax,[_dc_colormap]

  lea  ebx,[edx+ecx]
  shr  edx,25
  
  jmp  [scalecalls+4+ebp*4]
; R_DrawColumnLow ends

hdone:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

CODE_SYM_DEF R_DrawColumnFastLEA
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  ebx,[_dc_yl]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,ebx         ; ebp = pixel count
  js   near hdone

  ; set plane
  mov  ecx,[_dc_x]
  add  edi,[_destview]
  mov  esi, ecx
  
  and  cl,3
  mov  dx,SC_INDEX+1
  mov  al,1
  shl  al,cl
  out  dx,al

  shr esi,2
  mov eax, ebx
  add edi,esi

  mov   ecx,[_dc_iscale]

patchCentery3:
  sub   eax,0x12345678
  imul  ecx

  mov   esi,[_dc_source]

  shl   ecx,9 ; 7 significant bits, 25 frac

  ; EVEN/ODD ?
  test ebp,1
  jne .odd

.even:
  mov   ebx,[_dc_texturemid]

  add   ebx,eax
  shl   ebx,9 ; 7 significant bits, 25 frac

  mov  eax,[_dc_colormap]

  lea  edx,[ebx+ecx]
  shr  ebx, 25

  jmp  [scalecalls+4+ebp*4]

.odd:
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov  eax,[_dc_colormap]

  lea  ebx,[edx+ecx]
  shr  edx,25

  jmp  [scalecalls+4+ebp*4]
; R_DrawColumn ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    %if LINE % 2 = 0
      mov  al,[esi+edx]                   ; get source pixel
      lea  edx,[ebx+ecx]                  ; 386:2cc, 486:1cc
      mov  al,[eax]                       ; translate the color
      shr  ebx, 25                        ; 386:3cc, 486:2cc
      mov  [edi-(LINE-1)*80],al           ; draw a pixel to the buffer
    %else
      mov  al,[esi+ebx]                   ; get source pixel
      lea  ebx,[edx+ecx]                  ; 386:2cc, 486:1cc
      mov  al,[eax]                       ; translate the color
      shr  edx, 25                        ; 386:3cc, 486:2cc
      mov  [edi-(LINE-1)*80],al           ; draw a pixel to the buffer
    %endif
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
	pop	ecx
	pop	ebx
  pop	edi
  ret

; ========================================
; R_DrawColumnPotatoRollFastLEA, the roll
; renderer of the potato detail level
; ========================================
CODE_SYM_DEF R_DrawColumnPotatoRollFastLEA
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
  js   near  pdoner

  add  edi,[_destview]
  add  edi,[_dc_x]

  mov   ecx,[_dc_iscale]

patchCentery1Roll:
  sub   eax,0x12345678
  imul  ecx

  mov   esi,[_dc_source]

  shl   ecx,9 ; 7 significant bits, 25 frac

  ; EVEN/ODD of the number of rows ?
  test  ebp,1
  jne   short .eventwos

  ; odd number of rows: ebx the column of the first one of them, edx the
  ; position of the row of the turn.  The first row is drawn out of the
  ; loop, the loop of the pair of the rows of the turn being entered with
  ; the column of it in edx and the position of the next in ebx.
  mov   ebx,[_dc_texturemid]

  add   ebx,eax
  shl   ebx,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   edx,[ebx+ecx]
  shr   ebx, 25

  mov   al,[esi+ebx]                    ; get source pixel
  lea   ebx,[edx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   edx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,PLANEWIDTH

  dec   ebp
  js    near  pdoner
  jmp       .LoopRoll

.eventwos:
  ; even number of rows: edx the column of the first one of them, ebx the
  ; position of the row of the turn
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   ebx,[edx+ecx]
  shr   edx,25

  ; the rows are rolled down of the width of a plane per row, the whole
  ; column being of the four planes of its one byte per pixel store
.LoopRoll:
  mov   al,[esi+edx]                    ; get source pixel
  lea   edx,[ebx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   ebx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,PLANEWIDTH

  mov   al,[esi+ebx]                    ; get source pixel
  lea   ebx,[edx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   edx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,PLANEWIDTH

  sub   ebp,2
  jns   short .LoopRoll

pdoner:
	pop	ebp
	pop	esi
	pop	edx
	pop	ecx
	pop	ebx
  pop	edi
  ret
; R_DrawColumnPotatoRollFastLEA ends

; ===================================
; R_DrawColumnLowRollFastLEA, the roll
; renderer of the low detail level
; ===================================
CODE_SYM_DEF R_DrawColumnLowRollFastLEA
	push		edi
  push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  ebx,[_dc_yl]
  mov  edi,[_ylookup+ebx*4]
  sub  ebp,ebx         ; ebp = pixel count
  js   near  ldoner

  ; set plane
  mov  ecx,[_dc_x]
  add  edi,[_destview]
  mov  esi, ecx

  and  ecx,1
  shr esi,1
  lea  eax,[ecx+ecx*8+3]
  mov  dx,SC_INDEX+1
  out  dx,al

  mov eax, ebx
  add edi,esi

  mov   ecx,[_dc_iscale]

patchCentery2Roll:
  sub   eax,0x12345678
  imul  ecx

  mov   esi,[_dc_source]

  shl   ecx,9 ; 7 significant bits, 25 frac

  ; EVEN/ODD of the number of rows ?
  test  ebp,1
  jne   short .eventwos

  ; odd number of rows: ebx the column of the first one of them, edx the
  ; position of the row of the turn.  The first row is drawn out of the
  ; loop, the loop of the pair of the rows of the turn being entered with
  ; the column of it in edx and the position of the next in ebx.
  mov   ebx,[_dc_texturemid]

  add   ebx,eax
  shl   ebx,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   edx,[ebx+ecx]
  shr   ebx, 25

  mov   al,[esi+ebx]                    ; get source pixel
  lea   ebx,[edx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   edx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,PLANEWIDTH

  dec   ebp
  js    near  ldoner
  jmp       .LoopRoll

.eventwos:
  ; even number of rows: edx the column of the first one of them, ebx the
  ; position of the row of the turn
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   ebx,[edx+ecx]
  shr   edx,25

  ; the rows are rolled down of the width of a plane per row, the byte of
  ; each one of them reaching the two planes of the mask set above
.LoopRoll:
  mov   al,[esi+edx]                    ; get source pixel
  lea   edx,[ebx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   ebx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,PLANEWIDTH

  mov   al,[esi+ebx]                    ; get source pixel
  lea   ebx,[edx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   edx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,PLANEWIDTH

  sub   ebp,2
  jns   short .LoopRoll

ldoner:
	pop	ebp
	pop	esi
	pop	edx
	pop	ecx
	pop	ebx
  pop	edi
  ret
; R_DrawColumnLowRollFastLEA ends

; ==================================
; R_DrawColumnRollFastLEA, the roll
; renderer of the high detail level
; ==================================
CODE_SYM_DEF R_DrawColumnRollFastLEA
	push		edi
  push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  ebx,[_dc_yl]
  mov  edi,[_ylookup+ebx*4]
  sub  ebp,ebx         ; ebp = pixel count
  js   near  hdoner

  ; set plane
  mov  ecx,[_dc_x]
  add  edi,[_destview]
  mov  esi, ecx

  and  cl,3
  mov  dx,SC_INDEX+1
  mov  al,1
  shl  al,cl
  out  dx,al

  shr esi,2
  mov eax, ebx
  add edi,esi

  mov   ecx,[_dc_iscale]

patchCentery3Roll:
  sub   eax,0x12345678
  imul  ecx

  mov   esi,[_dc_source]

  shl   ecx,9 ; 7 significant bits, 25 frac

  ; EVEN/ODD of the number of rows ?
  test  ebp,1
  jne   short .eventwos

  ; odd number of rows: ebx the column of the first one of them, edx the
  ; position of the row of the turn.  The first row is drawn out of the
  ; loop, the loop of the pair of the rows of the turn being entered with
  ; the column of it in edx and the position of the next in ebx.
  mov   ebx,[_dc_texturemid]

  add   ebx,eax
  shl   ebx,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   edx,[ebx+ecx]
  shr   ebx, 25

  mov   al,[esi+ebx]                    ; get source pixel
  lea   ebx,[edx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   edx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,PLANEWIDTH

  dec   ebp
  js    near  hdoner
  jmp       .LoopRoll

.eventwos:
  ; even number of rows: edx the column of the first one of them, ebx the
  ; position of the row of the turn
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   ebx,[edx+ecx]
  shr   edx,25

  ; the rows are rolled down of the width of a plane per row, the byte of
  ; each one of them reaching the one plane of the mask set above
.LoopRoll:
  mov   al,[esi+edx]                    ; get source pixel
  lea   edx,[ebx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   ebx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,PLANEWIDTH

  mov   al,[esi+ebx]                    ; get source pixel
  lea   ebx,[edx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   edx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,PLANEWIDTH

  sub   ebp,2
  jns   short .LoopRoll

hdoner:
	pop	ebp
	pop	esi
	pop	edx
	pop	ecx
	pop	ebx
  pop	edi
  ret
; R_DrawColumnRollFastLEA ends

%endif
