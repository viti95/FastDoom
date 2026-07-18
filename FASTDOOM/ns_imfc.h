/*
 * IBM PC Music Feature card driver for FastDoom
 *
 * The IBM PC Music Feature (part number 31F3091) is an 8-voice FM
 * synthesizer card based on Yamaha YM2203-compatible technology.
 * It uses a 9-bit serial protocol over the PC bus.
 *
 * Base I/O addresses:
 *   Position 1: 0x2A20 (switches: OFF, ON)
 *   Position 2: 0x2A30 (switches: ON, ON)
 */

#ifndef __IMFC_H
#define __IMFC_H

/*
 * IMFC I/O Register offsets from base address
 */
#define IMFC_PIU0        0   /* Receive data port (8 bits, read-only) */
#define IMFC_PIU1        1   /* Transmit data port (8 bits, write-only) */
#define IMFC_PIU2        2   /* Status (read) / Control (write) */
#define IMFC_PCR         3   /* PIU Command Register */
#define IMFC_CNTRO       4   /* Counter 0 (Timer A) */
#define IMFC_CNTR1       5   /* Counter 1 (Timer B) */
#define IMFC_CNTR2       6   /* Counter 2 (prescaler for Timer B) */
#define IMFC_TCWR        7   /* Timer Control Word Register */
#define IMFC_TCR         8   /* Total Control Register */
#define IMFC_TSR         12  /* Total Status Register */

/*
 * Base addresses for the card
 */
#define IMFC_Address1    0x2A20
#define IMFC_Address2    0x2A30
#define IMFC_DefaultAddress IMFC_Address1

/*
 * PIU2 status bits (read)
 */
#define IMFC_PIU2_TXRDY      0x01  /* Transmitter Ready (bit 0) */
#define IMFC_PIU2_WIE        0x04  /* Write Interrupt Enable (bit 2) */
#define IMFC_PIU2_RXRDY      0x10  /* Receiver Ready (bit 4) */
#define IMFC_PIU2_RIE        0x20  /* Read Interrupt Enable (bit 3) */
#define IMFC_PIU2_EXRS       0x80  /* Expansion bit (bit 8 of received data, bit 7) */

/*
 * PIU2 control bits (write) - set via PCR Bit Set/Reset commands
 */
#define IMFC_PIU2_RIE_BIT    4     /* Read Interrupt Enable */
#define IMFC_PIU2_WIE_BIT    2     /* Write Interrupt Enable */

/*
 * PCR (PIU Command Register) commands
 */
#define IMFC_PCR_INIT        0xBC  /* Initialize the PIU */
#define IMFC_PCR_SET_WIE     0x05  /* Set Write Interrupt Enable (bit 2 of PIU2) */
#define IMFC_PCR_CLR_RIE     0x04  /* Reset Read Interrupt Enable (bit 4 of PIU2) */
#define IMFC_PCR_SET_RIE     0x09  /* Set Read Interrupt Enable (bit 4 of PIU2) */
#define IMFC_PCR_CLR_WIE     0x08  /* Reset Write Interrupt Enable (bit 2 of PIU2) */

/*
 * TCR (Total Control Register) bits
 */
#define IMFC_TCR_TAC         0x01  /* Timer A Clear (active low) */
#define IMFC_TCR_TBC         0x02  /* Timer B Clear (active low) */
#define IMFC_TCR_TAE         0x04  /* Timer A Enable */
#define IMFC_TCR_TBE         0x08  /* Timer B Enable */
#define IMFC_TCR_EXT8        0x10  /* Extended bit 8 for transmit data */
#define IMFC_TCR_TMSK        0x40  /* Total IRQ Mask (active low) */
#define IMFC_TCR_IBE         0x80  /* IRQ Bus Enable */

/*
 * TSR (Total Status Register) bits
 */
#define IMFC_TSR_TAS         0x01  /* Timer A Status */
#define IMFC_TSR_TBS         0x02  /* Timer B Status */
#define IMFC_TSR_TCS         0x80  /* Total Card Status */

/*
 * Timer control words (for TCWR)
 */
