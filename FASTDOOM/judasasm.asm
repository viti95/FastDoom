; This module contains soundcard IRQ handlers, DMA routines, mixing routines
; as well as some time-critical GUS routines and AC97 access code.
;
; N�m� ei mit��n (kovin) optimisoituja rutiineja ole, PERKELE!
;
; Modified by BSpider for NASM on 5th Sep 2006

                %idefine offset
                %include "segments.inc"
                %include "judascfg.inc"
                %include "judasgus.inc"
                %include "judasac.inc"

%define         MONO            0
%define         EIGHTBIT        0
%define         STEREO          1
%define         SIXTEENBIT      2

%define         VM_OFF          0
%define         VM_ON           1
%define         VM_LOOP         2
%define         VM_16BIT        4

%define         DEV_NOSOUND     0
%define         DEV_SB          1
%define         DEV_SBPRO       2
%define         DEV_SB16        3
%define         DEV_GUS         4
%define         DEV_AC97        5
%define         DEV_HDA         6
%define         DEV_FILE        7

%define         CACHESLOTS      16

%define         IPMINUS1        -1
%define         IP0              0
%define         IP1              1
%define         IP2              2

; predefined to 0
struc           CACHESLOT
                GDC_Pos         resd 1
                GDC_Length      resd 1
endstruc

; predefined to 0
struc           DMACHANNEL
                DMA_PagePort    resw 1
                DMA_AdrPort     resw 1
                DMA_LenPort     resw 1
                DMA_MaskPort    resw 1
                DMA_ModePort    resw 1
                DMA_ClearPort   resw 1
                DMA_Mask        resb 1
                DMA_UnMask      resb 1
                DMA_Unused      resw 1
endstruc

; predefined to 0
struc           CHANNEL
                Chn_Pos          resd 1
                Chn_Repeat       resd 1
                Chn_End          resd 1
                Chn_Sample       resd 1
                Chn_Freq         resd 1
                Chn_FractPos     resw 1
                Chn_MasterVol    resb 1
                Chn_Panning      resb 1
                Chn_Vol          resw 1
                Chn_VoiceMode    resb 1
                Chn_PrevVM       resb 1
                Chn_PrevPos      resd 1
                Chn_LastValL     resd 1
                Chn_LastValR     resd 1
                Chn_SmoothVolL   resd 1
                Chn_SmoothVolR   resd 1
endstruc



; not predefined
struc           AUDIO_PCI_DEV
                .vender_id              resw 1
                .device_id              resw 1
                .sub_vender_id          resw 1
                .sub_device_id          resw 1
                .device_bus_number      resw 1
                .irq                    resb 1
                .pin                    resb 1
                .command                resw 1
                .base0                  resd 1
                .base1                  resd 1
                .base2                  resd 1
                .base3                  resd 1
                .base4                  resd 1
                .base5                  resd 1
                .device_type            resd 1

                .mem_mode               resd 1
                .hda_mode               resd 1

; memory allocated for BDL and PCM buffers
                .bdl_buffer             resd 1
                .pcmout_buffer0         resd 1
                .pcmout_buffer1         resd 1
                .hda_buffer             resd 1
                .pcmout_bufsize         resd 1
                .pcmout_bdl_entries     resd 1
                .pcmout_bdl_size        resd 1
                .pcmout_dmasize         resd 1
                .pcmout_dma_lastgoodpos resd 1
                .pcmout_dma_pos_ptr     resd 1

; AC97 only properties
                .ac97_vra_supported     resd 1

