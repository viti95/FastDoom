
;*****************************************************************************
;* some asm constants to help us :->
;*****************************************************************************

%define TRUE  1
%define FALSE 0

%define BIT0  1
%define BIT1  2
%define BIT2  4
%define BIT3  8
%define BIT4  10h
%define BIT5  20h
%define BIT6  40h
%define BIT7  80h
%define BIT8  100h
%define BIT9  200h
%define BIT10 400h
%define BIT11 800h
%define BIT12 1000h
%define BIT13 2000h
%define BIT14 4000h
%define BIT15 8000h
%define BIT16 10000h
%define BIT17 20000h
%define BIT18 40000h
%define BIT19 80000h
%define BIT20 100000h
%define BIT21 200000h
%define BIT22 400000h
%define BIT23 800000h
%define BIT24 1000000h
%define BIT25 2000000h
%define BIT26 4000000h
%define BIT27 8000000h
%define BIT28 10000000h
%define BIT29 20000000h
%define BIT30 40000000h
%define BIT31 80000000h




;*****************************************************************************
;* Intel ICH controller equates (based on Jeff Leyda AC97 wav player SDK :-)
;*****************************************************************************

; supported controllers
%define INTEL_VID               8086h           ; Intel's PCI vendor ID
%define SIS_VID                 1039h           ; SIS PCI vendor ID
%define NVIDIA_VID              10deh           ; NVIDIA vendor ID
%define AMD_VID                 1022h           ; AMD vendor ID

%define ICH_DID                 2415h           ; ICH device ID
%define ICH0_DID                2425h           ; ICH0
%define ICH2_DID                2445h           ; ICH2
%define ICH3_DID                2485h           ; ICH3
%define ICH4_DID                24c5h           ; ICH4
%define ICH5R_DID               24d5h           ; ICH5R
%define ESB_DID                 25a6h           ; ESB (embedded - ICH4 comp)
%define ICH6_DID                266eh           ; ICH6
%define ICH6W_DID               2668h           ; ICH6W
%define ESB2_DID                2698h           ; ESB2 (embedded - ICH4 comp)
%define ICH7_DID                27deh           ; ICH7
%define ICH7W_DID               27d8h           ; ICH7W
%define ICHHD_DID               284bh           ; ICHHD
%define ICH9_DID                293eh           ; ICH9
%define ICH10_DID               3a6eh           ; ICH10
%define ICH10R_DID              3a3eh           ; ICH10R

%define MX440_DID               7195h           ; 440MX
%define SI7012_DID              7012h           ; SI7012

%define NFORCE_DID              01b1h        ; NFORCE
%define NFORCE_MCP04_DID        003ah        ; NFORCE MCP04
%define NFORCE2_DID             006ah        ; NFORCE2
%define NFORCE2b_DID            006bh        ; NFORCE2b (?)
%define NFORCE_CK8_DID          008ah        ; NFORCE CK8
%define NFORCE3_DID             00dah        ; NFORCE3
%define NFORCE_CK8S_DID         00eah        ; NFORCE CK8S
%define NFORCE4_DID             0059h        ; NFORCE4
%define NFORCE410_MCP_DID       026bh        ; NFORCE MCP 410

%define AMD8111_DID             764dh        ; AMD8111
%define AMD768_DID              7445h        ; AMD768


; PCI config space defines

%define NAMBAR_REG              10h          ; Native Audio Mixer BAR
 %define NAM_SIZE               256          ; 256 bytes required for registers

%define NABMBAR_REG             14h          ; Native Audio Bus Mastering BAR
 %define NABM_SIZE              64           ; 64 bytes required for registers


; BUS master registers, accessed via NABMBAR+offset
; ICH supports 3 different types of register sets for three types of things :
;
; PCM in (for recording) aka PI
; PCM out (for playback) aka PO
; MIC in (for recording) aka MC

