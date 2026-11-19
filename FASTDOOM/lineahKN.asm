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
;
; The roll version of the file keeps the lea/shr flip flop of the file, but
; walking down the screen one row at a time (add edi,SCREENWIDTH) instead of
; jumping into the unwound loop, as the roll renderer of linearh.asm does.
; It is the renderer of the CPUs which take the loop of that file, the UMC
; Green 486 and the Cyrix 5x86, the two of them of the FastLEA group of the
; CPUs.  The loop of it is of two rows per pass, one of them of each one of
; the two phases of the flip flop, so the phase of them is kept over the
; passes of the loop and the row of the turn of every one of the two rows of
; a pass is fed by the row which goes before it, exactly as of the unwound
; loop.  Of the odd number of the rows of a column the first of them is
; drawn out of the loop, the loop of them being entered with the phase of
; the flip flop of the row of the turn.
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

CODE_SYM_DEF R_PatchCenteryLinearHighKN
  push ebx
  mov   ebx,[_centery]
  mov   eax,patchCentery+1
  mov   [eax],ebx
  mov   eax,patchCenteryRoll+1
  mov   [eax],ebx
  pop ebx
  ret

; ======================
; R_DrawColumnBackbuffer
; ======================
CODE_SYM_DEF R_DrawColumnBackbufferFastLEA
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
    %if LINE % 2 = 0
      mov  al,[esi+edx]                   ; get source pixel
      lea  edx,[ebx+ecx]                  ; 386:2cc, 486:1cc
      mov  al,[eax]                       ; translate the color
      shr  ebx, 25                        ; 386:3cc, 486:2cc
      mov  [edi-(LINE-1)*SCREENWIDTH],al           ; draw a pixel to the buffer
    %else
      mov  al,[esi+ebx]                   ; get source pixel
      lea  ebx,[edx+ecx]                  ; 386:2cc, 486:1cc
      mov  al,[eax]                       ; translate the color
      shr  edx, 25                        ; 386:3cc, 486:2cc
      mov  [edi-(LINE-1)*SCREENWIDTH],al           ; draw a pixel to the buffer
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
	pop		ecx
	pop		ebx
  pop		edi
  ret


; ==================================
; R_DrawColumnBackbufferRollFastLEA
;
; The roll renderer of the FastLEA CPUs, the UMC Green 486 and the Cyrix
; 5x86.  edi starts at the top pixel of the column and is rolled down of a
; screen width per row, so the column of the rows above the top of the view
; is drawn as the plain roll renderer of linearh.asm draws it.  The pair of
; the rows of every pass of the loop is of the two phases of the flip flop
; of the file, the one of them taking the texture column out of edx and
; preparing the one of ebx, the other one the way about: the row of the
; turn of every one of them is fed by the row which goes before it, as of
; the unwound loop, and the phase of the flip flop is kept over the passes
; of the loop.  Of the odd number of the rows of a column the first of them
; is drawn out of the loop, the loop of them being entered with the phase
; of the row of the turn.
; ==================================
CODE_SYM_DEF R_DrawColumnBackbufferRollFastLEA
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
  js   near  donehr

  mov  ebx,[_dc_x]
  mov  ecx,[_dc_iscale]
  add  edi,[_columnofs+ebx*4]

patchCenteryRoll:
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
  add   edi,SCREENWIDTH

  dec   ebp
  js    near  donehr
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

  ; the rows are rolled down of a screen width per row, the rows above the
  ; top of the view being drawn as the plain roll renderer does
.LoopRoll:
  mov   al,[esi+edx]                    ; get source pixel
  lea   edx,[ebx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   ebx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,SCREENWIDTH

  mov   al,[esi+ebx]                    ; get source pixel
  lea   ebx,[edx+ecx]                   ; 386:2cc, 486:1cc
  mov   al,[eax]                        ; translate the color
  shr   edx, 25                         ; 386:3cc, 486:2cc
  mov   [edi],al                        ; draw a pixel to the buffer
  add   edi,SCREENWIDTH

  sub   ebp,2
  jns   short .LoopRoll

donehr:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnBackbufferRollFastLEA ends

%endif
