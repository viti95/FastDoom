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
  %rep SCREENWIDTH/4+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0
returnpoint: dd 0

BEGIN_CODE_SECTION

CODE_SYM_DEF R_PatchColumnofsPotatoPentium
  push  ebx
  mov   ebx,[_columnofs]
  mov   eax,patchColumnofs+2
  mov   [eax],ebx
  pop   ebx
  ret

; ==========================
; R_DrawSpanPotatoBackbuffer
; ==========================
CODE_SYM_DEF R_DrawSpanPotatoBackbufferPentium
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
  mov     eax,[_ds_colormap]

patchColumnofs:
  add     edi,0x12345678

  ; feed the pipeline and jump in

  mov   ebx,ecx
  mov   edx,ecx
  shr   ebx,4
  shr   edx,26
  and   ebx,0xFC0
  add   ecx,ebp
  or    ebx,edx

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
      %if LINE = SCREENWIDTH/4
        mov   al,[esi+ebx]           ; get source pixel
        mov   al,[eax]               ; translate color
        mov   ah,al
        mov   [edi+PLANE+PCOL*4],ax  ; write pixel
        mov   [edi+PLANE+PCOL*4+2],ax  ; write pixel
      %else
        mov   al,[esi+ebx]
        mov   bh,ch
        mov   dl,[eax]
        shr   ebx,4
        mov   dh,dl
        and   bx,0xFC0
        mov   [edi+PLANE+PCOL*4],dx
        mov   [edi+PLANE+PCOL*4+2],dx
        mov   edx,ecx
        shr   edx,26
        or    ebx,edx
        %if LINE < (SCREENWIDTH/4)-1
        add   ecx,ebp
        %endif
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

MAPLABEL LINE: 
  ret

%endif
