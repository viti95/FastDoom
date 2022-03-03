cd fastdoom
wmake clean
wmake fdoom13h.exe EXTERNOPT="/dMODE_EGA16 /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoome16.exe
cd ..
sb -r fdoome16.exe
ss fdoome16.exe dos32a.d32