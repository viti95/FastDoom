# FDSTART.EXE makefile (GNU make) - Open Watcom 16-bit large model

CC     = wcl
CCOPTS = -ml -ei -j -zq -zp1 -s -ot -c

OBJS   = fdstart.obj screen.obj util.obj groups.obj \
         menu.obj options.obj readme.obj warp.obj bench.obj

HDRS   = screen.h util.h keys.h groups.h menu.h options.h readme.h \
         warp.h bench.h texts.h

%.obj: %.c $(HDRS)
	$(CC) $(CCOPTS) -fo=$@ $<

fdstart.exe: $(OBJS)
	$(CC) -ml -zq -fe=fdstart.exe $(OBJS)

.PHONY: clean
clean:
	rm -f $(OBJS) fdstart.exe
