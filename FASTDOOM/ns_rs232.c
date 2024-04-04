#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include "ns_dpmi.h"
#include "ns_user.h"
#include "ns_rs232.h"
#include "options.h"
#include "ns_inter.h"

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

#define SERIAL_BASE_IO 0x3F8
#define SERIAL_BAUD_RATE 38400

/*---------------------------------------------------------------------
   Function: RS232_SendMidi

   Sends a byte of MIDI data to the music device.
---------------------------------------------------------------------*/

void RS232_SendMidi(int data)
{
    unsigned flags;
    int status;

    flags = DisableInterrupts();

    // Yuck, busy wait. Install an interrupt with DPMI?
    do
    {
        status = inp(SERIAL_BASE_IO + 5);
    } while ((status & 0x20) == 0);
    outp(SERIAL_BASE_IO, data);

    RestoreInterrupts(flags);
}

/*---------------------------------------------------------------------
   Function: RS232_NoteOff

   Sends a full MIDI note off event out to the music device.
---------------------------------------------------------------------*/

void RS232_NoteOff(int channel, int key, int velocity)
{
    RS232_SendMidi(MIDI_NOTE_OFF | channel);
    RS232_SendMidi(key);
    RS232_SendMidi(velocity);
}

/*---------------------------------------------------------------------
   Function: RS232_NoteOn

   Sends a full MIDI note on event out to the music device.
---------------------------------------------------------------------*/

void RS232_NoteOn(int channel, int key, int velocity)
{
    RS232_SendMidi(MIDI_NOTE_ON | channel);
    RS232_SendMidi(key);
    RS232_SendMidi(velocity);
}

/*---------------------------------------------------------------------
   Function: RS232_PolyAftertouch

   Sends a full MIDI polyphonic aftertouch event out to the music device.
---------------------------------------------------------------------*/

void RS232_PolyAftertouch(int channel, int key, int pressure)
{
    RS232_SendMidi(MIDI_POLY_AFTER_TCH | channel);
    RS232_SendMidi(key);
    RS232_SendMidi(pressure);
}

/*---------------------------------------------------------------------
   Function: RS232_ControlChange

   Sends a full MIDI control change event out to the music device.
---------------------------------------------------------------------*/

void RS232_ControlChange(int channel, int number, int value)
{
    RS232_SendMidi(MIDI_CONTROL_CHANGE | channel);
    RS232_SendMidi(number);
    RS232_SendMidi(value);
}

/*---------------------------------------------------------------------
   Function: RS232_ProgramChange

   Sends a full MIDI program change event out to the music device.
---------------------------------------------------------------------*/

void RS232_ProgramChange(int channel, int program)
{
    RS232_SendMidi(MIDI_PROGRAM_CHANGE | channel);
    RS232_SendMidi(program);
}

/*---------------------------------------------------------------------
   Function: RS232_ChannelAftertouch

   Sends a full MIDI channel aftertouch event out to the music device.
---------------------------------------------------------------------*/

void RS232_ChannelAftertouch(int channel, int pressure)
{
    RS232_SendMidi(MIDI_AFTER_TOUCH | channel);
    RS232_SendMidi(pressure);
}

/*---------------------------------------------------------------------
   Function: RS232_PitchBend

   Sends a full MIDI pitch bend event out to the music device.
---------------------------------------------------------------------*/

void RS232_PitchBend(int channel, int lsb, int msb)
{
    RS232_SendMidi(MIDI_PITCH_BEND | channel);
    RS232_SendMidi(lsb);
    RS232_SendMidi(msb);
}

int RS232_Reset()
{
    outp(SERIAL_BASE_IO + 1, 0x00);                      // Disable all interrupts
    outp(SERIAL_BASE_IO + 3, 0x80);                      // Enable the baud rate divisor
    outp(SERIAL_BASE_IO + 0, 115200 / SERIAL_BAUD_RATE); // Set the baud rate
    outp(SERIAL_BASE_IO + 1, 0x00);                      // Hi byte
    outp(SERIAL_BASE_IO + 3, 0x03);                      // 8 bits, no parity, one stop bit
    return RS232_Ok;
}

/*---------------------------------------------------------------------
   Function: RS232_Init

   Detects and initializes the music card.
---------------------------------------------------------------------*/

int RS232_Init()
{
    RS232_Reset();
    return RS232_Ok;
}
