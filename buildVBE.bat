cd fastdoom
wmake fdoomvbe.exe EXTERNOPT=/dMODE_VBE2 %1 %2 %3 %4 %5 %6 %7 %8 %9
copy fdoomvbe.exe ..
cd ..
sb -r fdoomvbe.exe