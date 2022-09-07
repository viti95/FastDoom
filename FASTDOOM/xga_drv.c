#include "xga_drv.h"
#include "i86.h"
#include "doomdef.h"
#include "stdint.h"
#include "i_system.h"
#include "doomtype.h"
#include "fastmath.h"
#include "i_ibm.h"

XGA_Info xga_info;

#define PHYSICAL_WIDTH	640
#define PHYSICAL_HEIGHT	480
#define SCREEN_WIDTH 	320
#define SCREEN_HEIGHT 	200

unsigned int XGA_TBL_PHYSICAL_OFFSET[SCREEN_HEIGHT];
unsigned int XGA_TBL_LOGICAL_OFFSET[SCREEN_HEIGHT];

unsigned int VGA_TBL_PHYSICAL_OFFSET[SCREEN_HEIGHT];
unsigned int VGA_TBL_LOGICAL_OFFSET[SCREEN_HEIGHT];

unsigned int *xga_vm_page_directory_unaligned;
unsigned int *xga_vm_page_directory;

unsigned int *xga_vm_page_table_0mb_unaligned;
unsigned int *xga_vm_page_table_0mb;
unsigned int *xga_vm_page_table_4mb_unaligned;
unsigned int *xga_vm_page_table_4mb;
unsigned int *xga_vm_page_table_8mb_unaligned;
unsigned int *xga_vm_page_table_8mb;
unsigned int *xga_vm_page_table_12mb_unaligned;
unsigned int *xga_vm_page_table_12mb;

unsigned int *xga_vm_dpmi_page_table_unaligned;
unsigned int *xga_vm_dpmi_page_table;

#define XGA_COP(REG) ( (unsigned int)xga->coproc_base + REG )

unsigned char xga_io_byte_r(XGA_Info *xga, unsigned char offset)
{
	outp(0x210A|(xga->instance*16), offset);
	return inp(0x210B|(xga->instance*16));
}

unsigned short xga_io_word_r(XGA_Info *xga, unsigned char offset)
{
	outp(0x210A|(xga->instance*16), offset);
	return inpw(0x210C|(xga->instance*16));
}

unsigned int xga_io_dword_r(XGA_Info *xga, unsigned char offset)
{
	unsigned int result = 0;

	outp(0x210A|(xga->instance*16), offset);
	result = inpw(0x210C|(xga->instance*16));
	result |= (((unsigned int)inpw(0x210C|(xga->instance*16))) << 16);

	return result;
}

void xga_io_byte_w(XGA_Info *xga, unsigned short addr, unsigned char val)
{
	outp(0x210A|(xga->instance*16), addr);
	outp(0x210B|(xga->instance*16), val);
}

void xga_cop_byte_w(XGA_Info *xga, XGA_CoprocRegister reg, unsigned char value)
{
	//fprintf(debugfile, "xga_cop_byte_w: %p %02X\n", (unsigned int)(xga->coproc_base) + reg, value);
	//*(unsigned char *)((unsigned int)xga->coproc_base + reg) = value;
	MMIO8(XGA_COP(reg)) = value;
}

void xga_cop_word_w(XGA_Info *xga, XGA_CoprocRegister reg, unsigned short value)
{
	//fprintf(debugfile, "xga_cop_word_w: %p %04X\n", (unsigned int)(xga->coproc_base) + reg, value);
	// *(unsigned short *)((unsigned int)xga->coproc_base + reg) = value;
	MMIO16(XGA_COP(reg)) = value;
}

void xga_cop_dword_w(XGA_Info *xga, XGA_CoprocRegister reg, unsigned int value)
{
	//fprintf(debugfile, "xga_cop_dword_w: %p %08lX\n", (unsigned int)(xga->coproc_base) + reg, value);
	// *(unsigned int *)((unsigned int)xga->coproc_base + reg) = value;
	MMIO32(XGA_COP(reg)) = value;
}

unsigned char xga_cop_byte_r(XGA_Info *xga, XGA_CoprocRegister reg)
{
	//return *(unsigned char *)((unsigned int)xga->coproc_base + reg);
	return MMIO8(XGA_COP(reg));
}

unsigned int xga_cop_dword_r(XGA_Info *xga, XGA_CoprocRegister reg)
{
	//return *(unsigned int *)((unsigned int)xga->coproc_base + reg);
	return MMIO32(XGA_COP(reg));
}