%define PI_BDBAR_REG            0            ; PCM in buffer descriptor BAR
%define PO_BDBAR_REG            10h          ; PCM out buffer descriptor BAR
%define MC_BDBAR_REG            20h          ; MIC in buffer descriptor BAR


; There are 32 entries in the BDL (256 bytes in total)
; Each entry is 8 bytes long. It holds a pointer to a data buffer,
; and the size (in bytes or samples) with execution flags for that buffer.

%define BDL_SIZE                32*8         ; Buffer Descriptor List size
%define INDEX_MASK              31           ; indexes must be 0-31


; 8bit read only
; The Current Index Value is a number of the buffer (0-31) which the codec
; is currently processing.  Once this counter hits 31, it wraps back to 0.

%define PI_CIV_REG              4            ; PCM in Current Index Value (RO)
%define PO_CIV_REG              14h          ; PCM out Current Index Value (RO)
%define MC_CIV_REG              24h          ; MIC in Current Index Value (RO)


; 8bit read/write
; The Last Valid Index is a number of the buffer (0-31) on which the codec
; will stop processing data.

%define PI_LVI_REG              5            ; PCM in Last Valid Index
%define PO_LVI_REG              15h          ; PCM out Last Valid Index
%define MC_LVI_REG              25h          ; MIC in Last Valid Index


; 16bit read/write
; status registers.

%define PI_SR_REG               6            ; PCM in Status Register
%define PO_SR_REG               16h          ; PCM out Status Register
%define MC_SR_REG               26h          ; MIC in Status Register


; status register bitfields

%define FIFO_ERR                BIT4    ; FIFO error (Over/Underrun)  W1TC.
%define BCIS                    BIT3    ; Buffer Completion Interrupt Status.
                                        ; Set whenever the last sample in ANY
                                        ; buffer is finished.  Bit is only
                                        ; set when the Interrupt on Complete
                                        ; (BIT4 of control register) is set.
%define LVBCI                   BIT2    ; Last Valid Buffer Completion Interrupt
                                        ; Set whenever the codec has processed
                                        ; the last buffer in the buffer list.
                                        ; Will fire an interrupt if IOC bit is
                                        ; set. Probably set after the last
                                        ; sample in the last buffer is
                                        ; processed.  W1TC
%define CELV                    BIT1    ; Current Equals Last Valid
                                        ; Bit is RO and remains set until LVI is
                                        ; cleared. Probably set up the start
                                        ; of processing for the last buffer.
%define DCH                     BIT0    ; DMA controller halted.
                                        ; Set whenever audio stream is stopped
                                        ; or something else goes wrong.


; 16bit read only
; Position In Current Buffer shows the number of dwords left to be
; processed in the current buffer.

%define PI_PICB_REG             8       ; PCM in position in current buffer(RO)
%define PO_PICB_REG             18h     ; PCM out position in current buffer(RO)
%define MC_PICB_REG             28h     ; MIC in position in current buffer (RO)


; 8bit, read only
; Prefetched Index Value Register.
; Tells which buffer number (0-31) has been prefetched.
; This value probably follows the current index value fairly closely. (CIV+1)

%define PI_PIV_REG              0ah     ; PCM in Prefected index value
%define PO_PIV_REG              1ah     ; PCM out Prefected index value
%define MC_PIV_REG              2ah     ; MIC in Prefected index value


; 8bit
; Control register can only be accessed as an 8bit value.
; Control register.

%define PI_CR_REG               0bh     ; PCM in Control Register
%define PO_CR_REG               1bh     ; PCM out Control Register
%define MC_CR_REG               2bh     ; MIC in Control Register


; control register bitfields.

%define IOCE                    BIT4    ; Interrupt On Complete Enable
                                        ; Set this bit if you want an interrupt
                                        ; to fire whenever LVBCI is set.
%define FEIFE                   BIT3    ; Fifo Error Interrupt Enable
                                        ; Set if you want an interrupt to fire
                                        ; whenever there is a FIFO (over or
                                        ; under) error.
