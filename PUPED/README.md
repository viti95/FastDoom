# PUPED - FDSetup PUP Screen Editor

A web-based editor for creating and editing 80x25 ANSI/ASCII screens in the FDSetup `.PUP` format.

## About

PUPED allows you to create and edit screen definitions used by the FDSetup program (FastDOOM setup). These screens define dialog boxes, menus, and configuration dialogs using the `.PUP` binary format.

## Quick Start

1. Open `index.html` in any modern web browser (no server required)
2. Import an existing `.PUP` file or start from scratch
3. Edit using the toolbar modes and keyboard
4. Preview, export, or generate C arrays

## File Format

The `.PUP` format consists of a `pup_t` header (10 bytes) followed by compressed drawing data:

### Header

| Offset | Size | Description |
|--------|------|-------------|
| 0 | 2 bytes | Magic: `0x03EA` (little-endian) |
| 2 | 1 byte | Window width (1-80) |
| 3 | 1 byte | Window height (1-25) |
| 4 | 1 byte | X position on screen (0-79) |
| 5 | 1 byte | Y position on screen (0-24) |
| 6 | 2 bytes | Reserved (0x0000) |
| 8 | 2 bytes | Reserved (0x0000) |

### Drawing Commands

| Opcode | Payload | Description |
|--------|---------|-------------|
| `0x00` | `ch attr` | Draw char; enters/exits "stringdraw" mode |
| `0xFF` | `ch attr count` | Repeat char N times (count = 16-bit LE) |
| `0x01-0xFE` | `attr` | Draw char (opcode IS the char byte) |

**Stringdraw mode:** First `0x00` enters the mode (reads ch+attr). Subsequent bytes are raw characters using that attribute until another `0x00` exits.

**VGA Attributes:** Standard VGA attribute byte — lower nibble = foreground, upper nibble = background. E.g., `0x1F` = white on blue, `0x70` = white on black.

## Editing Modes

- **✏️ Text** — Type characters directly onto the grid. Auto-advances to next cell.
- **🎨 Fill** — Click and drag to fill cells with current foreground/background colors.
- **⬜ Rect** — Click and drag two corners to draw a box with CP437 border characters (┌ ┐ └ ┘ ─ │).
- **🧹 Erase** — Click and drag to clear cells back to spaces with default colors.

## Import / Export

- **Import:** Click **📂 Import .PUP** or drag-and-drop a `.PUP` file onto the page.
- **Export:** Click **💾 Export .PUP** to download the binary file.

## Features

### Zoom

- **➕ / ➖** buttons for 10% steps
- **Ctrl+Scroll** on the grid for fine-grained 5% steps
- Exact percentage input field (10%-500%)
- **⊞** resets to 100%

### Colors

- 16-color VGA palette in the sidebar for foreground and background selection
- Click a swatch to select; current choices highlighted in magenta (FG) and green (BG)
- Attribute byte displayed as hex (e.g., `0x1F`)

### CP437 Character Panel

All 256 Code Page 437 characters are available in the sidebar. Click any character to place it at the current cursor position. Includes:

- Box drawing: ┌ ┐ └ ┘ ─ │ ├ ┬ ┤ ┼ ─
- Block elements: ░ ▒ ▓ ▀ ■
- Arrows: ↑ ↓ ← → ↕ ↔
- Greek letters: α Γ π Σ τ Φ Θ Ω ε δ
- Math symbols: ∞ √ ± ≥ ≤ ≡ ≈ °
- Accented characters: Ç ü é â à è ï î etc.

### Preview

Click **👁 Preview** to see the rendered window with proper VGA colors. Press **Escape** or click **Close Preview** to dismiss.

### Hex Dump

Click **🔧 Hex** to view:
- Full hex dump of the exported binary
- Header field breakdown
- C array output for embedding in `pup_data.c`

Press **Escape** or click **Close** to dismiss.

### Window Properties

Adjust the window position (X, Y) and size (Width, Height) in the sidebar. The exported `.PUP` only includes cells within the defined window area.

### Undo / Redo

- **↩ Undo** / **↪ Redo** buttons in the toolbar
- Up to 50 undo levels
- Stack counter shown on the right side of the toolbar

### Fullscreen

Click **⛶ Fullscreen** to toggle browser fullscreen mode (also available via F11).

## Keyboard

| Key | Action |
|-----|--------|
| Any printable character | Type into current cell (auto-advances) |
| Backspace | Replace current cell with space |
| Delete | Clear current cell (space + default colors) |
| Escape | Close preview / hex overlay |

## Integration with FDSetup

Exported `.PUP` files can be:

1. **Used directly** by FDSetup's screen loader
2. **Embedded as C arrays** — use the Hex dump view to copy the generated C array:

```c
char far myscreen[] = {0xea, 0x03, 0x50, 0x19, ...};
```

Add this to `pup_data.c` and reference it from your setup code.

Example screens are available in `../FDSETUP/SCREENS/` (TITLE.PUP, IDMAIN2.PUP, CONTROL.PUP, etc.).

## Browser Compatibility

Works in any modern browser with CSS Grid support (Chrome, Firefox, Edge, Safari).
