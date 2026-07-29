//
// AdLib Gold 1000/2000 hardware definitions
// Based on YMZ263 (MMA) datasheet and AdLib Gold Developer Toolkit
//

#ifndef __GOLDDEF_H
#define __GOLDDEF_H

/*
 * Default Gold card I/O configuration.
 *
 * The Gold card uses a block of 8 consecutive I/O ports starting at
 * the base address (0x388 by default):
 *   Base+0, Base+1  -> FM Bank 0 (OPL3)
 *   Base+2, Base+3  -> FM Bank 1 / Control Chip
 *   Base+4, Base+5  -> MMA Sampling Channel 0
 *   Base+6, Base+7  -> MMA Sampling Channel 1
 */
#define GOLD_DEFAULT_BASE       0x388
#define GOLD_DEFAULT_IRQ        5
#define GOLD_DEFAULT_DMA        1

/*
 * I/O port offsets from base address.
 */
#define GOLD_FM_BANK0_ADDR      0     /* FM Bank 0 address register     */
#define GOLD_FM_BANK0_DATA      1     /* FM Bank 0 data register        */
#define GOLD_CT_ADDR_OFFSET     2     /* Control Chip address register  */
#define GOLD_CT_DATA_OFFSET     3     /* Control Chip data register     */
#define GOLD_MMA0_ADDR_OFFSET   4     /* MMA Channel 0 register select  */
#define GOLD_MMA0_DATA_OFFSET   5     /* MMA Channel 0 data register    */
#define GOLD_MMA1_ADDR_OFFSET   6     /* MMA Channel 1 register select  */
#define GOLD_MMA1_DATA_OFFSET   7     /* MMA Channel 1 data register    */

/*
 * Control chip status register bits (read from CT_ADDR port).
 */
#define GOLD_CT_SCSI_BIT        (1 << 3)  /* SCSI interrupt pending     */
#define GOLD_CT_TEL_BIT         (1 << 2)  /* Telephone interrupt pending*/
#define GOLD_CT_SMP_BIT         (1 << 1)  /* Sampling interrupt pending */
#define GOLD_CT_FM_BIT          (1 << 0)  /* FM interrupt pending       */
#define GOLD_CT_SB_BIT          (1 << 5)  /* Card busy writing register */
#define GOLD_CT_RB_BIT          (1 << 6)  /* Card busy writing to EEPROM*/

/*
 * Control chip register numbers.
 */
#define GOLD_CT_REG_CONTROL_ID      0x00  /* Control/ID register        */
#define GOLD_CT_REG_TEL_CONTROL     0x01  /* Telephone control          */
#define GOLD_CT_REG_SAMP_GAIN_L     0x02  /* Sampling gain left         */
#define GOLD_CT_REG_SAMP_GAIN_R     0x03  /* Sampling gain right        */
#define GOLD_CT_REG_OUT_VOL_L       0x04  /* Final output volume left   */
#define GOLD_CT_REG_OUT_VOL_R       0x05  /* Final output volume right  */
#define GOLD_CT_REG_BASS            0x06  /* Bass control               */
#define GOLD_CT_REG_TREBLE          0x07  /* Treble control             */
#define GOLD_CT_REG_OUTPUT_MODE     0x08  /* Output mode                */
#define GOLD_CT_REG_FM_VOL_L        0x09  /* FM volume left             */
#define GOLD_CT_REG_FM_VOL_R        0x0A  /* FM volume right            */
#define GOLD_CT_REG_SAMP_VOL_L      0x0B  /* Sampling volume left       */
#define GOLD_CT_REG_SAMP_VOL_R      0x0C  /* Sampling volume right      */
#define GOLD_CT_REG_AUX_VOL_L       0x0D  /* AUX volume left            */
#define GOLD_CT_REG_AUX_VOL_R       0x0E  /* AUX volume right           */
#define GOLD_CT_REG_MIC_VOL         0x0F  /* Microphone volume          */
#define GOLD_CT_REG_TEL_VOL         0x10  /* Telephone volume           */
#define GOLD_CT_REG_MIX_SELECT      0x11  /* Mix/filter select          */
#define GOLD_CT_REG_UNUSED          0x12  /* Unused (set to 0)          */
#define GOLD_CT_REG_AUDIO_IRQ_DMA0  0x13  /* Audio IRQ/DMA select ch 0  */
#define GOLD_CT_REG_DMA_SEL1        0x14  /* DMA select channel 1       */
#define GOLD_CT_REG_AUDIO_RELOCATE  0x15  /* Audio port relocation      */
#define GOLD_CT_REG_SCSI_IRQ_DMA    0x16  /* SCSI IRQ/DMA select        */
#define GOLD_CT_REG_SCSI_RELOCATE   0x17  /* SCSI port relocation       */
#define GOLD_CT_REG_SURROUND        0x18  /* Surround control           */

