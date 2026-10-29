;
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
;	Assembly post-pass for the horizontal (two-pass) column
;	renderer implemented in C in r_drawhoriz.c. The pre-passes
;	(sprite/wall/sky column staging) stay in C for now; this
;	file only provides the screen-write half, which is the
;	inner loop that benefits from wide stores.
;
;	Symbols use the Watcom decorations from macros.inc:
;	C functions are referenced as name_, C variables as _name.
;
;	Stack policy: the body never uses the stack for
;	temporaries. The only stack traffic is the mandatory
;	push/pop of the callee-saved registers (EBX, ESI, EDI,
;	EBP) around the whole routine. Everything else lives in
;	registers or in the small _DATA scratch block below (the
;	flush is never reentrant, so fixed scratch memory is safe
;	and replaces the C off[4] array and locals).
;

BITS 32
%include "macros.inc"

; Same rule as SUPPORTS_HORIZ_RENDER in options.h. %ifdef
; USE_BACKBUFFER does not match MODE_VBE2_DIRECT builds, where the
; framebuffer is written directly through destview.
%assign HORIZ_ASM 0
%ifdef USE_BACKBUFFER
  %assign HORIZ_ASM 1
%endif
%ifdef MODE_VBE2_DIRECT
  %assign HORIZ_ASM 1
%endif

%if HORIZ_ASM

%include "defs.inc"

; =========================================================================
;
; Horizontal flush interface
; ==========================
;
; R_FlushHorizGroupAsm writes the currently staged 4-column group
; to the screen. Watcom register calling convention, same
; signature as the C reference R_FlushHorizGroupC:
;
;   eax = xstart    base screen x of the group (first column)
;   edx = group     number of columns in the group (1..4)
;   ebx = colormap  translation table for the staged bytes, or 0
;                   meaning copy the staged bytes verbatim (the
;                   staged columns were already translated by the
;                   wall pre-pass)
;
; EAX, EBX, ECX, EDX are all scratch in the Watcom convention
; (the C reference clobbers EBX too), so only ESI, EDI and EBP
; must be saved; EBX is pushed anyway, which is free to do. No
; other stack use.
;
; If horizymax < horizymin there is nothing staged: return
; immediately without touching anything.
;
; Staging state (plain C globals, r_drawhoriz.c):
;
;   _horiztemp32  dword array, SCREENHEIGHT entries. The four
;                 staged pixels of screen row y are the bytes of
;                 dword _horiztemp32[y]: byte 0 = column 0 of the
;                 group (screen x xstart), byte 1 = column 1, etc.
;                 Little-endian, so a dword load gives the four
;                 pixels ready for a dword store when the pixel
;                 width is 1 byte.
;   _horizrowmask byte per screen row. Bit c set = column c has a
;                 visible staged pixel on that row. The post-pass
;                 MUST clear each mask byte it has consumed
;                 (horizrowmask[y] = 0); the next group assumes
;                 consumed rows are zero. Rows outside
;                 horizymin..horizymax are always zero.
;   _horizymin    first row that can carry mask bits (or 0 >
;   _horizymax    horizymax when the group is empty)
;
; Destination addressing (identical to the C macros HORIZ_ROW /
; HORIZ_COFS / HORIZ_ROWSTRIDE in r_drawhoriz.c):
;
;   backbuffer modes:
;     row address  = [_ylookup + y*4]        (viewwindowy baked in)
;     column x off = [_columnofs + x*4]      (viewwindowx and the
;                                             detailshift pixel
;                                             width baked in)
;   MODE_VBE2_DIRECT:
;     row address  = [_destview] + y * SCREENWIDTH
;     column x off = x << [_detailshift]
;
;   Per-row behavior (mirrors R_FlushHorizGroupC exactly), one
;   loop per pixel width 1 << [_detailshift]:
;
;     m = horizrowmask[y]
;     m == 0                   : nothing on this row, skip
;     m == hz_fast (0 or 15)   : all four columns active and the
;                                group destination is contiguous:
;                                wide stores from one staged dword
;                                (1 dword / 2 dwords / 4 dwords at
;                                the pixel widths 1 / 2 / 4).
;     otherwise                : per-column writes at the set bits
;                                of m, cursor advanced by the
;                                precomputed off[c]-off[c-1] steps.
;     After writing: horizrowmask[y] = 0.
;
;   hz_fast is 15 only for full 4-column groups whose off[c]
;   advance by exactly one (duplicated) pixel per column; it is 0
;   otherwise (m is never 0 inside the mask test, so the wide path
;   is silently disabled for decimated CGA composite groups and
;   partial groups).
;
;   Colormap semantics:
;     eax == 0 : staged bytes are already screen colors. Sprites
;                always pass a table; for walls the wide width-1
;                path skips the lookups entirely (hz_raw), the
;                other paths go through a private identity table
;                (same result, the table is always in L1).
;     eax != 0 : translate each staged byte through [eax].
;
; =========================================================================

