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
  %rep SCREENWIDTH/4+1
    MAPDEFINE LINE
    %assign LINE LINE+1
  %endrep

callpoint:   dd 0
returnpoint: dd 0

BEGIN_CODE_SECTION

;============================================================================
;
; R_DrawSpanPotatoVBE2
;
; Horizontal texture mapping
;
;============================================================================
CODE_SYM_DEF R_DrawSpanPotatoVBE2Pentium
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
  mov  eax,[mapcalls+4+ebx*4]
  mov	 esi,[_ds_source]
  mov  [returnpoint],eax ; spot to patch a ret at
  mov  [eax], byte OP_RET

  mov  ebp,[_ds_y]
  mov  eax,[_ds_colormap]
  MulScreenWidthStart edi,ebp
  mov   ebx,ecx
  MulScreenWidthEnd edi
  mov   edx,ecx
  add  edi,[_destview]

  mov	 ebp,[_ds_step]

  shr   ebx,4
  shr   edx,26
  and   ebx,0xFC0
  add   ecx,ebp
  or    ebx,edx

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
      %if LINE = SCREENWIDTH/4
        mov   al,[esi+ebx]           ; get source pixel
        mov   al,[eax]               ; translate color
        mov   ah,al
        mov   [edi+PLANE+PCOL*4],ax  ; write pixel
        mov   [edi+PLANE+PCOL*4+2],ax  ; write pixel
      %else
        mov   al,[esi+ebx]           ; get source pixel
        mov   bh,ch
        mov   dl,[eax]               ; translate color
        shr   ebx,4
        mov   dh,dl
        and   bx,0xFC0
        mov   [edi+PLANE+PCOL*4],dx  ; write pixel        
        mov   [edi+PLANE+PCOL*4+2],dx  ; write pixel
        mov   edx,ecx
        shr   edx,26
        or    ebx,edx
        %if LINE < (SCREENWIDTH/4)-1
        add   ecx,ebp
        %endif
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

MAPLABEL LINE:
  ret

%endif
