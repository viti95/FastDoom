cd fastdoom
wmake clean
wmake fdoom13h.exe EXTERNOPT="/dMODE_EGA136 /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoome36.exe
cd ..
sb -r fdoome36.exe
ss fdoome36.exe dos32a.d32