#!/bin/bash
#!/bin/sh

rm -rf MacBuild
mkdir MacBuild
cd MacBuild
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/viti95/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
make

echo "RIP AND TEAR"
exit 0
