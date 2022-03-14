# CHANGELOG

## 0.8.10 (14 Mar 2022)
* Added new video modes
    * FDOOME16.EXE => FastDoom EGA 160x100
    * FDOOME36.EXE => FastDoom EGA 80x100 (122 pseudocolors)
    * FDOOMT43.EXE => FastDoom 80x43 16 colors text mode (only EGA cards)
    * FDOOMT86.EXE => FastDoom 80x43 16 colors text mode (virtual resolution of 80x86, only EGA cards)
* Multiple optimizations for special backbuffered modes
* Better colormaps / palettes for black&white modes, and 16 color dithered modes
* Removed FDOOMV.EXE, the scaling was horrible and didn't look good at all
* Revert custom palette for VGA 160x100 mode. The attribute controller requires the display to go blank to change each color association. Not doing this causes some random screen glitches while changing the palette (which I haven't been able to solve, since the hardware works this way). VGA 256 color mode doesn't have this problem.

## 0.8.9 (26 Jan 2022)
* Added new video modes. Huge thanks to @FrenkelS
    * FDOOMC36.EXE => FastDoom CGA 80x100 (122 pseudocolors)
    * FDOOMV36.EXE => FastDoom VGA 80x200 (122 pseudocolors)
    * FDOOMATI.EXE => FastDoom ATI Small Wonder 640x200 (dithering 16 colors)
* Small optimization for Potato and Low detail column rendering
* Optimized Plantronics ColorPlus, EGA, CGA and Hercules modes
* Added custom palette for VGA 160x200 mode
* Optimized CGA 160x100 and VGA 160x200 rendering
* Remove FDOOMVBP.EXE. It was very buggy and problematic, and in 99% of the cases a little bit slower than FDOOMVBR.EXE
* Converted all ASM files to NASM. This allows to build FastDoom with 100% open source compilers. Borland Turbo Assembler requirement is now gone. Huge thanks to @0x90
* Fix screen flickering on 80x50 and 80x100 text modes. Use the parameter "-pagefix" in case it flickers
* New mode for PC Speakers. Based on the PWM method, a little bit better but uses more CPU 

## 0.8.8 (23 Oct 2021)
* Added new video modes
    * FDOOMC16.EXE => FastDoom CGA 160x100 and 16 colors
    * FDOOMV16.EXE => FastDoom VGA 160x200 and 16 colors
    * FDOOME.EXE   => FastDoom EGA 640x200 (dithering 16 colors)
* Added support for Sound Blaster Direct mode. This mode doesn't require IRQ nor DMA, but it's a bit slower and limited to 8-bit mono sound. Useful for PCMCIA clone cards and problematic cards/systems.
* Better CGA 320x200 and 4 colors support
* Added new screen size with a minimalistic HUD
* New command line parameter that allows showing the FPS via the debug port 0x80 (-debugPort)

## 0.8.7 (16 Aug 2021)
* Added new video modes. Huge thanks to @PickledDog for implementing these
 * FDOOMPCP.EXE => FastDoom for Plantronics ColorPlus cards. 320x200 and 16 colors!
 * FDOOMCVB.EXE => FastDoom CGA composite mode. 160x200 and 16 colors!
* New command line parameter that allows disabling the defered demos (-disabledemo)

## 0.8.6 (4 Aug 2021)
* Added new video modes (for LGR's vertical monitor). Both support page flipping.
 * FDOOMV.EXE  => FastDoom Planar 320x350 vertical mode. The base 320x200 image is stretched to 320x350 (wrong aspect ratio)
 * FDOOMV2.EXE => FastDoom Planar 320x350 vertical mode. The base 320x200 image is centered in the screen, leaving black borders
 * FDOOMT52.EXE => FastDoom Text Mode 80x100 (80x50 double height)
* Optimizations for text modes

## 0.8.5 (20 Jul 2021)
* Added new video modes
 * FDOOMT1.EXE  => FastDoom 40x25 16 colors text mode
 * FDOOMT12.EXE => FastDoom 40x25 16 colors text mode (virtual resolution of 40x50)
 * FDOOMVBD.EXE => FastDoom for VBE 2.0 cards with LFB (triple buffered), all rendering is done directly onto the video card, the same way Mode Y does. Faster for systems with slow RAM access.
* More optimizations (thanks to @RamonUnch)

## 0.8.4 (1 Jul 2021)
* Added support for two new sound cards:
    * PC Speaker digitized (1 bit, 11KHz, mono)
    * COVOX LPT DAC (8 bit, 11KHz, mono)
* Fixed issue #37

## 0.8.3 (26 Jun 2021)
* Fix buttons incorrect behaviour (#36) 
* Lowered RAM usage.

## 0.8.2 (6 Jun 2021)
* Two new executables for VBE 2.0 modes (320x200 and 8 bit color support required). Most modern video cards should be faster with this mode (Vesa Local Bus and PCI cards). Only supports high detail mode.
    * FDOOMVBR.EXE - Uses basic real mode VBE 2.0 support. Most compatible mode, if you use UniVBE, use this executable
    * FDOOMVBP.EXE - Uses protected mode extensions from VBE 2.0. Should be a little bit faster with some video cards, but it's more problematic in most circunstances
* Fixed important bug that made Ultimate Doom switches not work as expected (#32)

## 0.8.1 (24 May 2021)
* New video modes!!
    * VGA Mode 13h (320x200 256 colors, without page flipping). Same mode as Heretic/Hexen. Only high detail mode is implemented, but it's usually faster than the original Mode Y. Recomended for fast 486 processors and upwards
    * CGA (320x200 4 colors). Ugly, needs a better conversion algorithm. Requires a fast CPU. Based on Mode 13h
    * CGA (640x200 monochrome, ordered dithering 2x2). Requires a fast CPU. Based on Mode 13h
    * EGA (320x200 16 colors, page flipped, 128Kb VRAM minimum). SLOW, most ISA 8-bit EGA video cards can't update fullscreen at decent framerates. Based on Mode 13h
    * Hercules (640x400 monochrome, ordered dithering 2x2). Requires a fast CPU. Based on Mode 13h
* Added basic IWAD selector on bootup (if multiple IWADs are detected)
* Fixed Super Shotgun animation
* As always more optimizations. Now text modes are much faster
* Fixed multiple bugs
* Removed color correction parameter "-fixcolors" for 16 color modes. Easier to use

## 0.8 (14 Apr 2021)
* Better RAM usage (also reduced memory footprint)
* More code optimizations
* Compiled with the latest OpenWatcom v2 version
* Stripped episode finale texts from the executable, now are stored in external text files
* Support for Doom II BFG edition. Use the command line parameter "-bfg"
* Smaller executable thanks to UPX compression tool
* New option to render the status bar background a little bit faster. Use the command line "-simplestatusbar". It replaces the status bar background with a simple grey color.
* Two new executables: FDOOMT25, which renders in text mode at 80x25 resolution (CGA, EGA and VGA supported, 16 colors and virtual resolution of 80x50) and FDOOMT50, which renders in text mode at 80x50 resolution (VGA only, 16 colors). Both support triple buffering. These executables have reduced even more the memory footprint as multiple graphics don't need to be loaded (fonts for example). Automap feature isn't supported. This release also includes the WAD ega_pal.wad, which replaces the original colormaps to a better one for text modes.
* New command line parameter "-cga", this let's the new video modes run properly with CGA video cards
* New command line parameter "-fixcolors", this is needed to correct the 256 to 16 colors conversion, as the original Doom palette is too dark for a direct conversion.
* Unified the 386 and 486 executables, the 486 executables were always slower and bigger than the 386 ones. Never figured out why this happens, maybe you can blame OpenWatcom.
* Unified executables between different versions of Doom onto a single one. The supported wads are the following:
    * DOOM.WAD -> DOOM Registered (3 episodes)
    * DOOM1.WAD -> DOOM Shareware (1 episode)
    * DOOMU.WAD -> Ultimate DOOM (4 episodes)
    * DOOM2.WAD -> DOOM II
    * PLUTONIA.WAD -> Final DOOM The Plutonia Experiment
    * TNT.WAD -> Final DOOM TNT Evilution
* New SETUP program! I've found how to edit and compile the original IDSETUP program, and modified it to create a custom version for FastDoom.
* Renamed the configuration file from default.cfg to fdoom.cfg. Also renamed save files from doomsav*.dsg to fdoomsv*.dsg.

## 0.7 (3 Dec 2020)
* Added Stereo OPL2, OPL3 and Stereo OPL3 music support (Adlib uses basic OPL2, Sound Blaster tries to detect the best possible option)
* Added Disney Sound Source (also Tandy Sound Source) sound support. You have to manually set "snd_sfxdevice" variable in the "default.cfg" file to 12 (Disney Sound Source) or 13 (Tandy Sound Source). There are three new command line options to force the parallel port in case auto-initialization process doesn't work ("-LPT1" -> port 3bc, "-LPT2" -> port 378, "-LPT3" -> port 278). COVOX / LPT DAC is not supported.
* Fixed Gravis Ultrasound music support
* Added profiling support in the makefile (Intel Pentium processor required)
* Added option that forces 8 bit audio mixing instead of 16 bit (-8bitsound)
* Fixed potato invisible column renderer (issue #2)
* Fixed chainsaw incorrect behaviour (issue #9)
* Fixed all items respawn when loading a savegame (issue #10)
* Fixed Arch-Vile fire spawned at the wrong location (original Vanilla Doom blatant error)
* Fixed Chaingun makes two sounds firing single bullet (another Vanilla Doom error, fixed as there is no multiplayer support)
* Fixed invulnerability sky colormap
* Fixed issue #14
* Fixed issue #16
* Fixed some command line parameters not working as expected
* Removed IDMYPOS cheat
* Added VSYNC support (-vsync)
* Added new command line parameters that disable some optimizations (bypassing the saved configuration, made for benchmarking): -normalsurfaces, -normalsky, -normalshadows, -normalsprites, -stereo, -melt, -capped, -novsync, -nofps.
* All new options finally are saved in default.cfg
* As always added more optimizations (mostly ASM optimized multiplications, divisions and modulo, faster screen melting code, faster cheats detection, optimized sound code, faster potato mode [thanks @Optimus6128 !!], faster melting screen, faster flat drawing functions)

## 0.666 (15 Sep 2020)
* Renamed executable to FDOOM.EXE / FDOOM2.EXE
* Added all FastDoom display options and sound options in the options menu (only screen size option is saved, next versions will fix this)
* Added "-reverseStereo" command line option to reverse Left/Right stereo speakers. Your Sound Blaster PRO will love this.
* Added "-size |screensize|" command line option to force screen size. Values allowed are 3 to 11, being 3 the smallest possible, 10 full screen with HUD and 11 the biggest, fullscreen without HUD.
* Added "-logTimedemo" option to save the benchmark result onto the file "bench.txt". With this it's possible to run multiple FastDoom benchmarks in a batch and save the results in a single text file. Requires "-timedemo".
* Removed "-cdrom" command line option.
* Fixed bug that made some sprites look corrupted (depending on screen size, only FastDoom 0.66)
* More engine code optimizations

## 0.66 (4 Sep 2020)
* Savegames are working again (broken in 0.6). Still not compatible with vanilla doom savegames, but at least it's working (latest version with compatible savegames is 0.5).
* Potato detail mode is now selectable from the options menu. Also it's selectable by pressing F5 ingame.
* Support for unlimited sprites. Doom II MAP30 now doesn't crash if there are too many enemies. Also NUTS.WAD is partially working (albeit your 386 won't be doing 144 fps 🤣).
* Experimental new uncapped fps mode. Enabled with "-uncapped". It has rendering problems and won't interpolate movement between frames. It allows you to see how "fast" can be your pc in realtime.
* Added "-forcePQ", "-forceLQ" and "-forceHQ" parameters. It allows setting the video detail quality from command line. Useful for benchmarks.
* More internal optimizations.

## 0.6 (23 Aug 2020)
* Added option "-nomelt" to avoid melting transition while loading a new levels. For 386 pc's where this functionality it's really slow.
* Fixed bug that made the framerate choppy with fast processors
* Lot's of internal optimizations (non-recursive R_RenderBSPNode, many ideas from GBADoom). This version is faster even without lowering image quality
* Flat surfaces now have diminished lightning enabled or not. The option "-flatsurfaces" has it enabled, and the new option "-flattersurfaces" has it disabled. The option "-flattersurfaces" is MUCH faster than the previous one.

## 0.5 (13 Aug 2020)
* Faster Potato mode. Now it's 100% native potato mode and doesn't use the LQ mode to draw the screen.
* Fixed Sega Saturn shadows in potato mode.
* Added "-init" parameter, it forces the user to press a key to start the game. This makes easier to see the initialization process.
* Now it's possible to use "-nomonsters" without "-warp level". Change made to test the IA performance impact.
* Fixed AWE32 music.
* More rendering and main code optimizations.
* Bring back the gamma correction, lot's of users with CRT monitors needed this functionality (F11 key)
* Remapped autorun to F12 key

## 0.4 (27 Jul 2020)
* Added autorun support (F11 key).
* Added mono sound support. Enabled with "-mono" parameter.
* Added low quality sound support (8000Hz instead of 11025Hz). Enabled with "-lowsound" parameter.
* Fixed Doom 1.9 shareware menu.
* Included sound library into the source code, instead of using a prebuilt library. Sound cache it's still not fixed, but now it's possible to add support for new sound cards, optimizations and fix bugs. May break some sound cards support (untested Gravis Ultrasound and Pro AudioSpectrum 16).
* Optimized build options. Now there are builds for 386 and 486 processors.
* Replaced DOS/4GW with DOS/32A providing a good speedup!
* More small code optimizations

## 0.3 (18 Jun 2020)
* NEW DETAIL LEVEL: POTATO. It renders the full scene with a quarter width resolution (max 80x200). 16-bit ISA video cards can play the game full screen much better!! My Western Digital Paradise PVGA1A ISA (1989) can render the game at a constant >25 fps with a 486DX-50. Enabled with "-potato" command parameter and low detail enabled.
* Memory allocation message now is showed in Kb format
* Added FPS ingame viewer. Enabled with "-fps" command parameter
* Removed Y mouse movement (move forward/backwards)
* New option to allow more than 8Mb of memory allocation. Enabled with "-ram" command parameter
* New option to show only objects that are not far away from the player. All the enemies are still rendered as they're important. Enabled with "-near" command parameter.
* Small code optimizations

## 0.2.1 (13 Jun 2020)
* Fixed bug where any sound caused the game crash with real hardware

## 0.2 (13 Jun 2020)
* Small optimizations
* Added Sega Saturn transparency support. Enable with "-saturn"

## 0.1 (10 Jun 2020)
* Initial release
* Added FPS calculation after timedemo runs
* Removed network gaming support
* Removed joystick support
* Added option to render visplanes (ceiling and floors) without textures
* Added option to render Spectres and invisible objects like real transparent objects (harder to see, faster to render)
* Added option to render sky as a flat color
* Added option (yet to be enabled) to render Spectre and invisible objects like the Sega Saturn port did
* PC Speaker uses all sounds available 
* Lot's of optimizations to make the game run faster / smoother
* Added a basic debug interface, so you can write to a second screen in DOS (MDA, monochrome)