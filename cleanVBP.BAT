cd fastdoom
wmake clean
wmake fdoomvbe.exe EXTERNOPT="/dMODE_VBE2 /dMODE_PM /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomvbe.exe ..\fdoomvbp.exe
cd ..
sb -r fdoomvbp.exe
ss fdoomvbp.exe dos32a.d32