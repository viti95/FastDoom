#!/bin/sh

if [ $# -lt 1 ]; then
  echo "Usage: $0 target.exe [buildopts]"
  echo "Check the README for possible targets."
  exit 1
fi

target=$1
shift 1

if [ "$target" = "fdoom.exe" ]; then
  buildopts="-dMODE_Y"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomcga.exe" ]; then
  buildopts="-dMODE_CGA"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomati.exe" ]; then
  buildopts="-dMODE_ATI640"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomew1.exe" ]; then
  buildopts="-dMODE_EGAW1"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomega.exe" ]; then
  buildopts="-dMODE_EGA"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoombwc.exe" ]; then
  buildopts="-dMODE_CGA_BW"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomhgc.exe" ]; then
  buildopts="-dMODE_HERC"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomt1.exe" ]; then
  buildopts="-dMODE_T4025"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomt12.exe" ]; then
  buildopts="-dMODE_T4050"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomt25.exe" ]; then
  buildopts="-dMODE_T8025"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomt43.exe" ]; then
  buildopts="-dMODE_T8043"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomt86.exe" ]; then
  buildopts="-dMODE_T8086"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomt50.exe" ]; then
  buildopts="-dMODE_T8050"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomt52.exe" ]; then
  buildopts="-dMODE_T80100"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomvbr.exe" ]; then
  buildopts="-dMODE_VBE2"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomvbd.exe" ]; then
  buildopts="-dMODE_VBE2_DIRECT"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomv2.exe" ]; then
  buildopts="-dMODE_V2"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoompcp.exe" ]; then
  buildopts="-dMODE_PCP"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomcvb.exe" ]; then
  buildopts="-dMODE_CVB"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomc16.exe" ]; then
  buildopts="-dMODE_CGA16"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoom512.exe" ]; then
  buildopts="-dMODE_CGA512"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoomcah.exe" ]; then
  buildopts="-dMODE_CGA_AFH"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoomv16.exe" ]; then
  buildopts="-dMODE_VGA16"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoome80.exe" ]; then
  buildopts="-dMODE_EGA80"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoome16.exe" ]; then
  buildopts="-dMODE_EGA16"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoome.exe" ]; then
  buildopts="-dMODE_EGA640"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoom13h.exe" ]; then
  buildopts="-dMODE_13H"
  buildtarget="fdoom.exe"

elif [ "$target" = "fdoommda.exe" ]; then
  buildopts="-dMODE_MDA"
  buildtarget="fdoom.exe"

elif [ "$target" = "clean" ]; then
  cd FASTDOOM
  wmake clean
  cd ..
  exit 0

else
  echo "Unknown target executable '$target' specified."
  exit 1

fi

cd FASTDOOM
wmake "$buildtarget" EXTERNOPT="$buildopts $@"
yes | cp -rf "$buildtarget" "../${target^^}"
cd ..
echo "RIP AND TEAR"
exit 0
