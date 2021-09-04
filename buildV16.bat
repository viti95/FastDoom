cd fastdoom
wmake fdoom13h.exe EXTERNOPT="/dMODE_VGA16 /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoomv16.exe
cd ..
sb -r fdoomv16.exe
ss fdoomv16.exe dos32a.d32