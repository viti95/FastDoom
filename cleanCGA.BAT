cd fastdoom
call make clean
call make fdoom13h.exe
copy fdoom13h.exe ..\fdoomcga.exe
cd ..
sb -r fdoomcga.exe