; HDA modified structure will be placed here.
                .codec_mask             resd 1
                .codec_index            resd 1

                .afg_root_nodenum       resw 1
                .afg_num_nodes          resd 1
                .afg_nodes              resd 1
                .def_amp_out_caps       resd 1
                .def_amp_in_caps        resd 1

                .dac_node               resd 1
                .out_pin_node           resd 1
                .adc_node               resd 1
                .in_pin_node            resd 1
                .input_items            resd 1
                .pcm_num_vols           resd 1
                .pcm_vols               resd 1

                .format_val             resd 1
                .dacout_num_bits        resd 1
                .dacout_num_channels    resd 1
                .stream_tag             resd 1
                .supported_formats      resd 1
                .supported_max_freq     resd 1
                .supported_max_bits     resd 1

                .freq_card              resd 1
                .chan_card              resd 1
                .bits_card              resd 1

                .codec_id1              resw 1
                .codec_id2              resw 1
                .device_name            resb 128
                .codec_name             resb 128
endstruc

%define         DEVICE_INTEL            0    ; AC97 device Intel ICH compatible
%define         DEVICE_SIS              1    ; AC97 device SIS compatible
%define         DEVICE_INTEL_ICH4       2    ; AC97 device Intel ICH4 compatible
%define         DEVICE_NFORCE           3    ; AC97 device nForce compatible
%define         DEVICE_HDA_INTEL        4    ; HDA audio device for Intel  and others
%define         DEVICE_HDA_ATI          5
%define         DEVICE_HDA_ATIHDMI      6
%define         DEVICE_HDA_NVIDIA       7
%define         DEVICE_HDA_SIS          8
%define         DEVICE_HDA_ULI          9
%define         DEVICE_HDA_VIA          10

                ; register calling convention for WATCOM C++
                global  judas_code_lock_start_
                global  judas_code_lock_end_
                global  judas_update_
                global  judas_get_ds_
                global  sb_handler_
                global  sb_aihandler_
                global  sb16_handler_
                global  gus_handler_
                global  gus_peek_
                global  gus_poke_
                global  gus_dmawait_
                global  gus_dmainit_
                global  gus_dmaprogram_
                global  gus_startchannels_
                global  fmixer_
                global  qmixer_
                global  safemixer_
                global  normalmix_
                global  ipmix_
                global  qmix_linear_
                global  qmix_cubic_
                global  dma_program_
                ; stack calling convention for anything else
                global  _judas_code_lock_start
                global  _judas_code_lock_end
                global  _judas_update
                global  _judas_get_ds
                global  _sb_handler
                global  _sb_aihandler
                global  _sb16_handler
                global  _gus_handler
                global  _gus_peek
                global  _gus_poke
                global  _gus_dmawait
                global  _gus_dmainit
                global  _gus_dmaprogram
                global  _gus_startchannels
                global  _fmixer
                global  _qmixer
                global  _safemixer
                global  _normalmix
                global  _ipmix
                global  _qmix_linear
                global  _qmix_cubic
                global  _dma_program

                extern   _judas_ds;word
                extern   _judas_initialized;byte
                extern   _judas_mixmode;byte
                extern   _judas_samplesize;byte
                extern   _judas_clipbuffer;dword
                extern   _judas_zladdbuffer;dword
                extern   _judas_zerolevell;dword
                extern   _judas_zerolevelr;dword
                extern   _judas_cliptable;dword
                extern   _judas_volumetable;dword
                extern   _judas_mixrate;dword
                extern   _judas_channel;dword
                extern   _judas_mixroutine;dword
                extern   _judas_mixersys;dword
                extern   _judas_device;dword
                extern   _judas_port;dword
                extern   _judas_irq;dword
                extern   _judas_dma;dword
                extern   _judas_irqcount;dword
                extern   _judas_bufferlength;dword
                extern   _judas_buffermask;dword
                extern   _judas_bpmcount;dword
                extern   _judas_bpmtempo;byte
                extern   _judas_player;dword
                extern   _judas_mixpos;dword
                extern   _dma_address;dword
                extern   _judas_clipped;byte
                extern   _audio_pci;AUDIO_PCI_DEV
                extern   _hda_civ ; dword
                extern   _hda_lpib ; dword

