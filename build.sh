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
  buildtarget="fdoomy.exe"

elif [ "$target" = "fdoomy.exe" ]; then
  buildopts="-dMODE_Y"
  buildtarget="fdoomy.exe"

elif [ "$target" = "fdoomcga.exe" ]; then
  buildopts="-dMODE_CGA -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoomega.exe" ]; then
  buildopts="-dMODE_EGA -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoombwc.exe" ]; then
  buildopts="-dMODE_CGA_BW -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoomhgc.exe" ]; then
  buildopts="-dMODE_HERC -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoomt1.exe" ]; then
  buildopts="-dMODE_T4025"
  buildtarget="fdoomtxt.exe"

elif [ "$target" = "fdoomt12.exe" ]; then
  buildopts="-dMODE_T4050"
  buildtarget="fdoomtxt.exe"

elif [ "$target" = "fdoomt25.exe" ]; then
  buildopts="-dMODE_T8025"
  buildtarget="fdoomtxt.exe"

elif [ "$target" = "fdoomt43.exe" ]; then
  buildopts="-dMODE_T8043"
  buildtarget="fdoomtxt.exe"

elif [ "$target" = "fdoomt86.exe" ]; then
  buildopts="-dMODE_T8086"
  buildtarget="fdoomtxt.exe"

elif [ "$target" = "fdoomt50.exe" ]; then
  buildopts="-dMODE_T8050"
  buildtarget="fdoomtxt.exe"

elif [ "$target" = "fdoomt52.exe" ]; then
  buildopts="-dMODE_T80100"
  buildtarget="fdoomtxt.exe"

elif [ "$target" = "fdoomvbr.exe" ]; then
  buildopts="-dMODE_VBE2 -dUSE_BACKBUFFER"
  buildtarget="fdoomvbe.exe"

elif [ "$target" = "fdoomvbd.exe" ]; then
  buildopts="-dMODE_VBE2_DIRECT"
  buildtarget="fdoomvbd.exe"

elif [ "$target" = "fdoomv2.exe" ]; then
  buildopts="-dMODE_V2 -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoompcp.exe" ]; then
  buildopts="-dMODE_PCP -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoomcvb.exe" ]; then
  buildopts="-dMODE_CVB -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoomc16.exe" ]; then
  buildopts="-dMODE_CGA16 -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoomc36.exe" ]; then
  buildopts="-dMODE_CGA136 -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoomv16.exe" ]; then
  buildopts="-dMODE_VGA16 -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoomv36.exe" ]; then
  buildopts="-dMODE_VGA136 -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoome36.exe" ]; then
  buildopts="-dMODE_EGA136 -dUSE_BACKBUFFER"
  buildtarget="fdoome3h.exe"

elif [ "$target" = "fdoome.exe" ]; then
  buildopts="-dMODE_EGA640 -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoom13h.exe" ]; then
  buildopts="-dMODE_13H -dUSE_BACKBUFFER"
  buildtarget="fdoom13h.exe"

elif [ "$target" = "fdoommda.exe" ]; then
  buildopts="-dMODE_13H -dUSE_BACKBUFFER"
  buildtarget="fdoommda.exe"

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
cp "$buildtarget" "../$target"
cd ..

exit 0
