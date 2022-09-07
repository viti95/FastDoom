#ifndef XGAREG_H
#define XGAREG_H

/* 
	Display Controller Registers - $21IX
	I = instance number
	X = register number 
	*/
// typedef enum {
// 	XGA_DCR_OPER_MODE			= 0x2100,
// 	XGA_DCR_APER_CTRL			= 0x2101,
// 	XGA_DCR_RESERVED2			= 0x2102,
// 	XGA_DCR_RESERVED3			= 0x2103,
// 	XGA_DCR_IER					= 0x2104,
// 	XGA_DCR_ISR					= 0x2105,
// 	XGA_DCR_VM_CTRL				= 0x2106,
// 	XGA_DCR_VM_ISR				= 0x2107,
// 	XGA_DCR_APER_IDX			= 0x2108,
// 	XGA_DCR_MEM_ACCESS			= 0x2109,
// 	XGA_DCR_IDX					= 0x210A,
// 	XGA_DCR_DATA_B				= 0x210B,
// 	XGA_DCR_DATA_C				= 0x210C,
// 	XGA_DCR_DATA_D				= 0x210D,
// 	XGA_DCR_DATA_E				= 0x210E,
// 	XGA_DCR_DATA_F				= 0x210F
// } XGA_DispCtrlRegs;

typedef enum {
	XGA_DCR_O_OPER_MODE			= 0x00,
	XGA_DCR_O_APER_CTRL			= 0x01,
	XGA_DCR_O_RESERVED2			= 0x02,
	XGA_DCR_O_RESERVED3			= 0x03,
	XGA_DCR_O_IER				= 0x04,
	XGA_DCR_O_ISR				= 0x05,
	XGA_DCR_O_VM_CTRL			= 0x06,
	XGA_DCR_O_VM_ISR			= 0x07,
	XGA_DCR_O_APER_IDX			= 0x08,
	XGA_DCR_O_MEM_ACCESS		= 0x09,
	XGA_DCR_O_IDX				= 0x0A,
	XGA_DCR_O_DATA_B			= 0x0B,
	XGA_DCR_O_DATA_C			= 0x0C,
	XGA_DCR_O_DATA_D			= 0x0D,
	XGA_DCR_O_DATA_E			= 0x0E,
	XGA_DCR_O_DATA_F			= 0x0F
} XGA_DispCtrlRegs_Offsets;