/*
 * Control chip register 0x13: Audio IRQ/DMA Select - Channel 0
 * D7:   DENO (DMA enable for channel 0)
 * D6-4: DMA SEL 0 (DMA channel selection)
 * D3:   AEN (Audio interrupt enable)
 * D2-0: INT SEL A (IRQ line selection)
 */
#define GOLD_CT_DENO_BIT            (1 << 7)  /* DMA enable ch 0        */
#define GOLD_CT_DMA_SEL_SHIFT       4         /* DMA channel select shift*/
#define GOLD_CT_AEN_BIT             (1 << 3)  /* Audio IRQ enable       */
#define GOLD_CT_INT_SEL_MASK        0x07      /* IRQ select mask (bits 0-2) */

/*
 * Control chip register 0x14: DMA Select - Channel 1
 * D7:   DEN1 (DMA enable for channel 1)
 * D6-4: DMA SEL 1 (DMA channel selection)
 */
#define GOLD_CT_DEN1_BIT            (1 << 7)  /* DMA enable ch 1        */

/*
 * DMA channel encodings for control chip.
 * 0 = DMA 0 (Gold 2000 only), 1 = DMA 1, 2 = DMA 2, 3 = DMA 3
 * Gold 1000 supports DMA 1, 2, 3 only.
 */

/*
 * IRQ encodings for control chip INT SEL A.
 * 0 = IRQ 3, 1 = IRQ 4, 2 = IRQ 5, 3 = IRQ 7
 * 4 = IRQ 10, 5 = IRQ 11, 6 = IRQ 12, 7 = IRQ 15 (Gold 2000 only)
 */

/*
 * MMA (YMZ263) register numbers.
 * These are written to the register select port (Base+4 for ch 0, Base+6 for ch 1)
 * to select which register is accessed via the data port (Base+5 for ch 0, Base+7 for ch 1).
 */
#define GOLD_MMA_TEST_REG           0x01  /* Test register (do not access) */
#define GOLD_MMA_TIMER0_L           0x02  /* Timer 0 low byte             */
#define GOLD_MMA_TIMER0_H           0x03  /* Timer 0 high byte            */
#define GOLD_MMA_BASE_COUNTER_L     0x04  /* Base counter low byte        */
#define GOLD_MMA_BASE_COUNTER_H     0x05  /* Base counter high byte       */
#define GOLD_MMA_TIMER2_L           0x06  /* Timer 2 low byte             */
#define GOLD_MMA_TIMER2_H           0x07  /* Timer 2 high byte            */
#define GOLD_MMA_TIMER_CTL          0x08  /* Timer control                */
#define GOLD_MMA_PLAY_REC_CTL       0x09  /* Playback/recording control   */
#define GOLD_MMA_VOLUME             0x0A  /* Output volume control        */
#define GOLD_MMA_PCM_DATA           0x0B  /* PCM data (FIFO read/write)   */
#define GOLD_MMA_FMT_CTL            0x0C  /* Sampling format and control  */
#define GOLD_MMA_MIDI_IRQ_CTL       0x0D  /* MIDI and interrupt control   */
#define GOLD_MMA_MIDI_DATA          0x0E  /* MIDI data                    */

