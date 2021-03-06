
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
#CCOPTS = $(EXTERNOPT) /omaxet /oh /ol+ /zp4 /4r /ei /j /zq

# Build options for 386DX/SX
CCOPTS = $(EXTERNOPT) /omaxet /oh /ol+ /zp4 /3r /ei /j /zq

# Build options for Pentium
#CCOPTS = $(EXTERNOPT) /omaxet /oh /ol+ /zp4 /5r /ei /j /zq

# Build options for profiling (Pentium required)
#CCOPTS = $(EXTERNOPT) /omaxet /oh /ol+ /zp4 /5r /ei /j /zq /et

GLOBOBJS_Y = &
 ns_dpmi.obj &
 ns_pcfx.obj &
 ns_task.obj &
 ns_llm.obj &
 ns_dma.obj &
 ns_irq.obj &
 ns_user.obj &
 ns_mp401.obj &
 ns_sb.obj &
 ns_sbmus.obj &
 ns_pas16.obj &
 ns_mix16.obj &
 ns_mix.obj &
 ns_gmtbr.obj &
 ns_scape.obj &
 ns_awe32.obj &
 ns_dsney.obj &
 ns_usrho.obj &
 ns_gusmi.obj &
 ns_gusau.obj &
 ns_gus.obj &
 ns_midi.obj &
 ns_music.obj &
 ns_fxm.obj &
 ns_multi.obj &
 ns_speak.obj &
 ns_lpt.obj &
 i_debug.obj &
 i_random.obj &
 i_main.obj &
 i_ibm.obj &
 i_sound.obj &
 planar.obj &
 tables.obj &
 f_finale.obj &
 d_main.obj &
 d_net.obj &
 g_game.obj &
 m_menu.obj &
 m_misc.obj &
 am_map.obj &
 p_ceilng.obj &
 p_doors.obj &
 p_enemy.obj &
 p_floor.obj &
 p_inter.obj &
 p_lights.obj &
 p_map.obj &
 p_maputl.obj &
 p_plats.obj &
 p_pspr.obj &
 p_setup.obj &
 p_sight.obj &
 p_spec.obj &
 p_switch.obj &
 p_mobj.obj &
 p_telept.obj &
 p_saveg.obj &
 p_tick.obj &
 p_user.obj &
 r_bsp.obj &
 r_data.obj &
 r_draw.obj &
 r_main.obj &
 r_sky.obj &
 r_plane.obj &
 r_segs.obj &
 r_things.obj &
 w_wad.obj &
 v_video.obj &
 z_zone.obj &
 st_stuff.obj &
 st_lib.obj &
 hu_stuff.obj &
 hu_lib.obj &
 wi_stuff.obj &
 s_sound.obj &
 sounds.obj &
 dutils.obj &
 f_wipe.obj &
 info.obj &
 dmx.obj &
 mus2mid.obj

GLOBOBJS_13H = &
 ns_dpmi.obj &
 ns_pcfx.obj &
 ns_task.obj &
 ns_llm.obj &
 ns_dma.obj &
 ns_irq.obj &
 ns_user.obj &
 ns_mp401.obj &
 ns_sb.obj &
 ns_sbmus.obj &
 ns_pas16.obj &
 ns_mix16.obj &
 ns_mix.obj &
 ns_gmtbr.obj &
 ns_scape.obj &
 ns_awe32.obj &
 ns_dsney.obj &
 ns_usrho.obj &
 ns_gusmi.obj &
 ns_gusau.obj &
 ns_gus.obj &
 ns_midi.obj &
 ns_music.obj &
 ns_fxm.obj &
 ns_multi.obj &
 ns_speak.obj &
 ns_lpt.obj &
 i_debug.obj &
 i_random.obj &
 i_main.obj &
 i_ibm.obj &
 i_sound.obj &
 linear.obj &
 tables.obj &
 f_finale.obj &
 d_main.obj &
 d_net.obj &
 g_game.obj &
 m_menu.obj &
 m_misc.obj &
 am_map.obj &
 p_ceilng.obj &
 p_doors.obj &
 p_enemy.obj &
 p_floor.obj &
 p_inter.obj &
 p_lights.obj &
 p_map.obj &
 p_maputl.obj &
 p_plats.obj &
 p_pspr.obj &
 p_setup.obj &
 p_sight.obj &
 p_spec.obj &
 p_switch.obj &
 p_mobj.obj &
 p_telept.obj &
 p_saveg.obj &
 p_tick.obj &
 p_user.obj &
 r_bsp.obj &
 r_data.obj &
 r_draw.obj &
 r_main.obj &
 r_sky.obj &
 r_plane.obj &
 r_segs.obj &
 r_things.obj &
 w_wad.obj &
 v_video.obj &
 z_zone.obj &
 st_stuff.obj &
 st_lib.obj &
 hu_stuff.obj &
 hu_lib.obj &
 wi_stuff.obj &
 s_sound.obj &
 sounds.obj &
 dutils.obj &
 f_wipe.obj &
 info.obj &
 dmx.obj &
 mus2mid.obj

