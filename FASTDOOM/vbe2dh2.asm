;
; DESCRIPTION: Assembly texture mapping routines for VESA modes
;

BITS 32
%include "macros.inc"

%ifdef MODE_VBE2_DIRECT
%include "defs.inc"

extern _destview
extern _viewheightminusone
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

CODE_SYM_DEF R_DrawFuzzColumnVBE2
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_viewheightminusone]
  
  xor  eax,ebp
  sub  eax,1
  sbb  ebp,0

  mov  eax,[_dc_yl]

  cmp  eax,1
  adc  eax,0
  
  lea  edi,[ebp+ebp*4]
  sub  ebp,eax ; ebp = pixel count
  js   near .done

  shl  edi,6
  add  edi,[_dc_x]
  add  edi,[_destview]

  xor eax,eax

  mov eax,[_colormaps]
  mov	ecx,[_fuzzposinverse]
  add eax,0x600
  mov edx,_fuzzoffsetinverse
  mov ebx,49

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
	mov		al,[eax]
  JMPTESTFUZZPOSDEFINE LINE
  mov   ecx,ebx
  TESTFUZZPOSDEFINE LINE:
  mov		[edi-(LINE-1)*320],al
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
