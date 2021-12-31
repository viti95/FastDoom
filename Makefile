#
# FastDoom Makefile for DOS
#

SRC_DIR=$(shell pwd)/FASTDOOM
BUILDDIR ?= $(shell pwd)/build

OLEVEL=O3
OCPU=486
OLTO=yes

ifeq "$(OCPU)" "386"
    OARCH=i386
    OTUNE=i386
else ifeq "$(OCPU)" "486"
    OARCH=i486
    OTUNE=i486
else
    $(error unknown cpu type $(OCPU))
endif

BINNAME=$(OCPU)q$(OLEVEL)$(LTOCHAR)

ifeq "$(OLTO)" "yes"
    LTOFLAGS=-flto
    BINNAME:=$(BINNAME)l
endif

CC=i386-pc-msdosdjgpp-gcc
ASM=nasm

CC_VERSION=$(shell $(CC) --version | head -n 1 | awk -F " " '{print $3}')

BASE_CFLAGS=-DSUBARCH=$(OCPU) -DOLEVEL="\"$(OLEVEL)\"" -DLTOFLAGS="\" $(LTOFLAGS)\"" -DCCVERSION="\"$(CC_VERSION)\"" -DMODE_Y
RELEASE_CFLAGS=$(BASE_CFLAGS) -s -$(OLEVEL) -march=$(OARCH) -mtune=$(OTUNE) -ffast-math \
								-fno-unwind-tables -fno-asynchronous-unwind-tables -funroll-loops \
								-fno-stack-protector -fexpensive-optimizations $(LTOFLAGS)\

ifeq "$(OPROFILE)" "yes"
BASE_CFLAGS+= -pg
endif

DEBUG_CFLAGS=$(BASE_CFLAGS)
CFLAGS?=$(BASE_CFLAGS) $(RELEASE_CFLAGS)
ASMFLAGS=-DDJGPP_ASM -f coff

LDFLAGS=-lm

DO_CC=$(CC) $(CFLAGS) -o $@ -c $<
DO_AS=$(ASM) $(ASMFLAGS) -o $@ $<

#############################################################################
# SETUP AND BUILD
#############################################################################

TARGETS=$(BUILDDIR)/$(BINNAME).exe

build_release: $(BUILDDIR)/$(BINNAME).exe

$(BUILDDIR)/FastDoom:
	mkdir -p $(BUILDDIR)/$(BINNAME)

all: $(BUILDDIR)/$(BINNAME).exe build_release $(TARGETS)

#############################################################################
# SVGALIB Quake
#############################################################################