%ifdef djgpp
section .text
%else
segment _TEXT
%endif

judas_get_ds_:
_judas_get_ds:
                mov     AX, DS
                mov     [_judas_ds], AX
                ret

judas_code_lock_start_:
_judas_code_lock_start:
; this code is constant - TASM declaration of .const

                align   4

DMAChannels:
                istruc DMACHANNEL
                    at DMA_PagePort,  dw 87h
                                        at DMA_AdrPort,   dw 0h
                                        at DMA_LenPort,   dw 1h
                                        at DMA_MaskPort,  dw 0ah
                                        at DMA_ModePort,  dw 0bh
                                        at DMA_ClearPort, dw 0ch
                                        at DMA_Mask,      db 4h
                                        at DMA_UnMask,    db 0h
                                        at DMA_Unused,    dw 0h
                iend

                istruc DMACHANNEL
                    at DMA_PagePort,  dw 83h
                                        at DMA_AdrPort,   dw 2h
                                        at DMA_LenPort,   dw 3h
                                        at DMA_MaskPort,  dw 0ah
                                        at DMA_ModePort,  dw 0bh
                                        at DMA_ClearPort, dw 0ch
                                        at DMA_Mask,      db 5h
                                        at DMA_UnMask,    db 1h
                                        at DMA_Unused,    dw 0h
                iend

                                istruc DMACHANNEL
                    at DMA_PagePort,  dw 81h
                                        at DMA_AdrPort,   dw 4h
                                        at DMA_LenPort,   dw 5h
                                        at DMA_MaskPort,  dw 0ah
                                        at DMA_ModePort,  dw 0bh
                                        at DMA_ClearPort, dw 0ch
                                        at DMA_Mask,      db 6h
                                        at DMA_UnMask,    db 2h
                                        at DMA_Unused,    dw 0h
                iend

                                istruc DMACHANNEL
                    at DMA_PagePort,  dw 82h
                                        at DMA_AdrPort,   dw 6h
                                        at DMA_LenPort,   dw 7h
                                        at DMA_MaskPort,  dw 0ah
                                        at DMA_ModePort,  dw 0bh
                                        at DMA_ClearPort, dw 0ch
                                        at DMA_Mask,      db 7h
                                        at DMA_UnMask,    db 3h
                                        at DMA_Unused,    dw 0h
                iend

                istruc DMACHANNEL
                    at DMA_PagePort,  dw 8fh
                                        at DMA_AdrPort,   dw 0c0h
                                        at DMA_LenPort,   dw 0c2h
                                        at DMA_MaskPort,  dw 0d4h
                                        at DMA_ModePort,  dw 0d6h
                                        at DMA_ClearPort, dw 0d8h
                                        at DMA_Mask,      db 4h
                                        at DMA_UnMask,    db 0h
                                        at DMA_Unused,    dw 0h
                iend

                istruc DMACHANNEL
                    at DMA_PagePort,  dw 8bh
                                        at DMA_AdrPort,   dw 0c4h
                                        at DMA_LenPort,   dw 0c6h
                                        at DMA_MaskPort,  dw 0d4h
                                        at DMA_ModePort,  dw 0d6h
                                        at DMA_ClearPort, dw 0d8h
                                        at DMA_Mask,      db 5h
                                        at DMA_UnMask,    db 1h
                                        at DMA_Unused,    dw 0h
                iend

                istruc DMACHANNEL
                    at DMA_PagePort,  dw 89h
                                        at DMA_AdrPort,   dw 0c8h
                                        at DMA_LenPort,   dw 0cah
                                        at DMA_MaskPort,  dw 0d4h
                                        at DMA_ModePort,  dw 0d6h
                                        at DMA_ClearPort, dw 0d8h
                                        at DMA_Mask,      db 6h
                                        at DMA_UnMask,    db 2h
                                        at DMA_Unused,    dw 0h
                iend

                                istruc DMACHANNEL
                    at DMA_PagePort,  dw 8ah
                                        at DMA_AdrPort,   dw 0cch
                                        at DMA_LenPort,   dw 0ceh
                                        at DMA_MaskPort,  dw 0d4h
                                        at DMA_ModePort,  dw 0d6h
                                        at DMA_ClearPort, dw 0d8h
                                        at DMA_Mask,      db 7h
                                        at DMA_UnMask,    db 3h
                                        at DMA_Unused,    dw 0h
                iend

                align   4