unsigned char xga_dcr_byte_r(XGA_Info *xga, unsigned char offset)
{
	return inp((unsigned short)xga->io_base + offset);
}

void xga_dcr_byte_w(XGA_Info *xga, unsigned char offset, unsigned char value)
{
	outp((unsigned short)xga->io_base + offset, value);
}

void xga_dump_cop_state(XGA_Info *xga)
{
	int state_a_dwords;
	int state_b_dwords;
	int offset = 0;

	// Suspend, State Save
	xga_cop_byte_w(xga, XGA_COP_CONTROL, 0x02|0x08);
	state_a_dwords = xga_cop_byte_r(xga, XGA_COP_STATE_A_LEN);
	state_b_dwords = xga_cop_byte_r(xga, XGA_COP_STATE_B_LEN);

	// read from I/O ports
	//fprintf(debugfile, "*** XGA State A (%d dwords)\n", state_a_dwords);
	for(state_a_dwords; state_a_dwords != 0; state_a_dwords--)
	{
		unsigned short result_hi, result_lo;

		// outp(0x210A|(xga->instance*16), 0x0C);
		result_hi = inpw(0x210C|(xga->instance*16));
		result_lo = inpw(0x210E|(xga->instance*16));
		//fprintf(debugfile, "%02X: %08X\n", offset, result_hi, result_lo);
		offset += 4;
	}

	// offset = 0;
	//fprintf(debugfile, "*** XGA State B (%d dwords)\n", state_b_dwords);
	// for(state_b_dwords; state_b_dwords != 0; state_b_dwords--)
	// {
	// 	fprintf(debugfile, "%02X: %08X\n", offset, xga_io_dword_r(xga, XGA_DCR_O_DATA_C));
	// 	offset += 4;
	// }
	
	// Unsuspend
	xga_cop_byte_w(xga, XGA_COP_CONTROL, 0x02);
}

void xga_check_fault(XGA_Info *xga)
{
	unsigned char isr = xga_dcr_byte_r(&xga_info, XGA_DCR_O_VM_ISR);
	unsigned int fault_address;

	if((isr & 0x80) == 0x80)
	{
		fault_address = xga_cop_dword_r(&xga_info, XGA_COP_CVAR);
		I_Error("XGA page fault at %08X!\n", fault_address);
	}
	if((isr & 0x40) == 0x40)
	{
		fault_address = xga_cop_dword_r(&xga_info, XGA_COP_CVAR);
		I_Error("XGA access violation at %08X!\n", fault_address);
	}
}

