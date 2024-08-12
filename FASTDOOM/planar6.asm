;
; Copyright (C) 1993-1996 Id Software, Inc.
; Copyright (C) 1993-2008 Raven Software
; Copyright (C) 2016-2017 Alexey Khokholov (Nuke.YKT)
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
; DESCRIPTION: Assembly texture mapping routines for planar VGA mode
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

CODE_SYM_DEF R_DrawColumnPotatoDirect
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
  js   short donep

  add  edi,[_destview]
  add  edi,[_dc_x]

  mov   ecx,[_dc_iscale]

  mov   esi,[_dc_source]
  mov   eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

donep:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

CODE_SYM_DEF R_DrawColumnLowDirect
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
  js   short donel

  ; set plane
  mov  ecx,[_dc_x]
  add  edi,[_destview]
  mov  esi, ecx
  
  and   ecx,1
  shr   esi,1
  lea   eax,[ecx+ecx*8+3]
  mov   dx,SC_INDEX+1
  add   edi,esi
  out   dx,al

  mov   esi,[_dc_source]
  mov   eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

donel:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

CODE_SYM_DEF R_DrawColumnDirect
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov   ebp,[_dc_yh]
  mov   ebx,[_dc_yl]
  mov   edi,[_ylookup+ebp*4]
  sub   ebp,ebx         ; ebp = pixel count
  js    short doneh

  mov   ecx,[_dc_x]
  add   edi,[_destview]
  mov   esi, ecx
  
  and   cl,3
  mov   dx,SC_INDEX+1
  mov   al,1
  shl   al,cl
  out   dx,al

  shr   esi,2
  add   edi,esi

  mov   esi,[_dc_source]
  mov   eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

doneh:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

CODE_SYM_DEF R_DrawColumnSkyFullDirect
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov   ebp,[_dc_yh]
  mov   ebx,[_dc_yl]
  mov   edi,[_ylookup+ebp*4]
  sub   ebp,ebx         ; ebp = pixel count
  js    short donehs

  mov   ecx,[_dc_x]
  add   edi,[_destview]
  mov   esi, ecx
  
  and   cl,3
  mov   dx,SC_INDEX+1
  mov   al,1
  shl   al,cl
  out   dx,al

  shr   esi,2
  add   edi,esi

  mov   esi,[_dc_source]
  add   esi,16
  mov   eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

donehs:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    mov  al,[esi]                   ; get source pixel
    mov  al,[eax]                       ; translate the color
    inc  esi
    mov  [edi-(LINE-1)*80],al           ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
	pop	ebp
  mov al,[esi]
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
