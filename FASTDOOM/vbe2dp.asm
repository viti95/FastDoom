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
; R_DrawColumnPotatoVBE2
;
;============================================================================

CODE_SYM_DEF R_DrawColumnPotatoVBE2
  pushad

  mov  ebp,[_dc_yh]
  mov  ebx,[_dc_x]
  mov  eax,[_dc_yl]
  shl  ebx,2
  lea  edi,[ebp+ebp*4]
  sub  ebp,eax ; ebp = pixel count
  js   short .done

  shl  edi,6
  add  edi,ebx
  add  edi,[_destview]

  mov  ecx,[_dc_iscale]

  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  xor   ebx,ebx
  shld  ebx,edx,7 ; get address of first location
  call  [scalecalls+4+ebp*4]

.done:
  popad
  ret
; R_DrawColumnPotatoVBE2 ends

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
    mov  [edi-(LINE-1)*SCREENWIDTH],al  ; draw a pixel to the buffer
    mov  [edi-(LINE-1)*SCREENWIDTH + 1],al  ; draw a pixel to the buffer
    mov  [edi-(LINE-1)*SCREENWIDTH + 2],al  ; draw a pixel to the buffer
    mov  [edi-(LINE-1)*SCREENWIDTH + 3],al  ; draw a pixel to the buffer
    shr  ebx,25
    %assign LINE LINE-1
%endrep

vscale1:
  mov al,[esi+ebx]
  mov al,[eax]
  mov [edi],al
  mov [edi+1],al
  mov [edi+2],al
  mov [edi+3],al

vscale0:
  ret

CONTINUE_DATA_SECTION

%macro MAPDEFINE 1
  dd hmap%1
%endmacro

align 4

mapcalls:
  %assign LINE 0
  %rep SCREENWIDTH/4+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0
returnpoint: dd 0

CONTINUE_CODE_SECTION

;============================================================================
;
; R_DrawSpanPotatoVBE2
;
; Horizontal texture mapping
;
;============================================================================
CODE_SYM_DEF R_DrawSpanPotatoVBE2
  pushad

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
  lea  edi,[ebp+ebp*4]
  shld  ebx,ecx,22      ; shift y units in
  shl  edi,6
  mov   ebp,0x0FFF  ; used to mask off slop high bits from position
  add  edi,[_destview]
  shld  ebx,ecx,6       ; shift x units in
  and   ebx,ebp         ; mask off slop bits
  add   ecx,edx
  
  ; feed the pipeline and jump in
  call  [callpoint]

  mov  ebx,[returnpoint]
  mov  [ebx],byte OP_MOVAL ; remove the ret patched in

  popad
  ret
; R_DrawSpanPotatoVBE2 ends

;============= HIGH DETAIL ============

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
        mov   [edi+PLANE+PCOL*4],al  ; write pixel
        mov   [edi+PLANE+PCOL*4+1],al  ; write pixel
        mov   [edi+PLANE+PCOL*4+2],al  ; write pixel
        mov   [edi+PLANE+PCOL*4+3],al  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        shld  ebx,ecx,22             ; shift y units in
        mov   al,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   [edi+PLANE+PCOL*4],al  ; write pixel
        mov   [edi+PLANE+PCOL*4+1],al  ; write pixel
        mov   [edi+PLANE+PCOL*4+2],al  ; write pixel
        mov   [edi+PLANE+PCOL*4+3],al  ; write pixel
        and   ebx,ebp                ; mask off slop bits
        add   ecx,edx                ; position += step
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

hmap80: ret

%endif
