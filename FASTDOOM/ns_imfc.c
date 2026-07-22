/*
 * IBM PC Music Feature card driver for FastDoom
 *
 * This driver implements support for the IBM PC Music Feature card
 * (part number 31F3091), an 8-voice FM synthesizer for the IBM PC/AT.
 *
 * The card uses a 9-bit serial protocol over the PC bus with base
 * I/O addresses at 0x2A20 or 0x2A30. It accepts standard MIDI
 * messages and Yamaha System Exclusive messages.
 *
 * Communication protocol:
 *   - 9-bit data words where bit 8 = 1 indicates a music card
 *     control message and bit 8 = 0 indicates a MIDI message.
 *   - Data is sent via PIU1 (8 bits) and TCR bit 4 (bit 8).
 *   - Data is received via PIU0 (8 bits) and PIU2 bit 7 (bit 8).
 *
 * References:
 *   - IBM PC Music Feature Technical Reference (1987)
 *   - Yamaha MIDI System Exclusive specification
 */

#include <conio.h>
#include <stdlib.h>
#include "ns_dpmi.h"
#include "doomstat.h"
#include "ns_inter.h"
#include "ns_imfc.h"
#include "options.h"

typedef struct {
    unsigned char bank;  // Banco ROM de la tarjeta (2, 3, 4, 5 o 6)
    unsigned char voice; // Índice de la voz dentro del banco (0 a 47)
} GM_To_IBM_Map;

// Tabla de mapeo para los 128 instrumentos General MIDI (índices 0 - 127)
static const GM_To_IBM_Map gm_to_ibm_table[128] = {
    /* 0-7: Pianos */
    {5, 0},  {2, 1},  {2, 2},  {2, 3},  {2, 4},  {5, 3},  {2, 13}, {2, 14},
    /* 8-15: Percusión Cromática */
    {3, 11}, {3, 13}, {3, 12}, {2, 16}, {2, 17}, {4, 11}, {3, 14}, {5, 12},
    /* 16-23: Órganos */
    {2, 8},  {2, 9},  {4, 3},  {5, 6},  {2, 11}, {2, 12}, {3, 24}, {5, 9},
    /* 24-31: Guitarras */
    {4, 19}, {4, 20}, {4, 21}, {4, 22}, {4, 23}, {4, 24}, {4, 24}, {4, 20},
    /* 32-39: Bajos */
    {2, 36}, {2, 40}, {2, 41}, {4, 28}, {4, 27}, {6, 28}, {3, 36}, {3, 37},
    /* 40-47: Cuerdas solistas / Orquesta */
    {4, 41}, {4, 42}, {4, 43}, {3, 34}, {5, 34}, {3, 35}, {3, 10}, {3, 15},
    /* 48-55: Ensamble */
    {2, 32}, {2, 33}, {5, 36}, {5, 37}, {5, 40}, {5, 38}, {5, 39}, {5, 41},
    /* 56-63: Metales (Brass) */
    {2, 26}, {2, 27}, {2, 28}, {3, 28}, {2, 25}, {5, 30}, {5, 31}, {5, 32},
    /* 64-71: Caña (Reed) */
    {5, 24}, {5, 25}, {5, 26}, {5, 27}, {2, 22}, {4, 32}, {2, 24}, {2, 23},
    /* 72-79: Viento madera (Pipe) */
    {3, 20}, {2, 21}, {3, 23}, {3, 22}, {5, 20}, {5, 21}, {5, 19}, {5, 20},
    /* 80-87: Sintetizador Lead */
    {2, 42}, {2, 43}, {3, 40}, {3, 41}, {3, 42}, {5, 38}, {4, 44}, {4, 45},
    /* 88-95: Sintetizador Pad */
    {3, 45}, {2, 35}, {2, 44}, {5, 40}, {3, 46}, {6, 44}, {6, 43}, {6, 45},
    /* 96-103: Sintetizador FX */
    {6, 42}, {6, 43}, {6, 44}, {3, 46}, {5, 44}, {6, 46}, {6, 47}, {5, 45},
    /* 104-111: Étnicos */
    {4, 16}, {4, 17}, {4, 15}, {4, 14}, {5, 13}, {5, 23}, {4, 41}, {5, 21},
    /* 112-119: Percusivos */
    {4, 12}, {5, 15}, {3, 16}, {5, 16}, {5, 17}, {3, 19}, {5, 18}, {2, 19},
    /* 120-127: Efectos de sonido */
    {3, 47}, {3, 47}, {3, 47}, {5, 47}, {5, 46}, {2, 20}, {3, 47}, {3, 47}
};

