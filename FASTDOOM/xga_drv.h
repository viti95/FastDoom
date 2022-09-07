#ifndef XGADRV_H
#define XGADRV_H

#include <stdio.h>
#include <dos.h>
#include <conio.h>

#include "mca.h"
#include "xga_reg.h"
#include "doomdef.h"

#define MMIO8(ADDR)  ( ( *(volatile unsigned char *)(ADDR) ) )
#define MMIO16(ADDR) ( ( *(volatile unsigned short *)(ADDR) ) )
#define MMIO32(ADDR) ( ( *(volatile unsigned int *)(ADDR) ) )

typedef struct xga_info_t
{
	unsigned short aperture_selector;
	unsigned short data_selector;
	unsigned char * dpmi_base;
	unsigned char instance;
	void * rom_base;
	void * coproc_base;
	void * io_base;
	void * vram_base;
	void * instance_base;
	//void * aperture_base;
} XGA_Info;

#define XGA_ERR_NOT_DETECTED -1

unsigned char xga_dcr_byte_r(XGA_Info *xga, unsigned char offset);
void xga_dcr_byte_w(XGA_Info *xga, unsigned char offset, unsigned char byte);

void xga_io_byte_w(XGA_Info *xga, unsigned short addr, unsigned char val);
void xga_set_palette(XGA_Info *xga, unsigned char index, unsigned char r, unsigned char g, unsigned char b);
void xga_set_xga_mode(XGA_Info *xga);
void xga_set_vga_mode(XGA_Info *xga);
int xga_setup(XGA_Info *xga);

void 	XGA_Init();
void 	XGA_ClearMemory(XGA_Info *xga);
int 	XGA_DpmiLockAperture(XGA_Info *xga);
int 	XGA_AllocateAndMap(XGA_Info *xga);
void	XGA_SetupTables(XGA_Info *xga);

/*******************************************/

void XGA_I_SetPalette(unsigned int numpalette);
void XGA_I_UpdateBox(unsigned char *src, int x, int y, int width, int height);

#define POS_ID_XGA ( 0x8FDB )

#endif

