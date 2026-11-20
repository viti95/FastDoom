;
; DESCRIPTION: Assembly texture mapping routines for VESA modes
;

BITS 32
%include "macros.inc"

%ifdef MODE_VBE2_DIRECT
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

;============================================================================

BEGIN_CODE_SECTION

;============================================================================
;
; R_DrawColumn
;
;============================================================================

CODE_SYM_DEF R_PatchCenteryVBE2High
  push  ebx
  mov   ebx,[_centery]
  mov   eax,patchCentery+1
  mov   [eax],ebx
  mov   eax,patchCenteryRoll+1
  mov   [eax],ebx
  mov   eax,patchCenteryMMX+1
  mov   [eax],ebx
  pop   ebx
  ret

CODE_SYM_DEF R_DrawColumnVBE2
	push		edi
  push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  MulScreenWidthStart edi, ebp
  sub  ebp,eax ; ebp = pixel count
  js   near doneh

  mov  ebx,[_dc_x]
  MulScreenWidthEnd edi
  mov  ecx,[_dc_iscale]
  add  edi,ebx
  add  edi,[_destview]

patchCentery:
  sub   eax,0x12345678
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  mov  ebx,edx
  shr  ebx,25 ; get address of first location
  jmp  [scalecalls+4+ebp*4]

doneh:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnVBE2 ends

;============ HIGH DETAIL ============

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    add  edx,ecx                         ; calculate next location
    mov  al,[esi+ebx]                    ; get source pixel
    mov  ebx,edx
    mov  al,[eax]                        ; translate the color    
    shr  ebx,25
    mov  [edi-(LINE-1)*SCREENWIDTH],al   ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  pop	 ebp
  mov  al,[esi+ebx]
  pop	 esi
  mov  al,[eax]
  pop	 edx
  mov  [edi],al

vscale0:
	pop		ecx
	pop		ebx
  pop		edi
  ret

CONTINUE_DATA_SECTION

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

CONTINUE_CODE_SECTION

;============================================================================
;
; R_DrawSpanVBE2
;
; Horizontal texture mapping
;
;============================================================================
CODE_SYM_DEF R_DrawSpanVBE2
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov  eax,[_ds_x1]
  mov  ebx,[_ds_x2]
  mov  eax,[mapcalls+eax*4]
  mov  ecx,[_ds_frac]        ; build composite position
  mov  [callpoint],eax ; spot to jump into unwound
  mov	 edx,[_ds_step]
  mov  eax,[mapcalls+4+ebx*4]
  mov	 esi,[_ds_source]
  mov  [returnpoint],eax ; spot to patch a ret at
  mov  [eax], byte OP_RET

  mov  ebp,[_ds_y]
  mov  eax,[_ds_colormap]
  MulScreenWidthStart edi, ebp
  shld  ebx,ecx,22      ; shift y units in
  MulScreenWidthEnd edi
  mov   ebp,0x0FFF  ; used to mask off slop high bits from position
  add  edi,[_destview]
  shld  ebx,ecx,6       ; shift x units in
  and   ebx,ebp         ; mask off slop bits
  add   ecx,edx

  ; feed the pipeline and jump in
  call  [callpoint]

  mov  ebx,[returnpoint]
  pop		ebp
  mov  [ebx],byte OP_MOVAL ; remove the ret patched in

	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanVBE2 ends

;============= HIGH DETAIL ============

%macro MAPLABEL 1
  hmap%1
%endmacro

%assign LINE 0
%assign PCOL 0
%rep SCREENWIDTH
  %assign PLANE 0
    MAPLABEL LINE:
      %assign LINE LINE+1
      %if LINE = SCREENWIDTH
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
        %if LINE < SCREENWIDTH-1
        add   ecx,edx                ; position += step
        %endif
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

MAPLABEL LINE:
  ret

CODE_SYM_DEF R_DrawColumnVBE2Roll
  push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  MulScreenWidthStart edi, eax
  sub  ebp,eax         ; ebp = pixel count
  js   donehr

  mov  ebx,[_dc_x]
  MulScreenWidthEnd edi
  mov  ecx,[_dc_iscale]
  add  edi,ebx
  add  edi,[_destview]

