cd fastdoom
wmake fdoomtxt.exe EXTERNOPT=/dMODE_T25 %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomtxt.exe ..\fdoomt25.exe
cd ..
sb -r fdoomt25.exe
