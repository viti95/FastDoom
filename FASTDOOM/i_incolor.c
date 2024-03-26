#include <string.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "doomtype.h"
#include "doomstat.h"
#include "i_ibm.h"
#include "v_video.h"
#include "tables.h"
#include "math.h"
#include "i_system.h"
#include "i_incolor.h"

#if defined(MODE_INCOLOR)

#define INDEX_REG 0x3B4
#define DATA_REG 0x3B5

#define DMC_PORT 0x3B8
#define STATUS_PORT 0x3BA
#define CONFIG_PORT 0x3BF

#define xmode_reg	0x14	//bit 0    char generator (ROM,RAM)
					//bit 1    Matrix width (9 or 8)
					//bit 2    RamFont mode (4k or 48k)
					//bit 3-7  unused

#define underscore_reg	0x15	//bit 0-3  underscore position
					//bit 4-7  underscore color

#define overstrike_reg	0x16	//bit 0-3  overstrike position
					//bit 4-7  overstrike color

#define exception_reg	0x17	//bit 0-3  cursor color
					//bit 4    palette enable/disable
					//bit 5    normal/alternate attributes
					//bit 6-7  unused

#define PLANE_MASK_REG 0x18
#define R_W_CONTROL_REG 0x19
#define R_W_COLOR_REG 0x1A
#define LATCH_PROTECT_REG 0x1B
#define PALETTE_REG 0x1C

#define grph 0x02 // equates for dmc_port
#define text 0x00
#define screen_on 0x08
#define screen_off 0x00
#define blinker_on 0x20
#define blinker_off 0x00
#define graph_page_1 0x80
#define graph_page_0 0x00

#define ROM_text 0x00 // equates for xMode Register
#define RAM_text 0x01
#define nine_wide 0x00
#define eight_wide 0x02
#define font_4k 0x00
#define font_48k 0x04

#define enable_palette 0x10 // equates for Exception Register
#define disable_palette 0x00
#define alternate_att 0x00
#define normal_att 0x20

#define display_all 0x0F // equates for Plane Mask Register
#define display_none 0x00
#define display_0 0x01
#define display_1 0x02
#define display_2 0x04
#define display_3 0x08
#define freeze_none 0x00
#define freeze_all 0xF0
#define freeze_0 0x10
#define freeze_1 0x20
#define freeze_2 0x40
#define freeze_3 0x80

#define care_all 0x00 // equates for r_w_control register
#define no_care_all 0x0F
#define write_mode_0 0x00
#define write_mode_1 0x10
#define write_mode_2 0x20
#define write_mode_3 0x30
#define set_if_equal 0x00
#define clear_if_equal 0x40

#define diag 0 // configuration port options
#define half 1
#define full 3

const byte colors[48] = {
    0x00, 0x00, 0x00,  // 0
    0x00, 0x00, 0x2A,  // 1
    0x00, 0x2A, 0x00,  // 2
    0x00, 0x2A, 0x2A,  // 3
    0x2A, 0x00, 0x00,  // 4
    0x2A, 0x00, 0x2A,  // 5
    0x2A, 0x15, 0x00,  // 6
    0x2A, 0x2A, 0x2A,  // 7
    0x15, 0x15, 0x15,  // 8
    0x15, 0x15, 0x3F,  // 9
    0x15, 0x3F, 0x15,  // 10
    0x15, 0x3F, 0x3F,  // 11
    0x3F, 0x15, 0x15,  // 12
    0x3F, 0x15, 0x3F,  // 13
    0x3F, 0x3F, 0x15,  // 14
    0x3F, 0x3F, 0x3F}; // 15

unsigned char lut16colors[14 * 256];
unsigned char *ptrlut16colors = lut16colors;

void I_ProcessPalette(byte *palette)
{
    int i, j;
    byte *ptr = gammatable[usegamma];

    for (i = 0; i < 14 * 256; i++, palette += 3)
    {
        int r1, g1, b1;

        unsigned char bestcolor;

        r1 = (int)ptr[*(palette)];
        g1 = (int)ptr[*(palette + 1)];
        b1 = (int)ptr[*(palette + 2)];

        bestcolor = GetClosestColor(colors, 16, r1, g1, b1);
        ptrlut16colors[i] = bestcolor;
    }
}

void I_SetPalette(int numpalette)
{
    ptrlut16colors = lut16colors + numpalette * 256;
}