%define LVBIE                   BIT2    ; Last Valid Buffer Interrupt Enable
                                        ; Set if you want an interrupt to fire
                                        ; whenever the completion of the last
                                        ; valid buffer.
%define RR                      BIT1    ; Reset busmaster Registers
                                        ; Nukes all regs except bits 4:2
                                        ; of this register.
                                        ; Only set this bit if BIT 0 is 0
%define RPBM                    BIT0    ; Start Busmaster operation (Run/Pause)
                                        ; set this bit to start the codec!


; Global Control Register with bitfields

%define GLOB_CNT_REG            2ch     ; Global control register
%define SMP_24_BIT              BIT23   ; 24 bit samples
%define SMP_20_BIT              BIT22   ; 20 bit samples
%define CHN_PCM_246_MASK        300000h ; 6 channels (not all chips)
%define CHN_PCM_6               BIT21   ; 6 channels (not all chips)
%define CHN_PCM_4               BIT20   ; 4 channels (not all chips)
%define CHN_PCM_2               0       ; 2 channels (stereo)
%define SEC_RES_EN              BIT5    ; Secondary Codec Resume event
                                        ; interrupt enable.
%define PRI_RES_EN              BIT4    ; Primary Codec Resume event
                                        ; interrupt enable.
%define ACLINK_OFF              BIT3    ; Turn off the AC97 link
%define ACWARM_RESET            BIT2    ; Awaken the AC97 link from sleep.
                                        ; registers preserved, bit self clears
%define ACCOLD_RESET            BIT1    ; Reset everything in the AC97 and
                                        ; reset all registers. Not self clearing
%define GPIIE                   BIT0    ; GPI Interrupt enable.
                                        ; set if you want an interrupt to
                                        ; fire upon ANY of the bits in the
                                        ; GPI changed?


; Global Status Register with bitfields

%define GLOB_STS_REG            30h     ; Global Status register (RO)
%define AUDIO_24_BIT            BIT23   ; 24 bit sample support
%define AUDIO_20_BIT            BIT22   ; 20 bit sample support
%define CHN_CAP_6               BIT21   ; 6 channels capability
%define CHN_CAP_4               BIT20   ; 4 channels capability
%define MD3                     BIT17   ; Modem powerdown status (yawn)
%define AD3                     BIT16   ; Audio powerdown status (yawn)
%define RD_COMPLETE_STS         BIT15   ; Codec read timed out. 0=normal
%define BIT3SLOT12              BIT14   ; shadowed status of bit 3 in slot 12
%define BIT2SLOT12              BIT13   ; shadowed status of bit 2 in slot 12
%define BIT1SLOT12              BIT12   ; shadowed status of bit 1 in slot 12
%define SEC_RESUME_STS          BIT11   ; secondary codec has resumed (and irqed)
%define PRI_RESUME_STS          BIT10   ; primary codec has resumed (and irqed)
%define SEC_CODEC_RDY           BIT9    ; Secondary codec is ready for action
%define PRI_CODEC_RDY           BIT8    ; Primary codec is ready for action
                                        ; software must check these bits before
                                        ; starting the codec!
%define MIC_IN_IRQ              BIT7    ; MIC in caused an interrupt
%define PCM_OUT_IRQ             BIT6    ; One of the PCM out channels IRQed
%define PCM_IN_IRQ              BIT5    ; One of the PCM in channels IRQed
%define MODEM_OUT_IRQ           BIT2    ; Modem out channel IRQed
%define MODEM_IN_IRQ            BIT1    ; Modem in channel IRQed
%define GPI_STS_CHANGE          BIT0    ; Set whenever GPI's have changed.
                                        ; BIT0 of slot 12 also reflects this.


; Codec Write Semaphore Register

%define ACC_SEMA_REG            34h     ; Codec write semaphore register
%define CODEC_BUSY              BIT0    ; codec register I/O is happening
                                        ; self clearing



