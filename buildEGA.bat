cd fastdoom
wmake fdoom13h.exe EXTERNOPT=/dMODE_EGA %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoomega.exe
cd ..
sb -r fdoomega.exe