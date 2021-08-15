cd fastdoom
wmake fdoomy.exe EXTERNOPT=/dMODE_Y %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomy.exe ..
cd ..
sb -r fdoomy.exe
ss fdoomy.exe dos32a.d32