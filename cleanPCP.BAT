cd fastdoom
wmake clean
wmake fdoom13h.exe EXTERNOPT="/dMODE_PCP /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoompcp.exe
cd ..
sb -r fdoompcp.exe
ss fdoompcp.exe dos32a.d32