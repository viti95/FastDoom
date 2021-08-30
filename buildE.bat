cd fastdoom
wmake fdoom13h.exe EXTERNOPT="/dMODE_EGA640 /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoome.exe
cd ..
sb -r fdoome.exe
ss fdoome.exe dos32a.d32