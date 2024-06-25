#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include "ns_dpmi.h"
#include "ns_user.h"
#include "ns_lptmusic.h"
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

#define SERIAL_BAUD_RATE 38400

int LPTMIDI_BaseAddr = LPTMIDI_DefaultAddress;

/*---------------------------------------------------------------------
   Function: LPTMIDI_SendMidi

   Sends a byte of MIDI data to the music device.
---------------------------------------------------------------------*/

void LPTMIDI_SendMidi(int data)
{
    unsigned flags;
    int status;

    flags = DisableInterrupts();

    // Yuck, busy wait. Install an interrupt with DPMI?
    do
    {
        status = inp(LPTMIDI_BaseAddr + 5);
    } while ((status & 0x20) == 0);
    outp(LPTMIDI_BaseAddr, data);

    RestoreInterrupts(flags);
}

/*---------------------------------------------------------------------
   Function: LPTMIDI_NoteOff

   Sends a full MIDI note off event out to the music device.
---------------------------------------------------------------------*/

void LPTMIDI_NoteOff(int channel, int key, int velocity)
{
    LPTMIDI_SendMidi(MIDI_NOTE_OFF | channel);
    LPTMIDI_SendMidi(key);
    LPTMIDI_SendMidi(velocity);
}

/*---------------------------------------------------------------------
   Function: LPTMIDI_NoteOn

   Sends a full MIDI note on event out to the music device.
---------------------------------------------------------------------*/

void LPTMIDI_NoteOn(int channel, int key, int velocity)
{
    LPTMIDI_SendMidi(MIDI_NOTE_ON | channel);
    LPTMIDI_SendMidi(key);
    LPTMIDI_SendMidi(velocity);
}

/*---------------------------------------------------------------------
   Function: LPTMIDI_PolyAftertouch

   Sends a full MIDI polyphonic aftertouch event out to the music device.
---------------------------------------------------------------------*/

void LPTMIDI_PolyAftertouch(int channel, int key, int pressure)
{
    LPTMIDI_SendMidi(MIDI_POLY_AFTER_TCH | channel);
    LPTMIDI_SendMidi(key);
    LPTMIDI_SendMidi(pressure);
}

/*---------------------------------------------------------------------
   Function: LPTMIDI_ControlChange

   Sends a full MIDI control change event out to the music device.
---------------------------------------------------------------------*/

void LPTMIDI_ControlChange(int channel, int number, int value)
{
    LPTMIDI_SendMidi(MIDI_CONTROL_CHANGE | channel);
    LPTMIDI_SendMidi(number);
    LPTMIDI_SendMidi(value);
}

/*---------------------------------------------------------------------
   Function: LPTMIDI_ProgramChange

   Sends a full MIDI program change event out to the music device.
---------------------------------------------------------------------*/

void LPTMIDI_ProgramChange(int channel, int program)
{
    LPTMIDI_SendMidi(MIDI_PROGRAM_CHANGE | channel);
    LPTMIDI_SendMidi(program);
}

/*---------------------------------------------------------------------
   Function: LPTMIDI_ChannelAftertouch

   Sends a full MIDI channel aftertouch event out to the music device.
---------------------------------------------------------------------*/

void LPTMIDI_ChannelAftertouch(int channel, int pressure)
{
    LPTMIDI_SendMidi(MIDI_AFTER_TOUCH | channel);
    LPTMIDI_SendMidi(pressure);
}

/*---------------------------------------------------------------------
   Function: LPTMIDI_PitchBend

   Sends a full MIDI pitch bend event out to the music device.
---------------------------------------------------------------------*/

void LPTMIDI_PitchBend(int channel, int lsb, int msb)
{
    LPTMIDI_SendMidi(MIDI_PITCH_BEND | channel);
    LPTMIDI_SendMidi(lsb);
    LPTMIDI_SendMidi(msb);
}

int LPTMIDI_Reset()
{
    outp(LPTMIDI_BaseAddr + 1, 0x00);                      // Disable all interrupts
    outp(LPTMIDI_BaseAddr + 3, 0x80);                      // Enable the baud rate divisor
    outp(LPTMIDI_BaseAddr + 0, 115200 / SERIAL_BAUD_RATE); // Set the baud rate
    outp(LPTMIDI_BaseAddr + 1, 0x00);                      // Hi byte
    outp(LPTMIDI_BaseAddr + 3, 0x03);                      // 8 bits, no parity, one stop bit
    return LPTMIDI_Ok;
}

/*---------------------------------------------------------------------
   Function: LPTMIDI_Init

   Detects and initializes the music card.
---------------------------------------------------------------------*/

int LPTMIDI_Init(int addr)
{
    LPTMIDI_BaseAddr = addr;

    LPTMIDI_Reset();
    return LPTMIDI_Ok;
}
