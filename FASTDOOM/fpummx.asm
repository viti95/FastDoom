;
; Copyright (C) 1993-1996 Id Software, Inc.
; Copyright (C) 1993-2008 Raven Software
; Copyright (C) 2025 Victor Nieto (viti95)
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
; DESCRIPTION: Assembly FPU/MMX routines

BITS 32

%include "macros.inc"

BEGIN_CODE_SECTION

CODE_SYM_DEF CopyQWordsMMX

.copy_loop_mmx:
    movq    mm0, [eax]
    add     eax, 8
    movq    [edx], mm0
    add     edx, 8
    dec     ebx
    jnz     .copy_loop_mmx

ret

CODE_SYM_DEF CopyQWordsFPU

.copy_loop_fpu:
    fild    qword [eax]
    add     eax, 8
    fistp   qword [edx]
    add     edx, 8
    dec     ebx
    jnz     .copy_loop_fpu

ret
