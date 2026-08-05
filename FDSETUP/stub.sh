#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"
workspace_root="$PWD"
screens_dir="$workspace_root/SCREENS"

echo "FDSETUP DOSBox launcher"
echo "Working dir: $workspace_root"

echo "Trying native DOSEMU2..."
_dosemu=""
type dosemu  >/dev/null 2>&1 && _dosemu=dosemu
type dosemu2 >/dev/null 2>&1 && _dosemu=dosemu2
if [ -n "$_dosemu" ]; then
  echo "Using $_dosemu"
  "$_dosemu" -dumb -E "mount c $screens_dir" -E "c:" -E "make.bat" -E "exit"
  exit 0
fi

echo "Trying native DOSBox-X..."
if type xvfb-run >/dev/null 2>&1 && type dosbox-x >/dev/null 2>&1; then
  echo "Using xvfb-run + DOSBox-X"
  xvfb-run -a dosbox-x -fastlaunch -nomenu -nogui -noautoexec -noconfig \
    -c "mount c $screens_dir" -c "c:" -c "make.bat" -c "if errorlevel 1 exit" -c "exit"
  exit 0
fi
if type dosbox-x >/dev/null 2>&1; then
  echo "Using native DOSBox-X"
  dosbox-x -fastlaunch -nomenu -nogui -noautoexec -noconfig \
    -c "mount c $screens_dir" -c "c:" -c "make.bat" -c "if errorlevel 1 exit" -c "exit"
  exit 0
fi

echo "Trying Flatpak DOSBox-X..."
if flatpak info com.dosbox_x.DOSBox-X >/dev/null 2>&1; then
  echo "Using Flatpak DOSBox-X"
  flatpak run com.dosbox_x.DOSBox-X -fastlaunch -nomenu -nogui -noautoexec -noconfig \
    -c "config -set cycles=max" -c "mount c $screens_dir" -c "c:" -c "make.bat" -c "if errorlevel 1 exit" -c "exit"
  exit 0
fi

echo "Trying native DOSBox..."
if type xvfb-run >/dev/null 2>&1 && type dosbox >/dev/null 2>&1; then
  echo "Using xvfb-run + native DOSBox (classic)"
  _dosbox_conf=$(mktemp /tmp/dosbox.XXXXXX.conf)
  printf '[sblaster]\nsbtype=none\n[mixer]\nnosound=true\n' > "$_dosbox_conf"
  SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy xvfb-run -a dosbox -exit \
    -conf "$_dosbox_conf" \
    -c "mount c $screens_dir" -c "c:" -c "make.bat" -c "if errorlevel 1 exit" -c "exit" &
  _dosbox_pid=$!
  timeout 60 tail --pid="$_dosbox_pid" -f /dev/null || { kill "$_dosbox_pid" 2>/dev/null; echo "DOSBox timed out after 60s"; rm -f "$_dosbox_conf"; exit 1; }
  rm -f "$_dosbox_conf"
  exit 0
fi
if type dosbox >/dev/null 2>&1; then
  echo "Using native DOSBox (classic)"
  _dosbox_conf=$(mktemp /tmp/dosbox.XXXXXX.conf)
  printf '[sblaster]\nsbtype=none\n[mixer]\nnosound=true\n' > "$_dosbox_conf"
  SDL_VIDEODRIVER=dummy dosbox  -exit -c "config -set cycles=max" -c "mount J $screens_dir" -c "J:" -c "SET DOS32A=J:\DOS32A" -c "SET PATH=%PATH%;J:\DOS32A\BINW" -c "make.bat > output.log" &>/dev/null
  _dosbox_pid=$!
  timeout 60 tail --pid="$_dosbox_pid" -f /dev/null || { kill "$_dosbox_pid" 2>/dev/null; echo "DOSBox timed out after 60s"; rm -f "$_dosbox_conf"; exit 1; }
  rm -f "$_dosbox_conf"
  exit 0
fi

echo "No suitable DOS emulator found. Abort"
exit 1
