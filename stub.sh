#!/bin/bash
set -e

executable=$1

if [ -z "$executable" ]
then
      #Stub all FastDoom executables
      parameter="stub.bat"
else
      #Stub only one executable
      parameter="stub.bat $executable"
fi

echo "FastDoom DOS32/A Stub script"

echo -n > STUB.LOG
tail -F STUB.LOG &
tail_pid=$!

# Try native apps
if type dosbox-x &>/dev/null; then
  echo "Using native DOSBox-X"
  dosbox-x -fastlaunch -silent -nomenu -nogui -noautoexec -noconfig -c "config -set cycles=max" -c "mount J ." -c "J:" -c "SET DOS32A=J:\DOS32A" -c "SET PATH=%PATH%;J:\DOS32A\BINW" -c "$parameter" &>/dev/null
  kill -TERM $tail_pid
  rm STUB.LOG
  echo "Done"
  exit
fi

echo "Native DOSBox-X not found"

# Try flatpak app
if flatpak info com.dosbox_x.DOSBox-X > /dev/null 2>&1; then
    echo "Using Flatpak DOSBox-X"
    flatpak run com.dosbox_x.DOSBox-X -fastlaunch -silent -nomenu -nogui -noautoexec -noconfig -c "config -set cycles=max" -c "mount J ." -c "J:" -c "SET DOS32A=J:\DOS32A" -c "SET PATH=%PATH%;J:\DOS32A\BINW" -c "$parameter" &>/dev/null
    kill -TERM $tail_pid
    rm STUB.LOG
    echo "Done"
    exit
fi

echo "Flatpak DOSBox-X not found"

if type dosbox &>/dev/null; then
  echo "Using native DOSBox (classic)"
  SDL_VIDEODRIVER=dummy dosbox  -exit -c "config -set cycles=max" -c "mount J ." -c "J:" -c "SET DOS32A=J:\DOS32A" -c "SET PATH=%PATH%;J:\DOS32A\BINW" -c "$parameter" &>/dev/null
  kill -TERM $tail_pid
  rm STUB.LOG
  echo "Done"
  exit
fi

echo "No suitable DOSBox-X installation found. Abort"