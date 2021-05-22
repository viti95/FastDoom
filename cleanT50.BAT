cd fastdoom
call make clean
call make fdoomtxt.exe
copy fdoomtxt.exe ..\fdoomt50.exe
cd ..
sb -r fdoomt50.exe