patchCenteryRoll:
  sub   eax,0x12345678
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  mov  ebx,edx
  shr  ebx,25 ; get address of first location

LoopRoll:
  add  edx,ecx                        ; calculate next location
  mov  al,[esi+ebx]                   ; get source pixel
  mov  ebx,edx
  mov  al,[eax]                       ; translate the color
  shr  ebx,25
  mov  [edi],al  ; draw a pixel to the buffer
  add  edi, SCREENWIDTH
  dec  ebp
  jns  LoopRoll

donehr:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

CODE_SYM_DEF R_DrawSpanVBE2Roll
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov  ecx,[_ds_frac]        ; build composite position
  mov	 esi,[_ds_source]

  mov  ebp,[_ds_y]

  MulScreenWidthStart edi, ebp

  mov  eax,[_ds_x1]
  mov  ebp,[_ds_x2]
  sub  ebp,eax           ; num pixels

  MulScreenWidthEnd edi
  shld    ebx,ecx,22
  add  edi,[_destview]
  shld    ebx,ecx,6
  add  edi,eax
  and  ebx,0xFFF

  mov	  edx,[_ds_step]
  mov   eax,[_ds_colormap]
  add   ecx,edx                ; position += step

LoopSpanRoll:
  mov   al,[esi+ebx]           ; get source pixel
  shld  ebx,ecx,22             ; shift y units in
  mov   al,[eax]               ; translate color
  shld  ebx,ecx,6              ; shift x units in
  mov   [edi],al  ; write pixel
  and   ebx,0xFFF              ; mask off slop bits
  add   ecx,edx                ; position += step
  inc   edi
  dec   ebp
  jns   LoopSpanRoll

	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

CODE_SYM_DEF R_DrawColumnVBE2MMX
  push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  MulScreenWidthStart edi, eax
  sub  ebp,eax         ; ebp = pixel count
  js   donehm

  mov  ebx,[_dc_x]
  MulScreenWidthEnd edi
  mov  ecx,[_dc_iscale]
  add  edi,ebx
  add  edi,[_destview]

patchCenteryMMX:
  sub   eax,0x12345678
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  cmp   ebp, 0
  je    SinglePixel

  test  ebp,1
  jnz   Even

  mov  ebx,edx
  shr  ebx,25 ; get address of first location
  mov  al,[esi+ebx]                   ; get source pixel
  add  edx,ecx
  mov  al,[eax]                       ; translate the color
  dec  ebp
  mov  [edi],al  ; draw a pixel to the buffer

  add  edi, SCREENWIDTH
  
  ; MMX 2 pixels render
Even:

  movd       mm0, edx
  
  movd       mm1, ecx

  mov        ecx, eax

  paddd      mm0, mm1

  movd       mm2, edx

  psllq      mm0, 32

  punpckldq  mm1, mm1  

  por        mm0, mm2

  paddd      mm1, mm1

  movq       mm3, mm0

  psrld      mm3, 25

LoopMMX:

  movd       ebx, mm3

  paddd      mm0, mm1

  psrlq      mm3,32

  mov  al,[ebx+esi]

  movd       edx, mm3

  movq       mm3, mm0

  mov  al,[eax]

  mov  cl,[edx+esi]

  mov  [edi], al

  mov  cl,[ecx]

  psrld      mm3, 25

  mov  [edi+SCREENWIDTH], cl

  add  edi, 2*SCREENWIDTH

  sub  ebp, 2
  jns  LoopMMX

donehm:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret

SinglePixel:

  shr  edx,25 ; get address of first location
  pop		ebp
  mov  al,[esi+edx]                   ; get source pixel
  pop		esi
  pop		edx
  mov  al,[eax]                       ; translate the color
  pop		ecx
	pop		ebx
  mov  [edi],al  ; draw a pixel to the buffer
  pop		edi
  ret
  