/*
 * Base I/O address for the card (set by IMFC_Init)
 */
int IMFC_BaseAddr = IMFC_DefaultAddress;

/*
 * Current volume level (0-255 for MIDI interface, mapped to 0-127 for card)
 */
static int IMFC_Volume = 127;

/* Storage for the last received bit 8 */
static int _IMFC_RxBit8 = 0;

/*---------------------------------------------------------------------
   Function: IMFC_WaitForTxReady

   Waits for the transmitter to be ready to accept data.
   Returns IMFC_Ok if ready, IMFC_Timeout if not.
---------------------------------------------------------------------*/
int IMFC_WaitForTxReady(void)
{
    unsigned count;
    int status;

    count = IMFC_MAX_WAIT;
    do
    {
        status = inp(IMFC_BaseAddr + IMFC_PIU2);
        if (status & IMFC_PIU2_TXRDY)
        {
            return IMFC_Ok;
        }
        count--;
    } while (count > 0);

    return IMFC_Timeout;
}

/*---------------------------------------------------------------------
   Function: IMFC_WaitForRxReady

   Waits for the receiver to have data ready and reads it.
   Returns the 8-bit data value; bit 8 is returned via *bit8.
   Returns -1 on timeout.
---------------------------------------------------------------------*/
int IMFC_WaitForRxReady(void)
{
    unsigned count;
    int status;
    int data;
    int bit8;

    count = IMFC_MAX_WAIT;
    do
    {
        status = inp(IMFC_BaseAddr + IMFC_PIU2);
        if (status & IMFC_PIU2_RXRDY)
        {
            /*
             * Bit 8 (EXRS) must be read from PIU2 BEFORE reading
             * PIU0, as reading PIU0 clears RxRDY and may lose bit 8.
             */
            bit8 = (status & IMFC_PIU2_EXRS) ? 1 : 0;

            /* Read the 8-bit data from PIU0 (this clears RxRDY) */
            data = inp(IMFC_BaseAddr + IMFC_PIU0);

            _IMFC_RxBit8 = bit8;
            return data;
        }
        count--;
    } while (count > 0);

    return -1;
}

/*---------------------------------------------------------------------
   Function: IMFC_SendByte

   Sends a 9-bit data byte to the music card.
   data  = lower 8 bits
   bit8  = the 9th bit (0 for MIDI, 1 for card messages)
---------------------------------------------------------------------*/
void IMFC_SendByte(int data, int bit8)
{
    int port_tcr;
    int port_piu1;

    /* Wait for transmitter ready */
    IMFC_WaitForTxReady();

    port_tcr = IMFC_BaseAddr + IMFC_TCR;
    port_piu1 = IMFC_BaseAddr + IMFC_PIU1;

    /*
     * Write TCR directly: 0 for data (bit 8 = 0),
     * TCR_EXT8 for command (bit 8 = 1).
     */
    if (bit8)
    {
        outp(port_tcr, IMFC_TCR_EXT8);
    }
    else
    {
        outp(port_tcr, 0);
    }

    /* Write the 8-bit data to PIU1 (this clears TxRDY) */
    outp(port_piu1, data & 0xFF);
}

