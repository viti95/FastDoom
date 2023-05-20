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

BEGIN_DATA_SECTION

dest:       dd 0
endplane:   dd 0
curplane:   dd 0
frac:       dd 0
fracpstep:  dd 0
curx:       dd 0
curpx:      dd 0
endpx:      dd 0
loopcount:  dd 0

BEGIN_CODE_SECTION

; ===========================================================================
; R_DrawSpan
; horizontal texture mapping, full detail
; ===========================================================================
CODE_SYM_DEF R_DrawSpan
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov  eax,[_ds_x1]
  mov  [curx],eax
  mov  ebx,eax
  and  ebx,3
  shr  eax,2
  mov  [endplane],ebx
  mov  ebp,[_ds_y]
  mov  [curplane],ebx
  lea  edi,[ebp+ebp*4]
  mov  ebx,[_ds_frac]
  shl  edi,4
  mov  [frac],ebx
  add  edi,eax
  mov  ebx,[_ds_step]
  add  edi,[_destview]
  shl   ebx,2
  mov  [dest],edi
  mov   [fracpstep],ebx
  mov   eax,.hpatch1+2
  mov   [eax],ebx
  mov   eax,.hpatch2+2
  mov   [eax],ebx
  mov   ecx,[curplane]
.hplane:
  mov   al,1
  shl   al,cl
  mov   dx,SC_INDEX+1
  out   dx,al
  mov   eax,[_ds_x2]
  cmp   [curx], eax
  ja   .hdone
  sub   eax,[curplane]
  js   .hdoneplane
  shr   eax,2
  mov   [endpx],eax
  dec   eax
  js   .hfillone
  mov   ebx,[curx]
  shr   ebx,2
  shr   eax,1
  cmp   [endpx], ebx
  jz   .hfillone
  mov   [curpx],ebx
  inc   ebx
  shr   ebx,1
  inc   eax
  sub   eax,ebx
  js   .hdoneplane
  mov   [loopcount],eax
  mov   esi,[_ds_source]
  mov   eax,[_ds_colormap]
  mov   edi,[dest]
  mov   ebx,eax
  mov   ebp,[frac]
  test  [curpx],dword 1
  jz    short .hfill
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  and   ecx,0x00000FFF
  mov   al,[esi+ecx]
  add   ebp,[fracpstep]
  mov   dl,[eax]
  mov   [edi],dl
  inc   edi
  jz   .hdoneplane
.hfill:
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  add   ebp,[fracpstep]
  and   ecx,0x00000FFF
  shld  edx,ebp,22
  shld  edx,ebp,6
  add   ebp,[fracpstep]
  and   edx,0x00000FFF
  mov   al,[esi+ecx]
  mov   bl,[esi+edx]
  mov   dl,[eax]
  test  [loopcount],dword -1
  jnz   short .hdoubleloop
  jmp   short .hchecklast
.hfillone:  
  mov   ebp,[frac]
  mov   esi,[_ds_source]
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  and   ecx,0x00000FFF
  mov   eax,[_ds_colormap]
  mov   al,[esi+ecx]
  mov   edi,[dest]
  mov   dl,[eax]
  mov   [edi],dl
  jmp   short .hdoneplane
.hdoubleloop:
  shld  ecx,ebp,22
  mov   dh,[ebx]
  shld  ecx,ebp,6
.hpatch1:
  add   ebp,0x12345678 ; runtime patched
  and   ecx,0x00000FFF
  mov   [edi],dx
  shld  edx,ebp,22
  add   edi,2
  shld  edx,ebp,6
.hpatch2:
  add   ebp,0x12345678 ; runtime patched
  and   edx,0x00000FFF
  mov   al,[esi+ecx]
  mov   bl,[esi+edx]
  dec   dword [loopcount]
  mov   dl,[eax]
  jnz   short .hdoubleloop
.hchecklast:
  test  [endpx],dword 1
  jnz   short .hdoneplane
  mov   [edi],dl
.hdoneplane:
  mov   ecx,[curplane]
  inc   ecx
  and   ecx,3
  jnz   short .hskip
  inc   dword [dest]
.hskip:
  cmp   [endplane],ecx
  jz    short .hdone
  mov   ebx,[frac]
  mov   [curplane],ecx
  add   ebx,[_ds_step]
  inc   dword [curx]
  mov   [frac],ebx  
  jmp   near .hplane
.hdone:
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpan ends


