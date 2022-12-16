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

pixelcount: dd 0
loopcount:  dd 0

BEGIN_CODE_SECTION

; ===========================================================================
; R_DrawColumn
; vertical texture mapping, full resolution
; ===========================================================================
CODE_SYM_DEF R_DrawColumn
  pushad

  mov  eax,[_dc_yh]
  mov  ebp,[_dc_yl]
  inc  eax
  sub  eax,ebp           ; pixel count

  js   .done             ; nothing to scale

  mov  ebx,[_dc_x]
  mov  [pixelcount],eax  ; save for final pixel
  mov  cl,bl
  shr  eax,1             ; double pixel count
  and  cl,3
  mov  [loopcount],eax
  mov  dx,SC_INDEX+1
  mov  al,1
  lea  edi,[ebp+ebp*4]
  shl  al,cl
  shl  edi,4
  shr  ebx,2
  out  dx,al
  add  edi,ebx
  mov   eax,[_centery]
  add  edi,[_destview]
  mov  ecx,[_dc_iscale]
  sub   eax,ebp
  imul  ecx
  mov   ebp,[_dc_texturemid]
  mov  esi,[_dc_source]
  sub   ebp,eax
  mov  ebx,[_dc_iscale]
  shl   ebp,9 ; 7 significant bits, 25 frac
  shl  ebx,9
  mov  eax,.patch1+2
  mov  [eax],ebx
  mov  eax,.patch2+2
  mov  [eax],ebx

  ; eax = aligned colormap
  ; ebx = aligned colormap
  ; ecx,edx = scratch
  ; esi = virtual source
  ; edi = moving destination pointer
  ; ebp = frac

  mov  ecx,ebp  ; begin calculating first pixel
  add  ebp,ebx  ; advance frac pointer
  shr  ecx,25   ; finish calculation for first pixel
  mov  edx,ebp  ; begin calculating second pixel
  mov  eax,[_dc_colormap]
  add  ebp,ebx  ; advance frac pointer
  shr  edx,25   ; finish calculation for second pixel
  mov  ebx,eax
  mov  al,[esi+ecx]  ; get first pixel
  mov  bl,[esi+edx]  ; get second pixel
  mov  al,[eax]      ; color translate first pixel
  mov  bl,[ebx]      ; color translate second pixel

  test  [pixelcount],dword -2
  jnz   short .doubleloop ; at least two pixels to map?
  jmp   short .checklast
.doubleloop:
  mov   ecx,ebp ; begin calculating third pixel
.patch1:
  add   ebp,0x12345678 ; advance frac pointer (runtime patched)
  mov   [edi],al       ; write first pixel
  shr   ecx,25         ; finish calculation for third pixel
  mov   edx,ebp        ; begin calculating fourth pixel
.patch2:
  add   ebp,0x12345678          ; advance frac pointer (runtime patched)
  mov   [edi+SCREENWIDTH/4],bl  ; write second pixel
  shr   edx,25                  ; finish calculation for fourth pixel
  mov   al,[esi+ecx]            ; get third pixel
  add   edi,SCREENWIDTH/2	      ; advance to third pixel destination
  mov   bl,[esi+edx]            ; get fourth pixel
  dec   dword [loopcount]       ; done with loop?
  mov   al,[eax]                ; color translate third pixel
  mov   bl,[ebx]                ; color translate fourth pixel
  jnz   short .doubleloop
.checklast:
  test  [pixelcount],dword 1
  jz    short .done
  mov   [edi],al ; write final pixel
.done:
  popad
  ret
; R_DrawColumn ends

CONTINUE_DATA_SECTION

dest:       dd 0
endplane:   dd 0
curplane:   dd 0
frac:       dd 0
fracpstep:  dd 0
curx:       dd 0
curpx:      dd 0
endpx:      dd 0

CONTINUE_CODE_SECTION

; ===========================================================================
; R_DrawSpan
; horizontal texture mapping, full detail
; ===========================================================================
CODE_SYM_DEF R_DrawSpan
  pushad

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
  popad
  ret
; R_DrawSpan ends


; ===========================================================================
; R_DrawSpanLow
; horizontal texture mapping, 2 columns at once
; ===========================================================================
CODE_SYM_DEF R_DrawSpanLow
  pushad

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
  popad
  ret
; R_DrawSpanLow ends

%endif