; Buffer Descriptors List
; Each buffer descriptor list is a set of (up to) 32 descriptors.
; Each descriptor is 8 bytes long. Bytes 0-3 of a descriptor entry point
; to a chunk of memory to either play from or record to. Bytes 4-7 of an
; entry describe various control things detailed below.
; Buffer pointers must always be aligned on a dword boundary.

%define IOC                     BIT31   ; Fire an interrupt whenever this
                                        ; buffer is complete.
%define BUP                     BIT30   ; Buffer Underrun Policy.
                                        ; If this buffer is the last buffer
                                        ; in a playback, fill the remaining
                                        ; samples with 0 (silence) or not.

; Bits 15:0 contain the length of the buffer, in number of samples,
; (or in number of bytes for the SIS controller) which are 16 bits each,
; coupled in left and right pairs, or 32bits each.
; A value of 0 in these bits means play no samples.




;*****************************************************************************
;* AC97 Codec registers include (based on Jeff Leyda AC97 wav player SDK :-)
;*****************************************************************************

; Not all codecs are created equal. Refer to the spec for your specific codec.
; All registers are 16bits wide.  Access to codec registers over the AC97 link
; is defined by the OEM.
; Secondary codec's are accessed by ORing in BIT7 of all register accesses.


; each codec/mixer register is 16bits

%define CODEC_RESET_REG                 00      ; reset codec
%define CODEC_MASTER_VOL_REG            02      ; master volume
%define CODEC_HP_VOL_REG                04      ; headphone volume
%define CODEC_MASTER_MONO_VOL_REG       06      ; master mono volume
%define CODEC_MASTER_TONE_REG           08      ; master tone (R+L)
%define CODEC_PCBEEP_VOL_REG            0ah     ; PC beep volume
%define CODEC_PHONE_VOL_REG             0ch     ; phone volume
%define CODEC_MIC_VOL_REG               0eh     ; MIC volume
%define CODEC_LINE_IN_VOL_REG           10h     ; line input volume
%define CODEC_CD_VOL_REG                12h     ; CD volume
%define CODEC_VID_VOL_REG               14h     ; video volume
%define CODEC_AUX_VOL_REG               16h     ; aux volume
%define CODEC_PCM_OUT_REG               18h     ; PCM output volume
%define CODEC_RECORD_SELECT_REG         1ah     ; record select input
%define CODEC_RECORD_VOL_REG            1ch     ; record volume
%define CODEC_RECORD_MIC_VOL_REG        1eh     ; record mic volume
%define CODEC_GP_REG                    20h     ; general purpose
%define CODEC_3D_CONTROL_REG            22h     ; 3D control
; 24h is reserved
%define CODEC_POWER_CTRL_REG            26h     ; powerdown control
%define CODEC_EXT_AUDIO_REG             28h     ; extended audio
%define CODEC_EXT_AUDIO_CTRL_REG        2ah     ; extended audio control
%define CODEC_PCM_FRONT_DACRATE_REG     2ch     ; PCM out sample rate
%define CODEC_PCM_SURND_DACRATE_REG     2eh     ; surround sound sample rate
%define CODEC_PCM_LFE_DACRATE_REG       30h     ; LFE sample rate
%define CODEC_LR_ADCRATE_REG            32h     ; PCM in sample rate
%define CODEC_MIC_ADCRATE_REG           34h     ; mic in sample rate


; registers 36-7a are reserved on the ICH

%define CODEC_VENDORID1_REG             7ch     ; codec vendor ID 1
%define CODEC_VENDORID2_REG             7eh     ; codec vendor ID 2


; When 2 codecs are present in the system, use BIT7 to access the 2nd
; set of registers, ie 80h-feh

%define PRIMARY_CODEC                   0       ; 0-7F for primary codec
%define SECONDARY_CODEC                 BIT7    ; 80-8f registers for 2ndary


; HDA defines (simplified info)

