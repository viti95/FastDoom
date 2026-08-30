# FDBENCH.EXE makefile - Open Watcom 16-bit large model

CC     = wcl
CCOPTS = -ml -ei -j -zq -zp1 -s -ot -c

.c.obj:
	$(CC) $(CCOPTS) -fo=$[*.obj $[*.c

fdbench.exe: fdbench.obj
	wcl -ml -zq -fe=fdbench.exe fdbench.obj

clean: .SYMBOLIC
	rm -f *.obj fdbench.exe
