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
; DESCRIPTION:
;
	.386
	.MODEL  small
	INCLUDE defs.inc

.DATA

EXTRN	_destview:DWORD
EXTRN	_centery:DWORD

pixelcount dd 0
loopcount dd 0


;=================================


.CODE

;
; R_DrawColumn
;
PROC  R_DrawColumnLow_
PUBLIC  R_DrawColumnLow_
	PUSHR
	mov		ebp,[_dc_yl]
	cmp 	ebp,[_dc_yh]
	jg		done
	lea		edi,[ebp+ebp*8]
	add		edi,ebp
	shl		edi,3
	mov		ebx,[_dc_x]
	mov		ecx,ebx
	shr		ebx,1
	add		edi,ebx
	add		edi,[_destview]
	and 	ecx,1
	shl		ecx,1
	mov		eax,3
	shl		eax,cl
	mov		edx,SC_INDEX+1
	out		dx,al
	jmp		cdraw
ENDP
	
PROC  R_DrawColumn_
PUBLIC  R_DrawColumn_
	PUSHR
	mov		ebp,[_dc_yl]
	cmp		ebp,[_dc_yh]
	jg		done
	lea		edi,[ebp+ebp*8]
	add		edi,ebp
	shl		edi,3
	mov		ebx,[_dc_x]
	mov		ecx,ebx
	shr		ebx,2
	add		edi,ebx
	add		edi,[_destview]
	and		ecx,3
	mov		eax,1
	shl		eax,cl
	mov		edx,SC_INDEX+1
	out		dx,al
cdraw:
	mov		eax,[_dc_yh]
	inc		eax
	sub		eax,ebp						; pixel count
	mov		[pixelcount],eax			; save for final pixel
	js		done						; nothing to scale
	shr		eax,1						; double pixel count
	mov		[loopcount],eax
	
	mov		ecx,[_dc_iscale]
	
	mov		eax,[_centery]
	sub		eax,ebp
	imul	ecx
	mov		ebp,[_dc_texturemid]
	sub		ebp,eax
	shl		ebp,9						; 7 significant bits, 25 frac
	
	mov		esi,[_dc_source]
	
	mov		ebx,[_dc_iscale]
	shl		ebx,9
	mov		eax,OFFSET patch1+2			; convice tasm to modify code...
	mov		[eax],ebx
	mov		eax,OFFSET patch2+2			; convice tasm to modify code...
	mov		[eax],ebx
	
; eax		aligned colormap
; ebx		aligned colormap
; ecx,edx	scratch
; esi		virtual source
; edi		moving destination pointer
; ebp		frac

	mov		ecx,ebp						; begin calculating first pixel
	add		ebp,ebx						; advance frac pointer
	shr		ecx,25						; finish calculation for first pixel
	mov		edx,ebp						; begin calculating second pixel
	add		ebp,ebx						; advance frac pointer
	shr		edx,25						; finish calculation for second pixel
	mov		eax,[_dc_colormap]
	mov		ebx,eax
	mov		al,[esi+ecx]				; get first pixel
	mov		bl,[esi+edx]				; get second pixel
	mov		al,[eax]					; color translate first pixel
	mov		bl,[ebx]					; color translate second pixel
	
	test	[pixelcount],0fffffffeh
	jnz		doubleloop					; at least two pixels to map
	jmp		checklast
doubleloop:
	mov		ecx,ebp						; begin calculating third pixel
patch1:
	add		ebp,12345678h				; advance frac pointer
	mov		[edi],al					; write first pixel
	shr		ecx,25						; finish calculation for third pixel
	mov		edx,ebp						; begin calculating fourth pixel
patch2:
	add		ebp,12345678h				; advance frac pointer
	mov		[edi+SCREENWIDTH/4],bl		; write second pixel
	shr		edx,25						; finish calculation for fourth pixel
	mov		al,[esi+ecx]				; get third pixel
	add		edi,SCREENWIDTH/2			; advance to third pixel destination
	mov		bl,[esi+edx]				; get fourth pixel
	dec		[loopcount]					; done with loop?
	mov		al,[eax]					; color translate third pixel
	mov		bl,[ebx]					; color translate fourth pixel
	jnz		doubleloop
checklast:
	test	[pixelcount],1
	jz		done
	mov		[edi],al					; write final pixel
done:
	POPR
	ret
ENDP

.DATA

dest dd 0
endplane dd 0
curplane dd 0
frac dd 0
fracstep dd 0
fracpstep dd 0
curx dd 0
curpx dd 0
endpx dd 0


.CODE