/*
 * MMA status register bits (read from register select port Base+4 or Base+6).
 */
#define GOLD_MMA_STATUS_OV_BIT      (1 << 7)  /* Digital overrun error    */
#define GOLD_MMA_STATUS_T2_BIT      (1 << 6)  /* Timer 2 expired          */
#define GOLD_MMA_STATUS_T1_BIT      (1 << 5)  /* Timer 1 expired          */
#define GOLD_MMA_STATUS_T0_BIT      (1 << 4)  /* Timer 0 expired          */
#define GOLD_MMA_STATUS_TRQ_BIT     (1 << 3)  /* MIDI transmit FIFO empty */
#define GOLD_MMA_STATUS_RRQ_BIT     (1 << 2)  /* MIDI receive has data    */
#define GOLD_MMA_STATUS_FIFO1_BIT   (1 << 1)  /* FIFO1 (ch1) interrupt    */
#define GOLD_MMA_STATUS_FIFO_BIT    (1 << 0)  /* FIFO0 (ch0) interrupt    */

/*
 * Register 0x0D: MIDI and interrupt control.
 * D7: IEN (interrupt enable) - 1=enable unmasked interrupts
 * D6: F1M (FIFO1 mask)        - 0=unmasked
 * D5: F0M (FIFO0 mask)        - 0=unmasked
 * D4: TIM (timer interrupt mask)
 * D3: T2M (timer 2 mask)
 * D2: T1M (timer 1 mask)
 * D1: T0M (timer 0 mask)
 * D0: MID (MIDI interrupt mask)
 */
#define GOLD_MMA_IEN_BIT            (1 << 7)  /* Interrupt enable         */
#define GOLD_MMA_F0M_BIT            (1 << 5)  /* FIFO0 interrupt mask     */
#define GOLD_MMA_F1M_BIT            (1 << 6)  /* FIFO1 interrupt mask     */

/*
 * Register 0x08: Timer Control
 * D7: SBY (standby mode, must be 0 for normal operation)
 * D6: T2M (timer 2 interrupt mask)
 * D5: T1M (timer 1 interrupt mask)
 * D4: T0M (timer 0 interrupt mask)
 * D3: STB (start/stop base counter)
 * D2: ST2 (start/stop timer 2)
 * D1: ST1 (start/stop timer 1)
 * D0: ST0 (start/stop timer 0)
 */
#define GOLD_MMA_SBY_BIT            (1 << 7)
#define GOLD_MMA_T2M_BIT            (1 << 6)
#define GOLD_MMA_T1M_BIT            (1 << 5)
#define GOLD_MMA_T0M_BIT            (1 << 4)
#define GOLD_MMA_STB_BIT            (1 << 3)
#define GOLD_MMA_ST2_BIT            (1 << 2)
#define GOLD_MMA_ST1_BIT            (1 << 1)
#define GOLD_MMA_ST0_BIT            (1 << 0)

/*
 * Register 0x09: Playback and Recording Control
 * D7: RST (reset PCM/ADPCM, clears FIFO)
 * D6: R (right channel output enable)
 * D5: L (left channel output enable)
 * D4-3: FREQ[1:0] (sample rate select)
 * D2: PCM (1=PCM mode, 0=ADPCM mode)
 * D1: PR (1=playback, 0=record)
 * D0: GO (1=start, 0=stop)
 */
#define GOLD_MMA_RST_BIT            (1 << 7)
#define GOLD_MMA_R_BIT              (1 << 6)
#define GOLD_MMA_L_BIT              (1 << 5)
#define GOLD_MMA_FREQ_SHIFT         3
#define GOLD_MMA_PCM_BIT            (1 << 2)
#define GOLD_MMA_PR_BIT             (1 << 1)
#define GOLD_MMA_GO_BIT             (1 << 0)

