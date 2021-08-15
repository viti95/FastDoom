cd fastdoom
wmake clean
wmake fdoom13h.exe EXTERNOPT="/dMODE_V2 /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoomv2.exe
cd ..
sb -r fdoomv2.exe
ss fdoomv2.exe dos32a.d32