/*---------------------------------------------------------------------
   Function: IMFC_ReadByte

   Reads a 9-bit data byte from the music card.
   Returns the 8-bit data; bit 8 is returned via *bit8.
---------------------------------------------------------------------*/
int IMFC_ReadByte(int *bit8)
{
    int data;

    data = IMFC_WaitForRxReady();
    if (data >= 0 && bit8 != NULL)
    {
        *bit8 = _IMFC_RxBit8;
    }

    return data;
}

/*---------------------------------------------------------------------
   Function: IMFC_SendMidi

   Sends a byte of MIDI data to the music card (bit 8 = 0).
---------------------------------------------------------------------*/
void IMFC_SendMidi(int data)
{
    IMFC_SendByte(data, 0);
}

/*---------------------------------------------------------------------
   Function: IMFC_SendCommand

   Sends a music card command message (bit 8 = 1).
   command = the command byte (e.g., 0x1E0 for mode select)
   data    = pointer to data bytes to send (can be NULL)
   count   = number of data bytes to send
---------------------------------------------------------------------*/
static void IMFC_SendCommand(int command, unsigned char *data, int count)
{
    int i;

    /* Send command byte with bit 8 = 1 */
    IMFC_SendByte(command, 1);

    /* Send data bytes with bit 8 = 1 */
    if (data != NULL)
    {
        for (i = 0; i < count; i++)
        {
            IMFC_SendByte(data[i], 1);
        }
    }
}

/*---------------------------------------------------------------------
   Function: IMFC_SendStatusRequest

   Sends a status request and reads the response.
   request = the request byte (e.g., 0x1D0 for card mode)
   response = buffer to store response data
   responsetype = expected number of response bytes (not counting status)
   Returns IMFC_Ok on success.
---------------------------------------------------------------------*/
static int IMFC_SendStatusRequest(int request, unsigned char *response, int responsecount)
{
    int i;
    int data;
    int bit8;
    int status_byte;

    /* Send the request */
    IMFC_SendByte(request, 1);

    /* Read status byte (echo of request) */
    data = IMFC_ReadByte(&bit8);
    if (data < 0)
    {
        return IMFC_Timeout;
    }
    status_byte = data;

    /* Read response data bytes */
    for (i = 0; i < responsecount; i++)
    {
        data = IMFC_ReadByte(&bit8);
        if (data < 0)
        {
            return IMFC_Timeout;
        }
        if (response != NULL)
        {
            response[i] = (unsigned char)data;
        }
    }

    return IMFC_Ok;
}

/*---------------------------------------------------------------------
   Function: IMFC_DiscardPendingMessages

   Discards any pending messages from the card (acknowledgements,
   error reports) to keep the communication stream clean.
---------------------------------------------------------------------*/
static void IMFC_DiscardPendingMessages(void)
{
    int data;
    int bit8;
    unsigned count;

    /* Drain any pending data for a short time */
    count = 100;
    while (count > 0)
    {
        /* Check if data is ready without blocking */
        {
            int status;
            unsigned flags;

            flags = DisableInterrupts();
            status = inp(IMFC_BaseAddr + IMFC_PIU2);
            RestoreInterrupts(flags);

            if (!(status & IMFC_PIU2_RXRDY))
            {
                break;  /* No more data */
            }
        }

        data = IMFC_ReadByte(&bit8);
        if (data < 0)
        {
            break;
        }
        count--;
    }
}

/*---------------------------------------------------------------------
   Function: IMFC_InitPIU

   Initializes the Parallel Interface Unit on the card.
---------------------------------------------------------------------*/
static void IMFC_InitPIU(void)
{
    /* Send INIT command to PCR */
    outp(IMFC_BaseAddr + IMFC_PCR, IMFC_PCR_INIT);

    /* Small delay to let the PIU initialize */
    {
        unsigned i;
        for (i = 0; i < 100; i++)
        {
            /* brief delay */
        }
    }
}

