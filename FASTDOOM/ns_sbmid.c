#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include "ns_dpmi.h"
#include "ns_user.h"
#include "ns_sbmid.h"
#include "ns_sb.h"
#include "ns_sbdef.h"
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

/*---------------------------------------------------------------------
   Function: SBMIDI_SendMidi

   Sends a byte of MIDI data to the music device.
---------------------------------------------------------------------*/

void SBMIDI_SendMidi(int data)
{
    unsigned flags;

    flags = DisableInterrupts();
    BLASTER_WriteDSP(DSP_MIDIWritePoll);
    BLASTER_WriteDSP(data);
    RestoreInterrupts(flags);
}

/*---------------------------------------------------------------------
   Function: SBMIDI_NoteOff

   Sends a full MIDI note off event out to the music device.
---------------------------------------------------------------------*/

void SBMIDI_NoteOff(int channel, int key, int velocity)
{
    SBMIDI_SendMidi(MIDI_NOTE_OFF | channel);
    SBMIDI_SendMidi(key);
    SBMIDI_SendMidi(velocity);
}

/*---------------------------------------------------------------------
   Function: SBMIDI_NoteOn

   Sends a full MIDI note on event out to the music device.
---------------------------------------------------------------------*/

void SBMIDI_NoteOn(int channel, int key, int velocity)
{
    SBMIDI_SendMidi(MIDI_NOTE_ON | channel);
    SBMIDI_SendMidi(key);
    SBMIDI_SendMidi(velocity);
}

/*---------------------------------------------------------------------
   Function: SBMIDI_PolyAftertouch

   Sends a full MIDI polyphonic aftertouch event out to the music device.
---------------------------------------------------------------------*/

void SBMIDI_PolyAftertouch(int channel, int key, int pressure)
{
    SBMIDI_SendMidi(MIDI_POLY_AFTER_TCH | channel);
    SBMIDI_SendMidi(key);
    SBMIDI_SendMidi(pressure);
}

/*---------------------------------------------------------------------
   Function: SBMIDI_ControlChange

   Sends a full MIDI control change event out to the music device.
---------------------------------------------------------------------*/

void SBMIDI_ControlChange(int channel, int number, int value)
{
    SBMIDI_SendMidi(MIDI_CONTROL_CHANGE | channel);
    SBMIDI_SendMidi(number);
    SBMIDI_SendMidi(value);
}

/*---------------------------------------------------------------------
   Function: SBMIDI_ProgramChange

   Sends a full MIDI program change event out to the music device.
---------------------------------------------------------------------*/

void SBMIDI_ProgramChange(int channel, int program)
{
    SBMIDI_SendMidi(MIDI_PROGRAM_CHANGE | channel);
    SBMIDI_SendMidi(program);
}

/*---------------------------------------------------------------------
   Function: SBMIDI_ChannelAftertouch

   Sends a full MIDI channel aftertouch event out to the music device.
---------------------------------------------------------------------*/

void SBMIDI_ChannelAftertouch(int channel, int pressure)
{
    SBMIDI_SendMidi(MIDI_AFTER_TOUCH | channel);
    SBMIDI_SendMidi(pressure);
}

/*---------------------------------------------------------------------
   Function: SBMIDI_PitchBend

   Sends a full MIDI pitch bend event out to the music device.
---------------------------------------------------------------------*/

void SBMIDI_PitchBend(int channel, int lsb, int msb)
{
    SBMIDI_SendMidi(MIDI_PITCH_BEND | channel);
    SBMIDI_SendMidi(lsb);
    SBMIDI_SendMidi(msb);
}

void SBMIDI_SysEx(unsigned char *ptr, int length)
{
    int c;

    SBMIDI_SendMidi(0xF0);

    for (c=0; c<length; c++)
    {
        SBMIDI_SendMidi(*(ptr));
        ptr++;
    }

    SBMIDI_SendMidi(0xF7);
}

/*---------------------------------------------------------------------
   Function: SBMIDI_Reset

   Resets the music card.
---------------------------------------------------------------------*/

int SBMIDI_Reset(void)
{
    SBMIDI_SendMidi(SBMIDI_CmdReset);
    return SBMIDI_Ok;
}

/*---------------------------------------------------------------------
   Function: SBMIDI_Init

   Detects and initializes the music card.
---------------------------------------------------------------------*/

int SBMIDI_Init()
{
    SBMIDI_Reset();
    return SBMIDI_Ok;
}
