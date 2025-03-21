#!/bin/bash

set -e

./buildall.sh

rm -rf PKG

mkdir PKG

cp *.TCF PKG/
cp FDOOM*.EXE PKG/
cp FDM*.EXE PKG/
cp WAD/*.WAD PKG/
cp README.txt PKG/
cp synthgs.sbk PKG/
cp MT32GM.MID PKG/
cp BENCH.BAT PKG/
cp -R INTER PKG/INTER
cp -R BENCH PKG/BENCH
cp -R LEVELS PKG/LEVELS
cp -R TEXT PKG/TEXT
cp -R DATA PKG/DATA
cp EXE/* PKG

versionstring=$(grep "#define FDOOMVERSION" FASTDOOM/version.h)
version=$(echo "$versionstring" | awk -F'"' '{print $2}')

rm -f FastDoom_$version.zip

cd PKG

if [[ -x "$(command -v 7z)" ]]; then
    7z a -r -mx9 ../FastDoom_$version.zip .
elif [[ -x "$(command -v 7zz)" ]]; then
    7zz a -r -mx9 ../FastDoom_$version.zip .
else
    echo 7-Zip not found >&2
    exit 1
fi

cd ..

rm -rf PKG