CODE_SYM_DEF R_DrawSpanVBE2MMX
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov  ecx,[_ds_frac]        ; build composite position
  mov	 esi,[_ds_source]

  mov  ebp,[_ds_y]

  MulScreenWidthStart edi, ebp

  mov  eax,[_ds_x1]
  mov  ebp,[_ds_x2]
  sub  ebp,eax           ; num pixels

  mov   edx,ecx
  MulScreenWidthEnd edi
  mov   ebx,ecx
  add  edi,[_destview]
  shr   edx,4
  shr   ebx,26
  and   edx,0xFC0
  add  edi,eax
  or    ebx,edx

  mov	  edx,[_ds_step]
  mov   eax,[_ds_colormap]

  cmp   ebp, 0
  je    SinglePixelSpan

  test  ebp,1
  jnz   EvenSpan

  mov  al,[esi+ebx]                   ; get source pixel
  dec   ebp
  mov  al,[eax]
  mov  [edi],al

  inc   edi

  ; MMX 2 pixels render
EvenSpan:

  mov   ebx, eax

  movd  mm0, ecx
  movd  mm1, edx
  movq  mm2, mm0
  paddd mm0, mm1
  mov   edx, 0x00000FC0
  psllq mm0, 32
  movd  mm4, edx
  por   mm0, mm2
  punpckldq mm1, mm1
  movq  mm3, mm0
  movq  mm2, mm0
  psrld mm3, 4
  psrld mm2, 26
  punpckldq mm4, mm4
  pand  mm3, mm4
  por   mm2, mm3
  paddd mm1,mm1

LoopSpanMMX:

  movd  ecx, mm2
  psrlq mm2,32
  mov  al,[esi+ecx]                   ; get source pixel
  movd  ecx, mm2
  paddd mm0, mm1
  mov  bl,[esi+ecx]                   ; get source pixel
  movq  mm3, mm0
  movq  mm2, mm0
  mov  dl,[eax]
  psrld mm3, 4
  psrld mm2, 26
  mov  dh,[ebx]
  pand  mm3, mm4
  mov  [edi],dx
  por   mm2, mm3
  add  edi, 2

  sub  ebp, 2
  jns  LoopSpanMMX

  pop   ebp
	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret

SinglePixelSpan:

  mov  al,[esi+ebx]                   ; get source pixel
  pop   ebp
  mov  al,[eax]
  mov  [edi],al	
  pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret

; ======================
; R_DrawSpanVBE2SSE2
;
; Four pixels per iteration of the loop, with the texture offsets of the
; four of them calculated at once with the SSE2 instructions and with the
; four colors of them joined and written to the video memory with a 32
; bit write.
;
; The pixels which are not a full group of four of them are rendered one
; by one with a scalar loop at the end.
; ======================
CODE_SYM_DEF R_DrawSpanVBE2SSE2
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    edi
  push    ebp

  mov     ecx,[_ds_frac]        ; build composite position
  mov     esi,[_ds_source]

  mov     ebp,[_ds_y]

  MulScreenWidthStart edi, ebp

  mov     eax,[_ds_x1]
  mov     ebp,[_ds_x2]
  sub     ebp,eax               ; EBP has the number of pixels less one

  mov     edx,ecx
  MulScreenWidthEnd edi
  mov     ebx,ecx
  add     edi,[_destview]
  shr     edx,4
  shr     ebx,26
  and     edx,0xFC0
  add     edi,eax
  or      ebx,edx               ; texture offset of the first pixel

  mov     edx,[_ds_step]        ; build composite step

  inc     ebp                   ; EBP has the number of pixels
  mov     ebx,ebp
  and     ebx,3                 ; pixels which are not a full group of four
  push    ebx                   ; them kept in the stack for the scalar loop
  shr     ebp,2                 ; groups of four pixels
  dec     ebp                   ; iterations of the loop less one

  ; xmm0 has the four positions of the loop, one in each dword of them,
  ; and xmm1 has the step of the position multiplied by four in all of
  ; them, with xmm4 having the mask of the vertical component

  movd    xmm0,ecx              ; position of the first pixel
  lea     eax,[ecx+edx]         ; position of the second pixel
  movd    xmm1,eax
  lea     eax,[eax+edx]         ; position of the third pixel
  movd    xmm2,eax
  lea     eax,[eax+edx]         ; position of the fourth pixel
  movd    xmm3,eax
  shufps  xmm0,xmm1,0           ; the first two positions of them
  shufps  xmm2,xmm3,0           ; the last two positions of them
  shufps  xmm0,xmm2,0x88        ; the four positions of them
  lea     eax,[edx*4]           ; step of the position multiplied by four
  movd    xmm1,eax
  shufps  xmm1,xmm1,0           ; it broadcasted to the four dwords
  mov     edx,0xFC0
  movd    xmm4,edx
  shufps  xmm4,xmm4,0           ; mask of the vertical component

  mov     eax,[_ds_colormap]    ; base of the color table
  mov     ebx,eax               ; the base of the color table for the pair
                                ;  of source pixels of each two pixels

  ; the instructions of them above do not modify the flags, so the sign
  ; of the iterations of the loop less one calculated with them is still
  ; in them

  js    TailSpanVBE2SSE2        ; there are not groups of four pixels