; ===========================================================================
; R_DrawSpanLow
; horizontal texture mapping, 2 columns at once
; ===========================================================================
CODE_SYM_DEF R_DrawSpanLow
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov  eax,[_ds_x1]
  mov  [curx],eax
  mov  ebx,eax
  and  ebx,1
  mov  [endplane],ebx
  mov  [curplane],ebx
  shr  eax,1
  mov  ebp,[_ds_y]
  lea  edi,[ebp+ebp*4]
  shl  edi,4
  add  edi,eax
  add  edi,[_destview]
  mov  [dest],edi

  mov  ebx,[_ds_frac]

  mov  [frac],ebx

  mov  ebx,[_ds_step]

  add   ebx, ebx
  mov   [fracpstep],ebx
  mov   eax,.lpatch1+2
  mov   [eax],ebx
  mov   eax,.lpatch2+2
  mov   [eax],ebx
  mov   ecx,[curplane]
.lplane:
  mov   al,3
  mov   dx,SC_INDEX+1
  shl   al,cl
  shl   al,cl
  out   dx,al
  mov   eax,[_ds_x2]
  cmp   [curx],eax
  ja   .ldone
  sub   eax,[curplane]
  js   .ldoneplane
  shr   eax,1
  mov   [endpx],eax
  dec   eax
  js    .lfillone
  mov   ebx,[curx]
  shr   ebx,1
  shr   eax,1
  cmp   [endpx],ebx
  jz    .lfillone
  mov   [curpx],ebx
  inc   ebx
  shr   ebx,1
  inc   eax
  sub   eax,ebx
  js    .ldoneplane
  mov   [loopcount],eax
  mov   esi,[_ds_source]
  mov   eax,[_ds_colormap]
  mov   edi,[dest]
  mov   ebx,eax
  mov   ebp,[frac]
  test  [curpx],dword 1
  jz    short .lfill
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  and   ecx,0x00000FFF
  mov   al,[esi+ecx]
  add   ebp,[fracpstep]
  mov   dl,[eax]
  mov   [edi],dl
  inc   edi
  jz    .ldoneplane
.lfill:
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  add   ebp,[fracpstep]
  and   ecx,0x00000FFF
  shld  edx,ebp,22
  shld  edx,ebp,6
  add   ebp,[fracpstep]
  and   edx,0x00000FFF
  mov   al,[esi+ecx]
  mov   bl,[esi+edx]
  mov   dl,[eax]
  test  [loopcount],dword -1
  jnz   short .ldoubleloop
  jmp   short .lchecklast
.lfillone:
  mov   ebp,[frac]
  mov   eax,[_ds_colormap]
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  and   ecx,0x00000FFF
  mov   esi,[_ds_source]
  mov   al,[esi+ecx]
  mov   edi,[dest]
  mov   dl,[eax]
  mov   [edi],dl
  jmp   short .ldoneplane
.ldoubleloop:
  shld  ecx,ebp,22
  mov   dh,[ebx]
  shld  ecx,ebp,6
.lpatch1:
  add   ebp,0x12345678 ; runtime patched
  and   ecx,0x00000FFF
  mov   [edi],dx
  shld  edx,ebp,22
  add   edi,2
  shld  edx,ebp,6
.lpatch2:
  add   ebp,0x12345678 ; runtime patched
  mov   al,[esi+ecx]
  and   edx,0x00000FFF
  dec   dword [loopcount]
  mov   bl,[esi+edx]
  mov   dl,[eax]
  jnz   short .ldoubleloop
.lchecklast:
  test  [endpx],dword 1
  jnz   short .ldoneplane
  mov   [edi],dl
.ldoneplane:
  mov   ecx,[curplane]
  inc   ecx
  and   ecx,1
  jnz   short .lskip
  inc   dword [dest]
.lskip:
  cmp   [endplane],ecx
  jz    short .ldone
  mov   ebx,[frac]
  mov   [curplane],ecx
  add   ebx,[_ds_step]
  inc   dword [curx]
  mov   [frac],ebx
  jmp   near .lplane
.ldone:
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanLow ends

CODE_SYM_DEF R_DrawSpanFlatPotato
	push		ebx
	push		ecx
	push		edi
	mov		eax,[_ds_source]
  xor   ebx,ebx
  mov   bl,[eax+0x74A]        ;FLATPIXELCOLOR
	mov		eax,[_ds_colormap]
  mov		ecx,[_ds_x2]
  mov		edi,[_ds_x1]
  sub   ecx,edi
	mov		al,byte [ebx+eax]
  mov		ebx,[_ds_y]
  add		edi,[_destview]
  and   eax,0xFF
  add   edi,[_ylookup+ebx*4]
	inc		ecx
	test  cl,1
	je		.writewords
  mov   [edi],al
  inc   edi
  dec   ecx
  jz    .done
.writewords:
  mov   ah,al
	sar		ecx,1
	rep stosw
.done:
	pop		edi
	pop		ecx
	pop		ebx
	ret

%endif
