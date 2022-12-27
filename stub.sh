#!/bin/bash

dosbox-x -fastlaunch -nogui -noautoexec -noconfig -c "mount J ." -c "J:" -c "SET DOS32A=J:\DOS32A" -c "SET PATH=%PATH%;J:\DOS32A\BINW" -c "stub2.bat" &>/dev/null