int xga_setup_tlb(XGA_Info *xga)
{
	// Fortunately, Doom never uses anything but the default selector that DOS4G sets up.
	union REGS in, out;
	struct SREGS sregs;

	segread(&sregs);
	xga->data_selector = sregs.ds;

	// this comes pre-set to 0
	xga_vm_page_directory_unaligned = (unsigned int *)I_AllocLow(8192);
	xga_vm_page_directory = (unsigned int *)((((unsigned int)xga_vm_page_directory_unaligned) + 4095) & 0xFFFFF000);
	printf("page directory at %08X (aligned to %08X)\n", xga_vm_page_directory_unaligned, xga_vm_page_directory);

	xga_vm_page_table_0mb_unaligned = (unsigned int *)I_AllocLow(8192);
	xga_vm_page_table_0mb = (unsigned int *)((((unsigned int)xga_vm_page_table_0mb_unaligned) + 4095) & 0xFFFFF000);
	printf("page directory at %08X (aligned to %08X)\n", xga_vm_page_table_0mb_unaligned, xga_vm_page_table_0mb);
	xga_vm_page_table_4mb_unaligned = (unsigned int *)I_AllocLow(8192);
	xga_vm_page_table_4mb = (unsigned int *)((((unsigned int)xga_vm_page_table_4mb_unaligned) + 4095) & 0xFFFFF000);
	printf("page directory at %08X (aligned to %08X)\n", xga_vm_page_table_4mb_unaligned, xga_vm_page_table_4mb);
	xga_vm_page_table_8mb_unaligned = (unsigned int *)I_AllocLow(8192);
	xga_vm_page_table_8mb = (unsigned int *)((((unsigned int)xga_vm_page_table_8mb_unaligned) + 4095) & 0xFFFFF000);
	printf("page directory at %08X (aligned to %08X)\n", xga_vm_page_table_8mb_unaligned, xga_vm_page_table_8mb);
	xga_vm_page_table_12mb_unaligned = (unsigned int *)I_AllocLow(8192);
	xga_vm_page_table_12mb = (unsigned int *)((((unsigned int)xga_vm_page_table_12mb_unaligned) + 4095) & 0xFFFFF000);
	printf("page directory at %08X (aligned to %08X)\n", xga_vm_page_table_12mb_unaligned, xga_vm_page_table_12mb);

	xga_vm_dpmi_page_table_unaligned = (unsigned int *)I_AllocLow(8192);
	xga_vm_dpmi_page_table = (unsigned int *)((((unsigned int)xga_vm_dpmi_page_table_unaligned) + 4095) & 0xFFFFF000);
	printf("XGA page table at %08X (aligned to %08X)\n", xga_vm_dpmi_page_table_unaligned, xga_vm_dpmi_page_table);

	printf("Loading page directory into XGA...\n");
	// build page tables for the first 16MB
	{
		int idx=0;
		for(idx=0; idx<1024; idx++)
		{
			xga_vm_page_table_0mb[idx] 	= (0x000000 + (idx * 0x1000)) | 1; // supervisor, readonly, present
			xga_vm_page_table_4mb[idx] 	= (0x400000 + (idx * 0x1000)) | 1; // supervisor, readonly, present
			//xga_vm_page_table_8mb[idx] 	= (0x800000 + (idx * 0x1000)) | 1; // supervisor, readonly, present
			//xga_vm_page_table_12mb[idx] = (0xC00000 + (idx * 0x1000)) | 1; // supervisor, readonly, present
		}

		xga_vm_page_directory[0] = (unsigned int)xga_vm_page_table_0mb | 1;
		xga_vm_page_directory[1] = (unsigned int)xga_vm_page_table_4mb | 1;
		//xga_vm_page_directory[2] = (unsigned int)xga_vm_page_table_8mb | 1;
		//xga_vm_page_directory[3] = (unsigned int)xga_vm_page_table_12mb | 1;
	}
	
	// now add a directory entry for the XGA under DPMI
	// each page table is 1024 entries, 4MB. take the DPMI address and figure out which table it belongs to
	{
		unsigned short page_table_index = ((unsigned int)xga->dpmi_base) / 0x400000;
		int idx=0;

		printf("Aperture = directory entry 0%03Xh\n", page_table_index);

		for(idx=0; idx<1024; idx++)
		{
			xga_vm_dpmi_page_table[idx] = (((unsigned int)xga->vram_base & 0xFFFFF000) + (0x1000*idx)) | 3; // supervisor, r/w, present
		}
		xga_vm_page_directory[page_table_index] = (unsigned int)xga_vm_dpmi_page_table | 3; // present
	}

	printf("xga_dcr_byte_w: %04X %02X\n", (unsigned short)xga->io_base + XGA_DCR_O_VM_CTRL, 0);
	xga_dcr_byte_w(xga, XGA_DCR_O_VM_CTRL, 0);
	// load page directory register
	printf("xga_cop_dword_w: %04X %08lX\n", (unsigned int)(xga->coproc_base) + XGA_COP_PDBAR, xga_vm_page_directory);
	xga_cop_dword_w(xga, XGA_COP_PDBAR, ((unsigned int)xga_vm_page_directory));
	// VM enabled, fault if we fucked it up
	printf("Enabling VM... ");
	xga_dcr_byte_w(xga, XGA_DCR_O_VM_CTRL, XGA_VMCR_EV);
	printf("OK.\n");

	// reset ISR bits
	xga_dcr_byte_w(xga, XGA_DCR_O_VM_ISR, 0xC0);

	return 0;
}

void xga_set_palette(XGA_Info *xga, unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
	/* 6-bit color */
	xga_io_byte_w(xga, 0x60, index);
	xga_io_byte_w(xga, 0x65, r);
	xga_io_byte_w(xga, 0x65, g);
	xga_io_byte_w(xga, 0x65, b);
}

void xga_set_xga_mode(XGA_Info *xga)
{
	const int reg_count = 50;
	int idx;

	for(idx=0; idx<reg_count; idx++)
	{
		unsigned short reg = xga_register_ids[idx];
		unsigned char data = mode_640x480x256[idx];

		if(reg >= 0x2100)
		{
			reg = reg|(xga->instance * 0x10);
			outp(reg, data);
		}
		else
		{
			xga_io_byte_w(xga, reg, data);
		}
	}

	// 2x scaling in both directions + 8-bit color, so 320x240 effective
	xga_io_byte_w(xga, XGA_REG_DISPLAY_CTRL_2, VSF_2|HSF_2|PELSIZE_8);

	outp(0x2101|(xga->instance*0x10), 0x00);
	outp(0x2108|(xga->instance*0x10), 0x00);
}

