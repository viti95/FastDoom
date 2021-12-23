# FastDOOM
Doom port for DOS, based on PCDoom by @nukeykt. The goal of this port is to make it as fast as posible for 386/486 personal computers.

## FastDOOM vs Original

* Added FPS ingame viewer
* Added FPS calculation after timedemo runs
* Added option to render visplanes (ceiling and floors) without textures
* Added option to render Spectres and invisible objects like real transparent objects (harder to see, a little faster to render)
* Added option to render sky as a flat fixed color
* Added option to render Spectre and invisible objects like the Sega Saturn port did
* New option to show only objects that are not far away from the player. All the enemies are still rendered as they're important
* PC Speaker uses all sounds available (just for fun!)
* New mode for PC Speaker (digitized sound!)
* Disney Sound Source support
* COVOX LPT DAC support
* Lot's of optimizations to make the game run faster / smoother
* Removed low memory limit (may cause crashes with low RAM)
* NEW DETAIL LEVEL: POTATO. It renders the full scene with a quarter width resolution (max 80x200). 8-bit and 16-bit ISA video cards can play the game full screen much better!! My Western Digital Paradise PVGA1A ISA (1989) can render the game at a constant >25 fps with a 486DX-50
* New option to allow more than 8Mb of memory allocation. Enabled with "-ram"
* Removed network gaming support
* Removed joystick support
* Removed Y mouse movement (move forward/backwards)
* Added autorun support (F12 key)
* Added mono sound support
* Added low quality sound support (8000Hz instead of 11025Hz). Enabled with "-lowsound" parameter.
* Replaced DOS/4GW with DOS/32A providing a good speedup!
* New setup program
* New video modes (Mode 13h, CGA, EGA, Hercules, Text modes, VESA 2.0)

## DEMO

|        | Doom 1.9 Audio | Doom 1.9 NoAudio | FastDoom 0.2 Audio | FastDoom 0.2 Audio FlatSurfaces | FastDoom 0.2 Audio FlatSurfaces FlatTransparency | FastDoom 0.2 Audio FlatSurfaces Sega Saturn transparency | FastDoom 0.2 NoAudio | FastDoom 0.2 NoAudio FlatSurfaces Sega Saturn transparency |
|--------|----------------|-------------------|--------------------|----------------------------------|----------------------------------------------------|-----------------------------------------------------------|-----------------------|--------------------------------------------------------------|
| FPS    | 19.56          | 21.68             | 22.44              | 25.77                            | 25.86                                              | 26.02                                                     | 24.79                 | 29.05                                                        |
| Gain | 100.00%        | 110.20%           | 114.70%            | 131.70%                          | 132.20%                                            | 133.00%                                                   | 126.74%               | 148.52%                                                      |

FastDoom 0.8 Live Demo:

https://user-images.githubusercontent.com/8323882/147228550-bf93cc50-3c92-4a7a-b84f-65c8bbe1d3a9.mp4

[Full video here](https://www.youtube.com/watch?v=qizwu6dozvc)

## Build instructions (FastDOOM)

1) Install Open Watcom C version 2.0
2) Install NASM
3) Install DOS/32A
4) Add Watcom's bin folder (binnt on Windows, binw on DOS) to the PATH.
5) Run cleanall.bat

## Build instructions (FDSETUP)

1) Install Borland C++ 4.52 (Windows 3.1, Windows 9x)
2) Install Laughing Dog Screen Maker 1.14 (MS-DOS)
3) Run .\FDSETUP\SCREENS\MAKE.BAT (MS-DOS)
4) Open setup.ide project in Borland C++ IDE
5) Build the project

## Contributing

Feel free to add issues or pull requests here on GitHub. I cannot guarantee that I will accept your changes, but feel free to fork the repo and make changes as you see fit. Thanks!