/*---------------------------------------------------------------------
   Function: IMFC_SetMusicMode

   Sets the card to MUSIC mode (sound generation mode).
   Default is MUSIC mode, but we set it explicitly.
---------------------------------------------------------------------*/
void IMFC_SetMusicMode(void)
{
    unsigned char mode_data;

    /* Command: 1E0, Data: 00 (MUSIC mode) */
    mode_data = IMFC_MODE_MUSIC;
    IMFC_SendCommand(IMFC_CMD_SELECT_MODE, &mode_data, 1);
}

/*---------------------------------------------------------------------
   Function: IMFC_SetErrorReporting

   Enables or disables error reporting from the card.
   enabled = 1 to enable, 0 to disable
---------------------------------------------------------------------*/
static void IMFC_SetErrorReporting(int enabled)
{
    unsigned char err_data;

    err_data = enabled ? IMFC_ERR_REPORT_ENABLE : IMFC_ERR_REPORT_DISABLE;
    IMFC_SendCommand(IMFC_CMD_SELECT_ERR_REPORT, &err_data, 1);

    /* Discard the acknowledgement */
    IMFC_DiscardPendingMessages();
}

/*---------------------------------------------------------------------
   Function: IMFC_SetPaths

   Configures the MIDI message routing paths on the card.
   Sets System->Sound Processor to accept all MIDI messages.

   Path values of 0x7F are used to accept all MIDI message types.
   While the manual documents only 5 bits per path byte, the card
   requires 0x7F (not 0x1F) to properly route all channel messages
   across all 8 voices.
---------------------------------------------------------------------*/
void IMFC_SetPaths(void)
{
    /*
     * Set Paths command (1E2) takes 5 data bytes:
     *   Byte 0: MIDI IN -> System path
     *   Byte 1: System -> MIDI OUT path
     *   Byte 2: MIDI IN -> Sound Processor path
     *   Byte 3: System -> Sound Processor path
     *   Byte 4: MIDI IN -> MIDI OUT path (THRU)
     *
     * Each byte has 5 bits (a=note, b=aftertouch/pitchbend,
     * c=ctrl/program, d=sysex/common, e=realtime)
     *
     * 0x7F is used for "accept all" to match the working reference
     * implementation. The card hardware requires this value for
     * proper multi-voice operation.
     */
    unsigned char paths[5];

    /* MIDI IN -> System: blocked */
    paths[0] = 0x00;

    /* System -> MIDI OUT: accept all (required for proper operation) */
    paths[1] = 0x7F;

    /* MIDI IN -> Sound Processor: blocked (we don't use external MIDI) */
    paths[2] = 0x00;

    /* System -> Sound Processor: accept all (our primary path for playback) */
    paths[3] = 0x7F;

    /* MIDI IN -> MIDI OUT: blocked */
    paths[4] = 0x00;

    IMFC_SendCommand(IMFC_CMD_SET_PATHS, paths, 5);
}

/*---------------------------------------------------------------------
   Function: IMFC_SetNodeParameters

   Sets the basic node parameters for the card.
   Uses configuration 17 (MONO 8) so that each of the 8 instruments
   responds to a different MIDI channel, enabling multi-channel playback.

   Preset configurations (stored in ROM):
     16 = SINGLE  (1 instrument, 8-note poly, 1 channel)
     17 = MONO 8  (8 instruments, 1-note mono, 8 channels)
     18 = DUAL    (split into 2 parts)
     19 = SPLIT   (split into 4 parts)

   Configuration 16 is the power-on default but only responds to one
   MIDI channel. Configuration 17 is required for multi-channel MIDI.
---------------------------------------------------------------------*/
static void IMFC_SetNodeParameters(void)
{
    /*
     * Set Node Parameters command (1E3) takes 8 data bytes:
     *   Byte 0: Node number (0-15)
     *   Byte 1: Memory protect (0=off, 1=on)
     *   Byte 2: Configuration number (0-19)
     *   Byte 3: Master tune (-64 to +63, 7-bit 2's complement)
     *   Byte 4: Master output level (0-127)
     *   Byte 5: CHAIN mode (0=off, 1=on)
     *   Byte 6: Reserved (0)
     *   Byte 7: Reserved (0)
     */
    unsigned char node_params[8];

    node_params[0] = IMFC_DEFAULT_NODE;     /* Node number 0 */
    node_params[1] = 0;                     /* Memory protect off */
    node_params[2] = 17;                    /* Configuration 17 (MONO 8) */
    node_params[3] = 0;                     /* Master tune: center (0) */
    node_params[4] = IMFC_DEFAULT_VOLUME;   /* Master volume: max */
    node_params[5] = 0;                     /* CHAIN mode: off */
    node_params[6] = 0;                     /* Reserved */
    node_params[7] = 0;                     /* Reserved */

    IMFC_SendCommand(IMFC_CMD_SET_NODE, node_params, 8);
}