void xga_set_vga_mode(XGA_Info *xga)
{
	unsigned short instance_offset = 0x10 * xga->instance;
	outp(0x2101|instance_offset, 0x00);
	outp(0x2104|instance_offset, 0x00);
	outp(0x2105|instance_offset, 0xFF);
	xga_io_byte_w(xga, 0x64, 0xFF);
	xga_io_byte_w(xga, 0x50, 0x15);
	xga_io_byte_w(xga, 0x50, 0x14);
	xga_io_byte_w(xga, 0x51, 0x00);
	xga_io_byte_w(xga, 0x54, 0x04);
	xga_io_byte_w(xga, 0x70, 0x00);
	xga_io_byte_w(xga, 0x2A, 0x20);
	outp(0x2100|instance_offset, 0x01);
	outp(0x3C3, 0x01);

	{
		union REGS in, out;
		in.h.ah = 0;
		in.h.al = 0;
		int386(0x10, &in, &out);
	}
}

int xga_setup(XGA_Info *xga)
{
	MCASlot slots[MCA_SLOT_COUNT];
	int xga_slot;
	int slotnum;

	unsigned short pos_base = mca_read_pos_base();

	xga_slot = 255;

	// printf("\n\n");
	// printf("PS/2 POS reg at %03Xh\n", pos_base);

	/* CRITICAL SECTION */
	_disable();
	// Max of 8 slots, ignore planar (slot 0)
	for(slotnum=0; slotnum<MCA_SLOT_COUNT; slotnum++)
	{
		mca_enable_for_setup(slotnum);
		mca_read_pos_bytes(&slots[slotnum].pos, slotnum, pos_base);
		mca_enable(slotnum);
	}
	_enable();
	/* CRITICAL SECTION */

	for(slotnum=0; slotnum<MCA_SLOT_COUNT; slotnum++)
	{
		// printf("*** Slot %d POS ID %02X", slotnum, slots[slotnum].pos.id);
		if(slots[slotnum].pos.id == POS_ID_XGA)
		{
			xga_slot = slotnum;
			// printf(" <-- XGA");
		}
		// printf("\n");
	}

	if(xga_slot == 255) return XGA_ERR_NOT_DETECTED;

	printf("XGA in slot %d\n", xga_slot);

	// printf("XGA POS regs: %02X %02X %02X %02X\n",
	// 	slots[xga_slot].pos.reg2, slots[xga_slot].pos.reg3,
	// 	slots[xga_slot].pos.reg4, slots[xga_slot].pos.reg5);

	// Decode the POS data into our configuration.
	xga->instance = (slots[xga_slot].pos.reg2 >> 1) & 7;
	// printf("XGA instance %d\n", xga->instance);

	xga->rom_base = (void *)(0xC0000 + ((slots[xga_slot].pos.reg2 >> 4) * 0x2000)); //MK_FP(0xC000, (slots[xga_slot].pos.reg2 >> 4) * 0x2000);
	// printf("XGA ROM base at %p\n", xga->rom_base);

	xga->coproc_base = (void *)(0xC0000 + (128 * xga->instance) + 0x1C00); //MK_FP(0xC000, ((128 * xga->instance) + 0x1C00));
	// printf("XGA coprocessor base at %p\n", xga->coproc_base);

	xga->io_base = (void *)(0x2100 + (0x10 * xga->instance));
	// printf("XGA IO base at 0%04Xh\n", xga->io_base);

	{
		unsigned int base_addr = (slots[xga_slot].pos.reg4 >> 1);
		base_addr <<= 25;
		base_addr += (xga->instance * 0x400000);
		xga->vram_base = (void *)base_addr;
		//xga->vram_base = (void *)( (((unsigned int)slots[xga_slot].pos.reg4 >> 1) << 25) | ((unsigned int)xga->instance << 22) );
		// printf("XGA 4M aperture base at %08lXh\n", xga->vram_base);
	}

	// xga->aperture_base = (void *)( (unsigned int)((slots[xga_slot].pos.reg5 & 15) * 0x100000) );
	// printf("XGA VRAM aperture at %07lXh\n", xga->aperture_base);

	return 0;
}

