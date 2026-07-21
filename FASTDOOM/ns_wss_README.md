# Windows Sound System (WSS) Driver for FastDoom

## Overview

This driver adds support for Windows Sound System (WSS) compatible sound cards to FastDoom. WSS was a popular sound card standard in the early-to-mid 1990s, used by many clone sound cards and integrated sound chips (OAK, ESS, CS423x, AD1848, etc.).

The driver is based on the AD1848 datasheet and implements direct hardware access using the WSS register interface.

## Hardware Support

Compatible hardware includes:
- OAK Technologies WSS boards
- ESS Technology ESS688/ESS700/ESS720 (ES1370)
- Crystal Semiconductor CS4232/CS4235
- Analog Devices AD1848
- Various WSS-compatible PCI/ISA clone cards
- Some integrated motherboard audio chips

## Configuration

The driver uses the `ULTRA16` environment variable for configuration. Set it in your `AUTOEXEC.BAT`:

```
SET ULTRA16=530,5,1
```

Where:
- `530` = Base I/O address (hexadecimal, no 0x prefix)
- `5` = IRQ number (decimal)
- `1` = DMA channel (decimal, 0-3 for 8-bit DMA)

### Common I/O Addresses
- 0x530 (default)
- 0x604
- 0x620

### Supported IRQs
- 5, 7, 10 (most common)

### Supported DMA Channels
- 0, 1, 3 (8-bit channels only)

### Sample Rate Support
- 8000 Hz
- 11025 Hz
- 16000 Hz
- 22050 Hz
- 32000 Hz
- 44100 Hz
- 48000 Hz

## Usage with FastDoom

### Method 1: Using FDSETUP

1. Run `FDSETUP.EXE`
2. Go to "Sound Card Configuration"
3. Select "Sound Effects Card"
4. Choose "WSS" from the list
5. Select your desired sample rate and number of digital channels
6. Save and exit

### Method 2: Command Line

Set the `ULTRA16` environment variable and configure FastDoom:
```
SET ULTRA16=530,5,1
FDOOM.EXE -sfxdevice 23
```

Note: The device number (23) corresponds to `snd_WSS` in the `cardenum_t` enum. Check `i_sound.h` for the current value.

### Method 3: Config File

Add to `FDOOM.CFG`:
```
snd_sfxdevice    23
snd_rate         2
snd_channels     4
```

Where:
- `snd_sfxdevice` = Sound card ID (23 = WSS)
- `snd_rate` = Sample rate index (0=7000, 1=8000, 2=11025, 3=12000, 4=16000, 5=22050, 6=24000, 7=32000, 8=44100)
- `snd_channels` = Number of simultaneous sound channels (1-8)

## Technical Details

### Architecture

The WSS driver follows the same architecture as other FastDoom sound drivers:

1. **ns_wssdef.h** - Hardware register definitions from AD1848 datasheet
2. **ns_wss.h** - Driver API header
3. **ns_wss.c** - Driver implementation including:
   - Card detection
   - DMA setup (auto-init mode)
   - IRQ handling
   - Double-buffered playback
   - Mixer configuration

### Integration Points

The driver is integrated into:
- `i_sound.h` - Added `snd_WSS` to `cardenum_t`
- `i_sound.c` - WSS detection in `I_sndArbitrateCards()`
- `ns_cards.h` - Added `WSS` to `soundcardnames`
- `ns_multi.c` - WSS cases in mixer switch statements
- `makefile` - Added `ns_wss.obj` to build

### DMA Transfer

The driver uses 8-bit DMA in auto-initialization mode with double buffering. The DMA controller automatically restarts transfers after each half-buffer completion, triggering an IRQ for buffer refilling.

### Limitations

- 8-bit PCM only (limited by FastDoom's mixer)
- No capture/recording support
- No FM synthesis (use with Adlib/SB for music)
- DMA buffer must be within the first 1MB of memory
- DMA buffer must not cross a 64KB boundary

## Troubleshooting

### "WSS isn't responding"
- Verify `ULTRA16` environment variable is set correctly
- Check I/O address with `MODEM.COM /D` or similar tool
- Ensure no other program is using the sound card
- Try different IRQ/DMA combinations

### No sound
- Check speaker volume on the WSS card
- Verify `ULTRA16` DMA channel is correct (0-3 only)
- Try a lower sample rate (11025 Hz)

### Crackling/distortion
- Reduce number of sound channels (`snd_channels`)
- Try a lower sample rate
- Ensure IRQ is not shared with other devices

## Building

The driver is automatically included when building FastDoom. The makefile has been updated to include `ns_wss.obj`.

```
wmake -bh=FASTDOOM -b=FASTDOOM fdoom
```

## References

- AD1848 Data Sheet - Analog Devices
- WSS Programmer's Guide - Microsoft
- WAVWSS project by Lefucjusz (inspiration for register access)

## License

Same license as FastDoom (GPL v2).
