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
; DESCRIPTION: "noshift" texture mapping for VESA VBE2 direct color modes,
;              low detail (issue #143, add/adc/and wall loop).
;
; eax   colormap pointer
; ebx   color byte, kept in bl and duplicated into bh for the 16 bit store,
;       also holds the pixel count while jumping into the unwound loop
; ecx   fractional increment (iscale<<9), low 7 bits always clear
; edx   fractional accumulator (position<<7), bits 6..0 hold the integer
;       increment which is never touched as the fractional increment has
;       its low 7 bits clear
; esi   start of source pixels
; edi   bottom pixel in the framebuffer to blit into
; ebp   integer part of the texture position (7 bit texture column)
;
; Each column draws two pixels, they share the same texel column, so the
; color byte is duplicated into the upper half of bx and written with one
; 16 bit store: the framebuffer keeps being written a word at a time, as
; the plain 386 loop does. The texture index is kept in ebp, which the
; plain loop uses for the same purpose and which is free once the pixel
; count has been used for the jump.
;

BITS 32
%include "macros.inc"

%ifdef MODE_VBE2_DIRECT
%include "defs.inc"

extern _destview
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

;============================================================================

BEGIN_CODE_SECTION

CODE_SYM_DEF R_PatchCenteryVBE2LowNS
  push ebx
  mov  ebx,[_centery]
  mov  eax,patchCentery+1
  mov  [eax],ebx
  pop  ebx
  ret

;============================================================================
; R_DrawColumnLowVBE2NS, the low detail column, sprite, psprite and sky
; renderer of the NOSHIFT 486 CPU renderer
;============================================================================

CODE_SYM_DEF R_DrawColumnLowVBE2NS
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  MulScreenWidthStart edi, ebp
  sub  ebp,eax ; ebp = pixel count
  js   near donel

  MulScreenWidthEnd edi
  mov  ebx,[_dc_x]
  mov  ecx,[_dc_iscale]
  lea  edi,[edi+ebx*2]

patchCentery:
  sub  eax,0x12345678
  add  edi,[_destview]

  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac

  ; split the position into fractional/integer parts
  mov   ebx,ebp ; keep the pixel count for the jump
  mov   ebp,edx
  shr   ebp,25 ; get address of first location
  shl   edx,7  ; fractional part of the position
  mov   eax,ecx
  shr   ecx,25 ; integer part of the increment
  shl   eax,7  ; fractional part of the increment
  or    edx,ecx ; integer increment rides along in bits 6..0
  mov   ecx,eax ; fractional increment
  mov   eax,[_dc_colormap]

  jmp  [scalecalls+4+ebx*4]

donel:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	pop		edi
  ret
; R_DrawColumnLowVBE2NS ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    add  edx,ecx                         ; fractional part, CF = overflow
    mov  al,[esi+ebp]                    ; get source pixel
    adc  ebp,edx                         ; integer part + integer delta + carry
    mov  bl,[eax]                        ; translate the color
    and  ebp,7Fh                         ; keep the 7 bit texture column
    mov  bh,bl
    mov  [edi-(LINE-1)*SCREENWIDTH],bx   ; draw two pixels to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  mov  al,[esi+ebp]
  pop	 ebp
  mov  bl,[eax]
  pop	 esi
  mov  bh,bl
  pop	 edx
  mov  [edi],bx
  pop	 ecx

vscale0:
	pop		ebx
	pop		edi
  ret

%endif
