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

BEGIN_DATA_SECTION

%macro MAPDEFINE 1
  dd hmap%1
%endmacro

align 4

mapcalls:
  %assign LINE 0
  %rep SCREENWIDTH/2+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0

BEGIN_CODE_SECTION

; =======================
; R_DrawSpanLowBackbuffer
; =======================
CODE_SYM_DEF R_DrawSpanLowBackbuffer386SX
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
  mov     cr2,eax     ; spot to patch a ret at
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

  mov     ebx,cr2
	pop		ebp
  mov     [ebx],byte OP_MOVAL         ; remove the ret patched in

	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanLowBackbuffer ends

%macro MAPLABEL 1
  hmap%1
%endmacro

%assign LINE 0
%assign PCOL 0
%rep SCREENWIDTH/2
  %assign PLANE 0
    MAPLABEL LINE:
      %assign LINE LINE+1
      %if LINE = SCREENWIDTH/2
        mov   al,[esi+ebx]           ; get source pixel
        mov   dl,[eax]               ; translate color
        mov   dh,dl
        mov   [edi+PLANE+PCOL*2],dx  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        shld  ebx,ecx,22             ; shift y units in
        mov   dl,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   dh,dl
        mov   [edi+PLANE+PCOL*2],dx  ; write pixel
        and   ebx,0x0FFF             ; mask off slop bits
        add   ecx,ebp                ; position += step
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

MAPLABEL LINE:
  ret

%endif
