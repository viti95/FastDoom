#include "mca.h"

int mca_enable(MCA_SLOT slot)
{
	union REGS in;
	union REGS out;

	in.h.ah = 0xC4;
	in.h.al = 0x02;
	in.h.bl = slot;

	int386(0x15, &in, &out);

	return out.x.cflag; // nonzero on failure
}

int mca_enable_for_setup(MCA_SLOT slot)
{
	union REGS in;
	union REGS out;

	in.h.ah = 0xC4;
	in.h.al = 0x01;
	in.h.bl = slot;

	int386(0x15, &in, &out);

	return out.x.cflag; // nonzero on failure
}

void mca_read_pos_bytes(POSData *out, unsigned char slot, unsigned short pos_base)
{
	out->id = (inp(pos_base+1) << 8) | inp(pos_base+0);
	out->reg2 = inp(pos_base+2);
	out->reg3 = inp(pos_base+3);
	out->reg4 = inp(pos_base+4);
	out->reg5 = inp(pos_base+5);
}

unsigned short mca_read_pos_base()
{
	union REGS in;
	union REGS out;

	in.h.ah = 0xC4;
	in.h.al = 0x00;

	int386(0x15, &in, &out);

	if(out.x.cflag) return 0xFF;

	return out.w.dx;
}
