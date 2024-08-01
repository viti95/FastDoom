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

extern _destview
extern _viewheightminusone
extern _fuzzoffsetinverse
extern _fuzzposinverse
extern _colormaps

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

CODE_SYM_DEF R_PatchFuzzColumnLinearHigh
  push ebx
  mov   ebx,[_viewheightminusone]
  mov   eax,patchViewHeight+1
  mov   [eax],ebx
  pop ebx
  ret

; ======================
; R_DrawColumnBackbuffer
; ======================
CODE_SYM_DEF R_DrawFuzzColumnBackbuffer
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		ebp

  mov  ebp,[_dc_yh]

patchViewHeight:
  mov  eax,0x12345678
  
  xor  eax,ebp
  sub  eax,1
  sbb  ebp,0

  mov  eax,[_dc_yl]

  cmp  eax,1
  adc  eax,0

  mov  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   short .done

  mov  ebx,[_dc_x]
  mov eax,[_colormaps]
  add  edi,[_columnofs+ebx*4]
  mov	ecx,[_fuzzposinverse]
  add eax,0x600
  mov edx,_fuzzoffsetinverse
  mov ebx,49

  jmp  [scalecalls+4+ebp*4]

.done:
	pop		ebp
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnBackbuffer ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%macro TESTFUZZPOSDEFINE 1
  testfuzzpos%1
%endmacro

%macro JMPTESTFUZZPOSDEFINE 1
  jne testfuzzpos%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT
  SCALELABEL LINE:
  mov		ebp,[edx+ecx*4]
	mov   al,[ebp+edi-(LINE-1)*SCREENWIDTH]
  dec   ecx
	mov		al,[eax]
  JMPTESTFUZZPOSDEFINE LINE
  mov   ecx,ebx
  TESTFUZZPOSDEFINE LINE:
  mov		[edi-(LINE-1)*SCREENWIDTH],al
  %assign LINE LINE-1
%endrep

vscale0:
  pop	ebp
  mov [_fuzzposinverse],ecx
  pop	edx
	pop	ecx
	pop	ebx
  pop	edi
  ret

%endif