FASTDOOM_OBJS = \
	$(BUILDDIR)/FastDoom/ns_dpmi.o \
	$(BUILDDIR)/FastDoom/ns_pcfx.o \
	$(BUILDDIR)/FastDoom/ns_task.o \
	$(BUILDDIR)/FastDoom/ns_llm.o \
	$(BUILDDIR)/FastDoom/ns_dma.o \
	$(BUILDDIR)/FastDoom/ns_irq.o \
	$(BUILDDIR)/FastDoom/ns_user.o \
	$(BUILDDIR)/FastDoom/ns_mp401.o \
	$(BUILDDIR)/FastDoom/ns_sb.o \
	$(BUILDDIR)/FastDoom/ns_sbmus.o \
	$(BUILDDIR)/FastDoom/ns_pas16.o \
	$(BUILDDIR)/FastDoom/ns_mix16.o \
	$(BUILDDIR)/FastDoom/ns_mix.o \
	$(BUILDDIR)/FastDoom/ns_gmtbr.o \
	$(BUILDDIR)/FastDoom/ns_scape.o \
	$(BUILDDIR)/FastDoom/ns_awe32.o \
	$(BUILDDIR)/FastDoom/ns_dsney.o \
	$(BUILDDIR)/FastDoom/ns_usrho.o \
	$(BUILDDIR)/FastDoom/ns_gusmi.o \
	$(BUILDDIR)/FastDoom/ns_gusau.o \
	$(BUILDDIR)/FastDoom/ns_gus.o \
	$(BUILDDIR)/FastDoom/ns_midi.o \
	$(BUILDDIR)/FastDoom/ns_music.o \
	$(BUILDDIR)/FastDoom/ns_fxm.o \
	$(BUILDDIR)/FastDoom/ns_multi.o \
	$(BUILDDIR)/FastDoom/ns_speak.o \
	$(BUILDDIR)/FastDoom/ns_pwm.o \
	$(BUILDDIR)/FastDoom/ns_lpt.o \
	$(BUILDDIR)/FastDoom/ns_sbdm.o \
	$(BUILDDIR)/FastDoom/i_debug.o \
	$(BUILDDIR)/FastDoom/i_random.o \
	$(BUILDDIR)/FastDoom/i_main.o \
	$(BUILDDIR)/FastDoom/i_ibm.o \
	$(BUILDDIR)/FastDoom/i_sound.o \
	$(BUILDDIR)/FastDoom/planar.o \
	$(BUILDDIR)/FastDoom/tables.o \
	$(BUILDDIR)/FastDoom/f_finale.o \
	$(BUILDDIR)/FastDoom/d_main.o \
	$(BUILDDIR)/FastDoom/d_net.o \
	$(BUILDDIR)/FastDoom/g_game.o \
	$(BUILDDIR)/FastDoom/m_menu.o \
	$(BUILDDIR)/FastDoom/m_misc.o \
	$(BUILDDIR)/FastDoom/am_map.o \
	$(BUILDDIR)/FastDoom/p_ceilng.o \
	$(BUILDDIR)/FastDoom/p_doors.o \
	$(BUILDDIR)/FastDoom/p_enemy.o \
	$(BUILDDIR)/FastDoom/p_floor.o \
	$(BUILDDIR)/FastDoom/p_inter.o \
	$(BUILDDIR)/FastDoom/p_lights.o \
	$(BUILDDIR)/FastDoom/p_map.o \
	$(BUILDDIR)/FastDoom/p_maputl.o \
	$(BUILDDIR)/FastDoom/p_plats.o \
	$(BUILDDIR)/FastDoom/p_pspr.o \
	$(BUILDDIR)/FastDoom/p_setup.o \
	$(BUILDDIR)/FastDoom/p_sight.o \
	$(BUILDDIR)/FastDoom/p_spec.o \
	$(BUILDDIR)/FastDoom/p_switch.o \
	$(BUILDDIR)/FastDoom/p_mobj.o \
	$(BUILDDIR)/FastDoom/p_telept.o \
	$(BUILDDIR)/FastDoom/p_saveg.o \
	$(BUILDDIR)/FastDoom/p_tick.o \
	$(BUILDDIR)/FastDoom/p_user.o \
	$(BUILDDIR)/FastDoom/r_bsp.o \
	$(BUILDDIR)/FastDoom/r_data.o \
	$(BUILDDIR)/FastDoom/r_draw.o \
	$(BUILDDIR)/FastDoom/r_main.o \
	$(BUILDDIR)/FastDoom/r_sky.o \
	$(BUILDDIR)/FastDoom/r_plane.o \
	$(BUILDDIR)/FastDoom/r_segs.o \
	$(BUILDDIR)/FastDoom/r_things.o \
	$(BUILDDIR)/FastDoom/w_wad.o \
	$(BUILDDIR)/FastDoom/v_video.o \
	$(BUILDDIR)/FastDoom/z_zone.o \
	$(BUILDDIR)/FastDoom/st_stuff.o \
	$(BUILDDIR)/FastDoom/st_lib.o \
	$(BUILDDIR)/FastDoom/hu_stuff.o \
	$(BUILDDIR)/FastDoom/hu_lib.o \
	$(BUILDDIR)/FastDoom/wi_stuff.o \
	$(BUILDDIR)/FastDoom/s_sound.o \
	$(BUILDDIR)/FastDoom/sounds.o \
	$(BUILDDIR)/FastDoom/dutils.o \
	$(BUILDDIR)/FastDoom/f_wipe.o \
	$(BUILDDIR)/FastDoom/info.o \
	$(BUILDDIR)/FastDoom/dmx.o \
	$(BUILDDIR)/FastDoom/mus2mid.o \
    
$(BUILDDIR)/$(BINNAME) : $(FASTDOOM_OBJS) $(FASTDOOM_COBJS)
	$(CC) $(RELEASE_CFLAGS) -o $@ $(FASTDOOM_COBJS) $(FASTDOOM_OBJS) $(LDFLAGS)

$(BUILDDIR)/$(BINNAME).exe : $(FASTDOOM_OBJS) $(FASTDOOM_COBJS)
	$(CC) $(RELEASE_CFLAGS) -o $@ $(FASTDOOM_COBJS) $(FASTDOOM_OBJS) $(LDFLAGS)

####

$(BUILDDIR)/FastDoom/ns_dpmi.o: $(SRC_DIR)/ns_dpmi.c 
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_pcfx.o: $(SRC_DIR)/ns_pcfx.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_task.o: $(SRC_DIR)/ns_task.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_llm.o: $(SRC_DIR)/ns_llm.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_dma.o: $(SRC_DIR)/ns_dma.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_irq.o: $(SRC_DIR)/ns_irq.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_user.o: $(SRC_DIR)/ns_user.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_mp401.o: $(SRC_DIR)/ns_mp401.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_sb.o: $(SRC_DIR)/ns_sb.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_sbmus.o: $(SRC_DIR)/ns_sbmus.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_pas16.o: $(SRC_DIR)/ns_pas16.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_mix16.o: $(SRC_DIR)/ns_mix16.asm
	$(DO_AS)
$(BUILDDIR)/FastDoom/ns_mix.o: $(SRC_DIR)/ns_mix.asm
	$(DO_AS)
