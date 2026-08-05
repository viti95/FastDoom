# FDSETUP.EXE makefile - Open Watcom 16-bit large model

CC     = wcl
CCOPTS = -ml -ei -j -zq -zp1 -s -ot -c

OBJS = &
 config.obj &
 control.obj &
 default.obj &
 main.obj &
 menu.obj &
 music.obj &
 pup_data.obj &
 scguess.obj &
 setup.obj &
 sfx.obj

.c.obj:
	$(CC) $(CCOPTS) -fo=$[*.obj $[*.c

fdsetup.exe: $(OBJS)
	wcl -ml -zq -fe=fdsetup.exe $(OBJS)

clean: .SYMBOLIC
	rm -f *.obj fdsetup.exe
