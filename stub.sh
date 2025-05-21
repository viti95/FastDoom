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

echo "Trying native DOSBox-X..."

if type dosbox-x &>/dev/null; then
  echo "Using native DOSBox-X"
  dosbox-x -fastlaunch -silent -nomenu -nogui -noautoexec -noconfig -c "config -set cycles=max" -c "mount J ." -c "J:" -c "SET DOS32A=J:\DOS32A" -c "SET PATH=%PATH%;J:\DOS32A\BINW" -c "$parameter" &>/dev/null
  kill -TERM $tail_pid
  rm -f STUB.LOG
  echo "Done"
  exit
fi

echo "Trying Flatpak DOSBox-X..."

if flatpak info com.dosbox_x.DOSBox-X > /dev/null 2>&1; then
    echo "Using Flatpak DOSBox-X"
    flatpak run com.dosbox_x.DOSBox-X -fastlaunch -silent -nomenu -nogui -noautoexec -noconfig -c "config -set cycles=max" -c "mount J ." -c "J:" -c "SET DOS32A=J:\DOS32A" -c "SET PATH=%PATH%;J:\DOS32A\BINW" -c "$parameter" &>/dev/null
    kill -TERM $tail_pid
    rm -f STUB.LOG
    echo "Done"
    exit
fi

echo "Trying native DOSBox..."

if type dosbox &>/dev/null; then
  echo "Using native DOSBox (classic)"
  # NOTE: dosbox does not support the advanced for included in stub.bat
  echo "@echo off" > stubdbox.bat 
  if [ -z "$executable" ] 
  then
    for f in FDOOM*.EXE FDM*.EXE; do
        [ -e "$f" ] || continue
        echo "sb /R $f >> stub.log" >> stubdbox.bat
        echo "ss $f dos32a.d32 >> stub.log" >> stubdbox.bat
    done
  else
    echo "sb /R $executable >> stub.log" >> stubdbox.bat
    echo "ss $executable dos32a.d32 >> stub.log" >> stubdbox.bat
  fi
  echo "exit" >> stubdbox.bat
  SDL_VIDEODRIVER=dummy dosbox  -exit -c "config -set cycles=max" -c "mount J ." -c "J:" -c "SET DOS32A=J:\DOS32A" -c "SET PATH=%PATH%;J:\DOS32A\BINW" -c "stubdbox.bat" &>/dev/null
  rm -f stubdbox.bat
  kill -TERM $tail_pid
  rm -f STUB.LOG
  echo "Done"
  exit
fi

echo "Trying native DOSEMU2..."

if type dosemu &>/dev/null; then

  # Check for DOSEMU2 and not DOSEMU
  testdosemu=$(dosemu --version 2> /dev/null | head -n 1)

  if [[ "$testdosemu" == dosemu2* ]]; then
    echo "Using native DOSEMU2"
      # NOTE: dosemu2 does not support the advanced for included in stub.bat
    echo "@echo off" > stubdbox.bat 
    if [ -z "$executable" ] 
    then
      for f in FDOOM*.EXE FDM*.EXE; do
          [ -e "$f" ] || continue
          echo "sb /R $f >> stub.log" >> stubdbox.bat
          echo "ss $f dos32a.d32 >> stub.log" >> stubdbox.bat
      done
    else
      echo "sb /R $executable >> stub.log" >> stubdbox.bat
      echo "ss $executable dos32a.d32 >> stub.log" >> stubdbox.bat
    fi
    dosemu -K . -n -t -E confDOS.bat &> /dev/null
    # NOTE: dosemu2 doesn't create files in uppercase
    for file in *.exe; do mv -- "$file" "${file^^}"; done
    rm -f stubdbox.bat
    kill -TERM $tail_pid
    rm -f STUB.LOG
    echo "Done"
    exit
  fi
fi

echo "No suitable DOS emulator found. Abort"