#define IMFC_TCWR_COUNTER0_MODE2  0x34  /* Counter 0, mode 2, 16-bit binary */
#define IMFC_TCWR_COUNTER1_MODE2  0x74  /* Counter 1, mode 2, 16-bit binary */
#define IMFC_TCWR_COUNTER2_MODE3  0xB6  /* Counter 2, mode 3, 16-bit binary */
#define IMFC_TCWR_LATCH_COUNTER0  0x00  /* Latch Counter 0 */
#define IMFC_TCWR_LATCH_COUNTER1  0x40  /* Latch Counter 1 */
#define IMFC_TCWR_LATCH_COUNTER2  0x80  /* Latch Counter 2 */

/*
 * IMFC Error codes
 */
enum IMFC_ERRORS
{
    IMFC_Warning = -2,
    IMFC_Error = -1,
    IMFC_Ok = 0,
    IMFC_NotFound,
    IMFC_Timeout,
    IMFC_DPMI_Error
};

/*
 * Music Card Messages (bit 8 = 1)
 * These are 9-bit commands sent between system and card
 */

/* Error report messages (card -> system): 1F0-1FF */
#define IMFC_MSG_FIFO_OVERFLOW_TX   0x1F0  /* FIFO (card->system) overflow */
#define IMFC_MSG_FIFO_OVERFLOW_RX   0x1F1  /* FIFO (MIDI->card) overflow */
#define IMFC_MSG_MIDI_RECEIVE_ERR   0x1F2  /* MIDI reception error */
#define IMFC_MSG_MIDI_OFFLINE_ERR   0x1F3  /* MIDI off-line error */
#define IMFC_MSG_TIMEOUT_RX         0x1F4  /* Time-out (MIDI->card) */
#define IMFC_MSG_TIMEOUT_TX         0x1F5  /* Time-out (system->card) */

/* Command messages (system -> card): 1E0-1E5 */
#define IMFC_CMD_SELECT_MODE        0x1E0  /* Select card mode (THRU/MUSIC) */
#define IMFC_CMD_SELECT_ERR_REPORT  0x1E1  /* Select error reporting mode */
#define IMFC_CMD_SET_PATHS          0x1E2  /* Set MIDI paths */
#define IMFC_CMD_SET_NODE           0x1E3  /* Set node parameters */
#define IMFC_CMD_REBOOT             0x1E5  /* Reboot the card */

/* Status request messages (system -> card): 1D0-1D3 */
#define IMFC_REQ_CARD_MODE          0x1D0  /* Request card mode status */
#define IMFC_REQ_ERR_REPORT_MODE    0x1D1  /* Request error report status */
#define IMFC_REQ_PATH_STATUS        0x1D2  /* Request path status */
#define IMFC_REQ_NODE_STATUS        0x1D3  /* Request node parameter status */

/* Mode selection data bytes */
#define IMFC_MODE_MUSIC             0x00  /* MUSIC mode (sound generation) */
#define IMFC_MODE_THRU              0x01  /* THRU mode (MIDI passthrough) */

/* Error reporting data bytes */
#define IMFC_ERR_REPORT_DISABLE     0x00  /* Disable error reports */
#define IMFC_ERR_REPORT_ENABLE      0x01  /* Enable error reports */

/*
 * MIDI message types (bit 8 = 0)
 */
#define IMFC_MIDI_NOTE_OFF         0x80
#define IMFC_MIDI_NOTE_ON          0x90
#define IMFC_MIDI_POLY_AFTER_TOUCH 0xA0
#define IMFC_MIDI_CONTROL_CHANGE   0xB0
#define IMFC_MIDI_PROGRAM_CHANGE   0xC0
#define IMFC_MIDI_AFTER_TOUCH      0xD0
#define IMFC_MIDI_PITCH_BEND       0xE0

#define IMFC_MIDI_SYSEX_START      0xF0
#define IMFC_MIDI_SYSEX_END        0xF7
#define IMFC_MIDI_ACTIVE_SENSING   0xFE
#define IMFC_MIDI_REALTIME_CLOCK   0xF8
#define IMFC_MIDI_START            0xFA
#define IMFC_MIDI_CONTINUE         0xFB
#define IMFC_MIDI_STOP             0xFC

