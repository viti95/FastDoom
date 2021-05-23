cd fastdoom
wmake clean
wmake fdoom13h.exe EXTERNOPT=/dMODE_HERC %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoomhgc.exe
cd ..
sb -r fdoomhgc.exe