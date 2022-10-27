#!/bin/bash

python -m nuitka --lto=yes --follow-imports fastdoom_ansifromhell.py

./fastdoom_ansifromhell.bin 0 2 results1.txt &
./fastdoom_ansifromhell.bin 2 4 results2.txt &
./fastdoom_ansifromhell.bin 4 6 results3.txt &
./fastdoom_ansifromhell.bin 6 8 results4.txt &
./fastdoom_ansifromhell.bin 8 10 results5.txt &
./fastdoom_ansifromhell.bin 10 12 results6.txt &
./fastdoom_ansifromhell.bin 12 14 results7.txt &
./fastdoom_ansifromhell.bin 14 16 results8.txt &