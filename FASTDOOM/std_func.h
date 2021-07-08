#define toupper(x) ((x >= 97 && x <= 122) ? x - 32 : x)
//#define toupper(x) ((x) - (((x) >= 97 && (x) <= 122) << 5))
#define toupperint(x) ((x) - (((x) - 'a'<26U)<<5))
#define isxdigit(x) ((x >= 48 && x <= 57) || (x >= 65 && x <= 70) || (x >= 97 && x <= 102) ? 1 : 0)
//#define abs(x) ((x) < 0 ? -(x) : (x))
#define abs(x) (((x) + ((x) >> 31)) ^ ((x) >> 31))

/*int abs(int value);
#pragma aux abs = \
    "cdq", \
    "add eax, edx", \
    "xor eax, edx", parm[eax] value[eax] modify exact[eax edx]*/
