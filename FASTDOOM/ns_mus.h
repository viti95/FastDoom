//
// Native DOOM MUS music format player.
// Directly interprets MUS event streams and dispatches MIDI messages
// through the midifuncs interface. No MIDI conversion required.
//
// MUS format runs at a fixed 70-tick-per-second clock.
//

#ifndef __NS_MUS_H
#define __NS_MUS_H

#include "doomtype.h"

// Initialize the MUS player with raw MUS data.
// data: pointer to MUS lump data
// length: total size of the MUS lump (in bytes)
// Returns 0 on success, non-zero on failure.
int MUS_InitPlayer(void *data, unsigned int length);

// Start/continue playback of the loaded MUS song.
void MUS_Play(void);

// Pause playback.
void MUS_Pause(void);

// Stop playback and free resources.
void MUS_Stop(void);

// Returns non-zero if a MUS song is currently playing.
int MUS_IsPlaying(void);

// Set loop flag (0 = no loop, non-zero = loop).
void MUS_PlayerSetLoop(int loopflag);

#endif /* __NS_MUS_H */