void I_FinishUpdate(void)
{
    byte plane_red[SCREENWIDTH * SCREENHEIGHT / 8];
    byte plane_green[SCREENWIDTH * SCREENHEIGHT / 8];
    byte plane_blue[SCREENWIDTH * SCREENHEIGHT / 8];
    byte plane_intensity[SCREENWIDTH * SCREENHEIGHT / 8];

    byte *ptr_plane_red         = plane_red;
    byte *ptr_plane_green       = plane_green;
    byte *ptr_plane_blue        = plane_blue;
    byte *ptr_plane_intensity   = plane_intensity;

    int i,j;

    int x, y;
    unsigned int base = 0;
    unsigned int plane_position = 0;

    // Chunky 2 planar conversion (hi Amiga fans!)

    for (y = 0; y < SCREENHEIGHT; y++){
        for (x = 0; x < SCREENWIDTH / 8; x++){
            unsigned char color0 = ptrlut16colors[backbuffer[base]];
            unsigned char color1 = ptrlut16colors[backbuffer[base + 1]];
            unsigned char color2 = ptrlut16colors[backbuffer[base + 2]];
            unsigned char color3 = ptrlut16colors[backbuffer[base + 3]];
            unsigned char color4 = ptrlut16colors[backbuffer[base + 4]];
            unsigned char color5 = ptrlut16colors[backbuffer[base + 5]];
            unsigned char color6 = ptrlut16colors[backbuffer[base + 6]];
            unsigned char color7 = ptrlut16colors[backbuffer[base + 7]];

            plane_red[plane_position] = ((color0 >> 3) & 1) << 7 | ((color1 >> 3) & 1) << 6 | ((color2 >> 3) & 1) << 5 | ((color3 >> 3) & 1) << 4 | ((color4 >> 3) & 1) << 3 | ((color5 >> 3) & 1) << 2 | ((color6 >> 3) & 1) << 1 | ((color7 >> 3) & 1);
            plane_green[plane_position] = ((color0 >> 2) & 1) << 7 | ((color1 >> 2) & 1) << 6 | ((color2 >> 2) & 1) << 5 | ((color3 >> 2) & 1) << 4 | ((color4 >> 2) & 1) << 3 | ((color5 >> 2) & 1) << 2 | ((color6 >> 2) & 1) << 1 | ((color7 >> 2) & 1);
            plane_blue[plane_position] = ((color0 >> 1) & 1) << 7 | ((color1 >> 1) & 1) << 6 | ((color2 >> 1) & 1) << 5 | ((color3 >> 1) & 1) << 4 | ((color4 >> 1) & 1) << 3 | ((color5 >> 1) & 1) << 2 | ((color6 >> 1) & 1) << 1 | ((color7 >> 1) & 1);
            plane_intensity[plane_position] = ((color0) & 1) << 7 | ((color1) & 1) << 6 | ((color2) & 1) << 5 | ((color3) & 1) << 4 | ((color4) & 1) << 3 | ((color5) & 1) << 2 | ((color6) & 1) << 1 | ((color7) & 1);

            plane_position++;
            base += 8;
        }
    }

    // Copy each bitplane

    outp(INDEX_REG, R_W_COLOR_REG);
    outp(INDEX_REG + 1, 1);

    outp(INDEX_REG, PLANE_MASK_REG);
    outp(INDEX_REG + 1, freeze_1+freeze_2+freeze_3+display_all);

    for (i = 0; i < 200/4; i++)
    {
        for (j = 0; j < 320/8; j++)
        {
            pcscreen[90*(i+18)+j+25]        = ptr_plane_intensity[j];
            pcscreen[90*(i+18)+j+0x2000+25] = ptr_plane_intensity[40+j];
            pcscreen[90*(i+18)+j+0x4000+25] = ptr_plane_intensity[80+j];
            pcscreen[90*(i+18)+j+0x6000+25] = ptr_plane_intensity[120+j];
        }

        ptr_plane_intensity += 160;
    }

    outp(INDEX_REG, R_W_COLOR_REG);
    outp(INDEX_REG + 1, 2);

    outp(INDEX_REG, PLANE_MASK_REG);
    outp(INDEX_REG + 1, freeze_0+freeze_2+freeze_3+display_all);

    for (i = 0; i < 200/4; i++)
    {
        for (j = 0; j < 320/8; j++)
        {
            pcscreen[90*(i+18)+j+25]        = ptr_plane_blue[j];
            pcscreen[90*(i+18)+j+0x2000+25] = ptr_plane_blue[40+j];
            pcscreen[90*(i+18)+j+0x4000+25] = ptr_plane_blue[80+j];
            pcscreen[90*(i+18)+j+0x6000+25] = ptr_plane_blue[120+j];
        }

        ptr_plane_blue += 160;
    }

    outp(INDEX_REG, R_W_COLOR_REG);
    outp(INDEX_REG + 1, 4);

    outp(INDEX_REG, PLANE_MASK_REG);
    outp(INDEX_REG + 1, freeze_0+freeze_1+freeze_3+display_all);

    for (i = 0; i < 200/4; i++)
    {
        for (j = 0; j < 320/8; j++)
        {
            pcscreen[90*(i+18)+j+25]        = ptr_plane_green[j];
            pcscreen[90*(i+18)+j+0x2000+25] = ptr_plane_green[40+j];
            pcscreen[90*(i+18)+j+0x4000+25] = ptr_plane_green[80+j];
            pcscreen[90*(i+18)+j+0x6000+25] = ptr_plane_green[120+j];
        }

        ptr_plane_green += 160;
    }

    outp(INDEX_REG, R_W_COLOR_REG);
    outp(INDEX_REG + 1, 8);

    outp(INDEX_REG, PLANE_MASK_REG);
    outp(INDEX_REG + 1, freeze_0+freeze_1+freeze_2+display_all);

    for (i = 0; i < 200/4; i++)
    {
        for (j = 0; j < 320/8; j++)
        {
            pcscreen[90*(i+18)+j+25]        = ptr_plane_red[j];
            pcscreen[90*(i+18)+j+0x2000+25] = ptr_plane_red[40+j];
            pcscreen[90*(i+18)+j+0x4000+25] = ptr_plane_red[80+j];
            pcscreen[90*(i+18)+j+0x6000+25] = ptr_plane_red[120+j];
        }

        ptr_plane_red += 160;
    }

}

