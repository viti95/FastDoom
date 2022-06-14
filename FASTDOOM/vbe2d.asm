;
; DESCRIPTION: Assembly texture mapping routines for VESA modes
;

BITS 32
%include "defs.inc"
%include "macros.inc"

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

CODE_SYM_DEF R_DrawColumnVBE2
  pushad

  mov  ebp,[_dc_yh]
  lea  edi,[ebp+ebp*4]
  sal  edi,6
  mov  ebx,[_dc_x]
  add  edi,ebx
  add  edi,[_destview]

  mov  eax,[_dc_yl]
  sub  ebp,eax ; ebp = pixel count
  or   ebp,ebp
  js   short .done

  mov  ecx,[_dc_iscale]

  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9 ; 7 significant bits, 25 frac

  shl  ecx,9 ; 7 significant bits, 25 frac
  mov  esi,[_dc_source]

  mov  eax,[_dc_colormap]

  xor   ebx,ebx
  shld  ebx,edx,7 ; get address of first location
  call  [scalecalls+4+ebp*4]

.done:
  popad
  ret
; R_DrawColumnVBE2 ends

;============ HIGH DETAIL ============

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    mov  al,[esi+ebx]                    ; get source pixel
    add  edx,ecx                         ; calculate next location
    mov  al,[eax]                        ; translate the color
    mov  ebx,edx
    shr  ebx,25
    mov  [edi-(LINE-1)*SCREENWIDTH],al   ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  mov  al,[esi+ebx]
  mov  al,[eax]
  mov  [edi],al

vscale0:
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
  pushad

  mov  eax,[_ds_x1]
  mov  ebx,[_ds_x2]
  mov  eax,[mapcalls+eax*4]
  mov  [callpoint],eax ; spot to jump into unwound
  mov  eax,[mapcalls+4+ebx*4]
  mov  [returnpoint],eax ; spot to patch a ret at
  mov  [eax], byte OP_RET

  ; build composite position

  mov  ecx,[_ds_frac]

  ; build composite step

  mov	edx,[_ds_step]

  mov	esi,[_ds_source]

  mov  ebp,[_ds_y]
  lea  edi,[ebp+ebp*4]
  sal  edi,6
  add  edi,[_destview]

  mov  eax,[_ds_colormap]

  ; feed the pipeline and jump in

  mov   ebp,0x00000FFF  ; used to mask off slop high bits from position
  shld  ebx,ecx,22      ; shift y units in
  shld  ebx,ecx,6       ; shift x units in
  and   ebx,ebp         ; mask off slop bits
  call  [callpoint]

  mov  ebx,[returnpoint]
  mov  [ebx],byte OP_MOVAL ; remove the ret patched in

  popad
  ret
; R_DrawSpanVBE2 ends

;============= HIGH DETAIL ============

%macro MAPLABEL 1
  hmap%1
%endmacro

%assign LINE 0
%assign PCOL 0
%rep SCREENWIDTH/4
  %assign PLANE 0
  %rep 4
    MAPLABEL LINE:
      %assign LINE LINE+1
      mov   al,[esi+ebx]           ; get source pixel
      shld  ebx,ecx,22             ; shift y units in
      shld  ebx,ecx,6              ; shift x units in
      mov   al,[eax]               ; translate color
      and   ebx,ebp                ; mask off slop bits
      add   ecx,edx                ; position += step
      mov   [edi+PLANE+PCOL*4],al  ; write pixel
      %assign PLANE PLANE+1
  %endrep
%assign PCOL PCOL+1
%endrep

hmap320:
  ret
