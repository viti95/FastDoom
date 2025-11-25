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

CODE_SYM_DEF R_PatchLinearHigh
  push  ebx
  
  mov   ebx,[_centery]
  mov   eax,patchCentery+1
  mov   [eax],ebx
  mov   eax,patchCenteryRoll+1
  mov   [eax],ebx

  mov   ebx,[_columnofs]
  mov   eax,patchColumnofs+2
  mov   [eax],ebx

  pop   ebx
  ret

; ======================
; R_DrawColumnBackbuffer
; ======================
CODE_SYM_DEF R_DrawColumnBackbuffer
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
  mov  ecx,[_dc_iscale]
  add  edi,[_columnofs+ebx*4]

patchCentery:
  sub   eax,0x12345678
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  mov  ebx,edx
  shr  ebx,25 ; get address of first location
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
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    add  edx,ecx                        ; calculate next location
    mov  al,[esi+ebx]                   ; get source pixel
    mov  ebx,edx
    mov  al,[eax]                       ; translate the color
    shr  ebx,25
    mov  [edi-(LINE-1)*SCREENWIDTH],al  ; draw a pixel to the buffer
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
	pop		ecx
	pop		ebx
  pop		edi
  ret

;============================================================================
; unwound horizontal texture mapping code
;
; eax   lighttable
; ebx   scratch register
; ecx   position 6.10 bits x, 6.10 bits y
; edx   step 6.10 bits x, 6.10 bits y
; esi   start of block
; edi   dest
; ebp   fff to mask bx
;
; ebp should by preset from ebx / ecx before calling
;============================================================================

CONTINUE_DATA_SECTION

%macro MAPDEFINE 1
  dd hmap%1
%endmacro

align 4

mapcalls:
  %assign LINE 0
  %rep SCREENWIDTH+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0
returnpoint: dd 0

CONTINUE_CODE_SECTION

; ====================
; R_DrawSpanBackbuffer
; ====================
CODE_SYM_DEF R_DrawSpanBackbuffer
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov     eax,[_ds_x1]
  mov     ebx,[_ds_x2]
  mov     eax,[mapcalls+eax*4]
  mov     ecx,[_ds_frac]        ; build composite position
  mov     [callpoint],eax       ; spot to jump into unwound
  mov     edx,[_ds_step]        ; build composite step
  mov     eax,[mapcalls+4+ebx*4]
  mov     esi,[_ds_source]
  mov     [returnpoint],eax     ; spot to patch a ret at
  mov     edi,[_ds_y]
  mov     [eax], byte OP_RET

  mov     edi,[_ylookup+edi*4]
  mov     eax,[_ds_colormap]

patchColumnofs:
  add     edi,0x12345678

  ; feed the pipeline and jump in

  shld    ebx,ecx,22  ; shift y units in
  mov     ebp,0x0FFF  ; used to mask off slop high bits from position
  shld    ebx,ecx,6   ; shift x units in
  and     ebx,ebp     ; mask off slop bits
  add     ecx,edx
  call    [callpoint]

  mov     ebx,[returnpoint]
	pop		ebp
  mov     [ebx],byte OP_MOVAL         ; remove the ret patched in

	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanBackbuffer ends

%macro MAPLABEL 1
  hmap%1
%endmacro

%assign LINE 0
%assign PCOL 0
%rep SCREENWIDTH
  %assign PLANE 0
    MAPLABEL LINE:
      %assign LINE LINE+1
      %if LINE = SCREENWIDTH
        mov   al,[esi+ebx]           ; get source pixel
        mov   al,[eax]               ; translate color
        mov   [edi+PLANE+PCOL],al  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        shld  ebx,ecx,22             ; shift y units in
        mov   al,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   [edi+PLANE+PCOL],al  ; write pixel
        and   ebx,ebp                ; mask off slop bits
        %if LINE < SCREENWIDTH-1
        add   ecx,edx                ; position += step
        %endif
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

MAPLABEL LINE:
  ret

CODE_SYM_DEF R_DrawColumnBackbufferRoll
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
  mov  ecx,[_dc_iscale]
  add  edi,[_columnofs+ebx*4]

patchCenteryRoll:
  sub   eax,0x12345678
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  cmp   ebp, 1
  je    SinglePixel

  test  ebp,1
  jz   Even

  mov  ebx,edx
  shr  ebx,25 ; get address of first location
  add  edx,ecx
  mov  al,[esi+ebx]                   ; get source pixel
  mov  al,[eax]                       ; translate the color
  mov  [edi],al  ; draw a pixel to the buffer

  dec  ebp

  ; MMX 2 pixels render
Even:
  
  pxor       mm4, mm4

  ;movd       mm0, eax
  movd       mm1, esi
  ;movd       mm2, edi
  movd       mm3, edx
  movd       mm4, ecx

  mov        ecx, eax

  ;punpckldq  mm0, mm0
  punpckldq  mm1, mm1
  ;punpckldq  mm2, mm2
  punpckldq  mm3, mm3

  paddd      mm3, mm4

  punpckldq  mm4, mm4

  paddd      mm4, mm4

LoopMMX:

  movq       mm5, mm3

  psrld      mm5, 25

  paddd      mm5, mm1

  movd       ebx, mm5

  psrlq      mm5,32

  mov  al,[ebx]

  movd       edx, mm5

  mov  al,[eax]

  mov  cl,[edx]

  mov  [edi+SCREENWIDTH], al

  mov  cl,[ecx]

  paddd      mm3, mm4

  mov  [edi], cl

  add  edi, 2*SCREENWIDTH

  sub  ebp, 2
  jns  LoopMMX

donehr:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

SinglePixel:

  shr  edx,25 ; get address of first location
  mov  al,[esi+edx]                   ; get source pixel
  mov  al,[eax]                       ; translate the color
  mov  [edi],al  ; draw a pixel to the buffer

  jmp  donehr


%endif
