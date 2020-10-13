#include "doomtype.h"

//
// M_Random
// Returns a 0-255 number
//
extern byte rndtable[256];
extern byte rndindex;
extern byte prndindex;

// Which one is deterministic?

// Returns a number from 0 to 255,
// from a lookup table.
#define M_Random rndtable[++rndindex]

// As M_Random, but used only by the play simulation.
#define P_Random rndtable[++prndindex]
