	.386
	.MODEL  small
	INCLUDE defs.inc


;============================================================================
;
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

.DATA

EXTRN	_destview:DWORD
EXTRN	_centery:DWORD

SCALEDEFINE     MACRO   number
	dd      vscale&number
ENDM

	ALIGN   4
scalecalls      LABEL
LINE    =       0
REPT    SCREENHEIGHT+1
	SCALEDEFINE     %LINE
LINE    =       LINE+1
ENDM


;=================================


.CODE

;================
;
; R_DrawColumn
;
;================

PROC   R_DrawColumnVBE2_
PUBLIC   R_DrawColumnVBE2_
	PUSHR

	mov		ebp,[_dc_yh]
	lea		edi,[ebp+ebp*4]
	sal		edi,6
	mov		ebx,[_dc_x]
	add		edi,ebx
	add		edi,[_destview]

	mov		eax,[_dc_yl]
	sub     ebp,eax                    ; ebp = pixel count
	or		ebp,ebp
	js		done

	mov     ecx,[_dc_iscale]

	sub		eax,[_centery]
	imul	ecx
	mov		edx,[_dc_texturemid]
	add		edx,eax
	shl		edx,9							; 7 significant bits, 25 frac

	shl		ecx,9							; 7 significant bits, 25 frac
	mov     esi,[_dc_source]

	mov     eax,[_dc_colormap]

	xor     ebx,ebx
	shld    ebx,edx,7						; get address of first location
	call    [scalecalls+4+ebp*4]

done:
	POPR
	ret

;============ HIGH DETAIL ============

SCALELABEL      MACRO   number
vscale&number:
ENDM

LINE    =       SCREENHEIGHT
REPT SCREENHEIGHT-1
	SCALELABEL      %LINE
	mov     al,[esi+ebx]                    ; get source pixel
	add     edx,ecx                         ; calculate next location
	mov     al,[eax]                        ; translate the color
;	xor             ebx,ebx
;	shld    ebx,edx,7                      ; get address of next location
	mov		ebx,edx
	shr		ebx,25
	mov     [edi-(LINE-1)*SCREENWIDTH],al   ; draw a pixel to the buffer
LINE    =       LINE-1
ENDM
vscale1:
	mov     al,[esi+ebx]
	mov     al,[eax]
	mov     [edi],al
vscale0:
	ret

ENDP



.DATA


MAPDEFINE     MACRO   number
	dd      hmap&number
ENDM

	ALIGN   4
mapcalls      LABEL
LINE    =       0
REPT    SCREENWIDTH+1
	MAPDEFINE     %LINE
LINE    =       LINE+1
ENDM


callpoint	dd  0
returnpoint	dd	0


.CODE

;================
;
; R_DrawSpan
;
; Horizontal texture mapping
;
;================


PROC   R_DrawSpanVBE2_
PUBLIC	R_DrawSpanVBE2_
	PUSHR

	mov	eax,[_ds_x1]
	mov	ebx,[_ds_x2]
	mov	eax,[mapcalls+eax*4]
	mov	[callpoint],eax       ; spot to jump into unwound
	mov	eax,[mapcalls+4+ebx*4]
	mov	[returnpoint],eax     ; spot to patch a ret at
	mov	BYTE PTR [eax], OP_RET

;
; build composite position
;
	mov	ecx,[_ds_xfrac]
	shl	ecx,10
	and	ecx,0ffff0000h
	mov	eax,[_ds_yfrac]
	shr	eax,6
	and	eax,0ffffh
	or	ecx,eax

;
; build composite step
;
	mov	edx,[_ds_xstep]
	shl	edx,10
	and	edx,0ffff0000h
	mov	eax,[_ds_ystep]
	shr	eax,6
	and	eax,0ffffh
	or	edx,eax

	mov	esi,[_ds_source]

	mov		ebp,[_ds_y]
	lea		edi,[ebp+ebp*4]
	sal		edi,6
	add		edi,[_destview]

	mov	eax,[_ds_colormap]

;
; feed the pipeline and jump in
;
	mov		ebp,0fffh		; used to mask off slop high bits from position
	shld	ebx,ecx,22				; shift y units in
	shld	ebx,ecx,6				; shift x units in
	and		ebx,ebp					; mask off slop bits
	call    [callpoint]

	mov	ebx,[returnpoint]
	mov	BYTE PTR [ebx],OP_MOVAL		; remove the ret patched in

	POPR
	ret


;============= HIGH DETAIL ============

.CODE

MAPLABEL      MACRO   number
hmap&number:
ENDM

LINE    =      0
PCOL	=	0
REPT SCREENWIDTH/4
PLANE	=	0
REPT	4
	MAPLABEL      %LINE
LINE    =	LINE + 1

	mov     al,[esi+ebx]            ; get source pixel
	shld	ebx,ecx,22				; shift y units in
	shld	ebx,ecx,6				; shift x units in
	mov     al,[eax]                ; translate color
	and		ebx,ebp					; mask off slop bits
	add		ecx,edx					; position += step
	mov     [edi+PLANE+PCOL*4],al       ; write pixel
PLANE	=	PLANE + 1
ENDM
PCOL	=	PCOL + 1
ENDM
hmap320:
	ret

ENDP


END
