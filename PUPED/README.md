# PUPED - FDSetup PUP Screen Editor

A web-based editor for creating and editing 80x25 ANSI/ASCII screens in the FDSetup `.PUP` format.

## About

PUPED allows you to create and edit screen definitions used by the FDSetup program (FastDOOM setup). These screens define dialog boxes, menus, and configuration dialogs using the LaughingDog `.PUP` binary format.

## File Format

The `.PUP` format consists of a `pup_t` header followed by compressed drawing data:

### Header (10 bytes)

```c
typedef struct {
    short pup_id;    // Magic: 0x03EA (little-endian)
    char width;      // Window width (1-80)
    char height;     // Window height (1-25)
    char x;          // X position on screen (0-79)
    char y;          // Y position on screen (0-24)
    short mystery1;  // Always 0x0000
    short mystery2;  // Always 0x0000
} pup_t;
```

### Drawing Commands

| Opcode | Format | Description |
|--------|--------|-------------|
| `0x00` | `0x00 ch attr` | Draw char, start "stringdraw" mode |
| `0x00` | `0x00` (in stringdraw) | End stringdraw mode |
| `0xFF` | `0xFF ch attr count` | Repeat char N times (16-bit LE) |
| `0x01-0xFE` | `ch attr` | Draw char (opcode IS the char) |

**Stringdraw mode:** The first `0x00` enters stringdraw mode and reads a character + attribute. Subsequent bytes are raw characters using that attribute until another `0x00` exits the mode. This compresses runs of text efficiently.

**VGA Attributes:** Standard VGA attribute byte format:
- Bits 0-3: Foreground color (0-15)
- Bit 4: Foreground intensity
- Bits 5-7: Background color (0-7)

Examples: `0x1F` = white on blue, `0x70` = white on black, `0x0C` = yellow on black

## Usage

### Opening

Simply open `index.html` in any modern web browser. No server required.

### Import

- Click **📂 Import .PUP** to load a `.PUP` file
- Or drag-and-drop a `.PUP` file onto the page

### Editing

- **✏️ Text mode** (default): Type characters directly onto the grid
- **🎨 Fill mode**: Click and drag to fill cells with current colors
- **⬜ Rect mode**: Click two corners to draw a box with borders
- **🧹 Erase mode**: Click and drag to clear cells

### Navigation

- Arrow keys to move cursor
- Home/End for line start/end
- PageUp/PageDown for page jumps
- F5 to jump to window origin

### Colors

Select foreground and background colors using the VGA palette in the right sidebar. The attribute byte is shown in hex format.

### Special Characters

The sidebar includes tabs for:
- **Box**: Box drawing characters (┌ ┐ └ ┘ ─ │ ├ ┬ ┤ ┼ etc.)
- **Block**: Block elements (░ ▒ ▓ ▀ ■)
- **Arrows**: Arrow characters (↑ ↓ ← → ↔ etc.)
- **Lines**: Line characters
- **Math**: Mathematical symbols (∞ √ ± ≥ ≤ etc.)

All characters use Code Page 437 (OEM US) encoding as expected by DOS VGA text mode.

### Export

- Click **💾 Export .PUP** to download the binary file
- Press `Ctrl+S` as a shortcut

### Preview

- Click **👁 Preview** to see the rendered window
- Press `Escape` to close preview

### Hex Dump

- Click **🔧 Hex** to view the binary output and C array format
- Useful for debugging and integrating into `pup_data.c`

### Undo/Redo

- `Ctrl+Z` to undo
- `Ctrl+Y` to redo
- Up to 50 undo levels

## Integration with FDSetup

Exported `.PUP` files can be:

1. **Used directly** by FDSetup's screen loader
2. **Embedded as C arrays** in `pup_data.c` (use the Hex dump → C array output)

The generated C array can be added to `pup_data.c`:

```c
char far myscreen[] = {0xea, 0x03, 0x50, 0x19, ...};
```

## Examples

Example screens are available in `../FDSETUP/SCREENS/`:
- `TITLE.PUP` - Title screen
- `IDMAIN2.PUP` - Main menu
- `CONTROL.PUP` - Controller selection
- And many more...

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `1` | Text mode |
| `2` | Fill mode |
| `3` | Rect mode |
| `4` | Erase mode |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+S` | Export |
| `Escape` | Close dialogs |
| `F5` | Jump to window origin |

## Browser Compatibility

Works in any modern browser with CSS Grid support (Chrome, Firefox, Edge, Safari).
