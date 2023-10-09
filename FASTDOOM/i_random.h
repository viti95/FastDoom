#include "doomtype.h"

//
// M_Random
// Returns a 0-255 number
//
extern byte rndtable[256];
extern byte rndtableMul5Mod3Plus1[256];
extern byte rndtableMul2Mod10Plus1[256];
extern byte rndtableMul4Mod10Plus1[256];
extern byte rndtableMul6Mod10Plus1[256];
extern byte rndtableMul3Mod5Plus1[256];
extern byte rndtableMul10Mod6Plus1[256];
extern char rndtableMod3Minus1[256];
extern byte rndtableAnd7Plus1[256];
extern byte rndtableAnd1[256];
extern byte rndtableAnd15[256];
extern byte rndtableAnd3[256];
extern byte rndtableAnd3Chg3is0[256];
extern byte rndtableAnd7[256];
extern byte rndtableLessThan10[256];
extern byte rndtableLessThan3[256];
extern byte rndtableLessThan40[256];
extern byte rndtableLessThan5[256];
extern byte rndtableMoreThan200[256];
extern byte rndtableMoreThan4[256];
extern byte rndtableAnd7Plus1Mul3[256];
extern byte rndtableAnd7Plus1Mul10[256];
extern byte rndtableAnd3Mul16[256];
extern int rndtableMinusPRandom[256];
extern byte rndindex;
extern byte prndindex;

// Which one is deterministic?

// Returns a number from 0 to 255,
// from a lookup table.
#define M_Random rndtable[++rndindex]
#define M_Random_Mod3_Minus1 rndtableMod3Minus1[++rndindex]
#define M_Random_And3_Chg3is0 rndtableAnd3Chg3is0[++rndindex]
#define M_Random_And15 rndtableAnd15[++rndindex]

// As M_Random, but used only by the play simulation.
#define P_Random rndtable[++prndindex]
#define P_Random_Mul5_Mod3_Plus1 rndtableMul5Mod3Plus1[++prndindex]
#define P_Random_Mul2_Mod10_Plus1 rndtableMul2Mod10Plus1[++prndindex]
#define P_Random_Mul4_Mod10_Plus1 rndtableMul4Mod10Plus1[++prndindex]
#define P_Random_Mul6_Mod10_Plus1 rndtableMul6Mod10Plus1[++prndindex]
#define P_Random_Mul3_Mod5_Plus1 rndtableMul3Mod5Plus1[++prndindex]
#define P_Random_Mul10_Mod6_Plus1 rndtableMul10Mod6Plus1[++prndindex]
#define P_Random_And7_Plus1 rndtableAnd7Plus1[++prndindex]
#define P_Random_And7_Plus1_Mul3 rndtableAnd7Plus1Mul3[++prndindex]
#define P_Random_And7_Plus1_Mul10 rndtableAnd7Plus1Mul10[++prndindex]
#define P_Random_And1 rndtableAnd1[++prndindex]
#define P_Random_And15 rndtableAnd15[++prndindex]
#define P_Random_And3 rndtableAnd3[++prndindex]
#define P_Random_And3_Mul16 rndtableAnd3Mul16[++prndindex]
#define P_Random_And3_Chg3is0 rndtableAnd3Chg3is0[++prndindex]
#define P_Random_And7 rndtableAnd7[++prndindex]
#define P_Random_LessThan10 rndtableLessThan10[++prndindex]
#define P_Random_LessThan3 rndtableLessThan3[++prndindex]
#define P_Random_LessThan40 rndtableLessThan40[++prndindex]
#define P_Random_LessThan5 rndtableLessThan5[++prndindex]
#define P_Random_MoreThan200 rndtableMoreThan200[++prndindex]
#define P_Random_MoreThan4 rndtableMoreThan4[++prndindex]
#define P_Random_Minus_P_Random rndtableMinusPRandom[prndindex]
