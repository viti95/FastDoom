cd fastdoom
call make clean
call make fdoom13h.exe
copy fdoom13h.exe ..\fdoombwc.exe
cd ..
sb -r fdoombwc.exe