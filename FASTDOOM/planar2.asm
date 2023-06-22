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
lutx2:      db 1,3,7,15
lutx1:      db 15,14,12,8

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
  mov  ecx,ebx
  lea  edi,[ebp+ebp*4]
  mov  ebx,[_ds_frac]
  add  edi,edi
  mov  [frac],ebx
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
  mov  ecx,ebx
  mov  ebp,[_ds_y]
  shr  eax,1
  lea  edi,[ebp+ebp*4]
  mov  ebx,[_ds_frac]
  add  edi,edi
  mov  [frac],ebx
  lea  edi,[eax+edi*8]
  mov  ebx,[_ds_step]
  add  edi,[_destview]
  add   ebx, ebx
  mov  [dest],edi

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

CODE_SYM_DEF R_DrawSpanFlat
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	push	ebp
	sub		esp,8
  mov		eax,[_ds_source]
  mov   ecx,[_ds_colormap]
  mov   cl,[eax+0x73A]        ;FLATPIXELCOLOR
  mov   al,byte [ecx]
  mov		ecx,dword [_ds_x1]
	mov		byte 4[esp],al
	mov		eax,dword [_ds_y]
	mov		ebp,dword [_ds_x2]
	lea		edi,[eax+eax*4]
	shl		edi,4
	add		edi,dword [_destview]
	mov		eax,ecx
  shr   ecx,2
	and		eax,3
  mov   ebx,ebp
	sar   ebx,2
	and		ebp,3
	lea		esi,[edi+ecx]
	cmp		ecx,ebx
	je		L$61
	cmp		eax,0
	jne		L$62
L$58:
	cmp		ebp,3
	je		L$59
	mov		edx,3c5H
	mov		al,byte lutx2[ebp]
	out		dx,al
	lea		eax,[edi+ebx]
	mov		dl,byte 4[esp]
	dec		ebx
	mov		byte [eax],dl
L$59:
	sub		ebx,ecx
	inc		ebx
	ja		L$63
L$61:
	mov		al,byte lutx1[eax]
	mov		edx,3c5H
	and		al,byte lutx2[ebp]
	out		dx,al
	mov		al,byte 4[esp]
	mov		byte [esi],al
	add		esp,8
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
L$62:
	mov		al,byte lutx1[eax]
  mov		edx,3c5H
	out		dx,al
	mov		al,byte 4[esp]
	inc		ecx
	mov		byte [esi],al
	jmp		L$58
L$63:
	mov		al,0fH
	mov		edx,3c5H
	out		dx,al
	add		edi,ecx
	test	bl,1
	je		L$64
	mov		al,byte 4[esp]
	dec		ebx
	mov		byte [edi],al
  inc		edi
L$64:
	test	ebx,ebx
	jbe		L$60
	mov   al,byte 4[esp]
  mov		ecx,ebx
  mov   ah,al
	shr		ecx,1
	rep stosw
L$60:
	add		esp,8
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

CODE_SYM_DEF R_DrawSpanFlatLow
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi
	push	ebp
	sub		esp,8
  mov		eax,[_ds_source]
  mov   ebx,[_ds_colormap]
  mov   bl,[eax+0x73A]        ;FLATPIXELCOLOR
	mov		ecx,dword [_ds_x1]
  mov   al,byte [ebx]
  mov   ebx,ecx
	mov		byte 4[esp],al
	mov		eax,dword [_ds_y]
  sar		ecx,1
  lea		edi,[eax+eax*4]
	shl		edi,4
	add		edi,[_destview]
  and   ebx,1
  mov		dword [esp],ebx
	mov		ebx,dword [_ds_x2]
  mov   ebp,ebx
  and   ebp,1
	sar		ebx,1
	lea		esi,[edi+ecx]
	cmp		ecx,ebx
	je		L$68
	cmp		dword [esp],0
	jne		L$69
L$65:
	cmp		ebp,1
	je		L$66
	mov		al,3
	mov		edx,3c5H
	out		dx,al
	lea		eax,[edi+ebx]
	mov		dl,byte 4[esp]
	dec		ebx
	mov		byte [eax],dl
L$66:
	sub		ebx,ecx
	inc		ebx
	jg		L$70
L$68:
	mov		eax,dword [esp]
	lea		ecx,[ebp+ebp*8+3]
	lea		eax,[eax+eax*8+3]
	mov		edx,3c5H
	or		eax,ecx
	out		dx,al
	mov		al,byte 4[esp]
	mov		byte [esi],al
	add		esp,8
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret
L$69:
	mov		al,0cH
	mov		edx,3c5H
	out		dx,al
	mov		al,byte 4[esp]
	inc		ecx
	mov		byte [esi],al
	jmp		L$65
L$70:
	mov		al,0fH
	mov		edx,3c5H
	out		dx,al
	add		edi,ecx
	test	bl,1
	je		L$71
	mov		al,byte 4[esp]
	dec		ebx
	mov		byte [edi],al
  inc		edi
L$71:
	test	ebx,ebx
	jle		L$72
  mov   al, byte 4[esp]
  sar   ebx,1
  mov   ah,al
  mov   ecx,ebx
	rep stosw
L$72:
	add		esp,8
	pop		ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	ret

CODE_SYM_DEF R_DrawSpanFlatPotato
	push		ebx
	push		ecx
	push		edi
	mov		eax,[_ds_source]
  mov   ebx,[_ds_colormap]
  mov   bl,[eax+0x73A]        ;FLATPIXELCOLOR
  mov		ecx,[_ds_x2]
  mov		edi,[_ds_x1]
  sub   ecx,edi
	mov		al,byte [ebx]
  mov		ebx,[_ds_y]
  add		edi,[_destview]
  and   eax,0xFF
  add   edi,[_ylookup+ebx*4]
	inc		ecx
	test  ecx,1
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
