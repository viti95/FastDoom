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

CODE_SYM_DEF R_PatchCenteryLinearDirect
  push ebx
  mov  ebx,[_centery]
  mov  eax,patchCentery+1
  mov  [eax],ebx
  pop  ebx
  ret

CODE_SYM_DEF R_DrawColumnBackbufferDirect2xRoll
  push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  mov  edi,[_ylookup+eax*4]
  sub  ebp,eax         ; ebp = pixel count
  js   donehr

  mov  ebx,[_dc_x]
  add  edi,[_columnofs+ebx*4]

  mov   esi,[_dc_source]
  mov   eax,[_dc_colormap]
  sub edi, 2*SCREENWIDTH

  test  ebp,1
  jnz   Even

  sub edi, SCREENWIDTH

Even:

  shr ebp,1

LoopRoll:
  mov  al,[esi]               ; get source pixel
  add  edi, 2*SCREENWIDTH
  mov  bl,[eax]               ; translate the color
  inc  esi
  mov  bh, bl
  dec  ebp
  mov  [edi],bx               ; draw a pixel to the buffer
  mov  [edi+SCREENWIDTH],bx
  jns  LoopRoll

donehr:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

CODE_SYM_DEF R_DrawColumnBackbufferSkyFullDirect
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
  js   near donehs

patchCentery:
  sub  eax,0x12345678
  mov  ebx,[_dc_x]

  lea  esi,[ebp+eax+0x64-(SCREENHEIGHT/2)]

  add  edi,[_columnofs+ebx*4]
  mov  eax,[_dc_colormap]

  add  esi,[_dc_source]

  jmp  [scalecalls+4+ebp*4]

donehs:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

CODE_SYM_DEF R_DrawColumnBackbufferDirect
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
  mov  esi,[_dc_source]
  add  edi,[_columnofs+ebx*4]
  mov  eax,[_dc_colormap]

  lea  esi,[esi+ebp-(SCREENHEIGHT/2)]

  jmp  [scalecalls+4+ebp*4]

doneh:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnBackbuffer ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%assign POSITION -(SCREENHEIGHT / 2)
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    mov  al,[esi+POSITION]              ; get source pixel
    mov  al,[eax]                       ; translate the color
    mov  [edi-(LINE-1)*SCREENWIDTH],al  ; draw a pixel to the buffer
    %assign LINE LINE-1
    %assign POSITION POSITION+1
%endrep

vscale1:
  pop	ebp
  mov al,[esi+POSITION]
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
