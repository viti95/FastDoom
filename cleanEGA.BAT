cd fastdoom
call make clean
call make fdoom13h.exe
copy fdoom13h.exe ..\fdoomega.exe
cd ..
sb -r fdoomega.exe