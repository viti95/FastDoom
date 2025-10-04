;
; DESCRIPTION: Assembly texture mapping routines for VESA modes
;

BITS 32
%include "macros.inc"

%ifdef MODE_VBE2_DIRECT
%include "defs.inc"

extern _destview

BEGIN_DATA_SECTION

%macro MAPDEFINE 1
  dd hmap%1
%endmacro

align 4

mapcalls:
  %assign LINE 0
  %rep SCREENWIDTH/2+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0

BEGIN_CODE_SECTION

;============================================================================
;
; R_DrawSpanLowVBE2
;
; Horizontal texture mapping
;
;============================================================================
CODE_SYM_DEF R_DrawSpanLowVBE2_386SX
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
  mov	 ebp,[_ds_step]
  mov  eax,[mapcalls+4+ebx*4]
  mov	 esi,[_ds_source]
  mov  cr2,eax ; spot to patch a ret at
  mov  [eax], byte OP_RET

  mov  edx,[_ds_y]
  mov  eax,[_ds_colormap]
  MulScreenWidthStart edi, edx
  shld  ebx,ecx,22      ; shift y units in
  MulScreenWidthEnd edi
  add  edi,[_destview]
  shld  ebx,ecx,6       ; shift x units in
  and   ebx,0x0FFF      ; mask off slop bits
  add   ecx,ebp
  
  ; feed the pipeline and jump in
  call  [callpoint]

  mov  ebx,cr2
	pop		ebp
  mov  [ebx],byte OP_MOVAL ; remove the ret patched in

	pop		edi
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  ret
; R_DrawSpanLowVBE2 ends

;============= HIGH DETAIL ============

%macro MAPLABEL 1
  hmap%1
%endmacro

%assign LINE 0
%assign PCOL 0
%rep SCREENWIDTH/2
  %assign PLANE 0
    MAPLABEL LINE:
      %assign LINE LINE+1
      %if LINE = SCREENWIDTH/2
        mov   al,[esi+ebx]           ; get source pixel
        mov   al,[eax]               ; translate color
        mov   ah,al
        mov   [edi+PLANE+PCOL*2],ax  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        shld  ebx,ecx,22             ; shift y units in
        mov   dl,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   dh,dl
        mov   [edi+PLANE+PCOL*2],dx  ; write pixel
        and   ebx,0x0FFF             ; mask off slop bits
        add   ecx,ebp                ; position += step
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

MAPLABEL LINE:
  ret

%endif
