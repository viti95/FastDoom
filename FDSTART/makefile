# FDSTART.EXE makefile - Open Watcom 16-bit large model

CC     = wcl
CCOPTS = -ml -ei -j -zq -zp1 -s -ot -c

OBJS   = fdstart.obj screen.obj util.obj groups.obj \
         menu.obj options.obj readme.obj

.c.obj:
	$(CC) $(CCOPTS) -fo=$[*.obj $[*.c

fdstart.exe: $(OBJS)
	wcl -ml -zq -fe=fdstart.exe $(OBJS)

clean: .SYMBOLIC
	rm -f *.obj fdstart.exe
