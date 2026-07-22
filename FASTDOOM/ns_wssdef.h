//
// WSS (Windows Sound System) hardware definitions
// Based on AD1848 datasheet
//

#ifndef __WSSDEF_H
#define __WSSDEF_H

/*
 * Direct register offsets from base I/O address
 * Index/Data pair at offsets 4/5, Status at offset 6
 */
#define WSS_INDEX_REG_OFFSET    4
#define WSS_IDATA_REG_OFFSET    5
#define WSS_STATUS_REG_OFFSET   6

/*
 * Indirect register addresses (written to index, then read/write data)
 */
#define WSS_LADC_REG            0x00  /* Left ADC (mic/line) control */
#define WSS_RADC_REG            0x01  /* Right ADC control */
#define WSS_LAUX1_REG           0x02  /* Left AUX1 (FM) control */
#define WSS_RAUX1_REG           0x03  /* Right AUX1 control */
#define WSS_LAUX2_REG           0x04  /* Left AUX2 (CD) control */
#define WSS_RAUX2_REG           0x05  /* Right AUX2 control */
#define WSS_LDAC_REG            0x06  /* Left DAC (PCM) volume */
#define WSS_RDAC_REG            0x07  /* Right DAC volume */
#define WSS_FORMAT_REG          0x08  /* Playback format register */
#define WSS_CONFIG_REG          0x09  /* Configuration register */
#define WSS_PIN_REG             0x0A  /* Interrupt pin control */
#define WSS_TEST_INIT_REG       0x0B  /* Test/Initialize register */
#define WSS_MISC_REG            0x0C  /* Miscellaneous register */
#define WSS_MIX_REG             0x0D  /* Mixer loopback control */
#define WSS_PLAYBACK_UCNT_REG   0x0E  /* Playback DMA count high byte */
#define WSS_PLAYBACK_LCNT_REG   0x0F  /* Playback DMA count low byte */

/*
 * Direct register bit definitions
 */
/* WSS_INDEX_REG bits */
#define WSS_INIT_BIT            (1 << 7)  /* Initialization bit (ready=0) */
#define WSS_MCE_BIT             (1 << 6)  /* Master Control Enable */
#define WSS_TRD_BIT             (1 << 5)  /* Tri-State Register/Data */

/*
 * Indirect register bit definitions
 */
/* WSS_LAUX1_REG, WSS_RAUX1_REG, WSS_LAUX2_REG, WSS_RAUX2_REG bits */
#define WSS_AUX_MUTE_BIT        (1 << 7)  /* Mute bit for AUX channels */

/* WSS_LDAC_REG / WSS_RDAC_REG fields */
#define WSS_DAC_MUTE_BIT        (1 << 7)  /* Mute DAC output */
#define WSS_DAC_ATTEN_BITS      6         /* 6 bits of attenuation */
#define WSS_DAC_ATTEN_MAX_VAL   ((1 << WSS_DAC_ATTEN_BITS) - 1)

/* WSS_FORMAT_REG bits */
#define WSS_FMT_BIT             (1 << 6)  /* 1=16-bit, 0=8-bit */
#define WSS_L_C_BIT             (1 << 5)  /* Loopback select */
#define WSS_S_M_BIT             (1 << 4)  /* 1=Stereo, 0=Mono */
#define WSS_CFS2_BIT            (1 << 3)  /* Clock Frequency Select bit 2 */
#define WSS_CFS1_BIT            (1 << 2)  /* Clock Frequency Select bit 1 */
#define WSS_CFS0_BIT            (1 << 1)  /* Clock Frequency Select bit 0 */
#define WSS_CSS_BIT             (1 << 0)  /* Clock Source Select */

/* WSS_CONFIG_REG bits */
#define WSS_CPIO_BIT            (1 << 7)  /* Capture Pin Output */
#define WSS_PPIO_BIT            (1 << 6)  /* Playback Pin Output */
#define WSS_ACAL_BIT            (1 << 3)  /* Auto-Calibrate */
#define WSS_SDC_BIT             (1 << 2)  /* Software DAC Control */
#define WSS_CEN_BIT             (1 << 1)  /* Capture Enable */
#define WSS_PEN_BIT             (1 << 0)  /* Playback Enable */

/* WSS_PIN_REG bits */
#define WSS_XCTL1_BIT           (1 << 7)  /* Xlate Control bit 1 */
#define WSS_XCTL0_BIT           (1 << 6)  /* Xlate Control bit 0 */
#define WSS_IEN_BIT             (1 << 1)  /* Interrupt Enable */

/* WSS_TEST_INIT_REG bits */
#define WSS_COR_BIT             (1 << 7)  /* Calibration Override */
#define WSS_PUR_BIT             (1 << 6)  /* Purge */
#define WSS_ACI_BIT             (1 << 5)  /* Auto-Calibration In Progress */
#define WSS_DRS_BIT             (1 << 4)  /* DAC Reset */

/* Default WSS I/O configuration */
#define WSS_DEFAULT_BASE        0x530
#define WSS_DEFAULT_IRQ         5
#define WSS_DEFAULT_DMA         1

/* WSS supported sample rates */
#define WSS_MinSamplingRate     8000
#define WSS_MaxSamplingRate     48000

/* Mix mode flags */
#define WSS_SIXTEEN_BIT 2
#define WSS_STEREO 1
#define WSS_MONO_8BIT 0
#define WSS_STEREO_8BIT (WSS_STEREO)

/* WSS maximum mix mode */
#define WSS_MaxMixMode          WSS_STEREO_8BIT
#define WSS_DefaultSampleRate   11025
#define WSS_DefaultMixMode      WSS_MONO_8BIT
#define WSS_MaxIrq              15

/* Double buffering */
#define WSS_NUM_BUFFERS         2

#endif
