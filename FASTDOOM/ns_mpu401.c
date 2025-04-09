#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include "ns_dpmi.h"
#include "ns_mpu401.h"
#include "options.h"

#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_ON 0x90
#define MIDI_POLY_AFTER_TCH 0xA0
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_AFTER_TOUCH 0xD0
#define MIDI_PITCH_BEND 0xE0
#define MIDI_META_EVENT 0xFF
#define MIDI_END_OF_TRACK 0x2F
#define MIDI_TEMPO_CHANGE 0x51
#define MIDI_MONO_MODE_ON 0x7E
#define MIDI_ALL_NOTES_OFF 0x7B

int MPU_BaseAddr = MPU_DefaultAddress;

unsigned MPU_Delay = 0x5000;

/*---------------------------------------------------------------------
   Function: MPU_SendMidi

   Sends a byte of MIDI data to the music device.
---------------------------------------------------------------------*/

void MPU_SendMidi(int data)
{
    int port = MPU_BaseAddr + 1;
    unsigned count;

    count = MPU_Delay;
    while (count > 0)
    {
        // check if status port says we're ready for write
        if (!(inp(port) & MPU_ReadyToWrite))
        {
            break;
        }

        count--;
    }

    port--;

    // Send the midi data
    outp(port, data);
}

/*---------------------------------------------------------------------
   Function: MPU_NoteOff

   Sends a full MIDI note off event out to the music device.
---------------------------------------------------------------------*/

void MPU_NoteOff(int channel, int key, int velocity)
{
    MPU_SendMidi(MIDI_NOTE_OFF | channel);
    MPU_SendMidi(key);
    MPU_SendMidi(velocity);
}

/*---------------------------------------------------------------------
   Function: MPU_NoteOn

   Sends a full MIDI note on event out to the music device.
---------------------------------------------------------------------*/

void MPU_NoteOn(int channel, int key, int velocity)
{
    MPU_SendMidi(MIDI_NOTE_ON | channel);
    MPU_SendMidi(key);
    MPU_SendMidi(velocity);
}

/*---------------------------------------------------------------------
   Function: MPU_PolyAftertouch

   Sends a full MIDI polyphonic aftertouch event out to the music device.
---------------------------------------------------------------------*/

void MPU_PolyAftertouch(int channel, int key, int pressure)
{
    MPU_SendMidi(MIDI_POLY_AFTER_TCH | channel);
    MPU_SendMidi(key);
    MPU_SendMidi(pressure);
}

/*---------------------------------------------------------------------
   Function: MPU_ControlChange

   Sends a full MIDI control change event out to the music device.
---------------------------------------------------------------------*/

void MPU_ControlChange(int channel, int number, int value)
{
    MPU_SendMidi(MIDI_CONTROL_CHANGE | channel);
    MPU_SendMidi(number);
    MPU_SendMidi(value);
}

/*---------------------------------------------------------------------
   Function: MPU_ProgramChange

   Sends a full MIDI program change event out to the music device.
---------------------------------------------------------------------*/

void MPU_ProgramChange(int channel, int program)
{
    MPU_SendMidi(MIDI_PROGRAM_CHANGE | channel);
    MPU_SendMidi(program);
}

/*---------------------------------------------------------------------
   Function: MPU_ChannelAftertouch

   Sends a full MIDI channel aftertouch event out to the music device.
---------------------------------------------------------------------*/

void MPU_ChannelAftertouch(int channel, int pressure)
{
    MPU_SendMidi(MIDI_AFTER_TOUCH | channel);
    MPU_SendMidi(pressure);
}

/*---------------------------------------------------------------------
   Function: MPU_PitchBend

   Sends a full MIDI pitch bend event out to the music device.
---------------------------------------------------------------------*/

void MPU_PitchBend(int channel, int lsb, int msb)
{
    MPU_SendMidi(MIDI_PITCH_BEND | channel);
    MPU_SendMidi(lsb);
    MPU_SendMidi(msb);
}

void MPU_SysEx(unsigned char *ptr, int length)
{
    int c;

    MPU_SendMidi(0xF0);

    for (c=0; c<length; c++)
    {
        MPU_SendMidi(*(ptr));
        ptr++;
    }

    MPU_SendMidi(0xF7);
}

/*---------------------------------------------------------------------
   Function: MPU_SendCommand

   Sends a command to the MPU401 card.
---------------------------------------------------------------------*/

void MPU_SendCommand(int data)
{
    int port = MPU_BaseAddr + 1;
    unsigned count;

    count = 0xffff;
    while (count > 0)
    {
        // check if status port says we're ready for write
        if (!(inp(port) & MPU_ReadyToWrite))
        {
            break;
        }
        count--;
    }

    outp(port, data);
}

/*---------------------------------------------------------------------
   Function: MPU_Reset

   Resets the MPU401 card.
---------------------------------------------------------------------*/

int MPU_Reset(void)
{
    int port = MPU_BaseAddr + 1;
    unsigned count;

    // Output "Reset" command via Command port
    MPU_SendCommand(MPU_CmdReset);

    // Wait for status port to be ready for read
    count = 0xffff;
    while (count > 0)
    {
        if (!(inp(port) & MPU_ReadyToRead))
        {
            port--;

            // Check for a successful reset
            if (inp(port) == MPU_CmdAcknowledge)
            {
                return (MPU_Ok);
            }

            port++;
        }
        count--;
    }

    // Failed to reset : MPU-401 not detected
    return (MPU_NotFound);
}

/*---------------------------------------------------------------------
   Function: MPU_EnterUART

   Sets the MPU401 card to operate in UART mode.
---------------------------------------------------------------------*/

int MPU_EnterUART(void)
{
    int port = MPU_BaseAddr + 1;
    unsigned count;

    // Output "Enter UART" command via Command port
    MPU_SendCommand(MPU_CmdEnterUART);

    // Wait for status port to be ready for read
    count = 0xffff;
    while (count > 0)
    {
        if (!(inp(port) & MPU_ReadyToRead))
        {
            port--;

            // Check for a successful reset
            if (inp(port) == MPU_CmdAcknowledge)
            {
                return (MPU_Ok);
            }

            port++;
        }
        count--;
    }

    // Failed to reset : MPU-401 not detected
    return (MPU_UARTFailed);
}

/*---------------------------------------------------------------------
   Function: MPU_Init

   Detects and initializes the MPU401 card.
---------------------------------------------------------------------*/

int MPU_Init(int addr)
{
    int status;
    int count;
    char *ptr;

    MPU_BaseAddr = addr;

    count = 4;
    while (count > 0)
    {
        status = MPU_Reset();
        if (status == MPU_Ok)
        {
            return (MPU_EnterUART());
        }
        count--;
    }

    return (status);
}
