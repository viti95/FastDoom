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

; ================
; R_DrawColumnLow
; ================
CODE_SYM_DEF R_DrawColumnLow
  pushad

  mov  ebp,[_dc_yh]
  mov  ebx,[_dc_yl]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,ebx         ; ebp = pixel count
  js   short .done

  ; set plane
  mov  ecx,[_dc_x]
  add  edi,[_destview]
  mov  esi, ecx
  
  and  cl,1
  mov  al,3
  add  cl, cl
  mov  dx,SC_INDEX+1
  shl  al,cl
  out  dx,al

  shr esi,1
  mov eax, ebx
  add edi,esi

  mov   ecx,[_dc_iscale]

  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  xor   ebx,ebx
  shld  ebx,edx,7
  call  [scalecalls+4+ebp*4]

.done:
  popad
  ret
; R_DrawColumn_13h ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    mov  al,[esi+ebx]                   ; get source pixel
    add  edx,ecx                        ; calculate next location
    mov  al,[eax]                       ; translate the color
    mov  ebx,edx
    mov  [edi-(LINE-1)*80],al  ; draw a pixel to the buffer
    shr  ebx,25
    %assign LINE LINE-1
%endrep

vscale1:
  mov al,[esi+ebx]
  mov al,[eax]
  mov [edi],al

vscale0:
  ret

%endif
