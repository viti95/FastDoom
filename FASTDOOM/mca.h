#ifndef MCA_H
#define MCA_H

#include <conio.h>
#include <dos.h>
#include <i86.h>

#define MCA_SLOT_COUNT ( 9 )

typedef enum
{
	MCA_SLOT_PLANAR,
	MCA_SLOT_1,
	MCA_SLOT_2,
	MCA_SLOT_3,
	MCA_SLOT_4,
	MCA_SLOT_5,
	MCA_SLOT_6,
	MCA_SLOT_7,
	MCA_SLOT_8
} MCA_SLOT;

typedef struct pos_data_t
{
	unsigned short id;
	unsigned char reg2;
	unsigned char reg3;
	unsigned char reg4;
	unsigned char reg5;
} POSData;

typedef struct mca_slot_t
{
	POSData pos;
} MCASlot;

int mca_enable_for_setup(MCA_SLOT slot);
int mca_enable(MCA_SLOT slot);
void mca_read_pos_bytes(POSData *out, unsigned char slot, unsigned short pos_base);
unsigned short mca_read_pos_base();

#endif
