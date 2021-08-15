cd fastdoom
wmake fdoomtxt.exe EXTERNOPT=/dMODE_T8050 %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomtxt.exe ..\fdoomt50.exe
cd ..
sb -r fdoomt50.exe
ss fdoomt50.exe dos32a.d32