cd fastdoom
call make clean
call make fdoomtxt.exe
copy fdoomtxt.exe ..
cd ..
sb -r fdoomtxt.exe