typedef enum
{
	XGA_REG_AUTO_CONFIGURATION	= 0x04,
	XGA_REG_COP_SAVE			= 0x0C,
	XGA_REG_COP_RESTORE			= 0x0D,
	XGA_REG_HDISP_TOTAL_LO		= 0x10,
	XGA_REG_HDISP_TOTAL_HI		= 0x11,
	XGA_REG_HDISP_END_LO		= 0x12,
	XGA_REG_HDISP_END_HI		= 0x13,
	XGA_REG_HBLANK_START_LO		= 0x14,
	XGA_REG_HBLANK_START_HI		= 0x15,
	XGA_REG_HBLANK_END_LO		= 0x16,
	XGA_REG_HBLANK_END_HI		= 0x17,
	XGA_REG_HSYNC_START_LO		= 0x18,
	XGA_REG_HSYNC_START_HI		= 0x19,
	XGA_REG_HSYNC_END_LO		= 0x1A,
	XGA_REG_HSYNC_END_HI		= 0x1B,
	XGA_REG_HSYNC_POS_A			= 0x1C,	/* 1C and 1E must be the same */
	XGA_REG_HSYNC_POS_B			= 0x1E, /* 1C and 1E must be the same */
	XGA_REG_VTOTAL_LO			= 0x20,
	XGA_REG_VTOTAL_HI			= 0x21,
	XGA_REG_VDISP_END_LO		= 0x22,
	XGA_REG_VDISP_END_HI		= 0x23,
	XGA_REG_VBLANK_START_LO		= 0x24,
	XGA_REG_VBLANK_START_HI		= 0x25,
	XGA_REG_VBLANK_END_LO		= 0x26,
	XGA_REG_VBLANK_END_HI		= 0x27,
	XGA_REG_VSYNC_START_LO		= 0x28,
	XGA_REG_VSYNC_START_HI		= 0x29,
	XGA_REG_VSYNC_END			= 0x2A,
	XGA_REG_VLINE_COMPARE_LO	= 0X2C,
	XGA_REG_VLINE_COMPARE_HI	= 0X2D,
	XGA_REG_SPRITE_H_START_LO	= 0X30,
	XGA_REG_SPRITE_H_START_HI	= 0X31,
	XGA_REG_SPRITE_H_PRESET		= 0X32,
	XGA_REG_SPRITE_V_START_LO	= 0X33,
	XGA_REG_SPRITE_V_START_HI	= 0X34,
	XGA_REG_SPRITE_V_PRESET		= 0X35,
	XGA_REG_SPRITE_CTRL			= 0X36,
	XGA_REG_SPRITE_COLOR0_R		= 0X38,
	XGA_REG_SPRITE_COLOR0_G		= 0X39,
	XGA_REG_SPRITE_COLOR0_B		= 0X3A,
	XGA_REG_SPRITE_COLOR1_R		= 0X3B,
	XGA_REG_SPRITE_COLOR1_G		= 0X3C,
	XGA_REG_SPRITE_COLOR1_B		= 0X3D,
	XGA_REG_DPM_OFFSET_LO		= 0X40,
	XGA_REG_DPM_OFFSET_MID		= 0X41,
	XGA_REG_DPM_OFFSET_HI		= 0X42,
	XGA_REG_DPM_WIDTH_LO		= 0X43,
	XGA_REG_DPM_WIDTH_HI		= 0X44,
	XGA_REG_DISPLAY_CTRL_1		= 0X50,
	XGA_REG_DISPLAY_CTRL_2		= 0X51,
	XGA_REG_DISPLAY_ID			= 0X52,
	XGA_REG_CLK_FREQ_SELECT		= 0X54,
	XGA_REG_BORDER_COLOR		= 0X55,
	XGA_REG_SPR_PAL_IDX_LO		= 0X60,
	XGA_REG_SPR_PAL_IDX_HI		= 0X61,
	XGA_REG_SPR_PAL_IDX_PRE_LO	= 0X62,
	XGA_REG_SPR_PAL_IDX_PRE_HI	= 0X63,
	XGA_REG_PAL_MASK			= 0X64,
	XGA_REG_PAL_DATA			= 0X65,
	XGA_REG_PAL_SEQUENCE		= 0X66,
	XGA_REG_PAL_RED_PRE			= 0X67,
	XGA_REG_PAL_GREEN_PRE		= 0X68,
	XGA_REG_PAL_BLUE_PRE		= 0X69,
	XGA_REG_SPR_DATA			= 0X6A,
	XGA_REG_SPR_PRE				= 0X6B,
	XGA_REG_XTRN_CLOCK_SELECT	= 0X70
} XGA_Register;

typedef enum
{
	XGA_COP_PDBAR				= 0x00, /* 32 */
	XGA_COP_CVAR				= 0x04, /* 32 */
	XGA_COP_STATE_A_LEN			= 0x0C,	/* 8 */
	XGA_COP_STATE_B_LEN			= 0x0D, /* 8 */
	XGA_COP_CONTROL				= 0x11, /* 8  */
	XGA_COP_PIXMAP_IDX			= 0x12, /* 8 */
	XGA_COP_PIXMAP_BASE			= 0x14,	/* 32 */
	XGA_COP_PIXMAP_WIDTH		= 0x18, /* 16 */
	XGA_COP_PIXMAP_HEIGHT		= 0x1A, /* 16 */
	XGA_COP_PIXMAP_FORMAT		= 0x1C,	/* 8  */
	XGA_COP_BRESENHAM_E			= 0x20, /* 16 */
	XGA_COP_BRESENHAM_K1		= 0x24, /* 16 */
	XGA_COP_BRESENHAM_K2		= 0x28, /* 16 */
	XGA_COP_DIR_STEPS			= 0x2C,	/* 32 */
	XGA_COP_FORE_MIX			= 0x48,	/* 8  */
	XGA_COP_BACK_MIX			= 0x49, /* 8  */
	XGA_COP_COLOR_COMP_COND		= 0x4A, /* 8  */
	XGA_COP_COLOR_COMP_VALUE	= 0x4C,	/* 32 */
	XGA_COP_PLANE_MASK			= 0x50,	/* 32 */
	XGA_COP_CARRY_CHAIN_MASK	= 0x54,	/* 32 */
	XGA_COP_FORE_COLOR			= 0x58,	/* 32 */
	XGA_COP_BACK_COLOR			= 0x5C, /* 32 */
	XGA_COP_OPER_DIM_1			= 0x60,	/* 16 */
	XGA_COP_OPER_DIM_2			= 0x62, /* 16 */
	XGA_COP_MASK_MAP_ORIGIN_X	= 0x6C, /* 16 */
	XGA_COP_MASK_MAP_ORIGIN_Y	= 0x6E, /* 16 */
	XGA_COP_SOURCE_X			= 0x70, /* 16 */
	XGA_COP_SOURCE_Y			= 0x72, /* 16 */
	XGA_COP_PATTERN_X			= 0x74, /* 16 */
	XGA_COP_PATTERN_Y			= 0x76, /* 16 */
	XGA_COP_DEST_X				= 0x78, /* 16 */
	XGA_COP_DEST_Y				= 0x7A, /* 16 */
	XGA_COP_PIXEL_OP			= 0x7C	/* 32 */
} XGA_CoprocRegister;

