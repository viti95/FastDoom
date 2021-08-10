cd fastdoom
wmake clean
wmake fdoomtxt.exe EXTERNOPT=/dMODE_T4050 %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomtxt.exe ..\fdoomt12.exe
cd ..
sb -r fdoomt12.exe
