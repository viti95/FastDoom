#!/bin/bash
set -e
dosbox-x -fastlaunch -nomenu -nogui -noautoexec -noconfig -c "config -set cycles=max" -c "mount J ." -c "J:" -c "SET DOS32A=J:\DOS32A" -c "SET PATH=%PATH%;J:\DOS32A\BINW" -c "stub2.bat" &>/dev/null