extern _detailshift
extern _horiztemp32
extern _horizrowmask
extern _horizymin
extern _horizymax

%ifdef USE_BACKBUFFER
extern _ylookup
extern _columnofs
%endif
%ifdef MODE_VBE2_DIRECT
extern _destview
%endif

; -------------------------------------------------------------------------
; Flush scratch state (never reentrant, one group in flight)
; -------------------------------------------------------------------------
BEGIN_DATA_SECTION

align 4
hz_off:   dd 0, 0, 0, 0     ; destination byte offsets of columns 0..3
hz_d1:    dd 0              ; off[1] - off[0]
hz_d2:    dd 0              ; off[2] - off[1]
hz_d3:    dd 0              ; off[3] - off[2]
hz_xstart: dd 0             ; saved xstart argument (a scratch
                            ; register is needed for the
                            ; empty-group test)
hz_group: dd 0              ; group argument (columns in the group)
hz_maparg: dd 0             ; saved colormap argument
hz_map:   dd 0              ; colormap base (identity table when raw)
hz_raw:   dd 0              ; 1 = colormap argument was NULL
hz_fast:  dd 0              ; fastmask: 15 or 0, see above

; Private identity table for the raw (already-translated) case, so
; the row loops never need a per-row "is this identity" test.
hz_identity:
%assign hzi 0
%rep 256
  db hzi
%assign hzi hzi + 1
%endrep

CONTINUE_CODE_SECTION

; -------------------------------------------------------------------------
; Row loop generator. %1 = label suffix (W1/W2/W4), %2 = pixel width
; in bytes (1/2/4).
;
; Invariants inside the loop:
;   EBP = current screen row y
;   EBX = horizymax + 1 (loop end, exclusive test is y > EBX)
;   ECX = colormap base (identity table for the raw case)
;   ESI = row address + off[0] (the wide-store destination; the
;       generic path walks it forward with the hz_dN deltas)
;   EAX/EDX/EDI = scratch
; -------------------------------------------------------------------------
%macro HZROWLOOP 2

hztop%1:
  movzx eax, byte [_horizrowmask + ebp]
  test  al, al
  jz    hzskip%1
  mov   byte [_horizrowmask + ebp], 0

  ; destination row base + offset of column 0
%ifdef USE_BACKBUFFER
  mov   esi, [_ylookup + ebp*4]
  add   esi, [hz_off]
%else
  MulScreenWidthStart esi, ebp
  MulScreenWidthEnd esi
  add   esi, [_destview]
  add   esi, [hz_off]
%endif

  cmp   eax, [hz_fast]
  je    hzwide%1

  ; ---- generic path: write the columns whose mask bits are set.
  ; ESI walks from column 0 to 3 with the precomputed deltas; a
  ; delta is added even for skipped columns so the cursor always
  ; tracks the current column.
  mov   edi, eax                    ; m
  test  al, 1
  jz    hzg1%1
  movzx edx, byte [_horiztemp32 + ebp*4]
  mov   dl, [ecx + edx]
%if %2 == 1
  mov   [esi], dl
%elif %2 == 2
  mov   dh, dl
  mov   [esi], dx
%else
  movzx edx, dl
  mov   eax, edx
  shl   eax, 8
  or    edx, eax
  mov   eax, edx
  shl   eax, 16
  or    edx, eax
  mov   [esi], edx
%endif

hzg1%1:
  add   esi, [hz_d1]
  test  edi, 2
  jz    hzg2%1
  movzx edx, byte [_horiztemp32 + ebp*4 + 1]
  mov   dl, [ecx + edx]
%if %2 == 1
  mov   [esi], dl
%elif %2 == 2
  mov   dh, dl
  mov   [esi], dx
%else
  movzx edx, dl
  mov   eax, edx
  shl   eax, 8
  or    edx, eax
  mov   eax, edx
  shl   eax, 16
  or    edx, eax
  mov   [esi], edx
%endif

hzg2%1:
  add   esi, [hz_d2]
  test  edi, 4
  jz    hzg3%1
  movzx edx, byte [_horiztemp32 + ebp*4 + 2]
  mov   dl, [ecx + edx]
%if %2 == 1
  mov   [esi], dl
%elif %2 == 2
  mov   dh, dl
  mov   [esi], dx
%else
  movzx edx, dl
  mov   eax, edx
  shl   eax, 8
  or    edx, eax
  mov   eax, edx
  shl   eax, 16
  or    edx, eax
  mov   [esi], edx
%endif

