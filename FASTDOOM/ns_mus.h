#ifndef __MUS_H
#define __MUS_H

#include "ns_midi.h"

/* MUS event command codes (bits 5-3 of the event descriptor) */

#define MUS_KEYRELEASE    0x00
#define MUS_KEYPRESS      0x10
#define MUS_PITCHWHEEL    0x20
#define MUS_SYSTEMEVENT   0x30
#define MUS_CONTROLLER    0x40
#define MUS_ENDOFMEASURE  0x50
#define MUS_SCOREEND      0x60

/* MUS system event numbers */

#define MUS_ALL_SOUNDS_OFF    10
#define MUS_ALL_NOTES_OFF     11
#define MUS_MONO_MODE         12
#define MUS_POLY_MODE         13
#define MUS_RESET_CONTROLLERS 14

/* MUS event command of the unused (7th) command */

#define MUS_UNUSED            0x70

/* Returns TRUE if the given data is a MUS module. */
int NMUS_IsMusFile(unsigned char *data);

/* Begins playback of a MUS module using the given device functions. */
int NMUS_PlaySong(unsigned char *song, int loop, midifuncs *funcs);

/* Sets the tempo of the current MUS module (in BPM).  The MUS
   time division is fixed (140 Hz in the Doom engine), so this
   only stores the tempo. */
void NMUS_SetTempo(int tempo);

/* Stops playback of the current MUS module. */
void NMUS_StopSong(void);

/* Pauses playback of the current MUS module. */
void NMUS_PauseSong(void);

/* Continues playback of a paused MUS module. */
void NMUS_ContinueSong(void);

/* Returns whether a MUS module is playing. */
int NMUS_SongPlaying(void);

#endif
