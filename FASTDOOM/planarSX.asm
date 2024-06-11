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
fracpstep:  dd 0
curx:       dd 0
curpx:      dd 0
endpx:      dd 0

BEGIN_CODE_SECTION

; ===========================================================================
; R_DrawSpan
; horizontal texture mapping, full detail
; ===========================================================================
CODE_SYM_DEF R_DrawSpan386SX
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov   [.hpatchESP1+2],esp
  mov   [.hpatchESP2+2],esp

  mov  eax,[_ds_x1]
  mov  [curx],eax
  mov  ebx,eax
  and  ebx,3
  shr  eax,2
  mov  [endplane],ebx
  mov  ebp,[_ds_y]
  mov  [curplane],ebx
  mov  ecx,ebx
  lea  edi,[ebp+ebp*4]
  mov  ebx,[_ds_frac]
  add  edi,edi
  mov  cr2,ebx
  lea  edi,[eax+edi*8]
  mov  ebx,[_ds_step]
  add  edi,[_destview]
  lea  ebx,[ebx*4]
  mov  [dest],edi
  mov   [fracpstep],ebx
  mov   eax,.hpatch1+2
  mov   [eax],ebx
  mov   eax,.hpatch2+2
  mov   [eax],ebx
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
  
  sub   esp,eax

  mov   esi,[_ds_source]
  mov   eax,[_ds_colormap]
  mov   edi,[dest]
  mov   ebx,eax
  mov   ebp,cr2
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

.hpatchESP1:
  cmp   esp, 0x12345678

  jnae   short .hdoubleloop
  jmp   short .hchecklast
.hfillone:  
  mov   ebp,cr2
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
  mov   dh,[ebx]
  shld  ecx,ebp,22
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
  inc   esp
  mov   dl,[eax]  
.hpatchESP2:
  cmp   esp, 0x12345678
  jl   short .hdoubleloop
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
  mov   ebx,cr2
  mov   [curplane],ecx
  add   ebx,[_ds_step]
  inc   dword [curx]
  mov   cr2,ebx  
  jmp   near .hplane
.hdone:
  mov  esp,[.hpatchESP1+2]
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
CODE_SYM_DEF R_DrawSpanLow386SX
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov   [.lpatchESP1+2],esp
  mov   [.lpatchESP2+2],esp

  mov  eax,[_ds_x1]
  mov  [curx],eax
  mov  ebx,eax
  and  ebx,1
  mov  [endplane],ebx
  mov  [curplane],ebx
  mov  ecx,ebx
  mov  ebp,[_ds_y]
  shr  eax,1
  lea  edi,[ebp+ebp*4]
  mov  ebx,[_ds_frac]
  add  edi,edi
  mov  cr2,ebx
  lea  edi,[eax+edi*8]
  mov  ebx,[_ds_step]
  add  edi,[_destview]
  mov  [dest],edi

  add   ebx, ebx
  mov   [fracpstep],ebx
  mov   eax,.lpatch1+2
  mov   [eax],ebx
  mov   eax,.lpatch2+2
  mov   [eax],ebx
.lplane:
  mov   dx,SC_INDEX+1
  lea   eax,[ecx+ecx*8+3]
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
  
  sub   esp,eax

  mov   esi,[_ds_source]
  mov   eax,[_ds_colormap]
  mov   edi,[dest]
  mov   ebx,eax
  mov   ebp,cr2
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

.lpatchESP1:
  cmp   esp, 0x12345678

  jnae  short .ldoubleloop
  jmp   short .lchecklast
.lfillone:
  mov   ebp,cr2
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
  mov   dh,[ebx]
  shld  ecx,ebp,22
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
  inc   esp
  mov   dl,[eax]  
.lpatchESP2:
  cmp   esp, 0x12345678
  jl    short .ldoubleloop
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
  mov   ebx,cr2
  mov   [curplane],ecx
  add   ebx,[_ds_step]
  inc   dword [curx]
  mov   cr2,ebx
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

CONTINUE_CODE_SECTION

; ================
; R_DrawSpanPotato
; ================
CODE_SYM_DEF R_DrawSpanPotato386SX
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
  mov     cr2,eax     ; spot to patch a ret at
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

  mov     ebx,cr2
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

MAPLABEL LINE:
  ret

%endif