;
; R_DrawSpan
; Horizontal texture mapping
;


PROC   R_DrawSpan_
PUBLIC	R_DrawSpan_
	PUSHR
	mov		eax,[_ds_x1]
	mov		[curx],eax
	mov 	ebx,eax
	and		ebx,3
	mov		[endplane],ebx
	mov		[curplane],ebx
	shr		eax,2
	mov		ebp,[_ds_y]
	lea		edi,[ebp+ebp*8]
	add		edi,ebp
	shl		edi,3
	add		edi,eax
	add		edi,[_destview]
	mov		[dest],edi
	
	mov		ebx,[_ds_xfrac]
	shl		ebx,10
	and		ebx,0ffff0000h
	mov		eax,[_ds_yfrac]
	shr		eax,6
	and		eax,0ffffh
	or		ebx,eax
	
	mov		[frac],ebx
	
	mov		ebx,[_ds_xstep]
	shl		ebx,10
	and		ebx,0ffff0000h
	mov		eax,[_ds_ystep]
	shr		eax,6
	and		eax,0ffffh
	or		ebx,eax
	
	mov		[fracstep],ebx
	
	shl		ebx,2
	mov		[fracpstep],ebx
	mov		eax,OFFSET hpatch1+2
	mov		[eax],ebx
	mov		eax,OFFSET hpatch2+2
	mov		[eax],ebx
	mov		ecx,[curplane]
hplane:
	mov		eax,1
	shl		eax,cl
	mov		edx,SC_INDEX+1
	out		dx,al
	mov		eax,[_ds_x2]
	cmp		eax,[curx]
	jb		hdone
	sub		eax,[curplane]
	js		hdoneplane
	shr		eax,2
	mov		[endpx],eax
	dec		eax
	js		hfillone
	shr		eax,1
	mov		ebx,[curx]
	shr		ebx,2
	cmp		ebx,[endpx]
	jz		hfillone
	mov		[curpx],ebx
	inc		ebx
	shr		ebx,1
	inc		eax
	sub		eax,ebx
	js		hdoneplane
	mov		[loopcount],eax
	mov		eax,[_ds_colormap]
	mov		ebx,eax
	mov		esi,[_ds_source]
	mov		edi,[dest]
	mov		ebp,[frac]
	test	[curpx],1
	jz		hfill
	shld	ecx,ebp,22
	shld	ecx,ebp,6
	add		ebp,[fracpstep]
	and		ecx,0fffh
	mov		al,[esi+ecx]
	mov		dl,[eax]
	mov		[edi],dl
	inc		edi
	jz		hdoneplane
hfill:
	shld	ecx,ebp,22
	shld	ecx,ebp,6
	add		ebp,[fracpstep]
	and		ecx,0fffh
	shld	edx,ebp,22
	shld	edx,ebp,6
	add		ebp,[fracpstep]
	and		edx,0fffh
	mov		al,[esi+ecx]
	mov		bl,[esi+edx]
	mov		dl,[eax]
	test	[loopcount],0ffffffffh
	jnz		hdoubleloop
	jmp		hchecklast
hfillone:
	mov		eax,[_ds_colormap]
	mov		esi,[_ds_source]
	mov		edi,[dest]
	mov		ebp,[frac]
	shld	ecx,ebp,22
	shld	ecx,ebp,6
	and		ecx,0fffh
	mov		al,[esi+ecx]
	mov		dl,[eax]
	mov		[edi],dl
	jmp		hdoneplane
hdoubleloop:
	shld	ecx,ebp,22
	mov		dh,[ebx]
	shld	ecx,ebp,6
hpatch1:
	add		ebp,12345678h
	and		ecx,0fffh
	mov		[edi],dx
	shld	edx,ebp,22
	add		edi,2
	shld	edx,ebp,6
hpatch2:
	add		ebp,12345678h
	and		edx,0fffh
	mov		al,[esi+ecx]
	mov		bl,[esi+edx]
	dec		[loopcount]
	mov		dl,[eax]
	jnz		hdoubleloop
hchecklast:
	test	[endpx],1
	jnz		hdoneplane
	mov		[edi],dl
hdoneplane:
	mov		ecx,[curplane]
	inc		ecx
	and		ecx,3
	jnz		hskip
	inc		[dest]
hskip:
	cmp		ecx,[endplane]
	jz		hdone
	mov		[curplane],ecx
	mov		ebx,[frac]
	add		ebx,[fracstep]
	mov		[frac],ebx
	inc		[curx]
	jmp		hplane
hdone:
	POPR
	ret
