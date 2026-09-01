# FDSTART.EXE makefile (GNU make) - Open Watcom 16-bit large model

CC     = wcl
CCOPTS = -ml -ei -j -zq -zp1 -s -ot -c

fdstart.obj: fdstart.c
	$(CC) $(CCOPTS) -fo=fdstart.obj fdstart.c

fdstart.exe: fdstart.obj
	$(CC) -ml -zq -fe=fdstart.exe fdstart.obj

.PHONY: clean
clean:
	rm -f fdstart.obj fdstart.exe
