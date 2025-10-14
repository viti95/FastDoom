
# FDOOM.EXE makefile

# --------------------------------------------------------------------------
#
#      4r  use 80486 timings and register argument passing
#       c  compile only
#      d1  include line number debugging information
#      d2  include full sybolic debugging information
#      ei  force enums to be of type int
#       j  change char default from unsigned to signed
#      oa  relax aliasing checking
#      od  do not optimize
#  oe[=#]  expand functions inline, # = quads (default 20)
#      oi  use the inline library functions
#      om  generate inline 80x87 code for math functions
#      ot  optimize for time
#      ox  maximum optimization
#       s  remove stack overflow checks
#     zp1  align structures on bytes
#      zq  use quiet mode
#  /i=dir  add include directories
#
# --------------------------------------------------------------------------

# Build options for 486DX/SX
#CCOPTS = $(EXTERNOPT) -omaxtnrih -ol+ -oe=32 -zp4 -4r -ei -j -zq -zc $(WCCOPTS)

# Build options for 386DX/SX
CCOPTS = $(EXTERNOPT) -omaxtnrih -ol+ -oe=32 -zp4 -3r -ei -j -zq -zc $(WCCOPTS)

# Build options for Pentium
#CCOPTS = $(EXTERNOPT) -omaxtnrih -ol+ -oe=32 -zp4 -5r -ei -j -zq -zc $(WCCOPTS)

# Build options for profiling (Pentium required)
#CCOPTS = $(EXTERNOPT) -omaxtnrih -ol+ -oe=32 -zp4 -5r -ei -j -zq -zc -et $(WCCOPTS)

NASMOPTS = $(EXTERNOPT) $(NASMOPT)

GLOBOBJS = \
 i_cgaafh.obj \
 cgaafh.obj \
 i_cga16.obj \
 cga16.obj \
 i_vga.obj \
 i_vgapal.obj \
 i_vga13h.obj \
 vga13h.obj \
 i_pcp.obj \
 pcp.obj \
 i_sigma.obj \
 sigma.obj \
 i_cga4.obj \
 cga4.obj \
 i_cgacvb.obj \
 cgacvb.obj \
 i_herc.obj \
 herc.obj \
 i_cgabw.obj \
 cgabw.obj \
 i_ega320.obj \
 ega320.obj \
 i_vesa.obj \
 vesa.obj \
 i_cga512.obj \
 cga512.obj \
 i_text.obj \
 i_mda.obj \
 i_vgay.obj \
 i_vgayh.obj \
 i_vgax.obj \
 i_incolor.obj \
 math.obj \
 ns_dpmi.obj \
 ns_pcfx.obj \
 ns_task.obj \
 ns_llm.obj \
 ns_dma.obj \
 ns_irq.obj \
 ns_mpu401.obj \
 ns_sb.obj \
 ns_sbmus.obj \
 ns_sbmid.obj \
 ns_rs232.obj \
 ns_lptmusic.obj \
 ns_pas16.obj \
 ns_mix.obj \
 ns_scape.obj \
 ns_awe32.obj \
 ns_dsney.obj \
 ns_gusmi.obj \
 ns_gusau.obj \
 ns_gus.obj \
 ns_midi.obj \
 ns_music.obj \
 ns_fxm.obj \
 ns_multi.obj \
 ns_speak.obj \
 ns_pwm.obj \
 ns_cms.obj \
 ns_lpt.obj \
 ns_sbdm.obj \
 ns_adbfx.obj \
 ns_tandy.obj \
 ns_cd.obj \
 i_debug.obj \
 i_random.obj \
 i_main.obj \
 i_ibm.obj \
 i_sound.obj \
 i_file.obj \
 i_gamma.obj \
 linearh.obj \
 linearh2.obj \
 linearh3.obj \
 linearh4.obj \
 linearh5.obj \
 lineahKN.obj \
 lineahPE.obj \
 linearl.obj \
 linearl2.obj \
 linearl3.obj \
 linearl4.obj \
 linearl5.obj \
 linealKN.obj \
 linealPE.obj \
 linearp.obj \
 linearp2.obj \
 linearp3.obj \
 linearp4.obj \
 linearp5.obj \
 lineapPE.obj \
 planar.obj \
 planar2.obj \
 planar3.obj \
 planar4.obj \
 planar5.obj \
 planar6.obj \
 planarKN.obj \
 planarSX.obj \
 vbe2dh.obj \
 vbe2dh2.obj \
 vbe2dh3.obj \
 vbe2dh4.obj \
 vbe2dh5.obj \
 vbe2dhPE.obj \
 vbe2dl.obj \
 vbe2dl2.obj \
 vbe2dl3.obj \
 vbe2dl4.obj \
 vbe2dl5.obj \
 vbe2dlPE.obj \
 vbe2dp.obj \
 vbe2dp2.obj \
 vbe2dp3.obj \
 vbe2dp4.obj \
 vbe2dp5.obj \
 vbe2dpPE.obj \
 tables.obj \
 f_finale.obj \
 d_main.obj \
 d_net.obj \
 g_game.obj \
 m_menu.obj \
 m_bench.obj \
 m_misc.obj \
 am_map.obj \
 p_ceilng.obj \
 p_doors.obj \
 p_enemy.obj \
 p_floor.obj \
 p_inter.obj \
 p_lights.obj \
 p_map.obj \
 p_maputl.obj \
 p_plats.obj \
 p_pspr.obj \
 p_setup.obj \
 p_sight.obj \
 p_spec.obj \
 p_switch.obj \
 p_mobj.obj \
 p_telept.obj \
 p_saveg.obj \
 p_tick.obj \
 p_user.obj \
 r_bsp.obj \
 r_data.obj \
 r_draw.obj \
 r_main.obj \
 r_plane.obj \
 r_segs.obj \
 r_things.obj \
 w_wad.obj \
 v_video.obj \
 z_zone.obj \
 st_stuff.obj \
 st_lib.obj \
 hu_stuff.obj \
 hu_lib.obj \
 wi_stuff.obj \
 s_sound.obj \
 sounds.obj \
 dutils.obj \
 f_wipe.obj \
 info.obj \
 dmx.obj \
 mus2mid.obj \
 fpummx.obj

fdoom.exe : $(GLOBOBJS)
	wlink @fdoom.lnk

%.obj : %.c
	wcc386 $(CCOPTS) $^ -fo=$@

%.obj : %.asm
	nasm -g $(NASMOPTS) -Oxv -f coff $^ -o $@

DELCMD=rm -f

clean:
	pwd
	-$(DELCMD) *.exe
	-$(DELCMD) *.map
	-$(DELCMD) *.err
	-$(DELCMD) *.obj
	-$(DELCMD) *.sym
	-$(DELCMD) *.EXE
	-$(DELCMD) *.MAP
	-$(DELCMD) *.ERR
	-$(DELCMD) *.OBJ
	-$(DELCMD) *.SYM

