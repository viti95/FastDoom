# FDBENCH.EXE makefile - Open Watcom 16-bit large model

CC     = wcl
CCOPTS = -ml -ei -j -zq -zp1 -s -ot -c

fdbench.c.obj: fdbench.c
	$(CC) $(CCOPTS) -fo=fdbench.c.obj fdbench.c

fdbench.exe: fdbench.c.obj
	wcl -ml -zq -fe=fdbench.exe fdbench.c.obj

clean: .SYMBOLIC
	rm -f fdbench.c.obj fdbench.exe
