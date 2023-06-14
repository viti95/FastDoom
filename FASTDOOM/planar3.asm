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
extern _viewheight
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

; ==================
; R_DrawColumnPotato
; ==================
CODE_SYM_DEF R_DrawFuzzColumnPotato
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  eax,[_viewheight]
  dec  eax

  mov  ebp,[_dc_yh]

  cmp  eax,ebp
  jne  dc_yhOKP

  dec  eax
  mov  ebp,eax

dc_yhOKP:
  mov  eax,[_dc_yl]

  test eax,eax
  jne dc_ylOKP

  mov  eax,1

dc_ylOKP:
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   .pdone

  add edi,[_destview]
  mov eax,[_colormaps]
  add edi,[_dc_x]
  mov	ecx,[_fuzzposinverse]
  add eax,0x600
  mov edx,_fuzzoffsetinverse
  mov ebx,49

  jmp  [scalecalls+4+ebp*4]

.pdone:
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
CODE_SYM_DEF R_DrawFuzzColumnLow
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  eax,[_viewheight]
  dec  eax

  mov  ebp,[_dc_yh]

  cmp  eax,ebp
  jne  dc_yhOKL

  dec  eax
  mov  ebp,eax

dc_yhOKL:
  mov  eax,[_dc_yl]

  test eax,eax
  jne dc_ylOKL

  mov  eax,1

dc_ylOKL:
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   .ldone

  mov ecx,[_dc_x]
  add edi,[_destview]
  mov esi,ecx

  ; outpw(GC_INDEX, GC_READMAP + ((dc_x & 1) << 9));
  and ecx,1
  mov eax,ecx
	shl	eax,9
	mov	dx,0x3CE
	add	eax,4
	out	dx,ax

  ; outp(SC_INDEX + 1, 3 << ((dc_x & 1) << 1));
  mov  al,3
  add  cl,cl
  mov  dx,SC_INDEX+1
  shl  al,cl
  out  dx,al

  shr esi,1
  mov eax,[_colormaps]
  add edi,esi
  mov	ecx,[_fuzzposinverse]
  add eax,0x600
  mov edx,_fuzzoffsetinverse
  mov ebx,49

  jmp  [scalecalls+4+ebp*4]

.ldone:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnLow ends

CODE_SYM_DEF R_DrawFuzzColumn
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  eax,[_viewheight]
  dec  eax

  mov  ebp,[_dc_yh]

  cmp  eax,ebp
  jne  dc_yhOK

  dec  eax
  mov  ebp,eax

dc_yhOK:
  mov  eax,[_dc_yl]

  test eax,eax
  jne dc_ylOK

  mov  eax,1

dc_ylOK:
  mov  edi,[_ylookup+ebp*4]
  sub  ebp,eax         ; ebp = pixel count
  js   short done

  mov ecx,[_dc_x]
  add edi,[_destview]
  mov esi,ecx

  ; outpw(GC_INDEX, GC_READMAP + ((dc_x & 3) << 8));
  mov al,4
  and cl,3
  mov	dx,0x3CE
  mov ah,cl
	out	dx,ax

  ; outp(SC_INDEX + 1, 1 << (dc_x & 3));
  mov  al,1
  shl  al,cl
  mov  dx,SC_INDEX+1
  out  dx,al

  shr esi,2
  mov eax,[_colormaps]
  add edi,esi
  mov	ecx,[_fuzzposinverse]
  add eax,0x600
  mov edx,_fuzzoffsetinverse
  mov ebx,49

  jmp  [scalecalls+4+ebp*4]

done:
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
	mov   al,[edi+ebp-(LINE-1)*80]
  dec   ecx
	mov		al,[eax]
  JMPTESTFUZZPOSDEFINE LINE
  mov   ecx,ebx
  TESTFUZZPOSDEFINE LINE:
  mov		[edi-(LINE-1)*80],al
  %assign LINE LINE-1
%endrep

vscale0:
	pop	ebp
  mov [_fuzzposinverse],ecx
  pop	esi
  pop	edx
	pop	ecx
	pop	ebx
  pop	edi
  ret

%endif
