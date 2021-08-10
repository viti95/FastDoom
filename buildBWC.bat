cd fastdoom
wmake fdoom13h.exe EXTERNOPT="/dMODE_CGA_BW /dUSE_BACKBUFFER" %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoom13h.exe ..\fdoombwc.exe
cd ..
sb -r fdoombwc.exe