/*---------------------------------------------------------------------
   Function: IMFC_Reset

   Resets the card to power-on defaults (except PIU state).
---------------------------------------------------------------------*/
void IMFC_Reset(void)
{
    /* Send reboot command (1E5, no data bytes) */
    IMFC_SendCommand(IMFC_CMD_REBOOT, NULL, 0);

    /* Small delay after reboot */
    {
        unsigned i;
        for (i = 0; i < 1000; i++)
        {
            /* brief delay */
        }
    }

    /* Reinitialize after reset */
    IMFC_InitPIU();
}

/*---------------------------------------------------------------------
   Function: IMFC_SetMasterVolume

   Sets the master output level of the card (0-127).
   Uses a SysEx node parameter change message.
---------------------------------------------------------------------*/
void IMFC_SetMasterVolume(int volume)
{
    /*
     * Use System Exclusive message to change master output level.
     * Format: F0 43 75 0s 10 24 vv F7
     *   F0  - SysEx start
     *   43  - Yamaha manufacturer ID
     *   75  - Sub-status (node address)
     *   0s  - Node number (s = 0-15, so 0x00 for node 0)
     *   10  - Message number (node parameter change)
     *   24  - Parameter number (master output level)
     *   vv  - Volume value (0-127)
     *   F7  - SysEx end
     */
    unsigned char sysex[8];

    if (volume > 127)
    {
        volume = 127;
    }
    if (volume < 0)
    {
        volume = 0;
    }

    sysex[0] = 0xF0;                       /* SysEx start */
    sysex[1] = IMFC_SYSEx_YAMAHA_ID;       /* Yamaha ID */
    sysex[2] = 0x75;                       /* Node sub-status */
    sysex[3] = (unsigned char)(IMFC_DEFAULT_NODE & 0x0F);  /* Node number */
    sysex[4] = 0x10;                       /* Node parameter change */
    sysex[5] = 0x24;                       /* Parameter: master output level */
    sysex[6] = (unsigned char)volume;      /* Volume value */
    sysex[7] = 0xF7;                       /* SysEx end */

    /* Send as MIDI data (bit 8 = 0) */
    {
        int i;
        for (i = 0; i < 8; i++)
        {
            IMFC_SendMidi(sysex[i]);
        }
    }

    IMFC_Volume = volume;
}

/*---------------------------------------------------------------------
   Function: IMFC_Detect

   Attempts to detect an IBM PC Music Feature card at the given address
   using a hardware-level test: write known values to the card and
   verify they are stored correctly.
   Returns IMFC_Ok if detected, IMFC_NotFound otherwise.
---------------------------------------------------------------------*/
int IMFC_Detect(int addr)
{
    /* Initialize PIU */
    outp(addr + IMFC_PCR, IMFC_PCR_INIT);

    /* Set the IMFC to COMMAND mode (bit 8 = 1) */
    outp(addr + IMFC_TCR, IMFC_TCR_EXT8);

    /* Write 0xFF to PIU1 and verify it "sticks" */
    outp(addr + IMFC_PIU1, 0xFF);

    /* Read PIU0 to allow the write to settle (acts as clock edge) */
    inp(addr + IMFC_PIU0);

    /* Check if the value stuck */
    if (inp(addr + IMFC_PIU1) != 0xFF)
    {
        return IMFC_NotFound;
    }

    /* Write 0x00 to PIU1 and verify it "sticks" */
    outp(addr + IMFC_PIU1, 0);

    /* Read PIU0 to allow the write to settle */
    inp(addr + IMFC_PIU0);

    /* Check if the value stuck */
    if (inp(addr + IMFC_PIU1) != 0)
    {
        return IMFC_NotFound;
    }

    return IMFC_Ok;
}

