cd fastdoom
wmake fdoom13h.exe EXTERNOPT="/dMODE_V /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoomv.exe
cd ..
sb -r fdoomv.exe
ss fdoomv.exe dos32a.d32