hzg3%1:
  add   esi, [hz_d3]
  test  edi, 8
  jz    hzg4%1
  movzx edx, byte [_horiztemp32 + ebp*4 + 3]
  mov   dl, [ecx + edx]
%if %2 == 1
  mov   [esi], dl
%elif %2 == 2
  mov   dh, dl
  mov   [esi], dx
%else
  movzx edx, dl
  mov   eax, edx
  shl   eax, 8
  or    edx, eax
  mov   eax, edx
  shl   eax, 16
  or    edx, eax
  mov   [esi], edx
%endif

hzg4%1:

hzskip%1:
  inc   ebp
  cmp   ebp, ebx
  jbe   hztop%1
  jmp   hzdone

  ; ---- wide path: all four columns active, one staged dword.
  ; EAX accumulates the first destination dword, EDX the current
  ; color/index, EDI a second scratch (m is no longer needed).
hzwide%1:
%if %2 == 1
  mov   edx, [_horiztemp32 + ebp*4]
  cmp   dword [hz_raw], 0
  jne   hzwide%1raw
  movzx eax, dl
  mov   al, [ecx + eax]             ; c0 (EAX zero-extended)
  movzx edx, dh
  mov   dl, [ecx + edx]             ; c1
  shl   edx, 8
  or    eax, edx
  mov   edx, [_horiztemp32 + ebp*4]
  shr   edx, 16
  movzx edx, dl
  mov   dl, [ecx + edx]             ; c2
  shl   edx, 16
  or    eax, edx
  mov   edx, [_horiztemp32 + ebp*4]
  shr   edx, 24
  mov   dl, [ecx + edx]             ; c3
  shl   edx, 24
  or    eax, edx
  mov   [esi], eax
  jmp   hzskip%1
hzwide%1raw:
  mov   edx, [_horiztemp32 + ebp*4]
  mov   [esi], edx                  ; verbatim 4-pixel copy
  jmp   hzskip%1
%elif %2 == 2
  ; two dwords, each holding one duplicated color per pixel:
  ; dword0 = c0 | c0<<8 | c1<<16 | c1<<24, dword1 from c2/c3.
  mov   edx, [_horiztemp32 + ebp*4]
  movzx eax, dl
  mov   al, [ecx + eax]             ; c0
  mov   edx, eax
  shl   edx, 8
  or    eax, edx                    ; c0 | c0<<8
  mov   edx, [_horiztemp32 + ebp*4]
  movzx edx, dh
  mov   dl, [ecx + edx]             ; c1
  mov   edi, edx
  shl   edx, 8
  or    edx, edi                    ; c1 | c1<<8
  shl   edx, 16
  or    eax, edx
  mov   [esi], eax
  mov   edx, [_horiztemp32 + ebp*4]
  shr   edx, 16
  movzx eax, dl
  mov   al, [ecx + eax]             ; c2
  mov   edx, eax
  shl   edx, 8
  or    eax, edx                    ; c2 | c2<<8
  mov   edx, [_horiztemp32 + ebp*4]
  shr   edx, 24
  mov   dl, [ecx + edx]             ; c3
  mov   edi, edx
  shl   edx, 8
  or    edx, edi                    ; c3 | c3<<8
  shl   edx, 16
  or    eax, edx
  mov   [esi+4], eax
  jmp   hzskip%1
%else
  ; four dwords, each one color replicated to all four bytes.
  mov   edx, [_horiztemp32 + ebp*4]
  movzx eax, dl
  mov   al, [ecx + eax]             ; c0
  mov   edx, eax
  shl   edx, 8
  or    eax, edx
  mov   edx, eax
  shl   eax, 16
  or    eax, edx
  mov   [esi], eax
  mov   edx, [_horiztemp32 + ebp*4]
  movzx edx, dh
  mov   dl, [ecx + edx]             ; c1
  mov   edi, edx
  shl   edx, 8
  or    edx, edi
  mov   edi, edx
  shl   edx, 16
  or    edx, edi
  mov   [esi+4], edx
  mov   edx, [_horiztemp32 + ebp*4]
  shr   edx, 16
  movzx edx, dl
  mov   dl, [ecx + edx]             ; c2
  mov   edi, edx
  shl   edx, 8
  or    edx, edi
  mov   edi, edx
  shl   edx, 16
  or    edx, edi
  mov   [esi+8], edx
  mov   edx, [_horiztemp32 + ebp*4]
  shr   edx, 24
  mov   dl, [ecx + edx]             ; c3
  mov   edi, edx
  shl   edx, 8
  or    edx, edi
  mov   edi, edx
  shl   edx, 16
  or    edx, edi
  mov   [esi+12], edx
  jmp   hzskip%1
%endif

%endmacro

