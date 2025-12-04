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


CODE_SYM_DEF R_DrawSpanVBE2MMX
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		edi
	push		ebp

  mov  ecx,[_ds_frac]        ; build composite position
  mov	 edx,[_ds_step]
  mov	 esi,[_ds_source]

  mov  ebp,[_ds_y]

  MulScreenWidthStart edi, ebp

  mov  eax,[_ds_x1]
  mov  ebp,[_ds_x2]
  sub  ebp,eax           ; num pixels

  shld  ebx,ecx,22      ; shift y units in
  MulScreenWidthEnd edi
  add  edi,[_destview]
  add  edi,eax
  shld  ebx,ecx,6       ; shift x units in
  and   ebx,0x0FFF         ; mask off slop bits
  add   ecx,edx

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
  movd  mm5, ecx
  paddd mm0, mm1
  mov   edx, 0x00000FC0
  psllq mm0, 32
  movd  mm6, edx
  por   mm0, mm5
  punpckldq mm1, mm1
  movq  mm3, mm0
  movq  mm2, mm0
  punpckldq mm4, mm4
  psrld mm3, 4
  psrld mm2, 26
  punpckldq mm6, mm6
  pand  mm3, mm6
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
  pand  mm3, mm6
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
	pop		edi
  mov  [edi],al	
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret

%endif