#define IMFC_MIDI_ALL_NOTES_OFF    0x7B
#define IMFC_MIDI_RESET_ALL_CTRL   0x79
#define IMFC_MIDI_RPN_MSB          0x65
#define IMFC_MIDI_RPN_LSB          0x64
#define IMFC_MIDI_DATAENTRY_MSB    0x06
#define IMFC_MIDI_DATAENTRY_LSB    0x26
#define IMFC_MIDI_PITCHBEND_MSB    0x02
#define IMFC_MIDI_VOLUME           0x07

/*
 * Path bit masks for Set Paths command
 * Each path has 5 bits: a=Note ON/OFF, b=AfterTouch/PitchBend,
 * c=Control/Program Change, d=System Exclusive/Common, e=Real-Time
 */
#define IMFC_PATH_NOTE_ONOFF       0x01  /* Bit a */
#define IMFC_PATH_AFTOUCH_PBEND    0x02  /* Bit b */
#define IMFC_PATH_CTRL_PROGCHANGE  0x04  /* Bit c */
#define IMFC_PATH_SYSEX_COMMON     0x08  /* Bit d */
#define IMFC_PATH_REALTIME         0x10  /* Bit e */
#define IMFC_PATH_ALL              0x1F  /* All message types */
#define IMFC_PATH_NONE             0x00  /* No messages */

/*
 * MIDI System Exclusive - Yamaha Manufacturer ID
 * The IBM PC Music Feature uses Yamaha's MIDI SysEx ID
 */
#define IMFC_SYSEx_YAMAHA_ID       0x43

/*
 * Yamaha SysEx sub-status values
 */
#define IMFC_SYSEX_SUB_CHANNEL     0x10  /* Channel address (0x1n) */
#define IMFC_SYSEX_SUB_INSTRUMENT  0x75  /* Instrument address (0x75) */
#define IMFC_SYSEX_SUB_NODE_DUMP   0x60  /* Node talk-back (0x6s) */

/*
 * Default volume for General MIDI
 */
#define IMFC_DEFAULT_VOLUME        127

/*
 * Maximum wait count for I/O operations (approximate timeout)
 */
#define IMFC_MAX_WAIT              0xFFFF

/*
 * Card detection: read card ID via SysEx dump request
 * The card responds with "YAMAHA IBM MUSIC" (16 chars)
 */
#define IMFC_CARD_ID_STRING        "YAMAHA IBM MUSIC"

/*
 * Node number used (0-15, default 0)
 */
#define IMFC_DEFAULT_NODE          0

/*
 * External variables
 */
extern int IMFC_BaseAddr;

/*
 * Function prototypes
 */

/* Initialize and detect the IBM PC Music Feature card */
int IMFC_Init(int addr);

/* Shutdown / cleanup the card */
void IMFC_Shutdown(void);

/* Send a single 9-bit data byte to the card */
void IMFC_SendByte(int data, int bit8);

/* Read a single 9-bit data byte from the card */
int IMFC_ReadByte(int *bit8);

/* Send a byte of MIDI data (bit 8 = 0) */
void IMFC_SendMidi(int data);

/* Wait for transmitter ready */
int IMFC_WaitForTxReady(void);

/* Wait for receiver ready and read data */
int IMFC_WaitForRxReady(void);

/* Check if card is present at given address */
int IMFC_Detect(int addr);

/* Reset the card to defaults */
void IMFC_Reset(void);

/* Set the card to MUSIC mode */
void IMFC_SetMusicMode(void);

/* Set up MIDI paths for playback */
void IMFC_SetPaths(void);

/* Set master volume (0-127) */
void IMFC_SetMasterVolume(int volume);

/*
 * MIDI function interface (matches midifuncs struct)
 */
void IMFC_NoteOff(int channel, int key, int velocity);
void IMFC_NoteOn(int channel, int key, int velocity);
void IMFC_PolyAftertouch(int channel, int key, int pressure);
void IMFC_ControlChange(int channel, int number, int value);
void IMFC_ProgramChange(int channel, int program);
void IMFC_ChannelAftertouch(int channel, int pressure);
void IMFC_PitchBend(int channel, int lsb, int msb);
void IMFC_SysEx(unsigned char *ptr, int length);

/* Set volume via SysEx (card-level volume, not per-channel) */
void IMFC_SetVolume(int volume);
int IMFC_GetVolume(void);

/* Send all notes off */
void IMFC_AllNotesOff(void);

#endif /* __IMFC_H */