/*---------------------------------------------------------------------
   Function: IMFC_Init

   Initializes the IBM PC Music Feature card.
   addr = base I/O address (0x2A20 or 0x2A30)
   Returns IMFC_Ok on success.

   Initialization sequence matches the working reference implementation:
   1. Initialize PIU (done by IMFC_Detect)
   2. Set Write Interrupt Enable
   3. Set MUSIC mode
   4. Set MIDI routing paths

   Commands are sent back-to-back without waiting for acknowledgments
   between them, as the card's FIFO handles buffering internally.
---------------------------------------------------------------------*/
int IMFC_Init(int addr)
{
    int detected;

    IMFC_BaseAddr = addr;

    /* Detect the card (this also initializes PIU) */
    detected = IMFC_Detect(addr);
    if (detected != IMFC_Ok && !ignoreSoundChecks)
    {
        return IMFC_NotFound;
    }

    /* Set Write Interrupt Enable */
    outp(IMFC_BaseAddr + IMFC_PCR, IMFC_PCR_SET_WIE);

    /* Set to MUSIC mode (commands sent back-to-back per reference) */
    IMFC_SetMusicMode();

    /* Disable error reporting */
    IMFC_SetErrorReporting(0);

    /* Set node parameters (configuration 17 = MONO 8 for multi-channel) */
    IMFC_SetNodeParameters();

    /* Set MIDI paths for playback (System -> SP must accept all messages) */
    IMFC_SetPaths();

    /* Set initial volume */
    IMFC_Volume = 127;
    IMFC_SetMasterVolume(IMFC_Volume);

    return IMFC_Ok;
}

/*---------------------------------------------------------------------
   Function: IMFC_Shutdown

   Shuts down the IBM PC Music Feature card.
---------------------------------------------------------------------*/
void IMFC_Shutdown(void)
{
    /* Turn off all sounds */
    IMFC_AllNotesOff();

    /* Set volume to 0 */
    IMFC_SetMasterVolume(0);
}

/*---------------------------------------------------------------------
   Function: IMFC_NoteOff

   Sends a Note OFF MIDI message to stop a note.
   channel = MIDI channel (0-15)
   key     = note number (0-127)
   velocity = release velocity (0-127, typically ignored)
---------------------------------------------------------------------*/
void IMFC_NoteOff(int channel, int key, int velocity)
{
    IMFC_SendMidi(IMFC_MIDI_NOTE_OFF | (channel & 0x0F));
    IMFC_SendMidi(key & 0x7F);
    IMFC_SendMidi(velocity & 0x7F);
}

/*---------------------------------------------------------------------
   Function: IMFC_NoteOn

   Sends a Note ON MIDI message to start a note.
   channel = MIDI channel (0-15)
   key     = note number (0-127)
   velocity = key velocity (0-127, 0 = note off)
---------------------------------------------------------------------*/
void IMFC_NoteOn(int channel, int key, int velocity)
{
    /* Velocity of 0 means note off on the IBM Music Feature */
    if (velocity == 0)
    {
        IMFC_SendMidi(IMFC_MIDI_NOTE_OFF | (channel & 0x0F));
        IMFC_SendMidi(key & 0x7F);
        IMFC_SendMidi(0);
    }
    else
    {
        IMFC_SendMidi(IMFC_MIDI_NOTE_ON | (channel & 0x0F));
        IMFC_SendMidi(key & 0x7F);
        IMFC_SendMidi(velocity & 0x7F);
    }
}

