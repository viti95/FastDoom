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

; =========================
; R_DrawColumnLowBackbuffer
; =========================
CODE_SYM_DEF R_DrawColumnLowBackbufferFlat
	push		edi
	push		ebp
  push		ebx

  mov  ebp,[_dc_yh]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,[_dc_yl]         ; ebp = pixel count
  js   short .done

  mov  al,[_dc_color]
  mov  ebx,[_dc_x]
  mov  ah,al
  add  edi,[_columnofs+ebx*4]

  jmp  [scalecalls+4+ebp*4]

.done:
	pop		ebx
  pop	  ebp
	pop		edi
  ret
; R_DrawColumnLowBackbuffer ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    mov  [edi-(LINE-1)*SCREENWIDTH],ax  ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  pop	ebx
  pop	ebp
  mov [edi],ax

vscale0:
	pop		edi
  ret

%endif