void XGA_Init()
{
	int result = xga_setup(&xga_info);
	if(result != 0)
	{
		I_Error("\nCould not set up XGA.");
	}

	printf("Setting up XGA lookup tables...\n");
	XGA_SetupTables(&xga_info);

	printf("Setting up XGA registers...\n");
	xga_set_xga_mode(&xga_info);

	printf("Mapping XGA into process...\n");
	result = XGA_AllocateAndMap(&xga_info);
	if(result != 0)
	{
		I_Error("\nDPMI error setting up descriptors: %04X\n", result);
	}

	printf("Setting up XGA virtual memory registers...\n");
	xga_setup_tlb(&xga_info);
	if(result != 0)
	{
		I_Error("\nDPMI error setting up XGA VM: %04X\n", result);
	}

	printf("Setting up DPMI region... ");
	result = XGA_DpmiLockAperture(&xga_info);
	if(result != 0)
	{
		I_Error("\nDPMI error locking XGA aperture: %04X\n", result);
	}
	printf("\n");

	getch();

	printf("Clearing XGA memory...\n");
	XGA_ClearMemory(&xga_info);
}

void XGA_ClearMemory(XGA_Info *xga)
{
	int idx;

	for(idx=0; idx<640*480; idx++)
	{
		xga->dpmi_base[idx] = 0;
	}
}

int XGA_AllocateAndMap(XGA_Info *xga)
{
	union REGS regs;

	/* allocate descriptor */
	regs.w.ax = 0;
	regs.w.cx = 1;
	int386(0x31, &regs, &regs);

	if(regs.w.cflag) return regs.w.ax;

	xga->aperture_selector = regs.w.ax;

	printf("XGA aperture selector %02X\n", xga->aperture_selector);

	/* set to 4MB limit */
	regs.w.ax = 0x0008;
	regs.w.bx = xga->aperture_selector;
	regs.w.cx = 0x003F;
	regs.w.dx = 0xFFFF;
	int386(0x31, &regs, &regs);
	if(regs.w.cflag) return regs.w.ax;

	printf("XGA aperture limit set\n");

	/* map into process space */
	regs.w.ax = 0x0800;
	regs.w.bx = (unsigned int)xga->vram_base >> 16;
	regs.w.cx = (unsigned int)xga->vram_base & 0xFFFF;
	regs.w.si = 0x0040;
	regs.w.di = 0x0000;
	int386(0x31, &regs, &regs);

	if(regs.w.cflag)
		return regs.w.ax;
	else
	{
		xga->dpmi_base = (unsigned char *)(((unsigned int)regs.w.bx << 16) | regs.w.cx);
		printf("XGA aperture virtualized at %08Xh\n", xga->dpmi_base);
		return 0;
	}	
}

int XGA_DpmiLockAperture(XGA_Info *xga)
{
	union REGS in;
	union REGS out;
	
	in.w.ax = 0x0600;
	
	in.w.bx = (unsigned int)xga->vram_base >> 16;
	in.w.cx = (unsigned int)xga->vram_base & 0xFFFF;
	
	in.w.si = 0x003f;
	in.w.di = 0xffff;

	int386(0x31, &in, &out);

	printf("rc = %04X\n", out.w.ax);

	if(out.w.cflag)
		return out.w.ax;
	else
		return 0;
}

/******************************************/

void XGA_I_SetPalette(unsigned int numpalette)
{
	// Reset palette index, write out palette bytes.
	// TODO: Could be done with word access?

	int i=0;
	int pos = Mul768(numpalette);
	unsigned char *ptrprocessedpalette = processedpalette + pos;

	// R,G,B,R,G,B...
	xga_io_byte_w(&xga_info, XGA_REG_PAL_SEQUENCE, 0);

	// Start at index 0
	xga_io_byte_w(&xga_info, XGA_REG_SPR_PAL_IDX_HI, 0);
	xga_io_byte_w(&xga_info, XGA_REG_SPR_PAL_IDX_LO, 0);
	for(i=0; i<768; i++){
		xga_io_byte_w(&xga_info, XGA_REG_PAL_DATA, *ptrprocessedpalette);
		ptrprocessedpalette++;
	}

}