shittable       dd      0, 60, 56, 52, 48, 44, 40, 36
                dd      32, 28, 24, 20, 16, 12, 8, 4

%ifdef djgpp
section .data
%else
segment _DATA
%endif

                align   4

gdc:
                %rep CACHESLOTS
                istruc CACHESLOT
                                    at GDC_Pos,       dd 0
                                        at GDC_Length,    dd 0
                iend
                                %endrep

                align   4

loopcount       dd      0
fractadd        dd      0
integeradd      dd      0
smpend          dd      0
smpsubtract     dd      0
samples         dd      0
totalwork       dd      0
postproc        dd      0
cptr            dd      0
dptr            dd      0
fptr            dd      0
ipminus1        dd      0
ip0             dd      0
ip1             dd      0
ip2             dd      0
leftvol         dd      0
rightvol        dd      0
SmoothVolL      dd      0
SmoothVolR      dd      0
saved_reg       dd      0

mix_exec        db      0
gus_dmainprogress db    0
ac97_buffer0_set  db    0
ac97_buffer1_set  db    0

%ifdef djgpp
section .text
%else
segment _TEXT
%endif

                align   4

        ;DMA functions. DMA polling is really fucked up: if reading the
        ;position too often (> 100 Hz) one may get bogus values. This is
        ;compensated by reading two values, and if their offset is too big or
        ;they're outside the buffer, the position is read again.
        ;
        ;Actually GUS fucks up just in the same way when reading the channel
        ;position. Shit, what is wrong with the hardware?!
        ;
        ;Previously I though that EMM386 causes these fuckups, but no, it
        ;wasn't so. However, under DPMI there's no fuckups!
        ;
        ;It would be really nice & simple to just update one bufferhalf at a
        ;time in the soundcard interrupt, but I think it's important to give
        ;the user full control of the sound updating, even at the expense of
        ;PAIN!!!

dma_program_:
_dma_program:
                push    ESI
                push    EDI
                push    ECX
                mov     ECX, EAX                        ;ECX = mode
                mov     EDI, EDX                        ;EDI = offset
                mov     ESI, [_judas_dma]               ;Get channel num
                cmp     ESI, 4
                jae     dma16_program
                shl     ESI, 4                          ;16 = dma struc len
                add     ESI, offset DMAChannels         ;Ptr now ready
                mov     DX, [ESI + DMA_MaskPort]
                mov     AL, [ESI + DMA_Mask]
                out     DX, AL                          ;Mask the DMA channel
                xor     AL, AL
                mov     DX, [ESI + DMA_ClearPort]
                out     DX, AL                          ;Clear byte ptr.
                mov     DX, [ESI + DMA_ModePort]
                mov     AL, CL                          ;Get mode
                or      AL, [ESI + DMA_UnMask]          ;Or with channel num
                out     DX, AL                          ;Set DMA mode
                mov     DX, [ESI + DMA_LenPort]
                dec     EBX                             ;EBX = length
                mov     AL, BL
                out     DX, AL                          ;Set length low and
                mov     AL, BH                          ;high bytes
                out     DX, AL
                mov     DX, [ESI + DMA_AdrPort]
                mov     EBX, [_dma_address]             ;Get DMA buffer address
                add     EBX, EDI                        ;Add offset
                mov     AL, BL
                out     DX, AL                          ;Set offset
                mov     AL, BH
                out     DX, AL
                mov     DX, [ESI + DMA_PagePort]
                shr     EBX, 16
                mov     AL, BL
                out     DX, AL                          ;Set page
                mov     DX, [ESI + DMA_MaskPort]
                mov     AL, [ESI + DMA_UnMask]
                out     DX, AL                          ;Unmask the DMA channel
                pop     ECX
                pop     EDI
                pop     ESI
                ret
