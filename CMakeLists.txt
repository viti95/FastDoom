# To use this example as a standalone project using CMake:
# mkdir Retro68
# cd Retro68
# cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
# make

add_application(FASTDOOM
    FASTDOOM/i_random.c
    FASTDOOM/i_soundmac.c
    FASTDOOM/info.c
    FASTDOOM/m_bench.c
    FASTDOOM/m_menu.c
    FASTDOOM/m_misc.c
    FASTDOOM/math.c
    FASTDOOM/mus2mid.c
    FASTDOOM/p_ceilng.c
    FASTDOOM/p_doors.c
    FASTDOOM/p_enemy.c
    FASTDOOM/p_floor.c
    FASTDOOM/p_inter.c
    FASTDOOM/p_lights.c
    FASTDOOM/p_map.c
    FASTDOOM/p_maputl.c
    FASTDOOM/p_mobj.c
    FASTDOOM/p_plats.c
    FASTDOOM/p_pspr.c
    FASTDOOM/p_saveg.c
    FASTDOOM/p_setup.c
    FASTDOOM/p_sight.c
    FASTDOOM/p_spec.c
    FASTDOOM/p_switch.c
    FASTDOOM/p_telept.c
    FASTDOOM/p_tick.c
    FASTDOOM/p_user.c
    FASTDOOM/r_bsp.c
    FASTDOOM/r_data.c
    FASTDOOM/r_draw.c
    FASTDOOM/r_main.c
    FASTDOOM/r_plane.c
    FASTDOOM/r_segs.c
    FASTDOOM/r_things.c
    FASTDOOM/s_soundmac.c
    FASTDOOM/sounds.c
    FASTDOOM/st_lib.c
    FASTDOOM/st_stuff.c
    FASTDOOM/tables.c
    FASTDOOM/v_video.c
    FASTDOOM/w_wad.c
    FASTDOOM/wi_stuff.c
    FASTDOOM/z_zone.c
    FASTDOOM/am_map.c
    FASTDOOM/d_main.c
    FASTDOOM/d_net.c
    FASTDOOM/dmx.c
    FASTDOOM/dutils.c
    FASTDOOM/f_finale.c
    FASTDOOM/f_wipe.c
    FASTDOOM/g_game.c
    FASTDOOM/hu_lib.c
    FASTDOOM/hu_stuff.c
    FASTDOOM/i_debug.c
    FASTDOOM/i_file.c
    FASTDOOM/i_ibm.c
    FASTDOOM/i_main.c
    FASTDOOM/i_mac.c
    )
target_link_libraries(FASTDOOM "-lm")

# save 200KB of code by removing unused stuff
set_target_properties(FASTDOOM PROPERTIES COMPILE_OPTIONS "-DSCREENWIDTH=320;-DSCREENHEIGHT=240;-DMAC;-mcpu=68000;-O3;-fgcse-sm" LINK_FLAGS "-Wl,-gc-sections")
