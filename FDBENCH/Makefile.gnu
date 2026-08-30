# FDBENCH.EXE makefile (GNU make) - Open Watcom 16-bit large model

CC     = wcl
CCOPTS = -ml -ei -j -zq -zp1 -s -ot -c

fdbench.obj: fdbench.c
	$(CC) $(CCOPTS) -fo=fdbench.obj fdbench.c

fdbench.exe: fdbench.obj
	$(CC) -ml -zq -fe=fdbench.exe fdbench.obj

.PHONY: clean
clean:
	rm -f fdbench.obj fdbench.exe
