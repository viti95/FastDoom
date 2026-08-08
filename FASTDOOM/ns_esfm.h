#ifndef __ESFM_H
#define __ESFM_H

// Native ESFM synthesizer driver for ESS AudioDrive cards.
// Uses the chip's native mode (18 four-op channels, per-operator registers)
// instead of OPL3 emulation. See ns_sbmus.c for the OPL/Adlib path.

enum ESFM_Errors
{
    ESFM_Warning = -2,
    ESFM_Error = -1,
    ESFM_Ok = 0,
};

extern int ESFM_PORT;

int ESFM_Init(int Address);
void ESFM_Shutdown(void);
int ESFM_DetectFM(void);

void ESFM_NoteOff(int channel, int key, int velocity);
void ESFM_NoteOn(int channel, int key, int velocity);
void ESFM_ControlChange(int channel, int number, int value);
void ESFM_ProgramChange(int channel, int program);
void ESFM_SetPitchBend(int channel, int lsb, int msb);

// Installs a raw ESS patch-table blob (offset table + patches). If never
// called, the built-in ESFM_DefaultBank is used.
void ESFM_RegisterBank(const unsigned char *bank);

#endif
