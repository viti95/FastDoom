; Ugly shit from GUS SDK

 %define GF1_MIDI_CTRL          100h  ; 3X0
 %define GF1_MIDI_DATA          101h  ; 3X1
 %define GF1_PAGE               102h  ; 3X2
 %define GF1_REG_SELECT         103h  ; 3X3
 %define GF1_DATA_LOW           104h  ; 3X4
 %define GF1_DATA_HI            105h  ; 3X5
 %define GF1_IRQ_STAT           006h  ; 2X6
 %define GF1_DRAM               107h  ; 3X7
 %define GF1_MIX_CTRL           000h  ; 2X0
 %define GF1_TIMER_CTRL         008h  ; 2X8
 %define GF1_TIMER_DATA         009h  ; 2X9
 %define GF1_IRQ_CTRL           00Bh  ; 2XB

; The GF1 Hardware clock.
 %define CLOCK_RATE             9878400

; Mixer control bits.
 %define ENABLE_LINE            01h
 %define ENABLE_DAC             02h
 %define ENABLE_MIC             04h

 %define DMA_CONTROL            41h
 %define SET_DMA_ADDRESS        42h
 %define SET_DRAM_LOW           43h
 %define SET_DRAM_HIGH          44h
 %define TIMER_CONTROL          45h
 %define TIMER1                 46h
 %define TIMER2                 47h
 %define SET_SAMPLE_RATE        48h
 %define SAMPLE_CONTROL         49h
 %define SET_JOYSTICK           4Bh
 %define MASTER_RESET           4Ch

; Voice register mapping.
 %define SET_CONTROL            00h
 %define SET_FREQUENCY          01h
 %define SET_START_HIGH         02h
 %define SET_START_LOW          03h
 %define SET_END_HIGH           04h
 %define SET_END_LOW            05h
 %define SET_VOLUME_RATE        06h
 %define SET_VOLUME_START       07h
 %define SET_VOLUME_END         08h
 %define SET_VOLUME             09h
 %define SET_ACC_HIGH           0ah
 %define SET_ACC_LOW            0bh
 %define SET_BALANCE            0ch
 %define SET_VOLUME_CONTROL     0dh
 %define SET_VOICES             0eh

 %define GET_CONTROL            80h
 %define GET_FREQUENCY          81h
 %define GET_START_HIGH         82h
 %define GET_START_LOW          83h
 %define GET_END_HIGH           84h
 %define GET_END_LOW            85h
 %define GET_VOLUME_RATE        86h
 %define GET_VOLUME_START       87h
 %define GET_VOLUME_END         88h
 %define GET_VOLUME             89h
 %define GET_ACC_HIGH           8ah
 %define GET_ACC_LOW            8bh
 %define GET_BALANCE            8ch
 %define GET_VOLUME_CONTROL     8dh
 %define GET_VOICES             8eh
 %define GET_IRQV               8fh

 %define MIDI_RESET	            3h

;*******************************************************************
;*
;* GF1 irq/dma programmable latches
;*
;******************************************************************

; GF1_IRQ_STATUS (port 3X6)
 %define MIDI_TX_IRQ            01h  ; pending MIDI xmit IRQ
 %define MIDI_RX_IRQ            02h  ; pending MIDI recv IRQ
 %define GF1_TIMER1_IRQ         04h  ; general purpose timer
 %define GF1_TIMER2_IRQ         08h  ; general purpose timer
 %define WAVETABLE_IRQ          20h  ; pending wavetable IRQ
 %define ENVELOPE_IRQ           40h  ; pending volume envelope IRQ
 %define DMA_TC_IRQ             80h  ; pending dma tc IRQ


; GF1_MIX_CTRL (port 2X0)
 %define ENABLE_LINE_IN         01h  ; 0=enable
 %define ENABLE_OUTPUT          02h  ; 0=enable
 %define ENABLE_MIC_IN          04h  ; 1=enable
 %define ENABLE_GF1_IRQ         08h  ; 1=enable
 %define GF122                  10  ; ??
 %define ENABLE_MIDI_LOOP       20h  ; 1=enable loop back
 %define SELECT_GF1_REG         40h  ; 1=irq latches
          ; 0=dma latches

;*******************************************************************
;*
;* GF1 global registers ($41-$4C)
;*
;*******************************************************************

; DMA control register
 %define DMA_ENABLE             01h
 %define DMA_READ               02h  ; 1=read,0=write
 %define DMA_WIDTH_16           04h  ; 1=16 bit,0=8 bit (dma chan width)
 %define DMA_RATE               18h  ; 00=fast, 11=slow
 %define DMA_IRQ_ENABLE         20h  ; 1=enable
 %define DMA_IRQ_PENDING        40h  ; read
 %define DMA_DATA_16            40h  ; write (data width)
 %define DMA_TWOS_COMP          80h  ; 1=do twos comp

