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
%include "./FastDoom/defs.inc"
%include "./FastDoom/macros.inc"

extern _destview
extern _centery

BEGIN_DATA_SECTION

pixelcount: dd 0
loopcount:  dd 0

BEGIN_CODE_SECTION

; ===========================================================================
; R_DrawColumnPotato
; vertical texture mapping, 4 columns at once
; ===========================================================================
CODE_SYM_DEF R_DrawColumnPotato
  pushad

  mov  ebp,[_dc_yl]
  lea  edi,[ebp+ebp*4]
  sal  edi,4
  mov  ebx,[_dc_x]
  add  edi,ebx
  add  edi,[_destview]

  mov  eax,[_dc_yh]
  inc  eax
  sub  eax,ebp           ; pixel count
  mov  [pixelcount],eax  ; save for final pixel
  js   .donep            ; nothing to scale
  shr  eax,1             ; double pixel count
  mov  [loopcount],eax

  mov  ecx,[_dc_iscale]

  mov   eax,[_centery]
  sub   eax,ebp
  imul  ecx
  mov   ebp,[_dc_texturemid]
  sub   ebp,eax
  shl   ebp,9  ; 7 significant bits, 25 frac

  mov  esi,[_dc_source]

  mov  ebx,[_dc_iscale]
  shl  ebx,9
  mov  eax,.patch1p+2  ; self-modifying code...
  mov  [eax],ebx
  mov  eax,.patch2p+2  ; self-modifying code...
  mov  [eax],ebx

  ; eax = aligned colormap
  ; ebx = aligned colormap
  ; ecx,edx = scratch
  ; esi = virtual source
  ; edi	= moving destination pointer
  ; ebp = frac

  mov  ecx,ebp  ; begin calculating first pixel
  add  ebp,ebx  ; advance frac pointer
  shr  ecx,25   ; finish calculation for first pixel
  mov  edx,ebp  ; begin calculating second pixel
  add  ebp,ebx  ; advance frac pointer
  shr  edx,25   ; finish calculation for second pixel
  mov  eax,[_dc_colormap]
  mov  ebx,eax
  mov  al,[esi+ecx]  ; get first pixel
  mov  bl,[esi+edx]  ; get second pixel
  mov  al,[eax]      ; color translate first pixel
  mov  bl,[ebx]      ; color translate second pixel

  test  [pixelcount],dword -2
  jnz   short .doubleloopp ; at least two pixels to map?
  jmp   short .checklastp
.doubleloopp:
  mov  ecx,ebp ; begin calculating third pixel
.patch1p:
  add  ebp,0x12345678 ; advance frac pointer (runtime patched)
  mov  [edi],al       ; write first pixel
  shr  ecx,25         ; finish calculation for third pixel
  mov  edx,ebp        ; begin calculating fourth pixel
.patch2p:
  add  ebp,0x12345678          ; advance frac pointer (runtime patched)
  mov  [edi+SCREENWIDTH/4],bl  ; write second pixel
  shr  edx,25                  ; finish calculation for fourth pixel
  mov  al,[esi+ecx]            ; get third pixel
  add  edi,SCREENWIDTH/2       ; advance to third pixel destination
  mov  bl,[esi+edx]            ; get fourth pixel
  dec  dword [loopcount]       ; done with loop?
  mov  al,[eax]                ; color translate third pixel
  mov  bl,[ebx]                ; color translate fourth pixel
  jnz  short .doubleloopp
.checklastp:
  test  [pixelcount],dword 1
  jz    short .donep
  mov   [edi],al ; write final pixel
.donep:
  popad
  ret
; R_DrawColumnPotato ends

