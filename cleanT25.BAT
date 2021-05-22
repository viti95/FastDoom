cd fastdoom
call make clean
call make fdoomtxt.exe
copy fdoomtxt.exe ..\fdoomt25.exe
cd ..
sb -r fdoomt25.exe
