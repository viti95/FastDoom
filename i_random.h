#include "doomtype.h"

//
// M_Random
// Returns a 0-255 number
//
extern byte rndtable[256];
extern byte rndtableMul5Mod3Plus1[256];
extern byte rndtableMul2Mod10Plus1[256];
extern byte rndtableMul3Mod5Plus1[256];
extern byte rndtableMul10Mod6Plus1[256];
extern byte rndindex;
extern byte prndindex;

// Which one is deterministic?

// Returns a number from 0 to 255,
// from a lookup table.
#define M_Random rndtable[++rndindex]

// As M_Random, but used only by the play simulation.
#define P_Random rndtable[++prndindex]
#define P_Random_Mul5_Mod3_Plus1 rndtableMul5Mod3Plus1[++prndindex]
#define P_Random_Mul2_Mod10_Plus1 rndtableMul2Mod10Plus1[++prndindex]
#define P_Random_Mul3_Mod5_Plus1 rndtableMul3Mod5Plus1[++prndindex]
#define P_Random_Mul10_Mod6_Plus1 rndtableMul10Mod6Plus1[++prndindex]