void InColor_InitGraphics(void)
{
    unsigned char palette_init[16] = {0, 1, 2, 3, 4, 5, 20, 7, 56, 57, 58, 59, 60, 61, 62, 63};
    byte Graph_720x350[12] = {0x35,0x2d,0x2e,0x07,0x5b,0x02,0x57,0x57,0x02,0x03,0x00,0x00};
    int i, j;

    // Set card mode
    outp(CONFIG_PORT, half);

    // Initialize palette
    outp(INDEX_REG, PALETTE_REG);
    inp(INDEX_REG + 1);

    for (i = 0; i < 16; i++)
    {
        outp(INDEX_REG + 1, palette_init[i]);
    }    

    outp(INDEX_REG, exception_reg);
    outp(INDEX_REG + 1, enable_palette + normal_att);

    // Enable graphics mode
    outp(DMC_PORT, grph);

    for (i = 0; i < 12; i++)
    {
        outp(INDEX_REG, i);
        outp(INDEX_REG + 1, Graph_720x350[i]);
    }

    pcscreen = destscreen = (byte *)0xB0000;

    //SetDWords(pcscreen, 0, 8192);

    outp(DMC_PORT, grph + screen_on);

    outp(INDEX_REG, R_W_CONTROL_REG);
    outp(INDEX_REG + 1, clear_if_equal+write_mode_0+care_all);

    // Init VRAM

    outp(INDEX_REG, R_W_COLOR_REG);
    outp(INDEX_REG + 1, 0);

    outp(INDEX_REG, PLANE_MASK_REG);
    outp(INDEX_REG + 1, display_all);

    for (i = 0; i < 348/4; i++)
    {
        for (j = 0; j < 720/8; j++)
        {
            pcscreen[90*i+j]        = 0;
            pcscreen[90*i+j+0x2000] = 0;
            pcscreen[90*i+j+0x4000] = 0;
            pcscreen[90*i+j+0x6000] = 0;
        }
    }
}

void InColor_ShutdownGraphics(void)
{
    byte Text_80x25[12] = {0x00, 0x61, 0x50, 0x52, 0x0F, 0x19, 0x06, 0x19, 0x19, 0x02, 0x0D, 0x08};
    int i;

    outp(0x03BF, Text_80x25[0]);
    for (i = 0; i < 10; i++)
    {
        outp(0x03B4, i);
        outp(0x03B5, Text_80x25[i + 1]);
    }
    outp(0x03B8, Text_80x25[11]);
}

#endif