%define HDA_GCAP                        0x00    ; Global Capabilities
%define HDA_VMIN                        0x02    ; Minor Version
%define HDA_VMAJ                        0x03    ; Major Version
%define HDA_OUTPAY                      0x04    ; Output Payload Capability
%define HDA_INPAY                       0x06    ; Input Payload Capability
%define HDA_GCTL                        0x08    ; Global Control
%define CRST                            BIT0    ; Controller reset
%define UREN                            BIT8    ; Unsolicited responses
%define HDA_WAKEEN                      0x0C    ; Wake Enable
%define HDA_STATESTS                    0x0E    ; Wake Status
%define HDA_GSTST                       0x10    ; Global Status
%define HDA_ECAP                        0x14    ; Extended capabilities - mobile only
%define HDA_OUTSTRMPAY                  0X18    ; Output stream payload capability
%define HDA_INSTRMPAY                   0x1A    ; Input stream payload capability

%define HDA_INTCTL                      0x20    ; Interrupt Control
%define HDA_INT_ALL_STREAM              0xff    ; all stream interrupts mask on IOC
%define HDA_INT_CTRL_EN                 BIT30   ; controller interrupt enable bit
%define HDA_INT_GLOBAL_EN               BIT31   ; global interrupt enable bit

%define HDA_INTSTS                      0x24    ; Interrupt Status
%define REG_WALCLK                      0x30    ; Wall Clock Counter
%define HDA_SSYNC                       0x34    ; Stream Synchronization

%define HDA_CORBLBASE                   0x40    ; CORB Lower Base Address
%define HDA_CORBUBASE                   0x44    ; CORB Upper Base Address
%define HDA_CORBWP                      0x48    ; CORB Write Pointer
%define HDA_CORBRP                      0x4A    ; CORB Read Pointer
%define HDA_CORBCTL                     0x4C    ; CORB Control
%define HDA_CORBSTS                     0x4D    ; CORB Status
%define HDA_CORBSIZE                    0x4E    ; CORB Size

%define HDA_RIRBLBASE                   0x50    ; RIRB Lower Base Address
%define HDA_RIRBUBASE                   0x54    ; RIRB Upper Base Address
%define HDA_RIRBWP                      0x58    ; RIRB Write Pointer
%define HDA_RINTCNT                     0x5A    ; Response Interrupt Count
%define HDA_RIRBCTL                     0x5C    ; RIRB Control
%define HDA_RIRBSTS                     0x5D    ; RIRB Status
%define HDA_RIRBSIZE                    0x5E    ; RIRB Size

%define HDA_IC                          0x60    ; Immediate Command
%define HDA_IR                          0x64    ; Immediate Response
%define HDA_IRS                         0x68    ; Immediate Command Status
%define IRS_BUSY                        BIT0    ; immediate command busy
%define IRS_VALID                       BIT1    ; immediate command valid
%define HDA_DPLBASE                     0x70    ; DMA Position Lower Base Address
%define HDA_DPUBASE                     0x74    ; DMA Position Upper Base Address
%define HDA_DPLBASE_ENABLE              0x1     ; Enable position buffer

%define HDA_SD_CTL                      0x0     ; stream register offsets from stream base
%define HDA_SD_STS                      0x3
%define HDA_SD_LPIB                     0x4
%define HDA_SD_CBL                      0x8
%define HDA_SD_LVI                      0xC
%define HDA_SD_FIFOW                    0xE
%define HDA_SD_FIFOSIZE                 0x10
%define HDA_SD_FORMAT                   0x12
%define HDA_SD_BDLPL                    0x18
%define HDA_SD_BDLPU                    0x1C
%define HDA_SD_LPIBA                    0x2004

; SDCTL—Stream Descriptor Control Register bits
%define SD_CTL_STREAM_RESET	            BIT0    ; stream reset bit
%define SD_CTL_DMA_START                BIT1    ; stream DMA start bit
%define SD_CTL_STREAM_TAG_MASK          (BIT20 + BIT21 + BIT22 + BIT23) ; set bits 20 - 23 of SD_CTL register
%define SD_CTL_STREAM_TAG_SHIFT         20