dma16_program:  shl     ESI, 4                          ;16 = dma struc len
                add     ESI, offset DMAChannels         ;Ptr now ready
                mov     DX, [ESI + DMA_MaskPort]
                mov     AL, [ESI + DMA_Mask]
                out     DX, AL                          ;Mask the DMA channel
                xor     AL, AL
                mov     DX, [ESI + DMA_ClearPort]
                out     DX, AL                          ;Clear byte ptr.
                mov     DX, [ESI + DMA_ModePort]
                mov     AL, CL                          ;Get mode
                or      AL, [ESI + DMA_UnMask]          ;Or with channel num
                out     DX, AL                          ;Set DMA mode
                mov     DX, [ESI + DMA_LenPort]
                shr     EBX, 1
                dec     EBX
                mov     AL, BL
                out     DX, AL                          ;Set length low and
                mov     AL, BH                          ;high bytes
                out     DX, AL
                mov     DX, [ESI + DMA_AdrPort]
                mov     EBX, [_dma_address]             ;Get DMA buffer address
                add     EBX, EDI                        ;Add offset
                shr     EBX, 1                          ;Because of 16-bitness
                mov     AL, BL
                out     DX, AL                          ;Set offset
                mov     AL, BH
                out     DX, AL
                mov     DX, [ESI + DMA_PagePort]
                shr     EBX, 15
                mov     AL, BL
                out     DX, AL                          ;Set page
                mov     DX, [ESI + DMA_MaskPort]
                mov     AL, [ESI + DMA_UnMask]
                out     DX, AL                          ;Unmask the DMA channel
                pop     ECX
                pop     EDI
                pop     ESI
                ret

dma_query_:     cli
                push    EBX
                push    ECX
                push    EDX
                push    ESI
                mov     ESI, [_judas_dma]
                cmp     ESI, 4
                jae     dma16_query
                shl     ESI, 4                          ;16 = dma struc len
                add     ESI, offset DMAChannels         ;Ptr now ready
                xor     EAX, EAX
                mov     DX, [ESI + DMA_ClearPort]       ;Clear flip-flop
                out     DX, AL
                mov     DX, [ESI + DMA_AdrPort]
dqloop1:        xor     EAX, EAX
                in      AL, DX
                xchg    AL, AH
                in      AL, DX
                xchg    AL, AH
                sub     AX, word [_dma_address]         ;Subtract page offset
                mov     EBX, EAX                        ;EBX = position 1
                in      AL, DX
                xchg    AL, AH
                in      AL, DX
                xchg    AL, AH
                sub     AX, word [_dma_address]         ;Subtract page offset
                mov     ECX, EAX                        ;ECX = position 2
                cmp     EBX, [_judas_bufferlength]      ;Outside buffer?
                jae     dqloop1
                mov     EAX, EBX
                sub     EAX, ECX
                cmp     EAX, 64
                jg      dqloop1
                cmp     EAX, -64
                jl      dqloop1
                mov     EAX, EBX
                pop     ESI
                pop     EDX
                pop     ECX
                pop     EBX
                sti
                ret
dma16_query:    shl     ESI, 4                          ;16 = dma struc len
                add     ESI, offset DMAChannels         ;Ptr now ready
                mov     DX, [ESI + DMA_ClearPort]       ;Clear flip-flop
                xor     EAX, EAX
                out     DX, AL
                mov     DX, [ESI + DMA_AdrPort]
                mov     ESI, [_dma_address]
                and     ESI, 1ffffh
