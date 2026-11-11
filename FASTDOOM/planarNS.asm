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
; DESCRIPTION: "noshift" texture mapping routines for planar VGA modes
;              (issue #143, add/adc/and wall loop).
;
; The texture position is kept as a 39 bit fixed point number split into
; two 32 bit registers, so the 7 bit texture column can be extracted with
; a single 'and' instead of the 'mov reg,edx / shr reg,25' pair:
;
;   eax   colormap pointer
;   ebx   integer part of the texture position (7 bit texture column)
;   ecx   [vvvvvvvv vvvvvvvv vvvvvvvv iIIIIII] fractional delta (delta<<7)
;   edx   [VVVVVVVV VVVVVVVV VVVVVVVV iIIIIII] fractional accumulator,
;                                            bits 31..7 = position<<7,
;                                            bits 6..0  = integer delta
;   esi   start of source pixels
;   edi   bottom pixel in the screenbuffer to draw into
;   ebp   pixel count (used for the jump into the unwound loop)
;
; The integer delta lives in the low 7 bits of the fractional accumulator,
; where it is never touched: the fractional delta has those bits cleared.
; That frees ebp for the jump table lookup, no base+index addressing is
; used in the loop and no shift is needed per pixel:
;
;   add edx,ecx   ;386:2cc, 486:1cc  fractional part, CF = overflow
;   adc ebx,edx   ;386:2cc, 486:1cc  integer part + integer delta + carry
;   and ebx,7Fh   ;386:2cc, 486:1cc  7 bit texture column
;
; The graphics wrap vertically at 128 pixels, exactly like the plain
; 'shr reg,25' loop does.
;

BITS 32
%include "macros.inc"

%ifdef MODE_Y
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

BEGIN_CODE_SECTION

CODE_SYM_DEF R_PatchCenteryPlanarNS
  push ebx
  mov   ebx,[_centery]
  mov   eax,patchCentery1+1
  mov   [eax],ebx
  mov   eax,patchCentery2+1
  mov   [eax],ebx
  mov   eax,patchCentery3+1
  mov   [eax],ebx
  pop   ebx
  ret

pdone:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

; =========================
; R_DrawColumnPotatoFastNS
; =========================
CODE_SYM_DEF R_DrawColumnPotatoFastNS
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

ldone:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

; ======================
; R_DrawColumnLowFastNS
; ======================
CODE_SYM_DEF R_DrawColumnLowFastNS
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

  and   ecx,1
  shr   esi,1
  lea   eax,[ecx+ecx*8+3]
  mov   dx,SC_INDEX+1
  add edi,esi
  out   dx,al

  mov eax, ebx

  mov   ecx,[_dc_iscale]

patchCentery2:
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

hdone:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

; =================
; R_DrawColumnFastNS
; =================
CODE_SYM_DEF R_DrawColumnFastNS
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
; R_DrawColumn ends

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
    mov  [edi-(LINE-1)*80],al           ; draw a pixel to the buffer
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

%endif