; SDSTS—Stream Descriptor Status Register bits
%define SD_INT_COMPLETE                 BIT2    ; completion interrupt
%define SD_INT_FIFO_ERR                 BIT3    ; FIFO error interrupt
%define SD_INT_DESC_ERR                 BIT4    ; descriptor error interrupt
%define RIRB_INT_MASK                   (BIT0 + BIT2)
%define SD_INT_MASK                     (SD_INT_DESC_ERR + SD_INT_FIFO_ERR + SD_INT_COMPLETE)
%define STATESTS_INT_MASK               (BIT0 + BIT1 + BIT2)

%define HDA_SDI0CTL                     0x80    ; Stream Descriptor Control
%define HDA_SDI0STS                     0x83    ; Stream Descriptor Status
%define HDA_SDI0LPIB                    0x84    ; Link Position in Current Buffer
%define HDA_SDI0CBL                     0x88    ; Cyclic Buffer Length
%define HDA_SDI0LVI                     0x8C    ; Last Valid Index
%define HDA_SDI0FIFOW                   0x8E    ; FIFO watermark
%define HDA_SDI0FIFOSIZE                0x90    ; FIFO Size
%define HDA_SDI0FORMAT                  0x92    ; Format
%define HDA_SDI0BDLPL                   0x98    ; List Pointer - Lower
%define HDA_SDI0BDLPU                   0x9C    ; List Pointer - Upper
%define HDA_SDI0LPIBA                   0x2084  ; Link Posiiton in Buffer n Alias

%define HDA_SDI1CTL                     0xA0    ; Stream Descriptor Control
%define HDA_SDI1STS                     0xA3    ; Stream Descriptor Status
%define HDA_SDI1LPIB                    0xA4    ; Link Position in Current Buffer
%define HDA_SDI1CBL                     0xA8    ; Cyclic Buffer Length
%define HDA_SDI1LVI                     0xAC    ; Last Valid Index
%define HDA_SDI1FIFOW                   0xAE    ; FIFO watermark
%define HDA_SDI1FIFOSIZE                0xB0    ; FIFO Size
%define HDA_SDI1FORMAT                  0xB2    ; Format
%define HDA_SDI1BDLPL                   0xB8    ; List Pointer - Lower
%define HDA_SDI1BDLPU                   0xBC    ; List Pointer - Upper
%define HDA_SDI1LPIBA                   0x20A4  ; Link Posiiton in Buffer n Alias

%define HDA_SDI2CTL                     0xC0    ; Stream Descriptor Control
%define HDA_SDI2STS                     0xC3    ; Stream Descriptor Status
%define HDA_SDI2LPIB                    0xC4    ; Link Position in Current Buffer
%define HDA_SDI2CBL                     0xC8    ; Cyclic Buffer Length
%define HDA_SDI2LVI                     0xCC    ; Last Valid Index
%define HDA_SDI2FIFOW                   0xCE    ; FIFO watermark
%define HDA_SDI2FIFOSIZ                 0xD0    ; FIFO Size
%define HDA_SDI2FORMAT                  0xD2    ; Format
%define HDA_SDI2BDLPL                   0xD8    ; List Pointer - Lower
%define HDA_SDI2BDLPU                   0xDC    ; List Pointer - Upper
%define HDA_SDI2LPIBA                   0x20D4  ; Link Posiiton in Buffer n Alias

%define HDA_SDI3CTL                     0xE0    ; Stream Descriptor Control
%define HDA_SDI3STS                     0xE3    ; Stream Descriptor Status
%define HDA_SDI3LPIB                    0xE4    ; Link Position in Current Buffer
%define HDA_SDI3CBL                     0xE8    ; Cyclic Buffer Length
%define HDA_SDI3LVI                     0xEC    ; Last Valid Index
%define HDA_SDI3FIFOW                   0xFE    ; FIFO watermark
%define HDA_SDI3FIFOSIZE                0xF0    ; FIFO Size
%define HDA_SDI3FORMAT                  0xF2    ; Format
%define HDA_SDI3BDLPL                   0xF8    ; List Pointer - Lower
%define HDA_SDI3BDLPU                   0xFC    ; List Pointer - Upper
%define HDA_SDI3LPIBA                   0x20E4  ; Link Posiiton in Buffer n Alias

