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

    /* Command: 1E0, Data: 100 (MUSIC mode) */
    mode_data = IMFC_MODE_MUSIC;
    IMFC_SendCommand(IMFC_CMD_SELECT_MODE, &mode_data, 1);

    /* Discard the acknowledgement */
    IMFC_DiscardPendingMessages();
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
     */
    unsigned char paths[5];

    /* MIDI IN -> System: blocked */
    paths[0] = IMFC_PATH_NONE;

    /* System -> MIDI OUT: blocked (we play internally) */
    paths[1] = IMFC_PATH_NONE;

    /* MIDI IN -> Sound Processor: accept all (for external MIDI) */
    paths[2] = IMFC_PATH_ALL;

    /* System -> Sound Processor: accept all (our primary path) */
    paths[3] = IMFC_PATH_ALL;

    /* MIDI IN -> MIDI OUT: blocked */
    paths[4] = IMFC_PATH_NONE;

    IMFC_SendCommand(IMFC_CMD_SET_PATHS, paths, 5);

    /* Discard the acknowledgement */
    IMFC_DiscardPendingMessages();
}

/*---------------------------------------------------------------------
   Function: IMFC_SetNodeParameters

   Sets the basic node parameters for the card.
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
    node_params[2] = 16;                    /* Configuration 16 (SINGLE preset) */
    node_params[3] = 0;                     /* Master tune: center (0) */
    node_params[4] = IMFC_DEFAULT_VOLUME;    /* Master volume: max */
    node_params[5] = 0;                     /* CHAIN mode: off */
    node_params[6] = 0;                     /* Reserved */
    node_params[7] = 0;                     /* Reserved */

    IMFC_SendCommand(IMFC_CMD_SET_NODE, node_params, 8);

    /* Discard the acknowledgement */
    IMFC_DiscardPendingMessages();
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

    /* Initialize the PIU */
    IMFC_InitPIU();

    /* Set Write Interrupt Enable (matching reference implementation) */
    outp(IMFC_BaseAddr + IMFC_PCR, IMFC_PCR_SET_WIE);

    /* Set to MUSIC mode */
    IMFC_SetMusicMode();

    /* Disable error reporting (we handle errors ourselves) */
    IMFC_SetErrorReporting(0);

    /* Set MIDI paths for playback */
    IMFC_SetPaths();

    /* Set node parameters (configuration, volume, etc.) */
    IMFC_SetNodeParameters();

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
