// Comment this out to indicate debugging is configured!
// Uncomment after reviewing the options. TODO - .gitignore?
//#define DEBUG_UNCONFIGURED

// Enables bounds checking for screen coordinates.
// This will die and show a backtrace if the coordinates are out of bounds.
#define BOUNDS_CHECK_ENABLED 1

// Asserts enabled
#define ASSERT_ENABLED 1

// Enables output to the bochs debug port displaed by dosbox-x and bochs
// when running on emulators. Disabled by default.
// Set the option 'bochs debug port e9 = true' in dosbox.conf to enable
// This doesn't correspond to any real hardware but it could, so we disable
// it by default.
#define BOCHS_DEBUG_ENABLED 0

// Enables the MDA debug output for a real or emulated MDA card
#define DEBUG_MDA_ENABLED 0

// Enables the file debug
#define DEBUG_FILE_ENABLED 1

// Enables the serial debug output and sets the parameters
// These are the default values for serial port 1
#define DEBUG_SERIAL_ENABLED 0
#define DEBUG_SERIAL_BASE_IO 0x3f8
#define DEBUG_SERIAL_BAUD 115200

// Whether to shutdown graphics and print the backtrace to the screen
// when a backtrace is requested. Otherwise, it will only be outputted to
// the other enabled debug outputs. This will also pause the game if
// DEBUG_DIE_ON_BACKTRACE is set to 0.
//
// When set to 0, the game will not show a backtraces on the screen, but they
// will be outputted to the other enabled debug outputs.
#define DEBUG_SHOW_BACKTRACE_ON_SCREEN 0
// Whether to quit the game when a backtrace is requested.
#define DEBUG_DIE_ON_BACKTRACE 1
