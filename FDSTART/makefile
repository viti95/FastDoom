# FDSTART.EXE makefile - Open Watcom 16-bit large model

CC     = wcl
CCOPTS = -ml -ei -j -zq -zp1 -s -ot -c

.c.obj:
	$(CC) $(CCOPTS) -fo=$[*.obj $[*.c

fdstart.exe: fdstart.obj
	wcl -ml -zq -fe=fdstart.exe fdstart.obj

clean: .SYMBOLIC
	rm -f *.obj fdstart.exe
