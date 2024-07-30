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
  %rep SCREENWIDTH+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0
returnpoint: dd 0

BEGIN_CODE_SECTION

; ==========
; R_DrawSpan
; ==========
CODE_SYM_DEF R_DrawSpan
  pushad

  mov     eax,[_ds_x1]
  mov     ebx,[_ds_x2]
  mov     eax,[mapcalls+eax*4]
  mov     [callpoint],eax       ; spot to jump into unwound
  
  mov     eax,[mapcalls+4+ebx*4]
  
  mov     [returnpoint],eax     ; spot to patch a ret at
  mov     edi,[_ds_y]
  mov     [eax], byte OP_RET

  mov     edx, SC_INDEX+1

  mov     esi,[_ds_source]
  mov     ecx,[_ds_frac]        ; build composite position
  mov     ebp,[_ds_step]        ; build composite step
  mov     edi,[_ylookup+edi*4]
  mov     eax,[_ds_colormap]
  add     edi,[_destview]

  ; feed the pipeline and jump in

  shld    ebx,ecx,22  ; shift y units in
  shld    ebx,ecx,6   ; shift x units in
  and     ebx,0x0FFF  ; mask off slop bits
  add     ecx,ebp  
  call    [callpoint]

  mov     ebx,[returnpoint]

  mov     [ebx],byte OP_MOVALPLANE         ; remove the ret patched in

  popad
  ret

%macro MAPLABEL 1
  hmap%1
%endmacro

%assign LINE 0
%assign PCOL 0
%rep SCREENWIDTH/4
  %assign PLANE 0
  %rep 4
      MAPLABEL LINE:
      mov al, 1 << PLANE
      out dx, al
      %assign LINE LINE+1
      %if LINE = 320
        mov   al,[esi+ebx]           ; get source pixel
        mov   al,[eax]               ; translate color
        mov   [edi+PCOL],al  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        shld  ebx,ecx,22             ; shift y units in
        mov   al,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   [edi+PCOL],al  ; write pixel
        and   ebx,0x0FFF             ; mask off slop bits
        add   ecx,ebp                ; position += step
      %endif
      %assign PLANE PLANE+1
      
  %endrep
%assign PCOL PCOL+1
%endrep

hmap320:
  ret

%endif
