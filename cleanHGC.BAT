cd fastdoom
call make clean
call make fdoom13h.exe
copy fdoom13h.exe ..\fdoomhgc.exe
cd ..
sb -r fdoomhgc.exe