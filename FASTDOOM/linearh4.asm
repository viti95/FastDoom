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

; ======================
; R_DrawColumnBackbuffer
; ======================
CODE_SYM_DEF R_DrawColumnBackbufferFlat
	push		edi
	push		ebx
	push		ebp

  mov  ebp,[_dc_yh]
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,[_dc_yl]         ; ebp = pixel count
  js   short .done

  mov  ebx,[_dc_x]
  add  edi,[_columnofs+ebx*4]

  mov  al,[_dc_color]

  jmp  [scalecalls+4+ebp*4]

.done:
	pop		ebp
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
    mov  [edi-(LINE-1)*SCREENWIDTH],al  ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  pop	ebp
  mov [edi],al

vscale0:
	pop		ebx
  pop		edi
  ret

%endif