; ===========================================================================
; R_DrawColumnLow
; vertical texture mapping, 2 columns at once
; ===========================================================================
CODE_SYM_DEF R_DrawColumnLow
  pushad

  mov  ebp,[_dc_yl]
  lea  edi,[ebp+ebp*4]
  shl  edi,4
  mov  ebx,[_dc_x]
  mov  ecx,ebx
  shr  ebx,1
  add  edi,ebx
  add  edi,[_destview]
  and  ecx,1
  shl  ecx,1
  mov  eax,3
  shl  eax,cl
  mov  edx,SC_INDEX+1
  out  dx,al

  mov  eax,[_dc_yh]
  inc  eax
  sub  eax,ebp           ; pixel count
  mov  [pixelcount],eax  ; save for final pixel
  js   .donel            ; nothing to scale
  shr  eax,1             ; double pixel count
  mov  [loopcount],eax

  mov  ecx,[_dc_iscale]

  mov   eax,[_centery]
  sub   eax,ebp
  imul  ecx
  mov   ebp,[_dc_texturemid]
  sub   ebp,eax
  shl   ebp,9 ; 7 significant bits, 25 frac

  mov  esi,[_dc_source]

  mov  ebx,[_dc_iscale]
  shl  ebx,9
  mov  eax,.patch1l+2  ; self-modifying code...
  mov  [eax],ebx
  mov  eax,.patch2l+2  ; self-modifying code...
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
  add  ebp,ebx  ; advance frac pointer
  shr  edx,25   ; finish calculation for second pixel
  mov  eax,[_dc_colormap]
  mov  ebx,eax
  mov  al,[esi+ecx]  ; get first pixel
  mov  bl,[esi+edx]  ; get second pixel
  mov  al,[eax]      ; color translate first pixel
  mov  bl,[ebx]      ; color translate second pixel

  test  [pixelcount],dword -2
  jnz   short .doubleloopl ; at least two pixels to map?
  jmp   short .checklastl
.doubleloopl:
  mov   ecx,ebp ; begin calculating third pixel
.patch1l:
  add   ebp,0x12345678  ; advance frac pointer (runtime patched)
  mov   [edi],al        ; write first pixel
  shr   ecx,25          ; finish calculation for third pixel
  mov   edx,ebp         ; begin calculating fourth pixel
.patch2l:
  add   ebp,0x12345678          ; advance frac pointer (runtime patched)
  mov   [edi+SCREENWIDTH/4],bl  ; write second pixel
  shr   edx,25                  ; finish calculation for fourth pixel
  mov   al,[esi+ecx]            ; get third pixel
  add   edi,SCREENWIDTH/2       ; advance to third pixel destination
  mov   bl,[esi+edx]            ; get fourth pixel
  dec   dword [loopcount]	      ; done with loop?
  mov   al,[eax]                ; color translate third pixel
  mov   bl,[ebx]                ; color translate fourth pixel
  jnz   short .doubleloopl
.checklastl:
  test  [pixelcount],dword 1
  jz    short .donel
  mov   [edi],al ; write final pixel
.donel:
  popad
  ret
; R_DrawColumnLow ends

; ===========================================================================
; R_DrawColumn
; vertical texture mapping, full resolution
; ===========================================================================
CODE_SYM_DEF R_DrawColumn
  pushad

  mov  ebp,[_dc_yl]
  lea  edi,[ebp+ebp*4]
  shl  edi,4
  mov  ebx,[_dc_x]
  mov  ecx,ebx
  shr  ebx,2
  add  edi,ebx
  add  edi,[_destview]
  and  ecx,3
  mov  eax,1
  shl  eax,cl
  mov  edx,SC_INDEX+1
  out  dx,al

  mov  eax,[_dc_yh]
  inc  eax
  sub  eax,ebp           ; pixel count
  mov  [pixelcount],eax  ; save for final pixel
  js   .done             ; nothing to scale
  shr  eax,1             ; double pixel count
  mov  [loopcount],eax

  mov  ecx,[_dc_iscale]

  mov   eax,[_centery]
  sub   eax,ebp
  imul  ecx
  mov   ebp,[_dc_texturemid]
  sub   ebp,eax
  shl   ebp,9 ; 7 significant bits, 25 frac

  mov  esi,[_dc_source]

  mov  ebx,[_dc_iscale]
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
  add  ebp,ebx  ; advance frac pointer
  shr  edx,25   ; finish calculation for second pixel
  mov  eax,[_dc_colormap]
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
fracstep:   dd 0
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
  mov  [endplane],ebx
  mov  [curplane],ebx
  shr  eax,2
  mov  ebp,[_ds_y]
  lea  edi,[ebp+ebp*4]
  shl  edi,4
  add  edi,eax
  add  edi,[_destview]
  mov  [dest],edi

  mov  ebx,[_ds_xfrac]
  shl  ebx,10
  and  ebx,0xFFFF0000
  mov  eax,[_ds_yfrac]
  shr  eax,6
  and  eax,0x0000FFFF
  or   ebx,eax

  mov  [frac],ebx

  mov  ebx,[_ds_xstep]
  shl  ebx,10
  and  ebx,0xFFFF0000
  mov  eax,[_ds_ystep]
  shr  eax,6
  and  eax,0x0000FFFF
  or   ebx,eax

  mov  [fracstep],ebx

  shl   ebx,2
  mov   [fracpstep],ebx
  mov   eax,.hpatch1+2
  mov   [eax],ebx
  mov   eax,.hpatch2+2
  mov   [eax],ebx
  mov   ecx,[curplane]
