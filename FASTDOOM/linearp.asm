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

; ============================
; R_DrawColumnPotatoBackbuffer
; ============================
CODE_SYM_DEF R_DrawColumnPotatoBackbuffer
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov  ebx,[_dc_x]
  mov  ebp,[_dc_yh]
  mov  edi,[_columnofs+ebx*4]
  mov  eax,[_dc_yl]
  add  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   near .done

  mov   ecx,[_dc_iscale]

  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

.done:
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawColumnPotatoBackbuffer ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    xor  ebp,ebp
    shld ebp,edx,7
    mov  al,[esi+ebp]                   ; get source pixel
    add  edx,ecx                        ; calculate next location
    mov  bl,[eax]                       ; translate the color
    mov  bh,bl
    mov  [edi-(LINE-1)*SCREENWIDTH],bx  ; draw a pixel to the buffer
    mov  [edi-(LINE-1)*SCREENWIDTH + 2],bx  ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  xor  ebp,ebp
  shld ebp,edx,7
  mov al,[esi+ebp]
  mov bl,[eax]
  mov bh,bl
  mov [edi],bx
  mov [edi+2],bx

vscale0:
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
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
  %rep SCREENWIDTH/4+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0
returnpoint: dd 0

CONTINUE_CODE_SECTION

; ==========================
; R_DrawSpanPotatoBackbuffer
; ==========================
CODE_SYM_DEF R_DrawSpanPotatoBackbuffer
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
  mov     ebp,[_ds_step]        ; build composite step
  mov     eax,[mapcalls+4+ebx*4]
  mov     esi,[_ds_source]
  mov     [returnpoint],eax     ; spot to patch a ret at
  mov     edi,[_ds_y]
  mov     [eax], byte OP_RET

  mov     edi,[_ylookup+edi*4]


  ; feed the pipeline and jump in

  shld    ebx,ecx,22  ; shift y units in
  mov     eax,[_ds_colormap]
  shld    ebx,ecx,6   ; shift x units in
  add     edi,[_columnofs]
  and     ebx,0x0FFF  ; mask off slop bits
  add     ecx,ebp
  xor     edx,edx
  call    [callpoint]

  mov     ebx,[returnpoint]
  mov     [ebx],byte OP_MOVAL         ; remove the ret patched in

	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanPotatoBackbuffer ends

%macro MAPLABEL 1
  hmap%1
%endmacro

%assign LINE 0
%assign PCOL 0
%rep SCREENWIDTH/4
  %assign PLANE 0
    MAPLABEL LINE:
      %assign LINE LINE+1
      %if LINE = 80
        mov   al,[esi+ebx]           ; get source pixel
        mov   dl,[eax]               ; translate color
        mov   dh,dl
        mov   [edi+PLANE+PCOL*4],dx  ; write pixel
        mov   [edi+PLANE+PCOL*4+2],dx  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        shld  ebx,ecx,22             ; shift y units in
        mov   dl,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   dh,dl
        mov   [edi+PLANE+PCOL*4],dx  ; write pixel        
        mov   [edi+PLANE+PCOL*4+2],dx  ; write pixel
        and   ebx,0x0FFF             ; mask off slop bits
        add   ecx,ebp                ; position += step
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

hmap80: ret

%endif
