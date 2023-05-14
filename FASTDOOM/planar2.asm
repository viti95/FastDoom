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

; ==================
; R_DrawColumnPotato
; ==================
CODE_SYM_DEF R_DrawColumnPotato
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
  js   short .done

  add  edi,[_destview]
  add  edi,[_dc_x]

  mov   ecx,[_dc_iscale]

  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  xor   ebx,ebx
  shld  ebx,edx,7
  jmp  [scalecalls+4+ebp*4]

.done:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnPotato ends

; ===============
; R_DrawColumnLow
; ===============
CODE_SYM_DEF R_DrawColumnLow
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
  js   short .done

  ; set plane
  mov  ecx,[_dc_x]
  add  edi,[_destview]
  mov  esi, ecx
  
  and  cl,1
  mov  al,3
  add  cl, cl
  mov  dx,SC_INDEX+1
  shl  al,cl
  out  dx,al

  shr esi,1
  mov eax, ebx
  add edi,esi

  mov   ecx,[_dc_iscale]

  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  xor   ebx,ebx
  shld  ebx,edx,7
  jmp  [scalecalls+4+ebp*4]

.done:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnLow ends

CODE_SYM_DEF R_DrawColumn
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
  js   .done

  ; set plane
  mov  ecx,[_dc_x]
  add  edi,[_destview]
  mov  esi, ecx
  
  and  cl,3
  mov  dx,SC_INDEX+1
  mov  al,1
  shl  al,cl
  out  dx,al

  shr esi,2
  mov eax, ebx
  add edi,esi

  mov   ecx,[_dc_iscale]

  sub   eax,[_centery]
  imul  ecx

  mov   esi,[_dc_source]

  ; EVEN/ODD ?
  test ebp,1
  jne .odd

.even:
  mov   ebx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   ebx,eax
  shl   ebx,9 ; 7 significant bits, 25 frac

  mov  edx,ebx
  shr  edx,25 ; get address of first location

  mov  eax,[_dc_colormap]
  jmp  [scalecalls+4+ebp*4]

.odd:
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  mov  ebx,edx
  shr  ebx,25 ; get address of first location
  mov  eax,[_dc_colormap]
  jmp  [scalecalls+4+ebp*4]

.done:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumn ends

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    %if LINE % 2 = 1
      mov  al,[esi+edx]                   ; get source pixel
      lea  edx, [ebx+ecx]                 ; 386:2cc, 486:1cc
      shr  ebx, 25                        ; 386:3cc, 486:2cc
      mov  al,[eax]                       ; translate the color
      mov  [edi-(LINE-1)*80],al           ; draw a pixel to the buffer
    %else
      mov  al,[esi+ebx]                   ; get source pixel
      lea  ebx, [edx+ecx]                 ; 386:2cc, 486:1cc
      shr  edx, 25                        ; 386:3cc, 486:2cc
      mov  al,[eax]                       ; translate the color
      mov  [edi-(LINE-1)*80],al           ; draw a pixel to the buffer
    %endif
    %assign LINE LINE-1
%endrep

vscale1:
	pop	ebp
  mov al,[esi+edx]
  pop	esi
  mov al,[eax]
  pop	edx
  mov [edi],al

vscale0:
	pop	ecx
	pop	ebx
  pop	edi
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
  %rep (SCREENWIDTH/4)+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0
returnpoint: dd 0

CONTINUE_CODE_SECTION

; ================
; R_DrawSpanPotato
; ================
CODE_SYM_DEF R_DrawSpanPotato
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
  add     edi,[_destview]

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
; R_DrawSpanPotato ends

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
        mov   al,[eax]               ; translate color
        mov   [edi+PLANE+PCOL],al  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        shld  ebx,ecx,22             ; shift y units in
        mov   al,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   [edi+PLANE+PCOL],al  ; write pixel
        and   ebx,ebp                ; mask off slop bits
        add   ecx,edx                ; position += step
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

hmap80: ret

%endif
