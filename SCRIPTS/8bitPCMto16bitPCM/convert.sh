#!/bin/sh

mkdir -p 16bit
for file in DS*.wav; do sox "$file" -b 16 "16bit_$file"; done
mv 16bit_*.wav 16bit/
cd 16bit
mmv '16bit_*.wav' '#1.wav'
