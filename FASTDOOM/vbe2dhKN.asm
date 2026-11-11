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
;              high detail (lea/shr wall loop, no shld, no mov reg,reg).
;
; eax   light table pointer, low byte overwritten by the source pixel
; ebx   one half of the position flip flop (raw position / 7 bit index)
; ecx   fractional step value
; edx   the other half of the position flip flop
; esi   start of source pixels
; edi   bottom pixel in the framebuffer to blit into
; ebp   pixel count, only used for the jump into the unwound loop
;
; The lea/shr pair is 2 instructions where the plain loop needs
; add/mov/shr, and the 2x unroll keeps the pair fed: every row reads the
; index prepared by the previous row and prepares the index of the next
; one, so no row ever waits on the shr of the row before it. The entry
; code picks the phase of the flip flop from the parity of the pixel
; count, the row that runs first is vscale(count+1).
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

CODE_SYM_DEF R_PatchCenteryVBE2HighKN
  push ebx
  mov  ebx,[_centery]
  mov  eax,patchCentery+1
  mov  [eax],ebx
  pop  ebx
  ret

;============================================================================
; R_DrawColumnVBE2FastLEA, the high detail column, sprite, psprite and
; sky renderer of the FastLEA CPU renderers
;============================================================================

CODE_SYM_DEF R_DrawColumnVBE2FastLEA
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
  js   near doneh

  mov  ebx,[_dc_x]
  MulScreenWidthEnd edi
  mov  ecx,[_dc_iscale]
  add  edi,ebx
  add  edi,[_destview]

patchCentery:
  sub   eax,0x12345678
  imul  ecx
  mov   esi,[_dc_source]
  shl   ecx,9 ; 7 significant bits, 25 frac

  ; EVEN/ODD ?
  test  ebp,1
  jne .odd

.even:
  mov   ebx,[_dc_texturemid]
  add   ebx,eax
  shl   ebx,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   edx,[ebx+ecx]
  shr   ebx,25

  jmp   [scalecalls+4+ebp*4]

.odd:
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov   eax,[_dc_colormap]

  lea   ebx,[edx+ecx]
  shr   edx,25

  jmp   [scalecalls+4+ebp*4]

doneh:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnVBE2FastLEA ends

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
      mov  [edi-(LINE-1)*SCREENWIDTH],al  ; draw a pixel to the buffer
    %else
      mov  al,[esi+ebx]                   ; get source pixel
      lea  ebx,[edx+ecx]                  ; 386:2cc, 486:1cc
      mov  al,[eax]                       ; translate the color
      shr  edx, 25                        ; 386:3cc, 486:2cc
      mov  [edi-(LINE-1)*SCREENWIDTH],al  ; draw a pixel to the buffer
    %endif
    %assign LINE LINE-1
%endrep

vscale1:
  pop	 ebp
  mov  al,[esi+ebx]
  pop	 esi
  mov  al,[eax]
  pop	 edx
  mov  [edi],al

vscale0:
	pop		ecx
	pop		ebx
  pop		edi
  ret

%endif