dqloop2:        xor     EAX, EAX
                in      AL, DX
                xchg    AL, AH
                in      AL, DX
                xchg    AL, AH
                shl     EAX, 1
                sub     EAX, ESI                        ;Subtract page offset
                mov     EBX, EAX                        ;EBX = position 1
                xor     EAX, EAX
                in      AL, DX
                xchg    AL, AH
                in      AL, DX
                xchg    AL, AH
                shl     EAX, 1
                sub     EAX, ESI                        ;Subtract page offset
                mov     ECX, EAX                        ;ECX = position 2
                cmp     EBX, [_judas_bufferlength]      ;Outside buffer?
                jae     dqloop2
                mov     EAX, EBX
                sub     EAX, ECX
                cmp     EAX, 64
                jg      dqloop2
                cmp     EAX, -64
                jl      dqloop2
                mov     EAX, EBX
                pop     ESI
                pop     EDX
                pop     ECX
                pop     EBX
                sti
                ret

        ;Generic send-EOI routine.

send_eoi:       inc     dword [_judas_irqcount]
                cmp     dword [_judas_irq], 8
                jae     highirq
                mov     AL, 20h
                out     20h, AL
                ret
highirq:        mov     AL, 20h
                out     0a0h, AL
                mov     AL, 00001011b
                out     0a0h, AL
                in      AL, 0a0h
                or      AL, AL
                jnz     sb_noeoi
                mov     AL, 20h
                out     20h, AL
sb_noeoi:       ret

        ;Soundblaster IRQ handlers, one for singlecycle, one for 8bit autoinit
        ;and one for 16bit autoinit.

sb_handler_:
_sb_handler:
                pushad
                push    DS
                mov     AX, [CS:_judas_ds]
                mov     DS, AX
                mov     EDX, [_judas_port]
                add     EDX, 0eh
                in      AL, DX
                sub     EDX, 2h
sb_wait1:       in      AL, DX
                or      AL, AL
                js      sb_wait1
                mov     AL, 14h
                out     DX, AL
sb_wait2:       in      AL, DX
                or      AL, AL
                js      sb_wait2
                mov     AX, 0fff0h
                out     DX, AL
sb_wait3:       in      AL, DX
                or      AL, AL
                js      sb_wait3
                mov     AL, AH
                out     DX, AL
                sti
                call    send_eoi
                pop     DS
                popad
                iretd

sb_aihandler_:
_sb_aihandler:
                pushad
                push    DS
                mov     AX, [CS:_judas_ds]
                mov     DS, AX
                mov     EDX, [_judas_port]
                add     EDX, 0eh
                in      AL, DX
                sti
                call    send_eoi
                pop     DS
                popad
                iretd

sb16_handler_:
_sb16_handler:
                pushad
                push    DS
                mov     AX, [CS:_judas_ds]
                mov     DS, AX
                mov     EDX, [_judas_port]
                add     EDX, 0fh
                in      AL, DX
                sti
                call    send_eoi
                pop     DS
                popad
                iretd


;*****************************************************************************
;               Intel ICH AC97 stuff
;*****************************************************************************
; When CIV == LVI, set LVI <> CIV to never run out of buffers to play.
ac97_updateLVI:
                push    eax
                push    edx
                cmp     dword [_audio_pci + AUDIO_PCI_DEV.mem_mode], 0   ; memory mapped IO?
                jne     ac97_updateLVI_mem

                mov     edx, [_judas_port]
                add     edx, PO_CIV_REG                                ; PCM OUT Current Index Value
                in      ax, dx                                         ; and Last Valid Index
                and     al, 01fh                                       ; bits 0-5 only (important for SIS)
                and     ah, 01fh                                       ; bits 0-5 only (important for SIS)
                cmp     al, ah                                         ; CIV == LVI?
                jnz     ac97_updateLVI_ok                              ; no, don't change LVI
                call    ac97_setNewIndex                               ; set LVI to something else
                jmp     short ac97_updateLVI_ok