/*---------------------------------------------------------------------
   Function: IMFC_PolyAftertouch

   Sends a Polyphonic Aftertouch MIDI message.
   channel = MIDI channel (0-15)
   key     = note number (0-127)
   pressure = aftertouch pressure (0-127)
---------------------------------------------------------------------*/
void IMFC_PolyAftertouch(int channel, int key, int pressure)
{
    IMFC_SendMidi(IMFC_MIDI_POLY_AFTER_TOUCH | (channel & 0x0F));
    IMFC_SendMidi(key & 0x7F);
    IMFC_SendMidi(pressure & 0x7F);
}

/*---------------------------------------------------------------------
   Function: IMFC_ControlChange

   Sends a Control Change MIDI message.
   channel = MIDI channel (0-15)
   number  = controller number (0-127)
   value   = controller value (0-127)
---------------------------------------------------------------------*/
void IMFC_ControlChange(int channel, int number, int value)
{
    /*
     * The IBM PC Music Feature supports these controllers:
     *   1  - Modulation wheel
     *   2  - Breath controller
     *   4  - Foot controller
     *   5  - Portamento time
     *   17 - Volume
     *   10 - Pan control
     *   64 - Sustain ON/OFF
     *   65 - Portamento ON/OFF
     *   66 - Sostenuto ON/OFF
     *   79 - All Sound Off
     *   121 - All Notes Off
     *   123 - All Notes Off (alternate)
     *   126 - Mono/Poly mode
     *   127 - Poly mode
     */
    IMFC_SendMidi(IMFC_MIDI_CONTROL_CHANGE | (channel & 0x0F));
    IMFC_SendMidi(number & 0x7F);
    IMFC_SendMidi(value & 0x7F);
}

/*---------------------------------------------------------------------
   Function: IMFC_ProgramChange

   Sends a Program Change (voice select) MIDI message.
   channel = MIDI channel (0-15)
   program = program/voice number (0-127)
---------------------------------------------------------------------*/
void IMFC_ProgramChange(int channel, int program)
{
    IMFC_SendMidi(IMFC_MIDI_PROGRAM_CHANGE | (channel & 0x0F));
    IMFC_SendMidi(program & 0x7F);
}

/*---------------------------------------------------------------------
   Function: IMFC_ChannelAftertouch

   Sends a Channel Aftertouch MIDI message.
   channel = MIDI channel (0-15)
   pressure = aftertouch pressure (0-127)
---------------------------------------------------------------------*/
void IMFC_ChannelAftertouch(int channel, int pressure)
{
    IMFC_SendMidi(IMFC_MIDI_AFTER_TOUCH | (channel & 0x0F));
    IMFC_SendMidi(pressure & 0x7F);
}

/*---------------------------------------------------------------------
   Function: IMFC_PitchBend

   Sends a Pitch Bend MIDI message.
   channel = MIDI channel (0-15)
   lsb     = pitch bend value low byte (0-127)
   msb     = pitch bend value high byte (0-127)
---------------------------------------------------------------------*/
void IMFC_PitchBend(int channel, int lsb, int msb)
{
    IMFC_SendMidi(IMFC_MIDI_PITCH_BEND | (channel & 0x0F));
    IMFC_SendMidi(lsb & 0x7F);
    IMFC_SendMidi(msb & 0x7F);
}

/*---------------------------------------------------------------------
   Function: IMFC_SysEx

   Sends a System Exclusive MIDI message.
   ptr    = pointer to SysEx data (without F0/F7 framing)
   length = number of data bytes
---------------------------------------------------------------------*/
void IMFC_SysEx(unsigned char *ptr, int length)
{
    int i;

    /* SysEx start */
    IMFC_SendMidi(IMFC_MIDI_SYSEX_START);

    /* Data bytes */
    for (i = 0; i < length; i++)
    {
        IMFC_SendMidi(ptr[i] & 0x7F);
    }

    /* SysEx end */
    IMFC_SendMidi(IMFC_MIDI_SYSEX_END);
}