.hplane:
  mov   eax,1
  shl   eax,cl
  mov   edx,SC_INDEX+1
  out   dx,al
  mov   eax,[_ds_x2]
  cmp   eax,[curx]
  jb   .hdone
  sub   eax,[curplane]
  js   .hdoneplane
  shr   eax,2
  mov   [endpx],eax
  dec   eax
  js   .hfillone
  shr   eax,1
  mov   ebx,[curx]
  shr   ebx,2
  cmp   ebx,[endpx]
  jz   .hfillone
  mov   [curpx],ebx
  inc   ebx
  shr   ebx,1
  inc   eax
  sub   eax,ebx
  js   .hdoneplane
  mov   [loopcount],eax
  mov   eax,[_ds_colormap]
  mov   ebx,eax
  mov   esi,[_ds_source]
  mov   edi,[dest]
  mov   ebp,[frac]
  test  [curpx],dword 1
  jz    short .hfill
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  add   ebp,[fracpstep]
  and   ecx,0x00000FFF
  mov   al,[esi+ecx]
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
  mov   eax,[_ds_colormap]
  mov   esi,[_ds_source]
  mov   edi,[dest]
  mov   ebp,[frac]
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  and   ecx,0x00000FFF
  mov   al,[esi+ecx]
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
  cmp   ecx,[endplane]
  jz    short .hdone
  mov   [curplane],ecx
  mov   ebx,[frac]
  add   ebx,[fracstep]
  mov   [frac],ebx
  inc   dword [curx]
  jmp   .hplane
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

  mov  ebx,[_ds_xfrac]
  shl  ebx,10
  and  ebx,0xFFFF0000
  mov  eax,[_ds_yfrac]
  shr  eax,6
  and  eax,0x0000FFFF
  or   ebx,eax

  mov  [frac],ebx

  mov  ebx,[_ds_xstep]
  shl  ebx,10
  and  ebx,0xFFFF0000
  mov  eax,[_ds_ystep]
  shr  eax,6
  and  eax,0x0000FFFF
  or   ebx,eax

  mov [fracstep],ebx

  shl   ebx,1
  mov   [fracpstep],ebx
  mov   eax,.lpatch1+2
  mov   [eax],ebx
  mov   eax,.lpatch2+2
  mov   [eax],ebx
  mov   ecx,[curplane]
.lplane:
  mov   eax,3
  shl   eax,cl
  shl   eax,cl
  mov   edx,SC_INDEX+1
  out   dx,al
  mov   eax,[_ds_x2]
  cmp   eax,[curx]
  jb   .ldone
  sub   eax,[curplane]
  js   .ldoneplane
  shr   eax,1
  mov   [endpx],eax
  dec   eax
  js    .lfillone
  shr   eax,1
  mov   ebx,[curx]
  shr   ebx,1
  cmp   ebx,[endpx]
  jz    .lfillone
  mov   [curpx],ebx
  inc   ebx
  shr   ebx,1
  inc   eax
  sub   eax,ebx
  js    .ldoneplane
  mov   [loopcount],eax
  mov   eax,[_ds_colormap]
  mov   ebx,eax
  mov   esi,[_ds_source]
  mov   edi,[dest]
  mov   ebp,[frac]
  test  [curpx],dword 1
  jz    short .lfill
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  add   ebp,[fracpstep]
  and   ecx,0x00000FFF
  mov   al,[esi+ecx]
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
  mov   eax,[_ds_colormap]
  mov   esi,[_ds_source]
  mov   edi,[dest]
  mov   ebp,[frac]
  shld  ecx,ebp,22
  shld  ecx,ebp,6
  and   ecx,0x00000FFF
  mov   al,[esi+ecx]
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
  and   edx,0x00000FFF
  mov   al,[esi+ecx]
  mov   bl,[esi+edx]
  dec   dword [loopcount]
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
  cmp   ecx,[endplane]
  jz    short .ldone
  mov   [curplane],ecx
  mov   ebx,[frac]
  add   ebx,[fracstep]
  mov   [frac],ebx
  inc   dword [curx]
  jmp   .lplane
.ldone:
  popad
  ret
; R_DrawSpanLow ends