ENDP
	
PROC   R_DrawSpanLow_
PUBLIC	R_DrawSpanLow_
	PUSHR
	mov		eax,[_ds_x1]
	mov		[curx],eax
	mov 	ebx,eax
	and		ebx,1
	mov		[endplane],ebx
	mov		[curplane],ebx
	shr		eax,1
	mov		ebp,[_ds_y]
	lea		edi,[ebp+ebp*8]
	add		edi,ebp
	shl		edi,3
	add		edi,eax
	add		edi,[_destview]
	mov		[dest],edi
	
	mov		ebx,[_ds_xfrac]
	shl		ebx,10
	and		ebx,0ffff0000h
	mov		eax,[_ds_yfrac]
	shr		eax,6
	and		eax,0ffffh
	or		ebx,eax
	
	mov		[frac],ebx
	
	mov		ebx,[_ds_xstep]
	shl		ebx,10
	and		ebx,0ffff0000h
	mov		eax,[_ds_ystep]
	shr		eax,6
	and		eax,0ffffh
	or		ebx,eax
	
	mov		[fracstep],ebx
	
	shl		ebx,1
	mov		[fracpstep],ebx
	mov		eax,OFFSET lpatch1+2
	mov		[eax],ebx
	mov		eax,OFFSET lpatch2+2
	mov		[eax],ebx
	mov		ecx,[curplane]
lplane:
	mov		eax,3
	shl		eax,cl
	shl		eax,cl
	mov		edx,SC_INDEX+1
	out		dx,al
	mov		eax,[_ds_x2]
	cmp		eax,[curx]
	jb		ldone
	sub		eax,[curplane]
	js		ldoneplane
	shr		eax,1
	mov		[endpx],eax
	dec		eax
	js		lfillone
	shr		eax,1
	mov		ebx,[curx]
	shr		ebx,1
	cmp		ebx,[endpx]
	jz		lfillone
	mov		[curpx],ebx
	inc		ebx
	shr		ebx,1
	inc		eax
	sub		eax,ebx
	js		ldoneplane
	mov		[loopcount],eax
	mov		eax,[_ds_colormap]
	mov		ebx,eax
	mov		esi,[_ds_source]
	mov		edi,[dest]
	mov		ebp,[frac]
	test	[curpx],1
	jz		lfill
	shld	ecx,ebp,22
	shld	ecx,ebp,6
	add		ebp,[fracpstep]
	and		ecx,0fffh
	mov		al,[esi+ecx]
	mov		dl,[eax]
	mov		[edi],dl
	inc		edi
	jz		ldoneplane
lfill:
	shld	ecx,ebp,22
	shld	ecx,ebp,6
	add		ebp,[fracpstep]
	and		ecx,0fffh
	shld	edx,ebp,22
	shld	edx,ebp,6
	add		ebp,[fracpstep]
	and		edx,0fffh
	mov		al,[esi+ecx]
	mov		bl,[esi+edx]
	mov		dl,[eax]
	test	[loopcount],0ffffffffh
	jnz		ldoubleloop
	jmp		lchecklast
lfillone:
	mov		eax,[_ds_colormap]
	mov		esi,[_ds_source]
	mov		edi,[dest]
	mov		ebp,[frac]
	shld	ecx,ebp,22
	shld	ecx,ebp,6
	and		ecx,0fffh
	mov		al,[esi+ecx]
	mov		dl,[eax]
	mov		[edi],dl
	jmp		ldoneplane
ldoubleloop:
	shld	ecx,ebp,22
	mov		dh,[ebx]
	shld	ecx,ebp,6
lpatch1:
	add		ebp,12345678h
	and		ecx,0fffh
	mov		[edi],dx
	shld	edx,ebp,22
	add		edi,2
	shld	edx,ebp,6
lpatch2:
	add		ebp,12345678h
	and		edx,0fffh
	mov		al,[esi+ecx]
	mov		bl,[esi+edx]
	dec		[loopcount]
	mov		dl,[eax]
	jnz		ldoubleloop
lchecklast:
	test	[endpx],1
	jnz		ldoneplane
	mov		[edi],dl
ldoneplane:
	mov		ecx,[curplane]
	inc		ecx
	and		ecx,1
	jnz		lskip
	inc		[dest]
lskip:
	cmp		ecx,[endplane]
	jz		ldone
	mov		[curplane],ecx
	mov		ebx,[frac]
	add		ebx,[fracstep]
	mov		[frac],ebx
	inc		[curx]
	jmp		lplane
ldone:
	POPR
	ret
ENDP

END