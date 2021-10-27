cd fastdoom
wmake fdoom13h.exe EXTERNOPT="/dMODE_ATI640 /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoomati.exe
cd ..
sb -r fdoomati.exe
ss fdoomati.exe dos32a.d32