cd fastdoom
wmake clean
wmake fdoom13h.exe EXTERNOPT="/dMODE_13H /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..
cd ..
sb -r fdoom13h.exe
ss fdoom13h.exe dos32a.d32