/* VM Control Register 21x6h */
#define XGA_VMCR_NPE			( 1 << 7 )
#define XGA_VMCR_PVE			( 1 << 6 )
#define XGA_VMCR_US				( 1 << 2 )
#define XGA_VMCR_EV				( 1 << 0 )

/* Display Ctrl 2 */
#define VSF_1					( 0 << 6 )
#define VSF_2					( 1 << 6 )
#define VSF_4					( 2 << 6 )
#define HSF_1					( 0 << 4 )
#define HSF_2					( 1 << 4 )
#define HSF_4					( 2 << 4 )
#define PELSIZE_1				( 0 << 0 )
#define PELSIZE_2				( 1 << 0 )
#define PELSIZE_4				( 2 << 0 )
#define PELSIZE_8				( 3 << 0 )
#define PELSIZE_16				( 4 << 0 )

/* pixel operation bits */
/* Background Source */
#define XGA_PIXEL_OP_BS_BACK	( 0 << 30 )
#define XGA_PIXEL_OP_BS_SRC		( 2 << 30 )

/* Foreground Source */
#define XGA_PIXEL_OP_FS_FORE	( 0 << 28 )
#define XGA_PIXEL_OP_FS_DEST	( 2 << 28 )

/* Step Function */
#define XGA_PIXEL_OP_STEP_DRAWSTEP_READ			( 2 << 24 )
#define XGA_PIXEL_OP_STEP_LINEDRAW_READ			( 3 << 24 )
#define XGA_PIXEL_OP_STEP_DRAWSTEP_WRITE		( 4 << 24 )
#define XGA_PIXEL_OP_STEP_LINEDRAW_WRITE		( 5 << 24 )
#define XGA_PIXEL_OP_STEP_PXBLT					( 8 << 24 )
#define XGA_PIXEL_OP_STEP_INV_PXBLT				( 9 << 24 )
#define XGA_PIXEL_OP_STEP_AREA_PXBLT			( 10 << 24 )

/* Source Pixel Map */
#define XGA_PIXEL_OP_SOURCE_PIXMAP_A			( 1 << 20 )
#define XGA_PIXEL_OP_SOURCE_PIXMAP_B			( 2 << 20 )
#define XGA_PIXEL_OP_SOURCE_PIXMAP_C			( 3 << 20 )

/* Destination Pixel Map */
#define XGA_PIXEL_OP_DEST_PIXMAP_A				( 1 << 16 )
#define XGA_PIXEL_OP_DEST_PIXMAP_B				( 2 << 16 )
#define XGA_PIXEL_OP_DEST_PIXMAP_C				( 3 << 16 )

/* Pattern Pixel Map */
#define XGA_PIXEL_OP_PATTERN_PIXMAP_A			( 1 << 12 )
#define XGA_PIXEL_OP_PATTERN_PIXMAP_B			( 2 << 12 )
#define XGA_PIXEL_OP_PATTERN_PIXMAP_C			( 3 << 12 )
#define XGA_PIXEL_OP_PATTERN_PIXMAP_FIXED		( 8 << 12 )
#define XGA_PIXEL_OP_PATTERN_PIXMAP_SRC			( 9 << 12 )

