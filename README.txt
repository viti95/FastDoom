                                                                          
 ######## ######## ######## ######## ######## ######## ######## ###    ###  
 ###      ###  ### ###  ### ######## ###  ### ###  ### ###  ### ##########  
 ###      ###  ### ####       ####   ###  ### ###  ### ###  ### ##########  
 ######   ########   ######   ####   ###  ### ###  ### ###  ### ##########  
 ###      ########      ###   ####   ###  ### ###  ### ###  ### ##########  
 ###       ##  ### ########   ####   ######## ###  ### ###  ### ### ## ###  
 ###           #     ###      #      ######   #### ### #### ###   #    ###  
 ###                                 ###       ######   ######         ###  
 #                                                #       #              #  

 About
 -----

 FastDoom is a Doom port for DOS, based on PCDoom by @nukeykt. The goal of
 this port is to make it as fast as posible for 386/486 personal computers

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
             graphics a fast VGA video card is recommended
 Controls:   Keyboard (mouse recommended)

 Supported WADS
 --------------

 The supported wads are the following:
    * DOOM.WAD -> DOOM Registered (3 episodes)
    * DOOM1.WAD -> DOOM Shareware (1 episode)
    * DOOMU.WAD -> Ultimate DOOM (4 episodes)
    * DOOM2.WAD -> DOOM II
    * PLUTONIA.WAD -> Final DOOM The Plutonia Experiment
    * TNT.WAD -> Final DOOM TNT Evilution 

 All of them must be the 1.9 version to work fine.

 Supported hardware
 ------------------
 
 CPU: Any x86 processor that supports 32 bit i386 instruction set
 Video cards: Hercules, CGA, EGA, VGA, Plantronics ColorPlus, 
              ATI Small Wonder
 Sound cards: Sound Blaster, PC Speaker, Disney Sound Source,
              Gravis Ultrasound, Pro Audio Spectrum, COVOX LPT DAC
 Music cards: Sound Blaster (OPL2 and OPL3), Adlib, MIDI, Gravis Ultrasound
              Sound Blaster AWE32

 Executables
 -----------

 * FDOOM.EXE    => FastDoom Mode Y, same as Vanilla Doom. Requires a VGA
                   video card
 * FDOOM13H.EXE => FastDoom Mode 13h, same as Heretic / Hexen. Only supports
                   high detail mode. Smoother with fast machines.
 * FDOOMCGA.EXE => FastDoom 320x200 4 color CGA. Ugly. Very ugly. Requires
                   a fast CPU.
 * FDOOMEGA.EXE => FastDoom 320x200 16 color EGA. Requires a very fast CPU.
                   ISA 8-bit EGA cards are usually very slow, so don't
                   expect this to run well (10 fps)
 * FDOOMBWC.EXE => FastDoom 640x200 monochrome CGA. Requires a fast CPU.
                   Uses ordered dithering 2x2. Internal resolution 320x200.
 * FDOOMHGC.EXE => FastDoom 640x400 monochrome Hercules. Requires a fast
                   CPU. Also uses ordered dithering 2x2. Internal resolution
                   320x200
 * FDOOMT1.EXE  => FastDoom 40x25 16 colors text mode
 * FDOOMT12.EXE => FastDoom 40x25 16 colors text mode (virtual resolution of
                   40x50)
 * FDOOMT25.EXE => FastDoom 80x25 16 colors text mode (virtual resolution of
                   80x50)
 * FDOOMT50.EXE => FastDoom 80x50 16 colors text mode
 * FDOOMT52.EXE => FastDoom 80x50 16 colors text mode (virtual resolution of
                   80x100)
 * FDOOMT43.EXE => FastDoom 80x43 16 colors text mode (only EGA cards)
 * FDOOMT86.EXE => FastDoom 80x43 16 colors text mode (virtual resolution of
                   80x86, only EGA cards)
 * FDOOMVBR.EXE => FastDoom for VBE 2.0 cards. Uses real mode (default, and
                   more compatible). If you use UniVBE use this version
 * FDOOMVBD.EXE => FastDoom for VBE 2.0 cards with LFB (triple buffered),
                   all rendering is done directly onto the video card, the
                   same way Mode Y does. Faster for systems with slow RAM
                   access.
 * FDOOMV2.EXE  => FastDoom Planar 320x350 vertical mode. The base 320x200 
                   image is centered in the screen, leaving black borders
 * FDOOMPCP.EXE => FastDoom for Plantronics ColorPlus cards. 320x200 and 
                   16 colors!
 * FDOOMCVB.EXE => FastDoom CGA composite mode. 160x200 and 16 colors!
 * FDOOMC16.EXE => FastDoom CGA 160x100 and 16 colors
 * FDOOMV16.EXE => FastDoom VGA 160x200 and 16 colors
 * FDOOME16.EXE => FastDoom EGA 160x100 and 16 colors
 * FDOOME.EXE   => FastDoom EGA 640x200. Requires a very fast CPU and 16-bit
                   ISA video card (at least). Dithering with 16 colors
 * FDOOMATI.EXE => FastDoom ATI Small Wonder 640x200. Requires a very fast 
                   CPU and 16-bit ISA video card (at least).
                   Dithering with 16 colors
 * FDOOMC36.EXE => FastDoom CGA 80x100 and 122 pseudocolors
 * FDOOMV36.EXE => FastDoom VGA 80x200 and 122 pseudocolors
 * FDOOME36.EXE => FastDoom EGA 80x100 and 122 pseudocolors
 * FDOOMMDA.EXE => FastDoom MDA 80x25 text mode. Internal resolution 80x50.
                   Very quick'n'dirty, only Neo can play this mode properly.
 * FDSETUP.EXE  => Utility to setup controls and sound cards

 Command line parameters
 -----------------------

 -nomonsters => Disables all monsters in game
 -respawn => Forces monsters respawn like in Nightmare mode
 -fast => Forces fast monsters like in Nightmare mode
 -forcePQ => Forces potato detail mode (80x200)
 -forceLQ => Forces low detail mode (160x200)
 -forceHQ => Forces high detail mode (320x200)
 -cga => Fixes text modes for CGA cards
 -pagefix => Fixes text modes (80x50 and 80x100) for newer VGA cards
 -lowsound => Plays all sounds at 8 KHz (lower cpu usage)
 -8bitsound => Forces all sounds to play at 8 bits resolution
 -ram => Allocates all memory available (default only allocates 8 MB)
 -singletics => Disables game throttling (runs at full speed) 
 -reverseStereo => Reverse audio output (left to right and viceversa)
 -logTimedemo => Saves the timedemo result in the file bench.txt
 -bfg => Enables Doom II BFG edition IWAD support
 -size XX => Forces screen scaling
 -turbo XX => Multiplies player movement speed by 10%
 -file => Loads an external PWAD
 -playdemo XX => Plays a stored demo
 -timedemo XX => Benchmarks a stored demo
 -skill X => Chooses a skill level
 -episode X => Starts one episode automatically
 -warp XX => Starts a game level
 -fps => Shows in-game frame rate (frames per second)
 -flattersurfaces => Forces visplanes to be rendered as flat colors (depth
                     illumination is enabled)
 -flatsurfaces => Forces visplanes to be rendered as flat colors (depth
                  illumination is disabled)
 -flatsky => Renders game skies as a flat color
 -flatshadows => Renders transparent enemies/player/items without fuzzy 
                 transparency
 -saturn => Renders transparent enemies/player/items with a half-tone
            pattern
 -mono => Forces audio to be mono only
 -near => Renders close items only
 -nomelt => Disables melting transition (fast for 386 processors)
 -uncapped => Disables frame rate limit (35 fps)
 -vsync => Forces screen updates synchronized with the VSync
 -simplestatusbar => Renders the status bar a simple grey color
 -normalsurfaces => Disable any optimization on visplanes
 -normalsky => Disable any optimization on skies
 -normalshadows => Disable any optimization on transparent things
 -normalsprites => Disables sprite culling
 -normalstatusbar => Renders the status bar normally
 -stereo => Forces stereo sound
 -melt => Enables screen melting transitions
 -capped => Forces 35 fps limit
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
 -LPT1 => Forces LPT1 port for Disney Sound Source
 -LPT2 => Forces LPT2 port for Disney Sound Source
 -LPT3 => Forces LPT3 port for Disney Sound Source
 -disabledemo => Disables defered demos
 -debugPort => Shows FPS via the debug port (0x80)
 -fixDAC => Fixes palette corruption with VGA cards
 -hercmap => Enable Hercules automap (requires dual video card setup)

 Author
 ------
 
 ViTi95

 Issues
 ------
 
 Have you found one? Report it to the FastDoom GitHub, i'll try to fix
 it ASAP!

 https://github.com/viti95/FastDoom/issues

                                                                      