void XGA_SetupTables(XGA_Info *xga)
{
	int i=0;
	for(i=0;i<SCREEN_HEIGHT;i++)
	{
		XGA_TBL_PHYSICAL_OFFSET[i] 	= PHYSICAL_WIDTH * i;
		XGA_TBL_LOGICAL_OFFSET[i] 	= SCREEN_WIDTH * i;
		VGA_TBL_PHYSICAL_OFFSET[i]	= 320*i;
		VGA_TBL_LOGICAL_OFFSET[i] 	= 320*i;
	}
}

void XGA_I_UpdateBox(unsigned char *src, int x, int y, int width, int height)
{
	// Copy a box of (width, height) from src to dst (x,y).
	unsigned char isr;
	unsigned int fault_address;
	
	unsigned int xga_offset = XGA_TBL_PHYSICAL_OFFSET[y] + x;
	unsigned int vga_offset = VGA_TBL_PHYSICAL_OFFSET[y] + x;
	unsigned char *dest = xga_info.dpmi_base;

	/*	
	int row, col;

	for(row=0; row<height; row++)
	{
		for(col=0; col<width; col++)
		{
			dest[xga_offset + XGA_TBL_PHYSICAL_OFFSET[row] + col] = src[vga_offset + VGA_TBL_PHYSICAL_OFFSET[row] + col];
		}
	}
	*/

	// This is a straight rectangular PxBlt.
	// set up map A, source. VGA 320x200 buffer
	xga_cop_word_w(&xga_info, XGA_COP_PIXMAP_IDX, 1);
	xga_cop_dword_w(&xga_info, XGA_COP_PIXMAP_BASE, (unsigned int)src);
	xga_cop_word_w(&xga_info, XGA_COP_PIXMAP_WIDTH, 319);
	xga_cop_word_w(&xga_info, XGA_COP_PIXMAP_HEIGHT, 199);
	xga_cop_byte_w(&xga_info, XGA_COP_PIXMAP_FORMAT, 0x03); // 8bpp

	// set up map B, destination. XGA 640x480 physical, 320x240 logical
	xga_cop_word_w(&xga_info, XGA_COP_PIXMAP_IDX, 2);
	xga_cop_dword_w(&xga_info, XGA_COP_PIXMAP_BASE, (unsigned int)dest);
	xga_cop_word_w(&xga_info, XGA_COP_PIXMAP_WIDTH, 639);
	xga_cop_word_w(&xga_info, XGA_COP_PIXMAP_HEIGHT, 479);
	xga_cop_byte_w(&xga_info, XGA_COP_PIXMAP_FORMAT, 0x03); // 8bpp

	// clear a few registers
	xga_cop_byte_w(&xga_info, XGA_COP_CONTROL, 0);
	xga_cop_byte_w(&xga_info, XGA_COP_COLOR_COMP_COND, 4);
	xga_cop_dword_w(&xga_info, XGA_COP_PLANE_MASK, 0xFF);
	xga_cop_dword_w(&xga_info, XGA_COP_CARRY_CHAIN_MASK, 0xFF);

	// D = S
	xga_cop_byte_w(&xga_info, XGA_COP_FORE_MIX, 3);
	// size
	xga_cop_word_w(&xga_info, XGA_COP_OPER_DIM_1, width-1);
	xga_cop_word_w(&xga_info, XGA_COP_OPER_DIM_2, height-1);

	// position
	xga_cop_word_w(&xga_info, XGA_COP_SOURCE_X, x);
	xga_cop_word_w(&xga_info, XGA_COP_SOURCE_Y, y);
	xga_cop_word_w(&xga_info, XGA_COP_DEST_X, x);
	xga_cop_word_w(&xga_info, XGA_COP_DEST_Y, y);

	// operation
	xga_cop_dword_w(&xga_info, XGA_COP_PIXEL_OP,
		XGA_PIXEL_OP_BS_SRC|XGA_PIXEL_OP_FS_DEST| 	// VRAM-VRAM copy
		XGA_PIXEL_OP_STEP_PXBLT|					// PxBlt operation
		XGA_PIXEL_OP_SOURCE_PIXMAP_A|				// from map A
		XGA_PIXEL_OP_DEST_PIXMAP_B|					// to map B
		XGA_PIXEL_OP_PATTERN_PIXMAP_FIXED|			// disable pattern
		XGA_PIXEL_OP_DM_DRAW_ALL_PIXELS|			// draw all pixels
		XGA_PIXEL_OP_MASK_MAP_OFF);					// no mask

	xga_check_fault(&xga_info);
}
