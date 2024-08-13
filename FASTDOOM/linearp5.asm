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

BEGIN_CODE_SECTION

CODE_SYM_DEF R_DrawColumnPotatoBackbufferSkyFullDirect
	push		edi
	push		esi
	push		edx
	push		ebp
  push		ebx
	push		ecx

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   short doneps

  sub  eax,[_centery]
  add  eax,0x64
  mov  esi,[_dc_source]
  and  eax,0x7FFFFF
  mov  ebx,[_dc_x]
  add  esi,eax
  add  edi,[_columnofs+ebx*4]
  mov  eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

doneps:
	pop		ecx
	pop		ebx
  pop	  ebp
	pop		edx
	pop		esi
	pop		edi
  ret

CODE_SYM_DEF R_DrawColumnPotatoBackbufferDirect
	push		edi
	push		esi
	push		edx
	push		ebp
  push		ebx
	push		ecx

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   short donep

  mov  ebx,[_dc_x]
  mov  esi,[_dc_source]
  add  edi,[_columnofs+ebx*4]
  mov  eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

donep:
	pop		ecx
	pop		ebx
  pop	  ebp
	pop		edx
	pop		esi
	pop		edi
  ret

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    mov  al,[esi]                   ; get source pixel
    mov  bl,[eax]                       ; translate the color
    mov  bh,bl
    inc  esi
    mov  [edi-(LINE-1)*SCREENWIDTH],bx  ; draw a pixel to the buffer
    mov  [edi-(LINE-1)*SCREENWIDTH + 2],bx  ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  pop	ecx
  pop	ebx
  mov al,[esi]
  pop	ebp
  mov al,[eax]
  pop	edx
  mov ah,al
  pop	esi
  mov [edi],ax
  mov [edi+2],ax

vscale0:
	pop		edi
  ret

%endif