ac97_updateLVI_mem:
                mov     edx, [_audio_pci + AUDIO_PCI_DEV.base3]          ; NABMBAR for memory mapped IO
                add     edx, PO_CIV_REG                                ; PCM OUT Current Index Value
                mov     ax, [edx]                                      ; and Last Valid Index
                and     al, 01fh                                       ; bits 0-5 only (important for SIS)
                and     ah, 01fh                                       ; bits 0-5 only (important for SIS)
                cmp     al, ah                                         ; CIV == LVI?
                jnz     ac97_updateLVI_ok                              ; no, don't change LVI
                call    ac97_setNewIndex                               ; set LVI to something else
ac97_updateLVI_ok:
                pop     edx
                pop     eax
                ret

; Set the Last Valid Index to 1 less than the Current Index Value,
; so that we never run out of buffers.
ac97_setNewIndex:
                push    eax
                call    ac97_getCurrentIndex                           ; get CIV
                dec     al                                             ; make LVI != CIV
                and     al, INDEX_MASK                                 ; make sure new value is 0-31
                call    ac97_setLastValidIndex                         ; write new LVI
                pop     eax
                ret

; return AL = PCM OUT Current Index Value
ac97_getCurrentIndex:
                push    edx
                cmp     dword [_audio_pci + AUDIO_PCI_DEV.mem_mode], 0   ; memory mapped IO?
                jne     ac97_getCurrentIndex_mem

                mov     edx, [_judas_port]
                add     edx, PO_CIV_REG
                in      al, dx
                jmp     short ac97_getCurrentIndex_ok
ac97_getCurrentIndex_mem:
                mov     edx, [_audio_pci + AUDIO_PCI_DEV.base3]          ; NABMBAR for memory mapped IO
                add     edx, PO_CIV_REG                                ; PCM OUT Current Index Value
                mov     ax, [edx]
ac97_getCurrentIndex_ok:
                pop     edx
                ret

; input AL = PCM OUT Last Valid Index (index to stop on)
ac97_setLastValidIndex:
                push    edx
                cmp     dword [_audio_pci + AUDIO_PCI_DEV.mem_mode], 0   ; memory mapped IO?
                jne     ac97_setLastValidIndex_mem

                mov     edx, [_judas_port]
                add     edx, PO_LVI_REG
                out     dx, al
                jmp     short ac97_setLastValidIndex_ok
ac97_setLastValidIndex_mem:
                mov     edx, [_audio_pci + AUDIO_PCI_DEV.base3]          ; NABMBAR for memory mapped IO
                add     edx, PO_LVI_REG
                mov     [edx], al                                      ; and Last Valid Index
ac97_setLastValidIndex_ok:
                pop     edx
                ret


;*****************************************************************************
;               Intel HDA stuff
;*****************************************************************************
hda_get_lpib:

                push    edx

                mov     edx, dword [_audio_pci + AUDIO_PCI_DEV.base0]
                add     edx, HDA_SDO0LPIB
                mov     eax, [edx]

                pop     edx
                ret

hda_dma_start:
                push    edx

                mov     edx, dword [_audio_pci + AUDIO_PCI_DEV.base0]
                add     edx, HDA_SDO0CTL
                mov     eax, [edx]
                or      eax, SD_CTL_DMA_START
                mov     [edx], eax

                pop     edx
                ret


        ;General DMAbuffer update routine (call either from main program or
        ;within your timer interrupt)

