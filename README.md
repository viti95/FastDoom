# FastDOOM
Doom port for DOS, based on PCDoom by @nukeykt. The goal of this port is to make it as fast as posible for 386/486 personal computers.

## FastDOOM vs Original

* Added FPS ingame viewer
* Added FPS calculation after timedemo runs
* Added option to render visplanes (ceiling and floors) without textures
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
* Lot's of optimizations to make the game run faster / smoother
* Removed low memory limit (may cause crashes with low RAM)
* New detail level: Potato. It renders the full scene with a quarter width resolution (max 80x200). Much faster rendering, specially on 386 cpu's and slow 8-bit VGA ISA cards.
* New option to allow more than 8Mb of memory allocation. Enabled with "-ram"
* Removed network gaming support
* Removed joystick support
* Removed Y mouse movement (move forward/backwards)
* Added autorun support (F12 key)
* Added mono sound support
* Replaced DOS/4GW with DOS/32A providing a good speedup!
* New setup program
* New video modes (Mode 13h, MDA, CGA, EGA, Hercules, Text modes, VESA 2.0, Plantronics Colorplus, Sigma Color 400)
* Advanced benchmark support

## DEMO

|        | Doom 1.9 Audio | Doom 1.9 NoAudio | FastDoom 0.2 Audio | FastDoom 0.2 Audio FlatVisplanes | FastDoom 0.2 Audio FlatVisplanes FlatTransparency | FastDoom 0.2 Audio FlatVisplanes Sega Saturn transparency | FastDoom 0.2 NoAudio | FastDoom 0.2 NoAudio flatVisplanes Sega Saturn transparency |
|--------|----------------|-------------------|--------------------|----------------------------------|----------------------------------------------------|-----------------------------------------------------------|-----------------------|--------------------------------------------------------------|
| FPS    | 19.56          | 21.68             | 22.44              | 25.77                            | 25.86                                              | 26.02                                                     | 24.79                 | 29.05                                                        |
| Gain | 100.00%        | 110.20%           | 114.70%            | 131.70%                          | 132.20%                                            | 133.00%                                                   | 126.74%               | 148.52%                                                      |

FastDoom 0.8 Live Demo (Texas Instruments 486DLC @40 MHz, Cirrus Logic GD-5422 ISA):

https://user-images.githubusercontent.com/8323882/147228550-bf93cc50-3c92-4a7a-b84f-65c8bbe1d3a9.mp4

[Full video here](https://www.youtube.com/watch?v=qizwu6dozvc)

## Build instructions (FastDOOM)

1) Install Open Watcom C version 2.0
2) Install NASM
3) Install DOS/32A
4) Add Watcom's bin folder (binnt on Windows, binw on DOS) to the PATH.
5) Run makeall.bat

## Build instructions (FDSETUP)

1) Install Borland C++ 4.52 (Windows 3.1, Windows 9x)
2) Install Laughing Dog Screen Maker 1.14 (MS-DOS)
3) Run .\FDSETUP\SCREENS\MAKE.BAT (MS-DOS)
4) Open setup.ide project in Borland C++ IDE
5) Build the project

## Contributors

This project exists thanks to all the people who contribute.

<a href="https://github.com/viti95/FastDoom/graphs/contributors">
<img src="https://contrib.rocks/image?repo=viti95/fastdoom" />
</a>

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=viti95/FastDoom&type=Date)](https://star-history.com/#viti95/FastDoom&Date)


## Contributing

Feel free to add issues or pull requests here on GitHub. I cannot guarantee that I will accept your changes, but feel free to fork the repo and make changes as you see fit. Thanks!
