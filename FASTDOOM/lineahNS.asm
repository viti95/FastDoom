;
; Copyright (C) 1993-1996 Id Software, Inc.
; Copyright (C) 2016-2017 Alexey Khokholov (Nuke.YKT)
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; DESCRIPTION: "noshift" texture mapping routines for backbuffered modes,
;              high detail (issue #143, add/adc/and wall loop).
;
; eax   colormap pointer
; ebx   integer part of the texture position (7 bit texture column)
; ecx   fractional increment (iscale<<16), low 7 bits always clear
; edx   fractional accumulator (position<<7), bits 6..0 hold the integer
;       increment which is never touched as the fractional increment has
;       its low 7 bits clear
; esi   start of source pixels
; edi   bottom pixel in screenbuffer to blit into
; ebp   pixel count, free for the jump into the unwound loop
;
; 386:2cc/486:1cc per add/adc/and, no shifts and no base+index addressing
; penalties inside the loop.
;

BITS 32
%include "macros.inc"

%ifdef USE_BACKBUFFER
%include "defs.inc"

extern _centery

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

CODE_SYM_DEF R_PatchCenteryLinearHighNS
  push ebx
  mov   ebx,[_centery]
  mov   eax,patchCentery+1
  mov   [eax],ebx
  pop   ebx
  ret

; ==========================
; R_DrawColumnBackbufferFastNS
; ==========================
CODE_SYM_DEF R_DrawColumnBackbufferFastNS
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

  ; split the position into fractional/integer parts
  mov   ebx,edx
  shr   ebx,25 ; get address of first location
  shl   edx,7  ; fractional part of the position
  mov   eax,ecx
  shr   ecx,25 ; integer part of the increment
  shl   eax,7  ; fractional part of the increment
  or    edx,ecx ; integer increment rides along in bits 6..0
  mov   ecx,eax ; fractional increment
  mov   eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

doneh:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnBackbufferFastNS ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    add  edx,ecx                        ; fractional part, CF = overflow
    mov  al,[esi+ebx]                   ; get source pixel
    adc  ebx,edx                        ; integer part + integer delta + carry
    mov  al,[eax]                       ; translate the color
    and  ebx,7Fh                        ; keep the 7 bit texture column
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

%endif
