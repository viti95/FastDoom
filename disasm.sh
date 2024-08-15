#!/bin/bash

rm -rf DISASM

mkdir DISASM

cd DISASM

for f in ../FASTDOOM/*.obj; do
    filename=$(basename "$f" .obj)
    #echo $filename.obj
    wdis -l="$filename.lst" ../FASTDOOM/"$filename".obj
done

cd ..

echo "Dissasembly finished!"
