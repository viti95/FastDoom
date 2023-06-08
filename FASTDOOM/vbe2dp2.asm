;
; DESCRIPTION: Assembly texture mapping routines for VESA modes
;

BITS 32
%include "macros.inc"

%ifdef MODE_VBE2_DIRECT
%include "defs.inc"

extern _destview
extern _viewheight
extern _fuzzoffsetinverse
extern _fuzzposinverse
extern _colormaps

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

%macro TESTFUZZPOSDEFINE 1
  dd testfuzzpos%1
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

CODE_SYM_DEF R_DrawFuzzColumnPotatoVBE2
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  eax,[_viewheight]
  dec  eax

  mov  ebp,[_dc_yh]

  cmp  eax,ebp
  jne  dc_yhOK

  dec  eax
  mov  ebp,eax

dc_yhOK:
  mov  eax,[_dc_yl]

  test eax,eax
  jne dc_ylOK

  mov  eax,1

dc_ylOK:
  lea  edi,[ebp+ebp*4]
  sub  ebp,eax ; ebp = pixel count
  js   near .done

  mov  eax,[_dc_x]
  shl  edi,6
  shl  eax,2
  add  edi,[_destview]
  add  edi,eax
  
  xor ebx,ebx

  mov eax,[_colormaps]
  mov	ecx,[_fuzzposinverse]
  add eax,0x600
  mov edx,_fuzzoffsetinverse
  mov esi,50

  jmp  [scalecalls+4+ebp*4]

.done:
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

%macro TESTFUZZPOSDEFINE 1
  testfuzzpos%1
%endmacro

%macro JMPTESTFUZZPOSDEFINE 1
  jne testfuzzpos%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT
  SCALELABEL LINE:

  mov		ebp,[edx+ecx*4]
	mov   al,[ebp+edi-(LINE-1)*320]
  dec   ecx
	mov		bl,[eax]
  mov   bh,bl
  mov		[edi-(LINE-1)*320],bx
  mov		[edi-(LINE-1)*320+2],bx

  JMPTESTFUZZPOSDEFINE LINE
  mov   ecx,esi

  TESTFUZZPOSDEFINE LINE:
  %assign LINE LINE-1
%endrep

vscale0:
  pop	ebp
  mov [_fuzzposinverse],ecx
  pop	esi
  pop	edx
	pop	ecx
	pop	ebx
  pop	edi
  ret

%endif