$(BUILDDIR)/FastDoom/ns_gmtbr.o: $(SRC_DIR)/ns_gmtbr.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_scape.o: $(SRC_DIR)/ns_scape.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_awe32.o: $(SRC_DIR)/ns_awe32.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_dsney.o: $(SRC_DIR)/ns_dsney.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_usrho.o: $(SRC_DIR)/ns_usrho.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_gusmi.o: $(SRC_DIR)/ns_gusmi.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_gusau.o: $(SRC_DIR)/ns_gusau.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_gus.o: $(SRC_DIR)/ns_gus.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_midi.o: $(SRC_DIR)/ns_midi.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_music.o: $(SRC_DIR)/ns_music.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_fxm.o: $(SRC_DIR)/ns_fxm.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_multi.o: $(SRC_DIR)/ns_multi.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_speak.o: $(SRC_DIR)/ns_speak.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_pwm.o: $(SRC_DIR)/ns_pwm.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_lpt.o: $(SRC_DIR)/ns_lpt.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/ns_sbdm.o: $(SRC_DIR)/ns_sbdm.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/i_debug.o: $(SRC_DIR)/i_debug.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/i_random.o: $(SRC_DIR)/i_random.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/i_main.o: $(SRC_DIR)/i_main.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/i_ibm.o: $(SRC_DIR)/i_ibm.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/i_sound.o: $(SRC_DIR)/i_sound.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/planar.o: $(SRC_DIR)/planar.asm
	$(DO_AS)
$(BUILDDIR)/FastDoom/tables.o: $(SRC_DIR)/tables.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/f_finale.o: $(SRC_DIR)/f_finale.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/d_main.o: $(SRC_DIR)/d_main.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/d_net.o: $(SRC_DIR)/d_net.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/g_game.o: $(SRC_DIR)/g_game.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/m_menu.o: $(SRC_DIR)/m_menu.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/m_misc.o: $(SRC_DIR)/m_misc.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/am_map.o: $(SRC_DIR)/am_map.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_ceilng.o: $(SRC_DIR)/p_ceilng.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_doors.o: $(SRC_DIR)/p_doors.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_enemy.o: $(SRC_DIR)/p_enemy.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_floor.o: $(SRC_DIR)/p_floor.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_inter.o: $(SRC_DIR)/p_inter.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_lights.o: $(SRC_DIR)/p_lights.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_map.o: $(SRC_DIR)/p_map.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_maputl.o: $(SRC_DIR)/p_maputl.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_plats.o: $(SRC_DIR)/p_plats.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_pspr.o: $(SRC_DIR)/p_pspr.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_setup.o: $(SRC_DIR)/p_setup.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_sight.o: $(SRC_DIR)/p_sight.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_spec.o: $(SRC_DIR)/p_spec.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_switch.o: $(SRC_DIR)/p_switch.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_mobj.o: $(SRC_DIR)/p_mobj.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_telept.o: $(SRC_DIR)/p_telept.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_saveg.o: $(SRC_DIR)/p_saveg.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_tick.o: $(SRC_DIR)/p_tick.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/p_user.o: $(SRC_DIR)/p_user.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/r_bsp.o: $(SRC_DIR)/r_bsp.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/r_data.o: $(SRC_DIR)/r_data.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/r_draw.o: $(SRC_DIR)/r_draw.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/r_main.o: $(SRC_DIR)/r_main.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/r_sky.o: $(SRC_DIR)/r_sky.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/r_plane.o: $(SRC_DIR)/r_plane.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/r_segs.o: $(SRC_DIR)/r_segs.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/r_things.o: $(SRC_DIR)/r_things.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/w_wad.o: $(SRC_DIR)/w_wad.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/v_video.o: $(SRC_DIR)/v_video.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/z_zone.o: $(SRC_DIR)/z_zone.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/st_stuff.o: $(SRC_DIR)/st_stuff.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/st_lib.o: $(SRC_DIR)/st_lib.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/hu_stuff.o: $(SRC_DIR)/hu_stuff.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/hu_lib.o: $(SRC_DIR)/hu_lib.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/wi_stuff.o: $(SRC_DIR)/wi_stuff.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/s_sound.o: $(SRC_DIR)/s_sound.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/sounds.o: $(SRC_DIR)/sounds.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/dutils.o: $(SRC_DIR)/dutils.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/f_wipe.o: $(SRC_DIR)/f_wipe.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/info.o: $(SRC_DIR)/info.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/dmx.o: $(SRC_DIR)/dmx.c
	$(DO_CC)
$(BUILDDIR)/FastDoom/mus2mid.o: $(SRC_DIR)/mus2mid.c
	$(DO_CC)

#############################################################################
# MISC
#############################################################################

clean:
	rm -f FastDoom.spec
	rm -f $(FASTDOOM_OBJS)

cleanmake: clean all