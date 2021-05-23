cd fastdoom
wmake fdoomtxt.exe EXTERNOPT=/dMODE_T50 %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomtxt.exe ..\fdoomt50.exe
cd ..
sb -r fdoomt50.exe
