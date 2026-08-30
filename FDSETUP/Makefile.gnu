# FDSETUP.EXE makefile (GNU make) - Open Watcom 16-bit large model

CC     = wcl
CCOPTS = -ml -ei -j -zq -zp1 -s -ot -c

OBJS = \
 config.obj \
 control.obj \
 default.obj \
 main.obj \
 menu.obj \
 music.obj \
 pup_data.obj \
 setup.obj \
 sfx.obj

config.obj : config.c
	$(CC) $(CCOPTS) -fo=config.obj config.c

control.obj : control.c
	$(CC) $(CCOPTS) -fo=control.obj control.c

default.obj : default.c
	$(CC) $(CCOPTS) -fo=default.obj default.c

main.obj : main.c
	$(CC) $(CCOPTS) -fo=main.obj main.c

menu.obj : menu.c
	$(CC) $(CCOPTS) -fo=menu.obj menu.c

music.obj : music.c
	$(CC) $(CCOPTS) -fo=music.obj music.c

pup_data.obj : pup_data.c
	$(CC) $(CCOPTS) -fo=pup_data.obj pup_data.c

setup.obj : setup.c
	$(CC) $(CCOPTS) -fo=setup.obj setup.c

sfx.obj : sfx.c
	$(CC) $(CCOPTS) -fo=sfx.obj sfx.c

fdsetup.exe: $(OBJS)
	$(CC) -ml -zq -fe=fdsetup.exe $(OBJS)

.PHONY: clean
clean:
	rm -f *.obj fdsetup.exe