judas_update_:
_judas_update:
                cmp     dword [_judas_device], DEV_NOSOUND
                je      near judas_gotohell
                cmp     dword [_judas_device], DEV_FILE
                je      near judas_gotohell
                cmp     byte [mix_exec], 0
                jne     near judas_gotohell
                cmp     byte [_judas_initialized], 0
                je      near judas_gotohell
                inc     byte [mix_exec]
                pushad
                cmp     dword [_judas_device], DEV_AC97
                je      near updateac97                 ; audio_pci update for AC97
                cmp     dword [_judas_device], DEV_HDA
                je      near updatehda                  ; audio_pci update for HDA
                call    dma_query_
                                                        ;Must be aligned on 8
                and     EAX, [_judas_buffermask]        ;samples (unrolling!)
                mov     EBX, [_judas_mixpos]            ;This is the old pos
                cmp     EAX, EBX
                je      judas_donothing
                jb      judas_wrap
judas_normal:   mov     [_judas_mixpos], EAX
                mov     EDX, EAX
                sub     EDX, EBX                        ;EDX = length to mix
                mov     EAX, EBX                        ;EAX = pos. to mix
                add     EAX, [_dma_address]
                call    dword [_judas_mixersys]
judas_donothing:popad
                dec     byte [mix_exec]
judas_gotohell: ret
judas_wrap:     mov     [_judas_mixpos], EAX
                mov     EAX, EBX                        ;Mix to buffer end
                mov     EDX, [_judas_bufferlength]
                sub     EDX, EBX
                add     EAX, [_dma_address]
                call    dword [_judas_mixersys]
                mov     EAX, [_dma_address]             ;Then to start
                mov     EDX, [_judas_mixpos]
                or      EDX, EDX
                jz      judas_donothing
                call    dword [_judas_mixersys]
                jmp     judas_donothing


updateac97:     call    ac97_updateLVI                 ; set CIV != LVI
                call    ac97_getCurrentIndex
update_hda_buffers:
                test    al, 1                          ; check parity
                jz      ac97_playing_buffer0

                ; playing buffer 1 -> refresh buffer 0 (Bus Master DMA)
ac97_playing_buffer1:
                cmp     byte [ac97_buffer0_set], 1     ; is buffer 0
                je      judas_donothing                ; already refreshed
                mov     eax, [_audio_pci + AUDIO_PCI_DEV.pcmout_buffer0]           ; buffer 0 address
                mov     edx, [_judas_bufferlength]     ; buffer 0 size
                call    dword [_judas_mixersys]
                mov     byte [ac97_buffer0_set], 1     ; set buffer 0
                mov     byte [ac97_buffer1_set], 0     ; as refreshed
                jmp     judas_donothing

                ; playing buffer 0 -> refresh buffer 1 (Bus Master DMA)
ac97_playing_buffer0:
                cmp     byte [ac97_buffer1_set], 1     ; is buffer 1
                je      near judas_donothing           ; already refreshed
                mov     eax, [_audio_pci + AUDIO_PCI_DEV.pcmout_buffer1]           ; buffer 1 address
                mov     edx, [_judas_bufferlength]     ; buffer 1 size
                call    dword [_judas_mixersys]
                mov     byte [ac97_buffer1_set], 1     ; set buffer 1
                mov     byte [ac97_buffer0_set], 0     ; as refreshed
                jmp     judas_donothing


updatehda:      mov     eax, [_hda_lpib]
                or      eax, eax
                jnz     hda_update_civ
                mov     eax, [_hda_civ]
                or      eax, eax
                jnz     hda_update_civ
                call    hda_dma_start                  ; 1st time run, start the DMA engine

hda_update_civ:
                call    hda_get_lpib                   ; get LPIB
                cmp     eax, dword [_hda_lpib]         ; compare wih last LPIB position
                jae     hda_skip_civ                   ; if no wrap around don't update CIV

                inc     dword [_hda_civ]
                cmp     dword [_hda_civ], 32
                jne     hda_skip_civ
                mov     dword [_hda_civ], 0

hda_skip_civ:
                mov     [_hda_lpib], eax
                mov     eax, [_hda_civ]
                jmp     update_hda_buffers             ; same as AC97 on next step