GLOBOBJS_VBE = &
 ns_dpmi.obj &
 ns_pcfx.obj &
 ns_task.obj &
 ns_llm.obj &
 ns_dma.obj &
 ns_irq.obj &
 ns_user.obj &
 ns_mp401.obj &
 ns_sb.obj &
 ns_sbmus.obj &
 ns_pas16.obj &
 ns_mix16.obj &
 ns_mix.obj &
 ns_gmtbr.obj &
 ns_scape.obj &
 ns_awe32.obj &
 ns_dsney.obj &
 ns_usrho.obj &
 ns_gusmi.obj &
 ns_gusau.obj &
 ns_gus.obj &
 ns_midi.obj &
 ns_music.obj &
 ns_fxm.obj &
 ns_multi.obj &
 ns_speak.obj &
 ns_lpt.obj &
 i_debug.obj &
 i_random.obj &
 i_main.obj &
 i_ibm.obj &
 i_sound.obj &
 i_vesa.obj &
 linear.obj &
 tables.obj &
 f_finale.obj &
 d_main.obj &
 d_net.obj &
 g_game.obj &
 m_menu.obj &
 m_misc.obj &
 am_map.obj &
 p_ceilng.obj &
 p_doors.obj &
 p_enemy.obj &
 p_floor.obj &
 p_inter.obj &
 p_lights.obj &
 p_map.obj &
 p_maputl.obj &
 p_plats.obj &
 p_pspr.obj &
 p_setup.obj &
 p_sight.obj &
 p_spec.obj &
 p_switch.obj &
 p_mobj.obj &
 p_telept.obj &
 p_saveg.obj &
 p_tick.obj &
 p_user.obj &
 r_bsp.obj &
 r_data.obj &
 r_draw.obj &
 r_main.obj &
 r_sky.obj &
 r_plane.obj &
 r_segs.obj &
 r_things.obj &
 w_wad.obj &
 v_video.obj &
 z_zone.obj &
 st_stuff.obj &
 st_lib.obj &
 hu_stuff.obj &
 hu_lib.obj &
 wi_stuff.obj &
 s_sound.obj &
 sounds.obj &
 dutils.obj &
 f_wipe.obj &
 info.obj &
 dmx.obj &
 mus2mid.obj

GLOBOBJS_TEXT = &
 ns_dpmi.obj &
 ns_pcfx.obj &
 ns_task.obj &
 ns_llm.obj &
 ns_dma.obj &
 ns_irq.obj &
 ns_user.obj &
 ns_mp401.obj &
 ns_sb.obj &
 ns_sbmus.obj &
 ns_pas16.obj &
 ns_mix16.obj &
 ns_mix.obj &
 ns_gmtbr.obj &
 ns_scape.obj &
 ns_awe32.obj &
 ns_dsney.obj &
 ns_usrho.obj &
 ns_gusmi.obj &
 ns_gusau.obj &
 ns_gus.obj &
 ns_midi.obj &
 ns_music.obj &
 ns_fxm.obj &
 ns_multi.obj &
 ns_speak.obj &
 ns_lpt.obj &
 i_debug.obj &
 i_random.obj &
 i_main.obj &
 i_ibm.obj &
 i_sound.obj &
 tables.obj &
 f_finale.obj &
 d_main.obj &
 d_net.obj &
 g_game.obj &
 m_menu.obj &
 m_misc.obj &
 am_map.obj &
 p_ceilng.obj &
 p_doors.obj &
 p_enemy.obj &
 p_floor.obj &
 p_inter.obj &
 p_lights.obj &
 p_map.obj &
 p_maputl.obj &
 p_plats.obj &
 p_pspr.obj &
 p_setup.obj &
 p_sight.obj &
 p_spec.obj &
 p_switch.obj &
 p_mobj.obj &
 p_telept.obj &
 p_saveg.obj &
 p_tick.obj &
 p_user.obj &
 r_bsp.obj &
 r_data.obj &
 r_draw.obj &
 r_main.obj &
 r_sky.obj &
 r_plane.obj &
 r_segs.obj &
 r_things.obj &
 w_wad.obj &
 v_video.obj &
 z_zone.obj &
 st_stuff.obj &
 st_lib.obj &
 hu_stuff.obj &
 hu_lib.obj &
 wi_stuff.obj &
 s_sound.obj &
 sounds.obj &
 dutils.obj &
 f_wipe.obj &
 info.obj &
 dmx.obj &
 mus2mid.obj

fdoomy.exe : $(GLOBOBJS_Y) 
 wlink @fdoomy.lnk 

fdoom13h.exe : $(GLOBOBJS_13H) 
 wlink @fdoom13h.lnk 

fdoomvbe.exe : $(GLOBOBJS_VBE)
 wlink @fdoomvbe.lnk

fdoomtxt.exe : $(GLOBOBJS_TEXT) 
 wlink @fdoomtxt.lnk 

.c.obj :
 wcc386 $(CCOPTS) $[*

.asm.obj :
 tasm /mx /q $[*

clean : .SYMBOLIC
 del *.exe
 del *.map
 del *.err
 del *.obj
 del *.sym