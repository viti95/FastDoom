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

jumpaddress:   dd 0

scalecalls:
  %assign LINE 0
  %rep SCREENHEIGHT+1
    SCALEDEFINE LINE
  %assign LINE LINE+1
  %endrep

BEGIN_CODE_SECTION

CODE_SYM_DEF R_PatchCenteryLinearLowKN
  push  ebx
  mov   ebx,[_centery]
  mov   eax,patchCentery+1
  mov   [eax],ebx
  pop   ebx
  ret

; =========================
; R_DrawColumnLowBackbuffer
; =========================
CODE_SYM_DEF R_DrawColumnLowBackbufferFastLEA
	push		edi
	push		ebp
	push		ecx
	push		edx
	push		esi
	push		ebx

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   short donel

  mov  ebx,[_dc_x]
  mov  esi,[scalecalls+4+ebp*4]
  mov  ecx,[_dc_iscale]
  mov [jumpaddress],esi
  add  edi,[_columnofs+ebx*4]

patchCentery:
  sub   eax,0x12345678
  imul  ecx

  mov   esi,[_dc_source]

  shl   ecx,9 ; 7 significant bits, 25 frac

  xor  ebx,ebx

  ; EVEN/ODD ?
  test ebp,1
  jne .odd

.even:
  mov   ebp,[_dc_texturemid]

  add   ebp,eax
  shl   ebp,9 ; 7 significant bits, 25 frac

  mov  eax,[_dc_colormap]

  lea  edx,[ebp+ecx]
  shr  ebp, 25

  jmp  [jumpaddress]

.odd:
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov  eax,[_dc_colormap]

  lea  ebp,[edx+ecx]
  shr  edx,25

  jmp  [jumpaddress]
; R_DrawColumn ends

donel:
	pop		ebx
	pop		esi
	pop		edx
	pop		ecx
	pop		ebp
  pop		edi
  ret
; R_DrawColumnLowBackbuffer ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    %if LINE % 2 = 0
      mov  al,[esi+edx]                   ; get source pixel
      lea  edx,[ebp+ecx]                  ; 386:2cc, 486:1cc
      mov  bl,[eax]                       ; translate the color
      shr  ebp, 25                        ; 386:3cc, 486:2cc
      mov  bh,bl
      mov  [edi-(LINE-1)*SCREENWIDTH],bx  ; draw a pixel to the buffer
    %else
      mov  al,[esi+ebp]                   ; get source pixel
      lea  ebp,[edx+ecx]                  ; 386:2cc, 486:1cc
      mov  bl,[eax]                       ; translate the color
      shr  edx, 25                        ; 386:3cc, 486:2cc
      mov  bh,bl
      mov  [edi-(LINE-1)*SCREENWIDTH],bx  ; draw a pixel to the buffer
    %endif
    %assign LINE LINE-1
%endrep

vscale1:
	pop	ebx
  mov al,[esi+ebp]
  pop	esi
  mov al,[eax]
  pop	edx
  mov ah,al
	pop	ecx
  mov [edi],ax

vscale0:
	pop	ebp
  pop	edi
  ret

%endif
