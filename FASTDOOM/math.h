#define BYTE0_USHORT(value) (((unsigned char *)&value)[0])
#define BYTE1_USHORT(value) (((unsigned char *)&value)[1])

#define BYTE0_UINT(value) (((unsigned char *)&value)[0])
#define BYTE1_UINT(value) (((unsigned char *)&value)[1])
#define BYTE2_UINT(value) (((unsigned char *)&value)[2])
#define BYTE3_UINT(value) (((unsigned char *)&value)[3])

int GetClosestColor(byte *colors, int num_colors, int r1, int g1, int b1);
