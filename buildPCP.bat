cd fastdoom
wmake fdoom13h.exe EXTERNOPT=/dMODE_PCP %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoompcp.exe
cd ..
sb -r fdoompcp.exe