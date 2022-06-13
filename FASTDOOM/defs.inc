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

SCREEN        equ 0xA0000
SCREENWIDTH   equ 320
SCREENHEIGHT  equ 200
PLANEWIDTH    equ 80
PLANESIZE     equ 80*200

PEL_WRITE_ADR  equ 0x3C8
PEL_DATA       equ 0x3C9

SC_INDEX    equ 0x3C4
SC_MAPMASK  equ 2

OP_RET      equ 0xC3
OP_MOVAL    equ 0x8A
OP_MOVDEST  equ 0x88

extern _dc_colormap
extern _dc_x
extern _dc_yl
extern _dc_yh
extern _dc_iscale
extern _dc_texturemid
extern _dc_source

extern _columnofs
extern _ylookup

extern _ds_y
extern _ds_x1
extern _ds_x2
extern _ds_colormap
extern _ds_xfrac
extern _ds_yfrac
extern _ds_step
extern _ds_source
