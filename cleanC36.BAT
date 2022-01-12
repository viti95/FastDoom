cd fastdoom
wmake clean
wmake fdoom13h.exe EXTERNOPT="/dMODE_CGA136 /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoomc36.exe
cd ..
sb -r fdoomc36.exe
ss fdoomc36.exe dos32a.d32