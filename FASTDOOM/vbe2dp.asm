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

; The rows of the column left to the loop of four of them, which is not of a
; register of the machine, of the five registers of them which the loop keeps
; being the source, the destination, the light table and the four texels of it
align 4
columnRowsPotatoSSE2: dd 0

;============================================================================

BEGIN_CODE_SECTION

;============================================================================
;
; R_DrawColumnPotatoVBE2
;
;============================================================================

CODE_SYM_DEF R_PatchCenteryVBE2Potato
  push  ebx
  mov   ebx,[_centery]
  mov   eax,patchCentery+1
  mov   [eax],ebx
  mov   eax,patchCenteryMMX+1
  mov   [eax],ebx
  mov   eax,patchCenterySSE2+1
  mov   [eax],ebx
  pop   ebx
  ret

CODE_SYM_DEF R_DrawColumnPotatoVBE2
	push		edi
	push		esi
	push		edx
	push		ebp
  push		ebx
	push		ecx

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  MulScreenWidthStart edi, ebp
  sub  ebp,eax ; ebp = pixel count
  js   near donep

  mov  ebx,[_dc_x]
  MulScreenWidthEnd edi
  mov  ecx,[_dc_iscale]
  lea  edi,[edi+ebx*4]

patchCentery:
  sub  eax,0x12345678
  add  edi,[_destview]

  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  jmp  [scalecalls+4+ebp*4]

donep:
	pop		ecx
	pop		ebx
  pop	  ebp
	pop		edx
	pop		esi
	pop		edi
  ret
; R_DrawColumnPotatoVBE2 ends

;============ HIGH DETAIL ============

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:
    mov  ebp,edx
    shr  ebp,25
    mov  al,[esi+ebp]                   ; get source pixel
    add  edx,ecx                        ; calculate next location
    mov  bl,[eax]                       ; translate the color
    mov  bh,bl
    mov  [edi-(LINE-1)*SCREENWIDTH],bx  ; draw a pixel to the buffer
    mov  [edi-(LINE-1)*SCREENWIDTH + 2],bx  ; draw a pixel to the buffer
    %assign LINE LINE-1
%endrep

vscale1:
  mov ebp,edx
  pop	ecx
  shr ebp,25
  pop	ebx
  mov al,[esi+ebp]
  pop	ebp
  mov al,[eax]
  pop	edx
  mov ah,al
  pop	esi
  mov [edi],ax
  mov [edi+2],ax

vscale0:
	pop		edi
  ret

; ================================
;
; R_DrawColumnPotatoVBE2MMX
;
; The column of the loop of the registers of MMX of the file of the detail of
; the potato, of two rows of them in each one of the iterations of the loop,
; the one of them in each one of the two dwords of the register of their
; positions, walked to the rows to come with one "paddd" of the step of a row,
; the two of them broadcasted to the two dwords of the register of it.
;
; The four pixels of a row of the detail of the potato, which are of the byte
; of the same one of them, go to the memory linear of the mode with one store
; of 32 bits, the byte of the color of the table of the light being multiplied
; by the value of the four bytes of one of them, 1.01.01.01 hex, with one
; "imul" of the registers of the machine, as the renderer of the mode of the
; file writes the four of them with two stores of 16 bits of them.
;
; The texels of the two rows go out of their positions with one "psrld", of
; the seven bits of them only, the graphics wrapping vertically at 128 pixels.
; The texel of the low dword of the positions goes to the registers of the
; machine with one "movd" and the texel of the high one with one "psrlq" and
; one "movd" of them.
;
; The byte of the source of every one of them names its pixel of the light
; table with the address of it, the table being of 256 bytes, which the
; renderers of the mode of the file name with their low byte written over.
; The pixels of them go to the registers of the texels of them, of the texels
; being read already, for the bytes of the high of the registers of the tables
; of the light, the ones of their addresses, not to be broken by the byte of
; the pixel of a row of them. The rows of them are of two registers of the
; table, the one of the row of the register of it and the one of the row to
; come of the copy of it.
;
; The row of the column which is not a pair of them of the loop goes before
; it, of the registers of the machine, of the rows of the column being an odd
; number of them, and of one row only of the column the row of it is of a loop
; of none of them, as of the renderer of the mode of the file.
;
; eax   the table of the light, of its low byte the texel of a row
; ebx   the texel of the low dword of the positions, and the four bytes of it
; ecx   the copy of the table of the light, of its low byte the texel of the
;       row to come
; edx   the texel of the high dword of the positions, and the four bytes of it
; esi   the source of the column
; edi   the pixel of the row of the view
; ebp   the rows of the column left, the loop of a register
;
; mm0   the positions of the two rows, one of them in each one of them
; mm1   the step of the two of them, of both the dwords of it
; mm3   the texels of the two rows, of the packing of their positions
;
; "emms" is not of the renderers of the project, which are of no x87 code of
; their own, and this renderer is of the same kind.
; ================================
CODE_SYM_DEF R_DrawColumnPotatoVBE2MMX
  push		edi
		push		ebx
		push		ecx
		push		edx
		push		esi
		push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  MulScreenWidthStart edi, eax
  sub  ebp,eax         ; ebp = the rows of the column less one
  js   near donepvx

  mov  ebx,[_dc_x]
  MulScreenWidthEnd edi
  mov  ecx,[_dc_iscale]
  lea  edi,[edi+ebx*4]

