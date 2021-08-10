cd fastdoom
wmake clean
wmake fdoomvbd.exe EXTERNOPT=/dMODE_VBE2_DIRECT %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomvbd.exe ..\fdoomvbd.exe
cd ..
sb -r fdoomvbd.exe