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
; DESCRIPTION: FastLEA texture mapping for VESA VBE2 direct color modes,
;              low detail (lea/shr wall loop, two pixels per row).
;
; eax   light table pointer, low byte overwritten by the source pixel
; ebx   the color byte, kept in bl and duplicated into bh for the 16 bit
;       store, also holds the pixel count while jumping into the loop
; ecx   fractional step value
; edx   one half of the position flip flop (raw position / 7 bit index)
; esi   start of source pixels
; edi   bottom pixel in the framebuffer to blit into
; ebp   the other half of the position flip flop
;
; Every row draws two pixels, they share the same texel column, so the
; color byte is duplicated in bx and written with a single 16 bit store.
; That costs ebx, and the flip flop costs edx and ebp, so the jump target
; is taken from a memory address instead of [scalecalls+4+ebp*4]: the
; target is loaded into esi while esi is still free and stored, exactly
; as the 16 bit backbuffer FastLEA renderer does.
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

jumpaddress:   dd 0

scalecalls:
  %assign LINE 0
  %rep SCREENHEIGHT+1
    SCALEDEFINE LINE
  %assign LINE LINE+1
  %endrep

;============================================================================

BEGIN_CODE_SECTION

CODE_SYM_DEF R_PatchCenteryVBE2LowKN
  push ebx
  mov  ebx,[_centery]
  mov  eax,patchCentery+1
  mov  [eax],ebx
  pop  ebx
  ret

;============================================================================
; R_DrawColumnLowVBE2FastLEA, the low detail column, sprite, psprite and
; sky renderer of the FastLEA CPU renderers
;============================================================================

CODE_SYM_DEF R_DrawColumnLowVBE2FastLEA
	push		edi
	push		ebp
	push		ecx
	push		edx
	push		esi
	push		ebx

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  MulScreenWidthStart edi, ebp
  sub  ebp,eax ; ebp = pixel count
  js   near donel

  MulScreenWidthEnd edi
  mov  ebx,[_dc_x]
  mov  esi,[scalecalls+4+ebp*4]
  mov  ecx,[_dc_iscale]
  mov  [jumpaddress],esi
  lea  edi,[edi+ebx*2]

patchCentery:
  sub   eax,0x12345678
  add   edi,[_destview]

  imul  ecx
  mov   esi,[_dc_source]
  shl   ecx,9 ; 7 significant bits, 25 frac

  xor   ebx,ebx

  ; EVEN/ODD ?
  test  ebp,1
  jne .odd

.even:
  mov   ebp,[_dc_texturemid]
  add   ebp,eax
  shl   ebp,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   edx,[ebp+ecx]
  shr   ebp,25

  jmp   [jumpaddress]

.odd:
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   ebp,[edx+ecx]
  shr   edx,25

  jmp   [jumpaddress]

donel:
	pop		ebx
	pop		esi
	pop		edx
	pop		ecx
	pop		ebp
  pop		edi
  ret
; R_DrawColumnLowVBE2FastLEA ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    %if LINE % 2 = 0
      mov  al,[esi+edx]                        ; get source pixel
      lea  edx,[ebp+ecx]                       ; 386:2cc, 486:1cc
      mov  bl,[eax]                            ; translate the color
      shr  ebp, 25                             ; 386:3cc, 486:2cc
      mov  bh,bl
      mov  [edi-(LINE-1)*SCREENWIDTH],bx       ; draw two pixels
    %else
      mov  al,[esi+ebp]                        ; get source pixel
      lea  ebp,[edx+ecx]                       ; 386:2cc, 486:1cc
      mov  bl,[eax]                            ; translate the color
      shr  edx, 25                             ; 386:3cc, 486:2cc
      mov  bh,bl
      mov  [edi-(LINE-1)*SCREENWIDTH],bx       ; draw two pixels
    %endif
    %assign LINE LINE-1
%endrep

vscale1:
  pop	 ebx
  mov  al,[esi+ebp]
  pop	 esi
  mov  al,[eax]
  pop	 edx
  mov  ah,al
  pop	 ecx
  mov  [edi],ax

vscale0:
	pop		ebp
  pop		edi
  ret

%endif