patchCenteryMMX:
  sub  eax,0x12345678
  add  edi,[_destview]

  imul ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  cmp   ebp,0
  je    near SinglePixelPotatoVX

  test  ebp,1
  jnz   near EvenPotatoVX

  ; the row of the column which is not a pair of them of the loop, of the
  ; rows of the column being an odd number of them

  mov   ebx,edx
  shr   ebx,25 ; get address of first location
  mov   al,[esi+ebx]                   ; get source pixel
  add   edx,ecx                        ; the position of the rows to come
  mov   bl,[eax]                       ; translate the color, the register of
  dec   ebp                            ;  the table being kept for the loop
  imul  ebx,ebx,0x01010101             ; the four bytes of the pixel of it
  mov   [edi],ebx                      ; the four pixels of the row

  add   edi,SCREENWIDTH

  ; MMX 4 pixels render
EvenPotatoVX:

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

LoopPotatoVX:

  movd       ebx, mm3         ; the texel of the row

  paddd      mm0, mm1         ; the positions of the two rows to come

  psrlq      mm3, 32

  mov  al,[ebx+esi]           ; the source pixel of the row

  movd       edx, mm3         ; the texel of the row to come

  movq       mm3, mm0         ; the texels of the two rows of the next one

  mov  bl,[eax]               ; its pixel of the table of the light

  mov  cl,[edx+esi]           ; the source pixel of the row to come, its
                              ;  texel being of the copy of the table

  imul ebx,ebx,0x01010101     ; the four bytes of the pixel of it

  mov  dl,[ecx]               ; its pixel of the table of the light

  psrld      mm3, 25

  imul edx,edx,0x01010101     ; the four bytes of the pixel of it

  mov  [edi],ebx              ; the four pixels of the row
  mov  [edi+SCREENWIDTH],edx  ; the four pixels of the row to come

  add  edi, 2*SCREENWIDTH

  sub  ebp, 2
  jns  LoopPotatoVX

donepvx:
		pop		ebp
		pop		esi
		pop		edx
		pop		ecx
		pop		ebx
  pop		edi
  ret
; R_DrawColumnPotatoVBE2MMX ends

SinglePixelPotatoVX:

  shr  edx,25 ; get address of first location
  pop		ebp
  mov  al,[esi+edx]                   ; get source pixel
  pop		esi
  pop		edx
  mov  al,[eax]                       ; translate the color
  pop		ecx
		pop		ebx
  movzx eax,al                        ; the byte of it of its own
  imul  eax,eax,0x01010101            ; the four bytes of the pixel of it
  mov  [edi],eax                      ; the four pixels of the row
  pop		edi
  ret