; These are the xfer rate bits ...
 %define DMA_R0  00h  ; Fastest DMA xfer (~650khz)
 %define DMA_R1  08h  ; fastest / 2
 %define DMA_R2  10h  ; fastest / 4
 %define DMA_R3  18h  ; Slowest DMA xfer (fastest / 8)

; SAMPLE control register
 %define ENABLE_ADC             01h
 %define ADC_MODE               02h  ; 0=mono, 1=stereo
 %define ADC_DMA_WIDTH          04h  ; 0=8 bit, 1=16 bit
 %define ADC_IRQ_ENABLE         20h  ; 1=enable
 %define ADC_IRQ_PENDING        40h  ; 1=irq pending
 %define ADC_TWOS_COMP          80h  ; 1=do twos comp

; RESET control register
 %define GF1_MASTER_RESET       01h  ; 0=hold in reset
 %define GF1_OUTPUT_ENABLE      02h  ; enable output
 %define GF1_MASTER_IRQ         04h  ; master IRQ enable

;*******************************************************************
;*
;* GF1 voice specific registers ($00 - $0E and $80-$8f)
;*
;******************************************************************

; ($0,$80) Voice control register
 %define VOICE_STOPPED          01h  ; voice has stopped
 %define STOP_VOICE             02h  ; stop voice
 %define VC_DATA_TYPE           04h  ; 0=8 bit,1=16 bit
 %define VC_LOOP_ENABLE         08h  ; 1=enable
 %define VC_BI_LOOP             10h  ; 1=bi directional looping
 %define VC_WAVE_IRQ            20h  ; 1=enable voice's wave irq
 %define VC_DIRECT              40h  ; 0=increasing,1=decreasing
 %define VC_IRQ_PENDING         80h  ; 1=wavetable irq pending

; ($1,$81) Frequency control
; Bit 0  - Unused
; Bits 1-9 - Fractional portion
; Bits 10-15 - Integer portion

; ($2,$82) Accumulator start address (high)
; Bits 0-11 - HIGH 12 bits of address
; Bits 12-15 - Unused

; ($3,$83) Accumulator start address (low)
; Bits 0-4 - Unused
; Bits 5-8 - Fractional portion
; Bits 9-15 - Low 7 bits of integer portion

; ($4,$84) Accumulator end address (high)
; Bits 0-11 - HIGH 12 bits of address
; Bits 12-15 - Unused

; ($5,$85) Accumulator end address (low)
; Bits 0-4 - Unused
; Bits 5-8 - Fractional portion
; Bits 9-15 - Low 7 bits of integer portion


; ($6,$86) Volume Envelope control register
 %define VL_RATE_MANTISSA       3fh
 %define VL_RATE_RANGE          0C0h

; ($7,$87) Volume envelope start
 %define VL_START_MANT          0Fh
 %define VL_START_EXP           0F0h

; ($8,$88) Volume envelope end
 %define VL_END_MANT            0Fh
 %define VL_END_EXP             0F0h

; ($9,$89) Current volume register
; Bits 0-3 are unused
; Bits 4-11 - Mantissa of current volume
; Bits 10-15 - Exponent of current volume

; ($A,$8A) Accumulator value (high)
; Bits 0-12 - HIGH 13 bits of current position (a19-a7)

; ($B,$8B) Accumulator value (low)
; Bits 0-8 - Fractional portion
; Bits 9-15 - Integer portion of low adress (a6-a0)

; ($C,$8C) Pan (balance) position
; Bits 0-3 - Balance position  0=full left, = 0f=full right

; ($D,$8D) Volume control register
 %define VOLUME_STOPPED         01h  ; volume has stopped
 %define STOP_VOLUME            02h  ; stop volume
 %define VC_ROLLOVER            04h  ; Roll PAST end & gen IRQ
 %define VL_LOOP_ENABLE         08h  ; 1=enable
 %define VL_BI_LOOP             10h  ; 1=bi directional looping
 %define VL_WAVE_IRQ            20h  ; 1=enable voice's wave irq
 %define VL_DIRECT              40h  ; 0=increasing,1=decreasing
 %define VL_IRQ_PENDING         80h  ; 1=wavetable irq pending

; ($E,$8E) # of Active voices
; Bits 0-5 - # of active voices -1

; ($F,$8F) - Sources of IRQs
; Bits 0-4 - interrupting voice number
; Bit 5 - Always a 1
 %define VOICE_VOLUME_IRQ       40h  ; individual voice irq bit
 %define VOICE_WAVE_IRQ         80h  ; individual waveform irq bit