/*
 * FREQ encoding for register 0x09.
 * FREQ    PCM rate    ADPCM rate
 * 0       44.1 kHz    22.05 kHz
 * 1       22.05 kHz   11.025 kHz
 * 2       11.025 kHz  7.35 kHz
 * 3       7.35 kHz    5.5125 kHz
 */
#define GOLD_MMA_FREQ_44100         0
#define GOLD_MMA_FREQ_22050         1
#define GOLD_MMA_FREQ_11025         2
#define GOLD_MMA_FREQ_7350          3

/*
 * Register 0x0C: Sampling Format and Control
 * D7: ILV (interleave, channel 0 only)
 * D6-5: DATA FORMAT[1:0] (data format select)
 * D4-1: FIFO INT[3:0] (FIFO interrupt level)
 * D0: ENB (DMA enable)
 */
#define GOLD_MMA_ILV_BIT            (1 << 7)  /* Interleave channels    */
#define GOLD_MMA_DATA_FMT_SHIFT     5         /* Data format shift      */
#define GOLD_MMA_FIFO_INT_SHIFT     1         /* FIFO INT level shift   */
#define GOLD_MMA_DMA_ENB_BIT        (1 << 0)  /* DMA mode enable        */

/*
 * DATA FORMAT encodings.
 * 0 = 1-byte format (8 MSBs of 12-bit sample)
 * 1 = 2-byte format (8 LSBs + 4 MSBs with sign extension)
 * 2 = 2-byte format (4 LSBs + 8 MSBs)
 * 3 = invalid (ignored in ADPCM mode)
 */
#define GOLD_MMA_DATA_FMT_8BIT      0x00  /* 8-bit (8 MSBs)             */
#define GOLD_MMA_DATA_FMT_12BIT_A   0x01  /* 12-bit format A (2 bytes)  */
#define GOLD_MMA_DATA_FMT_12BIT_B   0x02  /* 12-bit format B (2 bytes)  */

/*
 * FIFO INT encodings.
 * These set the remaining FIFO byte threshold that triggers an interrupt.
 * Value   Remaining bytes at interrupt
 * 0       112
 * 1       96
 * 2       80
 * 3       64
 * 4       48
 * 5       32
 * 6       16
 * 7       prohibited
 */
#define GOLD_MMA_FIFO_INT_112       0x00
#define GOLD_MMA_FIFO_INT_96        0x01
#define GOLD_MMA_FIFO_INT_80        0x02
#define GOLD_MMA_FIFO_INT_64        0x03
#define GOLD_MMA_FIFO_INT_48        0x04
#define GOLD_MMA_FIFO_INT_32        0x05
#define GOLD_MMA_FIFO_INT_16        0x06
/* FIFO_INT_7 = 0x07 is prohibited */

/*
 * Gold card supported sample rate range.
 */
#define GOLD_MinSamplingRate        7350
#define GOLD_MaxSamplingRate        44100

/*
 * Mix mode flags.
 */
#define GOLD_SIXTEEN_BIT 2
#define GOLD_STEREO 1
#define GOLD_MONO_8BIT 0
#define GOLD_STEREO_8BIT (GOLD_STEREO)

/*
 * Gold card maximum mix mode and defaults.
 * The YMZ263 supports 12-bit samples but we use 8-bit MSB format
 * for simplicity and compatibility with the FastDoom mixer.
 */
#define GOLD_MaxMixMode             GOLD_MONO_8BIT
#define GOLD_DefaultSampleRate      11025
#define GOLD_DefaultMixMode         GOLD_MONO_8BIT
#define GOLD_MaxIrq                 15

/*
 * FIFO buffer size of the YMZ263.
 */
#define GOLD_MMA_FIFO_SIZE          128

#endif
