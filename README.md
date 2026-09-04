# FastDOOM
Doom port for DOS, based on PCDoom by @nukeykt. The goal of this port is to make it as fast as possible for 386/486 personal computers.

## FastDOOM vs Original

* Added FPS ingame viewer
* Added FPS calculation after timedemo runs
* Added option to render visplanes (ceiling and floors) without textures
* Added option to render walls without textures
* Added option to render Spectres and invisible objects like real transparent objects (harder to see, a little faster to render)
* Added option to render sky as a flat fixed color
* Added option to render Spectre and invisible objects like the Sega Saturn port did
* Added option to render Spectre and invisible objects like Heretic/Hexen
* New option to show only objects that are not far away from the player. All the enemies are still rendered as they're important
* PC Speaker uses all sounds available (just for fun!)
* New mode for PC Speaker (digitized sound!)
* Disney Sound Source support
* COVOX LPT DAC support
* Adlib OPL2 PCM support
* OPL2LPT and OPL3LPT support
* Tandy 3-voice PCM support (SN76489)
* Creative Music System / Game Blaster support
* Audio-CD music support
* PCM music support (through sound fx device)
* Hercules automap support
* Native Roland MT-32 support (GM + display)
* Roland SC-55 support (display)
* Yamaha TG300 support (display)
* Yamaha MU80 support (display)
* IBM PC Music Feature card support
* ESS ESFM support
* Adlib Gold digital sound support
* Windows Sound System (WSS) support
* Lot's of optimizations to make the game run faster / smoother
* Removed low memory limit (may cause crashes with low RAM)
* New detail level: Potato. It renders the full scene with a quarter width resolution (max 80x200). Much faster rendering, specially on 386 cpu's and slow 8-bit VGA ISA cards.
* Better use of system RAM (full memory allocation)
* Removed network gaming support
* Removed joystick support
* Removed Y mouse movement (move forward/backwards)
* Added autorun support (F12 key)
* Added mono sound support
* Replaced DOS/4GW with DOS/32A providing a good speedup!
* New setup program (FDSetup 2.0, fully open source toolchain)
* New video modes (Mode 13h, MDA, CGA, EGA, Hercules, Text modes, VESA 2.0, Plantronics Colorplus, Sigma Color 400, Hercules InColor)
* HiRes VESA video modes support (up to 1600x1200)
* Advanced benchmark support
* Uncapped FPS mode support (frame interpolation, more than 35 fps)
* Next/previous weapon keys
* Mouse buttons can be bound to different actions (Fire, Move Forward/Backward, Strafe, Use, Speed, Previous/Next Weapon or None)
* Secrets completed counter on the automap
* VT100 terminal output support

## DEMO

|        | Doom 1.9 Audio | Doom 1.9 NoAudio | FastDoom 0.2 Audio | FastDoom 0.2 Audio FlatVisplanes | FastDoom 0.2 Audio FlatVisplanes FlatTransparency | FastDoom 0.2 Audio FlatVisplanes Sega Saturn transparency | FastDoom 0.2 NoAudio | FastDoom 0.2 NoAudio flatVisplanes Sega Saturn transparency |
|--------|----------------|-------------------|--------------------|----------------------------------|----------------------------------------------------|-----------------------------------------------------------|-----------------------|--------------------------------------------------------------|
| FPS    | 19.56          | 21.68             | 22.44              | 25.77                            | 25.86                                              | 26.02                                                     | 24.79                 | 29.05                                                        |
| Gain | 100.00%        | 110.20%           | 114.70%            | 131.70%                          | 132.20%                                            | 133.00%                                                   | 126.74%               | 148.52%                                                      |

FastDoom 0.8 Live Demo (Texas Instruments 486DLC @40 MHz, Cirrus Logic GD-5422 ISA):

https://user-images.githubusercontent.com/8323882/147228550-bf93cc50-3c92-4a7a-b84f-65c8bbe1d3a9.mp4

[Full video here](https://www.youtube.com/watch?v=qizwu6dozvc)

## Build instructions

Requirements: Linux or WSL2 on Windows.

1. Install the toolchain:
    - OpenWatcom v2
    - NASM
    - GNU Make (recommended, significantly speeds up builds)
    - A DOS emulator for adding the DOS/32 stub: DOSEMU2 (preferred), DOSBox-X or classic DOSBox
    - 7-Zip (only for `package.sh`)
2. Execute `source fdenv.sh`
3. Build:
    - Single executable: `./build.sh <executable> [parameters]` where "executable" is any of the supported targets (fdoom.exe, fdoomega.exe, fdoomt50.exe, fdm1024r.exe, fdsetup.exe, fdstart.exe, ...) and "parameters" can be any combination of:
        * `-clean`: cleans all generated OBJs before building
        * `-stub`: adds the DOS/32 stub (requires a DOS emulator)
        * `-debug`: generates a debug executable with symbols, traceable stack frames and a .map file; adjust `dbgcfg.h` to your needs
        * `clean`: as the target name, cleans the FASTDOOM project only
    - All the executables + automatic DOS/32 stubs: `./buildall.sh`
    - Generate a full release package (ZIP): `./package.sh`

Example to build a fresh FastDoom executable ready to use on real hardware:

```
source fdenv.sh
./build.sh fdoom.exe -clean -stub
```

Example to build FDSETUP and FDSTART:

```
source fdenv.sh
./build.sh fdsetup.exe -clean
./build.sh fdstart.exe -clean
```

## Contributors

This project exists thanks to all the people who contribute.

<a href="https://github.com/viti95/FastDoom/graphs/contributors">
<img src="https://contrib.rocks/image?repo=viti95/fastdoom" />
</a>

## Contributing

Feel free to add issues or pull requests here on GitHub. I cannot guarantee that I will accept your changes, but feel free to fork the repo and make changes as you see fit. Thanks!

## License

FastDoom is released under the terms of the [GPLv2 license](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html#SEC1).
