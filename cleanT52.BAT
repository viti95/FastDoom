cd fastdoom
wmake clean
wmake fdoomtxt.exe EXTERNOPT=/dMODE_T80100 %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomtxt.exe ..\fdoomt52.exe
cd ..
sb -r fdoomt52.exe
ss fdoomt52.exe dos32a.d32