%define HDA_SDO0CTL                     0x100   ; Stream Descriptor Control
%define HDA_SDO0STS                     0x103   ; Stream Descriptor Status
%define HDA_SDO0LPIB                    0x104   ; Link Position in Current Buffer
%define HDA_SDO0CBL                     0x108   ; Cyclic Buffer Length
%define HDA_SDO0LVI                     0x10C   ; Last Valid Index
%define HDA_SDO0FIFOW                   0x10E   ; FIFO watermark
%define HDA_SDO0FIFOSIZE                0x110   ; FIFO Size
%define HDA_SDO0FORMAT                  0x112   ; Format
%define HDA_SDO0BDLPL                   0x118   ; List Pointer - Lower
%define HDA_SDO0BDLPU                   0x11C   ; List Pointer - Upper
%define HDA_SDO0LPIBA                   0x2104  ; Link Posiiton in Buffer n Alias

%define HDA_SDO1CTL                     0x120   ; Stream Descriptor Control
%define HDA_SDO1STS                     0x123   ; Stream Descriptor Status
%define HDA_SDO1LPIB                    0x124   ; Link Position in Current Buffer
%define HDA_SDO1CBL                     0x128   ; Cyclic Buffer Length
%define HDA_SDO1LVI                     0x12C   ; Last Valid Index
%define HDA_SDO1FIFOW                   0x12E   ; FIFO watermark
%define HDA_SDO1FIFOSIZE                0x130   ; FIFO Size
%define HDA_SDO1FORMAT                  0x132   ; Format
%define HDA_SDO1BDLPL                   0x138   ; List Pointer - Lower
%define HDA_SDO1BDLPU                   0x13C   ; List Pointer - Upper
%define HDA_SDO1LPIBA                   0x2124  ; Link Posiiton in Buffer n Alias

%define HDA_SDO2CTL                     0x140   ; Stream Descriptor Control
%define HDA_SDO2STS                     0x143   ; Stream Descriptor Status
%define HDA_SDO2LPIB                    0x144   ; Link Position in Current Buffer
%define HDA_SDO2CBL                     0x148   ; Cyclic Buffer Length
%define HDA_SDO2LVI                     0x14C   ; Last Valid Index
%define HDA_SDO2FIFOW                   0x14E   ; FIFO watermark
%define HDA_SDO2FIFOSIZE                0x150   ; FIFO Size
%define HDA_SDO2FORMAT                  0x152   ; Format
%define HDA_SDO2BDLPL                   0x158   ; List Pointer - Lower
%define HDA_SDO2BDLPU                   0x15C   ; List Pointer - Upper
%define HDA_SDO2LPIBA                   0x2144  ; Link Posiiton in Buffer n Alias

%define HDA_SDO3CTL                     0x160   ; Stream Descriptor Control
%define HDA_SDO3STS                     0x163   ; Stream Descriptor Status
%define HDA_SDO3LPIB                    0x164   ; Link Position in Current Buffer
%define HDA_SDO3CBL                     0x168   ; Cyclic Buffer Length
%define HDA_SDO3LVI                     0x16C   ; Last Valid Index
%define HDA_SDO3FIFOW                   0x16E   ; FIFO watermark
%define HDA_SDO3FIFOSIZE                0x170   ; FIFO Size
%define HDA_SDO3FORMAT                  0x172   ; Format
%define HDA_SDO3BDLPL                   0x178   ; List Pointer - Lower
%define HDA_SDO3BDLPU                   0x17C   ; List Pointer - Upper