LoopSpanVBE2SSE2:
  movdqa  xmm2,xmm0
  psrld   xmm2,26               ; horizontal components of the positions
  movdqa  xmm3,xmm0
  psrld   xmm3,4
  pand    xmm3,xmm4             ; vertical components of the positions
  por     xmm2,xmm3             ; texture offsets of the four pixels
  paddd   xmm0,xmm1             ; positions of the next four pixels

  movd    ecx,xmm2              ; offset of the first pixel
  psrldq  xmm2,4
  movd    edx,xmm2              ; offset of the second pixel

  ; the source pixels are gotten with the low byte of the base of the
  ; color table in the eax and ebx registers of them, with the two
  ; colors of them kept in xmm5 while the offsets are gotten

  mov     al,[esi+ecx]          ; get first source pixel
  mov     bl,[esi+edx]          ; get second source pixel
  mov     cl,[eax]              ; translate color of the first pixel
  mov     ch,[ebx]              ; translate color of the second pixel
  movd    xmm5,ecx              ; the first two colors of them

  psrldq  xmm2,4
  movd    ecx,xmm2              ; offset of the third pixel
  psrldq  xmm2,4
  movd    edx,xmm2              ; offset of the fourth pixel

  mov     al,[esi+ecx]          ; get third source pixel
  mov     bl,[esi+edx]          ; get fourth source pixel
  mov     dl,[eax]              ; translate color of the third pixel
  mov     dh,[ebx]              ; translate color of the fourth pixel

  ; the four colors of them joined in a dword, two of them in the low
  ; half of it and the other two in the high half of it

  movd    ecx,xmm5              ; the first two colors of them
  movzx   ecx,cx
  shl     edx,16                ; the last two colors of them
  or      edx,ecx
  mov     [edi],edx             ; write four pixels of them
  add     edi,4

  dec     ebp
  jns   LoopSpanVBE2SSE2

  ; the pixels which are not a multiple of four of them, one by one,
  ; with the positions of them in the dwords of xmm0

TailSpanVBE2SSE2:
  pop   ecx                     ; pixels which are not a full group of four

  test  ecx,ecx
  je    EndSpanVBE2SSE2

  ; the base of the color table is in eax, the source is in esi and the
  ; destination of the pixels is in edi

LoopTailSpanVBE2SSE2:
  movd  edx,xmm0                ; position of the pixel
  psrldq xmm0,4                 ; positions of the following pixels
  mov   ebx,edx
  shr   edx,26                  ; horizontal component of the position
  shr   ebx,4
  and   ebx,0xFC0               ; vertical component of the position
  or    edx,ebx                 ; texture offset of the pixel

  mov   al,[esi+edx]            ; get source pixel
  mov   al,[eax]                ; translate color
  mov   [edi],al                ; write the pixel
  inc   edi

  dec ecx
  jnz LoopTailSpanVBE2SSE2

EndSpanVBE2SSE2:
  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret

%endif