/**
 * Establece un instrumento General MIDI en un canal determinado para IBM PCMF.
 * 
 * @param channel Canal MIDI (0 a 15).
 * @param gm_program Programa General MIDI (0 a 127 o 1 a 128 según tu parser).
 */
void IMFC_SetGMProgram(int channel, int gm_program)
{
    unsigned char target_bank;
    unsigned char target_voice;
    unsigned char sysex_bank_change[5];

    // Normalizar si tu software lee GM de 1 a 128
    // gm_program = gm_program - 1;

    // Asegurar límites dentro del rango estándar MIDI
    if (gm_program < 0 || gm_program > 127) return;

    // Obtener configuración mapeada de la tabla
    target_bank  = gm_to_ibm_table[gm_program].bank;
    target_voice = gm_to_ibm_table[gm_program].voice;

    /* 1. Construir trama SysEx para cambiar el banco del canal:
       - 0x43: ID Fabricante Yamaha
       - 0x10 | channel: Sub-estatus del canal MIDI (0x10 a 0x1F)
       - 0x15: Grupo de parámetros de instrumento
       - 0x04: Dirección del parámetro (Voice Bank Number)
       - target_bank: Banco destino (0x02 - 0x06)
    */
    sysex_bank_change[0] = 0x43;
    sysex_bank_change[1] = 0x10 | (channel & 0x0F);
    sysex_bank_change[2] = 0x15;
    sysex_bank_change[3] = 0x04;
    sysex_bank_change[4] = target_bank;

    // Transmitir SysEx de selección de banco (incluye SysEx Start/End automáticamente)
    IMFC_SysEx(sysex_bank_change, 5);

    /* 2. Seleccionar la voz específica dentro del banco asignado */
    IMFC_ProgramChange(channel, target_voice);
}

/*---------------------------------------------------------------------
   Function: IMFC_SetVolume

   Sets the music volume via the card's master output level.
   volume = 0-255 (mapped to card's 0-127 range)
---------------------------------------------------------------------*/
void IMFC_SetVolume(int volume)
{
    int cardvolume;

    if (volume > 255)
    {
        volume = 255;
    }
    if (volume < 0)
    {
        volume = 0;
    }

    /* Map 0-255 to 0-127 */
    cardvolume = volume >> 1;

    IMFC_Volume = cardvolume;
    IMFC_SetMasterVolume(cardvolume);
}

/*---------------------------------------------------------------------
   Function: IMFC_GetVolume

   Returns the current volume setting (0-255).
---------------------------------------------------------------------*/
int IMFC_GetVolume(void)
{
    return IMFC_Volume << 1;  /* Map 0-127 back to 0-255 */
}

/*---------------------------------------------------------------------
   Function: IMFC_AllNotesOff

   Sends All Notes Off to all 16 MIDI channels.
   Also sends All Sound Off and Reset All Controllers for safety.
---------------------------------------------------------------------*/
void IMFC_AllNotesOff(void)
{
    int channel;

    for (channel = 0; channel < 16; channel++)
    {
        /* Sustain OFF */
        IMFC_SendMidi(IMFC_MIDI_CONTROL_CHANGE | channel);
        IMFC_SendMidi(0x40);  /* Sustain pedal */
        IMFC_SendMidi(0x00);

        /* All Notes Off */
        IMFC_SendMidi(IMFC_MIDI_CONTROL_CHANGE | channel);
        IMFC_SendMidi(0x7B);  /* All Notes Off */
        IMFC_SendMidi(0x00);

        /* All Sound Off */
        IMFC_SendMidi(IMFC_MIDI_CONTROL_CHANGE | channel);
        IMFC_SendMidi(0x78);  /* All Sound Off */
        IMFC_SendMidi(0x00);
    }
}
