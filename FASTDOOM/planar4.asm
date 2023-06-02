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
CODE_SYM_DEF R_DrawFuzzColumnFlatPotato
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
  js   .pdone

  add edi,[_destview]
  add edi,[_dc_x]

  xor ecx,ecx
  xor edx,edx

  mov ebx,[_colormaps]
  add ebx,0x600

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
CODE_SYM_DEF R_DrawFuzzColumnFlatLow
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
  js   .ldone

  mov ecx,[_dc_x]
  add edi,[_destview]
  mov esi,ecx

  ; outpw(GC_INDEX, GC_READMAP + ((dc_x & 1) << 9));
  mov eax,ecx
	and	eax,1
	shl	eax,9
	mov	dx,0x3CE
	add	eax,4
	out	dx,ax

  ; outp(SC_INDEX + 1, 3 << ((dc_x & 1) << 1));
  and  cl,1
  mov  al,3
  add  cl, cl
  mov  dx,SC_INDEX+1
  shl  al,cl
  out  dx,al

  shr esi,1
  add edi,esi

  xor ecx,ecx
  xor edx,edx

  mov ebx,[_colormaps]
  add ebx,0x600

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

CODE_SYM_DEF R_DrawFuzzColumnFlat
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
  js   short done

  mov ecx,[_dc_x]
  add edi,[_destview]
  mov esi,ecx

  ; outpw(GC_INDEX, GC_READMAP + ((dc_x & 3) << 8));
  mov eax,ecx
	and	eax,3
	shl	eax,8
	mov	dx,0x3CE
	add	eax,4
	out	dx,ax

  ; outp(SC_INDEX + 1, 1 << (dc_x & 3));
  and  cl,3
  mov  dx,SC_INDEX+1
  mov  al,1
  shl  al,cl
  out  dx,al

  shr esi,2
  add edi,esi

  xor ecx,ecx
  xor edx,edx

  mov ebx,[_colormaps]
  add ebx,0x600

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

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:

	mov   cl,[edi-(LINE-1)*80]
	mov		dl,[ecx+ebx]
  mov		[edi-(LINE-1)*80],dl

  %assign LINE LINE-1
%endrep

vscale1:

  mov   cl,[edi-(LINE-1)*80]
	pop	ebp
	mov		dl,[ecx+ebx]
  pop	esi
  mov		[edi-(LINE-1)*80],dl
  pop	edx

vscale0:
	pop	ecx
	pop	ebx
  pop	edi
  ret

%endif