; =========================
;
; R_DrawColumnPotatoVBE2SSE2
;
; Four rows per iteration of the loop, of the positions of them in the four
; dwords of one register of SSE2, walked with one "paddd" of the step of a row
; broadcasted to the four dwords of another one.
;
; The texels of the four of them are out of the positions with one "psrld",
; joined in the four bytes of one dword with one "packssdw" and one
; "packuswb", of the texels being of seven bits only, the graphics wrapping
; vertically at 128 pixels, and gotten to the registers of the machine with
; one "movd", three "movzx" and one "shr".  The byte of the source of every
; one of them names its pixel of the light table with the address of it, the
; table being of 256 bytes, the register of it being of the "and" of it with
; the value of them, for the byte of the source naming its pixel of the table
; being of one "movzx" of them.
;
; The four pixels of a row of the detail of the file, which are of the byte of
; the same one of them, go to the memory linear of the mode with one store of
; 32 bits, the byte of the pixel of the light table being multiplied by the
; value of the four bytes of one of them, 1.01.01.01 hex, with one "imul" of
; the registers of the machine, as the renderer of the mode of the file writes
; the four of them with the byte of the table in the four bytes of the register
; of them.
;
; The rows of the column which are not a full group of four of them are
; rendered one by one with a scalar loop at the end, of their positions in the
; dwords of the register which the loop of the groups leaves, the last group
; of them having stepped to the rows of them.
;
; eax   the rows of the column and a register of the texels
; ebx   a register of the texels
; ecx   a register of the texels
; edx   a register of the texels
; esi   the source of the column
; edi   the pixel of the row of the view
; ebp   the light table, of its low byte zero
;
; xmm0  the positions of the four rows, one of them in each dword
; xmm1  the step of a row, of the four dwords of it
; xmm2  the positions of the four rows, of their texels in the bytes of them
;
; The registers of SSE2 are not of the ones of the registers of MMX, so one
; "emms" is not needed of the return of this renderer, as of the renderers of
; the project which are of the instructions of SSE2.
; =========================
CODE_SYM_DEF R_DrawColumnPotatoVBE2SSE2
  push    edi
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  MulScreenWidthStart edi, eax
  sub  ebp,eax         ; ebp = pixels of the column less one
  js   near donepvss   ; a column of no rows
  inc  ebp             ; ebp = the rows of the column

  mov  ebx,[_dc_x]
  MulScreenWidthEnd edi
  mov  ecx,[_dc_iscale]
  lea  edi,[edi+ebx*4]
  add  edi,[_destview]

patchCenterySSE2:
  sub   eax,0x12345678
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9 ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9 ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]
  and   eax,-256 ; the tables of the game are of 256 bytes, as the byte of
                 ; the source naming its pixel of the table is of one "movzx"
  xchg  eax,ebp ; the light table of the column in ebp, the rows in eax

  mov   ebx,eax
  and   eax,3 ; the rows of the column which are not a full group of four
  shr   ebx,2 ; groups of four rows
  dec   ebx   ; iterations of the loop less one
  mov   [columnRowsPotatoSSE2],ebx
  push  eax   ; the rows of the column left for the scalar loop

  ; xmm0 has the positions of the first group of four rows, one of them in
  ; each dword of it, and xmm1 the step of a row in all of the four of them

  movd  xmm0,edx          ; the position of the first row
  lea   eax,[edx+ecx]
  movd  xmm1,eax          ; ... of the second one
  lea   eax,[eax+ecx]
  movd  xmm2,eax          ; ... of the third one
  lea   eax,[eax+ecx]
  movd  xmm3,eax          ; ... of the fourth one
  shufps xmm0,xmm1,0      ; the two first positions of them
  shufps xmm2,xmm3,0      ; the two last positions of them
  shufps xmm0,xmm2,0x88   ; the four positions of them
  lea   eax,[ecx*4]       ; the step of four rows
  movd  xmm1,eax
  shufps xmm1,xmm1,0      ; it broadcasted to the four dwords

  ; the instructions of them above do not modify the flags, so the sign of
  ; the iterations of the loop less one calculated with them is still in them

  js    near TailPotatoVS ; there are not groups of four rows

