cd fastdoom
wmake clean
wmake fdoom13h.exe EXTERNOPT="/dMODE_VGA136 /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoomv36.exe
cd ..
sb -r fdoomv36.exe
ss fdoomv36.exe dos32a.d32