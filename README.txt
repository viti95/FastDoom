                                                                          
 ######## ######## ######## ######## ######## ######## ######## ###    ###  
 ###      ###  ### ###  ### ######## ###  ### ###  ### ###  ### ##########  
 ###      ###  ### ####       ####   ###  ### ###  ### ###  ### ##########  
 ######   ########   ######   ####   ###  ### ###  ### ###  ### ##########  
 ###      ########      ###   ####   ###  ### ###  ### ###  ### ##########  
 ###       ##  ### ########   ####   ######## ###  ### ###  ### ### ## ###  
 ###           #     ###      #      ######   #### ### #### ###   #    ###  
 ###                                 ###       ######   ######         ###  
 #                                                #       #              #  

 Version
 -------
 1.1.6

 About
 -----

 FastDoom is a Doom port for DOS, based on PCDoom by @nukeykt. The goal of
 this port is to make it as fast as posible for 386/486 personal computers

 https://github.com/viti95/FastDoom

 Motivation
 ----------
 
 Vanilla Doom required a powerful machine when it was released in 1993. 
 To run it properly, a 486 was mandatory, with at least 8 MB of RAM and a 
 Vesa Local Bus video card. A machine with those specs in 1993 was
 pretty damn expensive. The game itself run pretty slow on 386 PCs
 even at low detail and small screen size. I started this project in April 
 2020, after suffering the COVID-19 disease, nearly 27 years after release
 of Doom. I was very sick, and after the recovery I decided that somebody 
 had to make Doom run well on all 386 personal computers, and make their 
 owners proud of their machines.

 Minimum requirements
 --------------------
 
 CPU:        386SX at 16 MHz
 RAM:        4 MB (the executable runs even with less RAM, but it's 
             possible to see random crashes due to low RAM)
 Video card: Any ISA 8-bit Hercules, CGA, EGA or VGA video card. For cool
             graphics a fast VGA video card is recommended. Also MDA,
             Plantronics Colorplus, Sigma Color 400 and Hercules InColor 
             cards are supported
 Controls:   Keyboard (mouse recommended)

 Supported IWADs
 ---------------

 FastDoom guarantees support for the following IWADs, provided the correct 
 file names and specified versions are used:

  DOOM Registered (3 episodes) -> DOOM.WAD (v1.9, 11159840 bytes)
  DOOM Shareware (1 episode) -> DOOM1.WAD (v1.9, 4196020 bytes)
  Ultimate DOOM (4 episodes) -> DOOMU.WAD (v1.9, 12408292 bytes)
  DOOM II -> DOOM2.WAD (v1.9, 14604584 bytes)
  Final DOOM The Plutonia Experiment -> PLUTONIA.WAD (v1.9, 17420824 
                                                      or 18240172 bytes)
  Final DOOM TNT Evilution -> TNT.WAD (v1.9, 18195736 or 18654796 bytes)
  FreeDoom Phase 1 -> FREEDM1.WAD (experimental support)
  FreeDoom Phase 2 -> FREEDM2.WAD (experimental support)

 Any other version used may cause issues and not function correctly. 
 Remember to rename the IWADs according to your needs.

 Supported hardware
 ------------------
 
 CPU: Any x86 processor that supports 32 bit i386 instruction set
 Video cards: MDA, Hercules, CGA, EGA, VGA, Plantronics ColorPlus
              Sigma Color 400, Hercules InColor, SVGA (VBE)
 Sound cards: Sound Blaster, PC Speaker, Disney Sound Source,
              Gravis Ultrasound, Pro Audio Spectrum, COVOX LPT DAC,
              Creative Music System, Tandy 3-voice, OPL2LPT,
              OPL3LPT, Ensoniq Soundscape, Adlib
 Music cards: Sound Blaster (OPL2 and OPL3), Adlib, MIDI, Gravis Ultrasound
              Sound Blaster AWE32, OPL2LPT, OPL3LPT, AudioCD (MSCDEX),
              PCM music (through sound card), Ensoniq Soundscape
              Serial MIDI, DreamBlaster S2P, Roland MT-32,
              Roland SC-55, Yamaha MU80, Yamaha TG300

 Executables
 -----------

 * FDOOM.EXE    => FastDoom Mode Y, same as Vanilla Doom. Requires a VGA
                   video card.
 * FDOOMX.EXE   => FastDoom Mode X, 320x240 resolution.
 * FDOOMH.EXE   => FastDoom Mode Y half height, 320x100 resolution.
 * FDOOM13H.EXE => FastDoom Mode 13h, same as Heretic / Hexen. Also works
                   with MCGA video cards.
 * FDOOMINC.EXE => FastDoom 320x200 Hercules InColor mode (16 colors)
 * FDOOMCGA.EXE => FastDoom 320x200 4 color CGA.
 * FDOOMEGA.EXE => FastDoom 320x200 16 color EGA.
 * FDOOMBWC.EXE => FastDoom 640x200 monochrome CGA.
 * FDOOMHGC.EXE => FastDoom 640x400 monochrome Hercules.
 * FDOOMT1.EXE  => FastDoom 40x25 16 colors text mode
 * FDOOMT12.EXE => FastDoom 40x25 16 colors text mode (virtual resolution of
                   40x50)
 * FDOOMT25.EXE => FastDoom 80x25 16 colors text mode (virtual resolution of
                   80x50)
 * FDOOMT50.EXE => FastDoom 80x50 16 colors text mode
 * FDOOMT43.EXE => FastDoom 80x43 16 colors text mode (only EGA cards)
 * FDOOMVBR.EXE => FastDoom for VBE 2.0 cards. Uses real mode (default, and
                   more compatible).
 * FDOOMVBD.EXE => FastDoom for VBE 2.0 cards with LFB (triple buffered),
                   all rendering is done directly onto the video card, the
                   same way Mode Y does.
 * FDOOMPCP.EXE => FastDoom for Plantronics ColorPlus cards. 320x200 and 
                   16 colors.
 * FDOOMCVB.EXE => FastDoom CGA composite mode. 160x200 and 16 colors.
 * FDOOMC16.EXE => FastDoom CGA 160x100 and 16 colors
 * FDOOMMDA.EXE => FastDoom MDA 80x25 text mode. Internal resolution 80x50.
                   Very quick'n'dirty, only Neo can play this mode properly.
 * FDOOMCDA.EXE => FastDoom MDA Color 80x25 text mode (virtual resolution of
                   80x50). Works only on revision 0 and modded cards
 * FDOOMCAH.EXE => FastDoom CGA ANSI from Hell (320x100, 16 colors)
 * FDOOM512.EXE => FastDoom CGA "512 color" composite mode (80x100)
 * FDOOM400.EXE => FastDoom Sigma Color 400 (320x200, 16 colors)
 * FDM240R.EXE  => FastDoom VESA 320x240 backbuffered mode
 * FDM240D.EXE  => FastDoom VESA 320x240 direct rendering mode
 * FDM300R.EXE  => FastDoom VESA 400x300 backbuffered mode
 * FDM300D.EXE  => FastDoom VESA 400x300 direct rendering mode
 * FDM384R.EXE  => FastDoom VESA 512x384 backbuffered mode
 * FDM384D.EXE  => FastDoom VESA 512x384 direct rendering mode
 * FDM400R.EXE  => FastDoom VESA 640x400 backbuffered mode
 * FDM400D.EXE  => FastDoom VESA 640x400 direct rendering mode
 * FDM480R.EXE  => FastDoom VESA 640x480 backbuffered mode
 * FDM480D.EXE  => FastDoom VESA 640x480 direct rendering mode
 * FDM600R.EXE  => FastDoom VESA 800x600 backbuffered mode
 * FDM600D.EXE  => FastDoom VESA 800x600 direct rendering mode
 * FDM768R.EXE  => FastDoom VESA 1024x768 backbuffered mode
 * FDM768D.EXE  => FastDoom VESA 1024x768 direct rendering mode
 * FDM800R.EXE  => FastDoom VESA 1280x800 backbuffered mode
 * FDM800D.EXE  => FastDoom VESA 1280x800 direct rendering mode
 * FDM1024R.EXE => FastDoom VESA 1280x1024 backbuffered mode
 * FDM1024D.EXE => FastDoom VESA 1280x1024 direct rendering mode
 * FDM1200R.EXE => FastDoom VESA 1600x1200 backbuffered mode
 * FDM1200D.EXE => FastDoom VESA 1600x1200 direct rendering mode
 * FDSETUP.EXE  => Utility to setup controls and sound cards
 * FDBENCH.EXE  => Utility to make benchmarks easier to execute
 * BENCH.BAT    => Scripted benchmark, instructions are included in the script

 Command line parameters
 -----------------------

 -respawn => Forces monsters respawn like in Nightmare mode
 -fast => Forces fast monsters like in Nightmare mode
 -potato => Forces potato detail mode (80x200)
 -low => Forces low detail mode (160x200)
 -high => Forces high detail mode (320x200)
 -cga => Fixes text modes for CGA cards
 -pagefix => Fixes text modes (80x50 and 80x100) for newer VGA cards
 -reverseStereo => Reverse audio output (left to right and viceversa)
 -csv => Saves the timedemo result in the file bench.csv
 -size XX => Forces screen scaling
 -file => Loads an external PWAD
 -playdemo XX => Plays a stored demo
 -timedemo XX => Benchmarks a stored demo
 -skill X => Chooses a skill level
 -episode X => Starts one episode automatically
 -warp XX => Starts a game level
 -fps => Shows in-game frame rate (frames per second)
 -flatterSpan => Forces visplanes to be rendered as flat colors (depth
                 illumination is enabled)
 -flatSpan => Forces visplanes to be rendered as flat colors (depth
              illumination is disabled)
 -flatsky => Renders game skies as a flat color
 -flatInv => Renders transparent enemies/player/items without fuzzy 
             transparency
 -flatsaturn => Renders transparent enemies/player/items with a half-tone
            black pattern
 -saturn => Renders transparent enemies/player/items with a half-tone
            pattern
 -translucent => Renders transparent enemies/player/items using real 
                 transparency
 -mono => Forces audio to be mono only
 -near => Renders close items only
 -nomelt => Disables melting transition (for 386 processors)
 -vsync => Forces screen updates synchronized with the VSync
 -defSpan => Disable any optimization on visplanes
 -defSky => Disable any optimization on skies
 -defInv => Disable any optimization on transparent things
 -far => Disables sprite culling
 -stereo => Forces stereo sound
 -melt => Enables screen melting transitions
 -novsync => Disables VSync
 -nofps => Hides fps ingame
 -record => Stores a demo lump containing player actions ingame
 -loadgame => Plays a demo lump
 -maxdemo => Defines maximum size for a demo lump to record
 -nomouse => Disables mouse control
 -nosound => Disables all sound/music devices
 -nosfx => Disables sound
 -nomusic => Disables music
 -config XX => Loads an alternative configuration file 
 -disabledemo => Disables defered demos
 -debugCard2 => Shows FPS via debug card (2 digits, port 0x80)
 -debugCard4 => Shows FPS via debug card (4 digits, port 0x80)
 -fixDAC => Fixes palette corruption with VGA cards
 -hercmap => Enable Hercules automap (requires dual video card setup)
 -snow => Fix for snow on IBM CGA cards
 -palette1 => Choose the black-cyan-magenta-white palette on mode
              CGA 320x200 4-color
 -complevel X => Force any compatibility level. Supported
                 compatibility levels:
                 2 - Doom 1.9 (Also Doom II)
                 3 - Ultimate Doom
                 4 - Final Doom
 -iwad X => Load an IWAD file
 -sbk X => Load a SBK soundfont for AWE32/AWE64 soundcards
 -benchmark file XX YY => Run multiple XX demo benchmarks, using
                          configuration benchmark YY
 -benchmark single XX => Run XX demo benchmark and save results 
                         in a CSV file
 -advanced => Run frametime analysis on benchmarks. Only works with
              command line parameter "-benchmark"
 -umc486 => Use UMC Green 486 codepath
 -i486 => Use Intel 486 codepath
 -cy386 => Use 386SLC/386DLC codepath
 -cy486 => Use Cyrix Cx486 codepath
 -386dx => Use Intel 386DX codepath
 -386sx => Use Intel 386SX codepath
 -cy5x86 => Use Cyrix 5x86 codepath
 -k5 => Use AMD K5 codepath
 -pentium => Use Intel Pentium codepath
 -limitram 32768 => Limit maximum memory to 32MB
 -freeram 128 => Leaves 128 KB free
 -uncapped => Enable uncapped FPS mode (>35 fps)
 -capped => Disable uncapped FPS mode (max 35 fps)
 -8bpp => (Only VBE2 modes) force the use of video modes with
          8 bits per pixel
 -15bpp => (Only VBE2 modes) force the use of video modes with
           15 bits per pixel
 -16bpp => (Only VBE2 modes) force the use of video modes with
           16 bits per pixel
 -24bpp => (Only VBE2 modes) force the use of video modes with
           24 bits per pixel
 -32bpp => (Only VBE2 modes) force the use of video modes with
           32 bits per pixel
 -noLFB => (Only VBE2 modes) disables Linear FrameBuffer video
           modes. Slower, use only if there are compatibility
           issues
 -xt => Enable XT keyboard support

 Limitations / Known bugs
 ------------------------
 * Gravis UltraSound cards require IRQ to be 7 or less, otherwise those
   cards won't work. This is a limitation of the Apogee Sound System.
 * Some executables will show snow issues on IBM CGA cards even if '-snow'
   command line parameter is used. This is due to technical limitations.
 * Showing FPS on a debug card can show weird values if a Sound Blaster
   card is used, and can cause sound issues. This is due to port 0x80 
   being also used for DMA transfers.
 * No sound with SBEMU: FastDoom requires VCPI to be disabled before
   running the game. Run 'jemmex.exe novcpi' before launching FastDoom.
 * Older VGA cards may exhibit snow issues during palette changes. 
   These cards need to wait for VSYNC before changing the palette. 
   Use the '-fixDAC' command line option to fix this.
 * Some newer VGA cards may display a distorted image in 80x50 text 
   mode. This is caused by a different video page size on newer cards
   compared to older ones. Use the '-pagefix' option to fix this.
 * VBE2 Direct modes (fdm***d.exe and fdoomvbd.exe) do not support 15-bit,
   16-bit, 24-bit, or 32-bit video modes.

 PCM music format
 ----------------

 PCM music format is unsigned 8-bit PCM, and supports 11025, 22050 or 
 44100 Hz frequencies. 
 
 Use SOX to convert files to RAW format, for example:

 sox D_E1M1.ogg -r 44100 -e unsigned -b 8 -c 1 MUS_1.RAW
 
 Folders used for music are these:
 * Doom:     \MUSIC\DOOM1\MUS_*.RAW
 * Doom2:    \MUSIC\DOOM2\MUS_*.RAW
 * TNT:      \MUSIC\TNT\MUS_*.RAW
 * Plutonia: \MUSIC\PLUTONIA\MUS_*.RAW

 PCM music / Audio-CD tracks numbers
 -----------------------------------

 Doom Shareware / Doom / Ultimate Doom

 1: mus_e1m1
 2: mus_e1m2
 3: mus_e1m3
 4: mus_e1m4
 5: mus_e1m5
 6: mus_e1m6, mus_e3m6
 7: mus_e1m7, mus_e2m5, mus_e3m5
 8: mus_e1m8, mus_e3m4
 9: mus_e1m9, mus_e3m9
 10: mus_e2m1
 11: mus_e2m2
 12: mus_e2m3, mus_inter
 13: mus_e2m4
 14: mus_e2m6
 15: mus_e2m7, mus_e3m7
 16: mus_e2m8
 17: mus_e2m9, mus_e3m1
 18: mus_e3m2
 19: mus_e3m3
 20: mus_e3m8
 21: mus_intro, mus_introa
 22: mus_bunny
 23: mus_victor

 Doom 2

 1: mus_runnin - MAP1, mus_stlks2 - MAP11, mus_runni2 - MAP15
 2: mus_stalks - MAP2, mus_stlks3 - MAP17
 3: mus_countd - MAP3, mus_count2 - MAP21
 4: mus_betwee - MAP4
 5: mus_doom - MAP5, mus_doom2 - MAP13
 6: mus_the_da - MAP6, mus_theda2 - MAP12, mus_theda3 - MAP24
 7: mus_shawn - MAP7, mus_shawn2 - MAP19, mus_shawn3 - MAP29
 8: mus_ddtblu - MAP8, mus_ddtbl2 - MAP14, mus_ddtbl3 - MAP22
 9: mus_in_cit - MAP9
 10: mus_dead - MAP10, mus_dead2 - MAP16
 11: mus_romero - MAP18, mus_romer2 - MAP27
 12: mus_messag - MAP20, mus_messg2 - MAP26
 13: mus_ampie - MAP23
 14: mus_adrian - MAP25
 15: mus_tense - MAP28
 16: mus_openin - MAP30
 17: mus_evil - MAP31
 18: mus_ultima - MAP32
 19: mus_read_m
 20: mus_dm2ttl
 21: mus_dm2int

 TNT: Evilution

 1: mus_runnin - MAP1, mus_in_cit - MAP9
 2: mus_stalks - MAP2, mus_runni2 - MAP15
 3: mus_countd - MAP3
 4: mus_betwee - MAP4, mus_doom2 - MAP13, mus_shawn3 - MAP29
 5: mus_doom - MAP5, mus_stlks3 - MAP17
 6: mus_the_da - MAP6
 7: mus_shawn - MAP7
 8: mus_ddtblu - MAP8, mus_romer2 - MAP27, mus_openin - MAP30
 9: mus_dead - MAP10, mus_romero - MAP18
 10: mus_stlks2 - MAP11
 11: mus_theda2 - MAP12
 12: mus_ddtbl2 - MAP14
 13: mus_dead2 - MAP16, mus_messg2 - MAP26
 14: mus_shawn2 - MAP19
 15: mus_messag - MAP20
 16: mus_count2 - MAP21, mus_ultima - MAP32
 17: mus_ddtbl3 - MAP22, mus_tense - MAP28
 18: mus_ampie - MAP23
 19: mus_theda3 - MAP24
 20: mus_adrian - MAP25
 21: mus_evil - MAP31, mus_dm2int
 22: mus_read_m
 23: mus_dm2ttl

 The Plutonia Experiment

 1: mus_runnin - MAP1
 2: mus_stalks - MAP2
 3: mus_countd - MAP3
 4: mus_betwee - MAP4
 5: mus_doom - MAP5
 6: mus_the_da - MAP6, mus_evil - MAP31
 7: mus_shawn - MAP7, mus_romer2 - MAP27
 8: mus_ddtblu - MAP8, mus_tense - MAP28
 9: mus_in_cit - MAP9
 10: mus_dead - MAP10, mus_romero - MAP18
 11: mus_stlks2 - MAP11
 12: mus_theda2 - MAP12
 13: mus_doom2 - MAP13
 14: mus_ddtbl2 - MAP14, mus_ultima - MAP32
 15: mus_runni2 - MAP15
 16: mus_dead2 - MAP16
 17: mus_stlks3 - MAP17, mus_shawn3 - MAP29
 18: mus_shawn2 - MAP19
 19: mus_messag - MAP20, mus_messg2 - MAP26
 20: mus_count2 - MAP21, mus_read_m
 21: mus_ddtbl3 - MAP22
 22: mus_ampie - MAP23
 23: mus_theda3 - MAP24
 24: mus_adrian - MAP25
 25: mus_openin - MAP30
 26: mus_dm2ttl
 27: mus_dm2int

 Author
 ------
 
 ViTi95

 Contributors (in no particular order)
 -------------------------------------

 JNechaevsky
 RamonUnch
 PickledDog
 FrenkelS
 jsmolina
 deat322
 AXDOOMER
 bnied
 Tronix286
 noop0x90
 DeathEgg
 CarlosTex
 FreddoUK
 maximilien-noal
 ghderty
 appiah4
 Redneckerz
 javiergutierrezchamorro
 SpitFire-666
 Optimus6128
 Shogun38
 viciious
 cher-nov
 neuralnetworkz
 Ethaniel-404
 zokum-no
 FavoritoHJS
 rasz_pl
 Doug Johnson
 efliks
 crazii
 tigrouind
 DiSCATTe

 And pretty sure more people I don't remember right now, 
 if you're not in the list contact me :)

 Issues
 ------
 
 Have you found one? Report it to the FastDoom GitHub, i'll try to fix
 it ASAP!

 https://github.com/viti95/FastDoom/issues