align 16
LoopPotatoVS:
  movdqa xmm2,xmm0
  psrld  xmm2,25            ; the texels of the four rows
  paddd  xmm0,xmm1          ; the positions of the four rows to come
  packssdw xmm2,xmm2
  packuswb xmm2,xmm2        ; the four texels of them, one in each byte

  movd  eax,xmm2
  movzx edx,al              ; the texel of the first row
  movzx ebx,ah              ; ... of the second one
  shr   eax,16
  movzx ecx,al              ; ... of the third one
  movzx eax,ah              ; ... of the fourth one

  movzx edx,byte [esi+edx]  ; the byte of the source of the first row
  movzx edx,byte [ebp+edx]  ; its pixel of the light table
  imul  edx,edx,0x01010101  ; the four bytes of the pixel of it
  mov   [edi],edx           ; the four pixels of the row

  movzx ebx,byte [esi+ebx]  ; the byte of the source of the second row
  movzx ebx,byte [ebp+ebx]  ; its pixel of the light table
  imul  ebx,ebx,0x01010101  ; the four bytes of the pixel of it
  mov   [edi+SCREENWIDTH],ebx

  movzx ecx,byte [esi+ecx]  ; the byte of the source of the third row
  movzx ecx,byte [ebp+ecx]  ; its pixel of the light table
  imul  ecx,ecx,0x01010101  ; the four bytes of the pixel of it
  mov   [edi+2*SCREENWIDTH],ecx

  movzx eax,byte [esi+eax]  ; the byte of the source of the fourth row
  movzx eax,byte [ebp+eax]  ; its pixel of the light table
  imul  eax,eax,0x01010101  ; the four bytes of the pixel of it
  mov   [edi+3*SCREENWIDTH],eax

  add   edi,4*SCREENWIDTH

  dec   dword [columnRowsPotatoSSE2]
  jns   near LoopPotatoVS

  ; the rows of the column which are not a full group of four of them, one by
  ; one, with the positions of them in the dwords of xmm0

TailPotatoVS:
  pop   ecx                 ; the rows which are not a full group of four
  test  ecx,ecx
  je    near EndPotatoVS

  ; the source is in esi, the destination of the pixels in edi and the light
  ; table in ebp

align 16
LoopTailPotatoVS:
  movd  edx,xmm0            ; the position of the row
  psrldq xmm0,4             ; the positions of the rows to come
  shr   edx,25              ; the texel of the row
  movzx edx,byte [esi+edx]  ; the byte of the source
  movzx edx,byte [ebp+edx]  ; its pixel of the light table
  imul  edx,edx,0x01010101  ; the four bytes of the pixel of it
  mov   [edi],edx           ; the four pixels of the row
  add   edi,SCREENWIDTH

  dec   ecx
  jnz   near LoopTailPotatoVS

EndPotatoVS:
  pop   ebp
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  pop   edi
  ret

  ; a column of no rows, of the rows of it being in the sign of the flags

donepvss:
  pop   ebp
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  pop   edi
  ret
; R_DrawColumnPotatoVBE2SSE2 ends

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
  mov  [returnpoint],eax ; spot to patch a ret at
  mov  [eax], byte OP_RET

  mov  edx,[_ds_y]
  mov  eax,[_ds_colormap]
  MulScreenWidthStart edi, edx
  shld  ebx,ecx,22      ; shift y units in
  MulScreenWidthEnd edi
  add  edi,[_destview]
  shld  ebx,ecx,6       ; shift x units in
  and     ebx,0x0FFF  ; mask off slop bits
  add     ecx,ebp

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
        shld  ebx,ecx,22             ; shift y units in
        mov   dl,[eax]               ; translate color
        shld  ebx,ecx,6              ; shift x units in
        mov   dh,dl
        mov   [edi+PLANE+PCOL*4],dx  ; write pixel
        mov   [edi+PLANE+PCOL*4+2],dx  ; write pixel
        and   ebx,0x0FFF             ; mask off slop bits
        %if LINE < (SCREENWIDTH/4)-1
        add   ecx,ebp                ; position += step
        %endif
      %endif
      %assign PLANE PLANE+1
%assign PCOL PCOL+1
%endrep

MAPLABEL LINE:
  rep

%endif