/* Mask Pixel Map */
#define XGA_PIXEL_OP_MASK_MAP_OFF				( 0 << 6 )
#define XGA_PIXEL_OP_MASK_MAP_BOUNDARY			( 1 << 6 )
#define XGA_PIXEL_OP_MASK_MAP_ON				( 2 << 6 )

/* Drawing Mode Register */
#define XGA_PIXEL_OP_DM_DRAW_ALL_PIXELS			( 0 << 4 )
#define XGA_PIXEL_OP_DM_FIRST_PIXEL_NULL		( 1 << 4 )
#define XGA_PIXEL_OP_DM_LAST_PIXEL_NULL			( 2 << 4 )
#define XGA_PIXEL_OP_DM_AREA_BOUNDARY			( 3 << 4 )

/* Direction Octant */
#define XGA_PIXEL_OP_OCTANT_DZ					( 0 )
#define XGA_PIXEL_OP_OCTANT_DY					( 1 ) 
#define XGA_PIXEL_OP_OCTANT_DX					( 2 )


/* registers for mode setting */
static unsigned short xga_register_ids[] = {
	XGA_DCR_O_IER,
	XGA_DCR_O_ISR,
	XGA_DCR_O_OPER_MODE,
	XGA_REG_PAL_MASK,
	XGA_DCR_O_APER_CTRL,
	XGA_DCR_O_APER_IDX,
	XGA_DCR_O_VM_CTRL,
	XGA_DCR_O_MEM_ACCESS,
	XGA_REG_DISPLAY_CTRL_1,
	XGA_REG_DISPLAY_CTRL_1,
	0x10,
	0x11,
	0x12,
	0x13,
	0x14,
	0x15,
	0x16,
	0x17,
	0x18,
	0x19,
	0x1A,
	0x1B,
	0x1C,
	0x1E,
	0x20,
	0x21,
	0x22,
	0x23,
	0x24,
	0x25,
	0x26,
	0x27,
	0x28,
	0x29,
	0x2A,
	0x2C,
	0x2D,
	0x36,
	0x40,
	0x41,
	0x42,
	0x43,
	0x44,
	XGA_REG_CLK_FREQ_SELECT,
	XGA_REG_DISPLAY_CTRL_2,
	XGA_REG_XTRN_CLOCK_SELECT,
	XGA_REG_DISPLAY_CTRL_1,
	XGA_REG_BORDER_COLOR,
	XGA_REG_PAL_MASK
};

static unsigned char mode_640x480x256[] = {
	0x00,
	0xFF,
	0x04,
	0x00, /* 64 palette mask */
	
	0x00,
	0x00,
	0x00,
	0x03, /* 21x9 mem access mode */
	
	0x01,
	0x00,
	0x63,
	0x00, /* 11 horiz total hi */
	
	0x4F,
	0x00,
	0x4F,
	0x00, /* 15 horiz blank start hi */
	
	0x63,
	0x00,
	0x55,
	0x00, /* 19 horiz sync start hi */
	
	0x61, /* 1a */
	0x00, /* 1b */
	0x00, /* 1c */
	0x00, /* 1e horiz sync position */

	0x0C, /* 20 */
	0x02, /* 21 */
	0xDF, /* 22 */
	0x01, /* 23 vert disp end hi */

	0xDF, /* 24 */
	0x01, /* 25 */
	0x0C, /* 26 */
	0x02, /* 27 blank end hi */

	0xEA, /* 28 */
	0x01, /* 29 */
	0xEC, /* 2a */
	0xFF, /* 2c vert line comp hi */

	0xFF, /* 2d */
	0x00, /* 36 */
	0x00, /* 40 */
	0x00, /* 41 start addr med */

	0x00, /* 42 */
	0x50, /* 43 */
	0x00, /* 44 */
	0x00, /* 54 clock sel */

	0x03, /* 51 */
	0x00, /* 70 */
	0xC7, /* display mode 1 */

	/* load palette here */

	0x00, /* 55 */
	0xFF, /* 64 palette mask */
};

#endif
