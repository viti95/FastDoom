cd fastdoom
wmake clean
wmake fdoomvbe.exe EXTERNOPT="/dMODE_VBE2 /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomvbe.exe ..\fdoomvbr.exe
cd ..
sb -r fdoomvbr.exe