BEGIN_CODE_SECTION

; ======================
; R_FlushHorizGroupAsm
; ======================
; eax = xstart, edx = group, ebx = colormap (0 = verbatim copy)
;
; Watcom 32-bit register convention passes the first integer
; arguments in EAX, EDX, EBX (verified against the WATCOM
; disassembly of the C reference: mov edi,eax / mov ebp,edx /
; test ebx,ebx in the prologue, and the C reference itself
; clobbers EBX, so it is a scratch register in this ABI).
CODE_SYM_DEF R_FlushHorizGroupAsm

  ; Park the register arguments in scratch memory first: the
  ; empty-group test below needs a scratch register before the
  ; callee-saved block is pushed.
  mov   [hz_xstart], eax
  mov   [hz_group], edx
  mov   [hz_maparg], ebx

  ; Nothing staged? (checked before the prologue, classic style)
  mov   eax, [_horizymax]
  cmp   eax, [_horizymin]
  jl    hzret

  push  ebx
  push  esi
  push  edi
  push  ebp

  ; ---- colormap selection: NULL routes the lookups through a
  ; private identity table; hz_raw lets the width-1 wide path skip
  ; them entirely (pure dword copy), like the C reference.
  xor   ebx, ebx
  mov   eax, [hz_maparg]
  test  eax, eax
  jnz   hzmapok
  mov   eax, hz_identity
  mov   ebx, 1
hzmapok:
  mov   [hz_map], eax
  mov   [hz_raw], ebx

  ; ---- destination offsets of the four columns. Partial groups
  ; only load the columns that exist; higher mask bits are never
  ; set for them, so the stale scratch entries are never used.
  mov   edx, [hz_xstart]
%ifdef USE_BACKBUFFER
  mov   eax, [_columnofs + edx*4]
  mov   [hz_off], eax
  cmp   dword [hz_group], 2
  jb    hzoffdone
  mov   eax, [_columnofs + edx*4 + 4]
  mov   [hz_off+4], eax
  cmp   dword [hz_group], 3
  jb    hzoffdone
  mov   eax, [_columnofs + edx*4 + 8]
  mov   [hz_off+8], eax
  cmp   dword [hz_group], 4
  jb    hzoffdone
  mov   eax, [_columnofs + edx*4 + 12]
  mov   [hz_off+12], eax
hzoffdone:
%else
  ; off[c] = (xstart + c) << detailshift (linear, step = 1<<ds)
  mov   ecx, [_detailshift]
  mov   ebx, edx
  shl   ebx, cl
  mov   [hz_off], ebx         ; off[0]
  mov   eax, 1
  shl   eax, cl               ; one duplicated pixel
  add   ebx, eax
  mov   [hz_off+4], ebx
  add   ebx, eax
  mov   [hz_off+8], ebx
  add   ebx, eax
  mov   [hz_off+12], ebx
%endif

  ; ---- per-column destination deltas (used by the generic path)
  mov   eax, [hz_off+4]
  sub   eax, [hz_off]
  mov   [hz_d1], eax
  mov   eax, [hz_off+8]
  sub   eax, [hz_off+4]
  mov   [hz_d2], eax
  mov   eax, [hz_off+12]
  sub   eax, [hz_off+8]
  mov   [hz_d3], eax

  ; ---- fastmask: wide stores only for full groups whose
  ; destinations advance by exactly one pixel (1 << detailshift)
  ; per column. d1|d2|d3 == step only when all three deltas are
  ; the step (offsets are strictly increasing).
  mov   dword [hz_fast], 0
  cmp   dword [hz_group], 4
  jne   hzfmdone
  mov   ebx, 1
  mov   ecx, [_detailshift]
  shl   ebx, cl
  mov   eax, [hz_d1]
  or    eax, [hz_d2]
  or    eax, [hz_d3]
  cmp   eax, ebx
  jne   hzfmdone
  mov   dword [hz_fast], 15
hzfmdone:

  ; ---- loop registers: ECX = map, EBP = y, EBX = ymax + 1
  mov   ecx, [hz_map]
  mov   ebp, [_horizymin]
  mov   ebx, [_horizymax]
  inc   ebx

  ; ---- one row loop per pixel width
  mov   eax, [_detailshift]
  test  eax, eax
  jz    hztopW1
  dec   eax
  jz    hztopW2
  jmp   hztopW4

  ; The three row loops (instantiated below) jump back here when
  ; the row range is exhausted.
hzdone:
  pop   ebp
  pop   edi
  pop   esi
  pop   ebx
hzret:
  ret

; The row loops themselves: pixel widths 1, 2 and 4 bytes.
HZROWLOOP W1, 1
HZROWLOOP W2, 2
HZROWLOOP W4, 4

%endif
